// SPDX-License-Identifier: Apache-2.0
/**
 * @file i2s.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <limits>
#include <span>
#include <type_traits>

#include "microhal/core/concepts.hpp"

namespace microhal::i2s
{

/**
 * @brief Compile-time I2S configuration.
 */
template <std::uint8_t InstanceIndex, i2s_mode Mode = i2s_mode::master_tx,
          i2s_standard Standard = i2s_standard::philips, std::uint32_t SampleRate = 48000U,
          bool EnableMasterClock = false, bool EnableDma = true, transfer_mode TransferMode = transfer_mode::automatic>
struct i2s_config final
{
  static constexpr std::uint8_t instance_index{InstanceIndex};
  static constexpr i2s_mode mode{Mode};
  static constexpr i2s_standard standard{Standard};
  static constexpr std::uint32_t sample_rate{SampleRate};
  static constexpr bool enable_master_clock{EnableMasterClock};
  static constexpr transfer_mode transfer_mode_value{TransferMode};
  static constexpr bool enable_dma{TransferMode == transfer_mode::dma ||
                                   (TransferMode == transfer_mode::automatic && EnableDma)};
};

template <typename Binding, i2s_mode Mode = i2s_mode::master_tx, i2s_standard Standard = i2s_standard::philips,
          std::uint32_t SampleRate = 48000U, bool EnableMasterClock = false, bool EnableDma = true,
          transfer_mode TransferMode = transfer_mode::automatic>
struct bound_i2s_config final
{
  using binding = Binding;

  static constexpr i2s_mode mode{Mode};
  static constexpr i2s_standard standard{Standard};
  static constexpr std::uint32_t sample_rate{SampleRate};
  static constexpr bool enable_master_clock{EnableMasterClock};
  static constexpr transfer_mode transfer_mode_value{TransferMode};
  static constexpr bool enable_dma{TransferMode == transfer_mode::dma ||
                                   (TransferMode == transfer_mode::automatic && EnableDma)};
};

/**
 * @brief Zero-overhead I2S audio interface facade with compile-time validation.
 */
template <typename Backend, typename Config>
  requires I2sConcept<Backend, Config>
struct interface final
{
  using backend_tag = Backend;
  using config_type = Config;

  static constexpr peripheral_kind kind{peripheral_kind::i2s};
  static constexpr transfer_mode transfer_mode_value{configured_transfer_mode<Config>()};
  using mode_api = serial_mode_api<Backend, peripheral_kind::i2s>;
  static constexpr bool has_full_duplex{backend_traits<Backend>::i2s_capability().full_duplex};
  static constexpr std::uint32_t max_sample_rate{backend_traits<Backend>::i2s_capability().max_sample_rate};
  static constexpr std::uint8_t sample_bits{backend_traits<Backend>::i2s_capability().sample_bits};

  [[nodiscard]] static inline status initialize() noexcept
  {
    if constexpr (transfer_mode_value != transfer_mode::automatic)
      return mode_api::template initialize<Config>();
    else
      return detail::as_status([] { return i2s_api_t<Backend>::template initialize<Config>(); });
  }

  [[nodiscard]] static inline status transmit_blocking(const std::span<const std::uint16_t> samples) noexcept
  {
    if (!is_valid_transfer(samples.size()))
    {
      return status::invalid_argument;
    }
    if constexpr (transfer_mode_value != transfer_mode::automatic)
      return mode_api::template transmit<Config, nullptr>(samples);
    else
      return detail::as_status([samples] { return i2s_api_t<Backend>::template transmit_blocking<Config>(samples); });
  }

  [[nodiscard]] static inline status receive_blocking(const std::span<std::uint16_t> samples) noexcept
  {
    if (!is_valid_transfer(samples.size()))
    {
      return status::invalid_argument;
    }
    if constexpr (transfer_mode_value != transfer_mode::automatic)
      return mode_api::template receive<Config, nullptr>(samples);
    else
      return detail::as_status([samples] { return i2s_api_t<Backend>::template receive_blocking<Config>(samples); });
  }

  template <auto CompletionHandler>
  [[nodiscard]] static inline status start_dma_tx(const std::span<const std::uint16_t> samples) noexcept
    requires(DmaCapableConcept<Backend, peripheral_kind::i2s> && backend_traits<Backend>::i2s_capability().transmit_dma)
  {
    static_assert(transfer_mode_value == transfer_mode::automatic ||
                  valid_serial_completion_handler<CompletionHandler>());
    if (!is_valid_transfer(samples.size()))
    {
      return status::invalid_argument;
    }
    if constexpr (transfer_mode_value == transfer_mode::dma)
      return mode_api::template transmit<Config, CompletionHandler>(samples);
    else if constexpr (transfer_mode_value != transfer_mode::automatic)
      return status::unsupported;
    else
      return detail::as_status(
          [samples] { return i2s_api_t<Backend>::template start_dma_tx<Config, CompletionHandler>(samples); });
  }

  template <auto CompletionHandler>
  [[nodiscard]] static status transmit_async(const std::span<const std::uint16_t> samples) noexcept
  {
    static_assert(valid_serial_completion_handler<CompletionHandler>());
    if (!is_valid_transfer(samples.size()))
      return status::invalid_argument;
    return mode_api::template transmit<Config, CompletionHandler>(samples);
  }
  template <auto CompletionHandler>
  [[nodiscard]] static status receive_async(const std::span<std::uint16_t> samples) noexcept
  {
    static_assert(valid_serial_completion_handler<CompletionHandler>());
    if (!is_valid_transfer(samples.size()))
      return status::invalid_argument;
    return mode_api::template receive<Config, CompletionHandler>(samples);
  }

  [[nodiscard]] static status transfer_blocking(const std::span<const std::uint16_t> tx,
                                                const std::span<std::uint16_t> rx) noexcept
  {
    if (!is_valid_transfer(tx.size()) || tx.size() != rx.size())
      return status::invalid_argument;
    if constexpr (!has_full_duplex)
      return status::unsupported;
    else if constexpr (transfer_mode_value != transfer_mode::automatic &&
                       requires { mode_api::template transfer<Config, nullptr>(tx, rx); })
      return mode_api::template transfer<Config, nullptr>(tx, rx);
    else if constexpr (transfer_mode_value != transfer_mode::automatic)
      return status::unsupported;
    else if constexpr (requires { i2s_api_t<Backend>::template transfer_blocking<Config>(tx, rx); })
      return detail::as_status([tx, rx] { return i2s_api_t<Backend>::template transfer_blocking<Config>(tx, rx); });
    else
      return status::unsupported;
  }

  template <auto CompletionHandler>
  [[nodiscard]] static status start_dma_rx(const std::span<std::uint16_t> samples) noexcept
    requires(DmaCapableConcept<Backend, peripheral_kind::i2s> && backend_traits<Backend>::i2s_capability().receive_dma)
  {
    static_assert(transfer_mode_value == transfer_mode::automatic ||
                  valid_serial_completion_handler<CompletionHandler>());
    if (!is_valid_transfer(samples.size()))
      return status::invalid_argument;
    if constexpr (transfer_mode_value == transfer_mode::dma)
      return mode_api::template receive<Config, CompletionHandler>(samples);
    else if constexpr (transfer_mode_value != transfer_mode::automatic)
      return status::unsupported;
    else if constexpr (requires { i2s_api_t<Backend>::template start_dma_rx<Config, CompletionHandler>(samples); })
      return detail::as_status(
          [samples] { return i2s_api_t<Backend>::template start_dma_rx<Config, CompletionHandler>(samples); });
    else
      return status::unsupported;
  }

  template <auto CompletionHandler>
  [[nodiscard]] static status start_dma_transfer(const std::span<const std::uint16_t> tx,
                                                 const std::span<std::uint16_t> rx) noexcept
    requires(DmaCapableConcept<Backend, peripheral_kind::i2s> && has_full_duplex &&
             backend_traits<Backend>::i2s_capability().transmit_dma &&
             backend_traits<Backend>::i2s_capability().receive_dma)
  {
    static_assert(transfer_mode_value == transfer_mode::automatic ||
                  valid_serial_completion_handler<CompletionHandler>());
    if (!is_valid_transfer(tx.size()) || tx.size() != rx.size())
      return status::invalid_argument;
    if constexpr (transfer_mode_value == transfer_mode::dma &&
                  requires { mode_api::template transfer<Config, CompletionHandler>(tx, rx); })
      return mode_api::template transfer<Config, CompletionHandler>(tx, rx);
    else if constexpr (transfer_mode_value != transfer_mode::automatic)
      return status::unsupported;
    else if constexpr (requires { i2s_api_t<Backend>::template start_dma_transfer<Config, CompletionHandler>(tx, rx); })
      return detail::as_status(
          [tx, rx] { return i2s_api_t<Backend>::template start_dma_transfer<Config, CompletionHandler>(tx, rx); });
    else
      return status::unsupported;
  }

  template <auto BlockHandler>
  [[nodiscard]] static status start_dma_stream(const std::span<const std::uint16_t> tx,
                                               const std::span<std::uint16_t> rx, const bool circular = true) noexcept
    requires DmaCapableConcept<Backend, peripheral_kind::i2s>
  {
    static_assert(std::is_nothrow_invocable_r_v<void, decltype(BlockHandler), status, std::span<const std::uint16_t>,
                                                std::span<std::uint16_t>>);
    if ((tx.empty() && rx.empty()) || (!tx.empty() && !is_valid_transfer(tx.size())) ||
        (!rx.empty() && !is_valid_transfer(rx.size())) || (!tx.empty() && !rx.empty() && tx.size() != rx.size()))
      return status::invalid_argument;
    if constexpr (transfer_mode_value != transfer_mode::automatic && transfer_mode_value != transfer_mode::dma)
      return status::unsupported;
    else if ((!tx.empty() && !backend_traits<Backend>::i2s_capability().transmit_dma) ||
             (!rx.empty() && !backend_traits<Backend>::i2s_capability().receive_dma) ||
             (!tx.empty() && !rx.empty() && !has_full_duplex))
      return status::unsupported;
    else if constexpr (requires {
                         i2s_api_t<Backend>::template start_dma_stream<Config, BlockHandler>(tx, rx, circular);
                       })
      return detail::as_status(
          [tx, rx, circular]
          { return i2s_api_t<Backend>::template start_dma_stream<Config, BlockHandler>(tx, rx, circular); });
    else
      return status::unsupported;
  }

  [[nodiscard]] static status stop() noexcept
  {
    if constexpr (requires { i2s_api_t<Backend>::template stop<Config>(); })
      return detail::as_status([] { return i2s_api_t<Backend>::template stop<Config>(); });
    else
      return mode_api::template abort<Config>();
  }

  [[nodiscard]] static status abort() noexcept
  {
    return mode_api::template abort<Config>();
  }
  [[nodiscard]] static status transfer_status() noexcept
  {
    return mode_api::template transfer_status<Config>();
  }
  [[nodiscard]] static status handle_interrupt() noexcept
  {
    return detail::as_status([] { return mode_api::template handle_interrupt<Config>(); });
  }
  [[nodiscard]] static status handle_dma_interrupt() noexcept
  {
    return detail::as_status([] { return mode_api::template handle_dma_interrupt<Config>(); });
  }

  [[nodiscard]] static constexpr bool is_valid_transfer(const std::size_t sample_count) noexcept
  {
    return (sample_count > 0U) && (sample_count <= std::numeric_limits<std::uint16_t>::max());
  }
};

} // namespace microhal::i2s
