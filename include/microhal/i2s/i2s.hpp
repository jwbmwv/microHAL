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

  /**
   * @brief Initializes the configured I2S interface.
   * @return Backend initialization status, including @ref status::unsupported when the selected
   *         configuration has no initialization path.
   */
  [[nodiscard]] static inline status initialize() noexcept
  {
    if constexpr (transfer_mode_value != transfer_mode::automatic)
      return mode_api::template initialize<Config>();
    else
      return detail::as_status([] { return i2s_api_t<Backend>::template initialize<Config>(); });
  }

  /**
   * @brief Transmits a finite sequence of 16-bit I2S samples and waits for completion.
   * @param[in] samples  Samples to transmit; storage is needed only until this call returns.
   * @return Transfer status, or @ref status::invalid_argument when the sample count is invalid.
   * @note The blocking path uses the configured backend timeout where available and requires live
   *       interrupt service when the selected engine needs it. Do not call from an ISR.
   */
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

  /**
   * @brief Receives a finite sequence of 16-bit I2S samples and waits for completion.
   * @param[out] samples  Destination samples; storage is needed only until this call returns.
   * @return Transfer status, or @ref status::invalid_argument when the sample count is invalid.
   * @note The blocking path uses the configured backend timeout where available and requires live
   *       interrupt service when the selected engine needs it. Do not call from an ISR.
   */
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

  /**
   * @brief Starts a finite DMA transmit.
   * @tparam CompletionHandler  No-throw callback accepting status or no arguments, as supported by
   *                            the selected serial mode.
   * @param[in] samples  Transmit samples; keep storage alive until completion or successful abort.
   * @return Submission status, or @ref status::invalid_argument for an invalid sample count, or
   *         @ref status::unsupported when the mode/backend does not implement TX DMA.
   * @note Requires backend DMA capability and TX-DMA support. The completion callback normally runs
   *       in interrupt context and must not block.
   */
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

  /**
   * @brief Starts a nonblocking transmit using the configured transfer mode.
   * @tparam CompletionHandler  No-throw callback accepting status or no arguments, as supported by
   *                            the selected serial mode.
   * @param[in] samples  Transmit samples; keep storage alive until completion or successful abort.
   * @return Submission status, or @ref status::invalid_argument for an invalid sample count.
   * @note Async support depends on the configured mode and backend. The callback normally runs in
   *       interrupt context and must not block.
   */
  template <auto CompletionHandler>
  [[nodiscard]] static status transmit_async(const std::span<const std::uint16_t> samples) noexcept
  {
    static_assert(valid_serial_completion_handler<CompletionHandler>());
    if (!is_valid_transfer(samples.size()))
      return status::invalid_argument;
    return mode_api::template transmit<Config, CompletionHandler>(samples);
  }
  /**
   * @brief Starts a nonblocking receive using the configured transfer mode.
   * @tparam CompletionHandler  No-throw callback accepting status or no arguments, as supported by
   *                            the selected serial mode.
   * @param[out] samples  Destination samples; keep storage alive until completion or successful abort.
   * @return Submission status, or @ref status::invalid_argument for an invalid sample count.
   * @note Async support depends on the configured mode and backend. The callback normally runs in
   *       interrupt context and must not block.
   */
  template <auto CompletionHandler>
  [[nodiscard]] static status receive_async(const std::span<std::uint16_t> samples) noexcept
  {
    static_assert(valid_serial_completion_handler<CompletionHandler>());
    if (!is_valid_transfer(samples.size()))
      return status::invalid_argument;
    return mode_api::template receive<Config, CompletionHandler>(samples);
  }

  /**
   * @brief Transmits and receives equal-length sample sequences simultaneously.
   * @param[in]  tx  Samples to transmit; storage is needed only until this call returns.
   * @param[out] rx  Destination samples; storage is needed only until this call returns.
   * @return Transfer status, @ref status::invalid_argument for invalid or unequal lengths, or
   *         @ref status::unsupported when the backend lacks full-duplex support.
   * @note This is blocking, uses the configured backend timeout where available, and must not be
   *       called from an ISR.
   */
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

  /**
   * @brief Starts a finite DMA receive.
   * @tparam CompletionHandler  No-throw callback accepting status or no arguments, as supported by
   *                            the selected serial mode.
   * @param[out] samples  Destination samples; keep storage alive until completion or successful abort.
   * @return Submission status, or @ref status::invalid_argument for an invalid sample count, or
   *         @ref status::unsupported when the mode/backend does not implement RX DMA.
   * @note Requires backend DMA capability and RX-DMA support. The completion callback normally runs
   *       in interrupt context and must not block.
   */
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

  /**
   * @brief Starts simultaneous finite DMA transmit and receive.
   * @tparam CompletionHandler  No-throw callback accepting status or no arguments, as supported by
   *                            the selected serial mode.
   * @param[in]  tx  Transmit samples; keep storage alive until completion or successful abort.
   * @param[out] rx  Destination samples; keep storage alive until completion or successful abort.
   * @return Submission status, @ref status::invalid_argument for invalid or unequal lengths, or
   *         @ref status::unsupported when full-duplex DMA is unavailable.
   * @note Requires backend DMA capability, full duplex, and both TX- and RX-DMA capabilities. The
   *       completion callback normally runs in interrupt context and must not block.
   */
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

  /**
   * @brief Starts a finite or circular DMA stream in the requested directions.
   * @tparam BlockHandler  No-throw callback invoked with status and completed TX/RX sample spans.
   * @param[in]  tx        TX sample storage, or empty to omit transmit; retain it until the stream stops.
   * @param[out] rx        RX sample storage, or empty to omit receive; retain it until the stream stops.
   * @param[in]  circular  Whether DMA repeats the supplied storage.
   * @return Submission status, @ref status::invalid_argument for empty directions or invalid/mismatched
   *         sample counts, or @ref status::unsupported when the mode or required DMA direction is absent.
   * @note Requires backend DMA capability. The handler normally runs in interrupt context; consume or
   *       copy callback spans there and do not block. Keep buffers alive until a successful stop/abort.
   */
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

  /**
   * @brief Stops active I2S activity using the backend stop operation or its abort fallback.
   * @return Backend stop/cancellation status, including @ref status::unsupported when neither path
   *         is available.
   */
  [[nodiscard]] static status stop() noexcept
  {
    if constexpr (requires { i2s_api_t<Backend>::template stop<Config>(); })
      return detail::as_status([] { return i2s_api_t<Backend>::template stop<Config>(); });
    else
      return abort();
  }

  /**
   * @brief Attempts to abort the active I2S transfer.
   * @return Backend cancellation status, or @ref status::unsupported when this mode/backend has no
   *         abort path. Retain active buffers unless cancellation succeeds.
   */
  [[nodiscard]] static status abort() noexcept
  {
    if constexpr (transfer_mode_value == transfer_mode::automatic && !std::same_as<Backend, nordic_nrf_tag>)
    {
      if constexpr (requires { i2s_api_t<Backend>::template abort<Config>(); })
        return detail::as_status([] { return i2s_api_t<Backend>::template abort<Config>(); });
      else
        return status::unsupported;
    }
    else
      return mode_api::template abort<Config>();
  }
  /**
   * @brief Queries the current or most recently completed I2S transfer status.
   * @return Backend transfer status, typically @ref status::busy while active.
   */
  [[nodiscard]] static status transfer_status() noexcept
  {
    return mode_api::template transfer_status<Config>();
  }
  /**
   * @brief Dispatches the configured I2S peripheral interrupt.
   * @return Backend interrupt-dispatch status.
   * @note Call from the peripheral ISR when required by the backend.
   */
  [[nodiscard]] static status handle_interrupt() noexcept
  {
    return detail::as_status([] { return mode_api::template handle_interrupt<Config>(); });
  }
  /**
   * @brief Dispatches the configured I2S DMA interrupt.
   * @return Backend interrupt-dispatch status.
   * @note Call from the DMA ISR when required by the backend.
   */
  [[nodiscard]] static status handle_dma_interrupt() noexcept
  {
    return detail::as_status([] { return mode_api::template handle_dma_interrupt<Config>(); });
  }

  /**
   * @brief Checks whether a sample count is nonzero and representable by the facade.
   * @param[in] sample_count  Number of 16-bit samples in a transfer.
   * @return @c true when the count is in the range 1 through @c UINT16_MAX; backend-specific
   *         alignment and capacity constraints may impose additional limits.
   */
  [[nodiscard]] static constexpr bool is_valid_transfer(const std::size_t sample_count) noexcept
  {
    return (sample_count > 0U) && (sample_count <= std::numeric_limits<std::uint16_t>::max());
  }
};

} // namespace microhal::i2s
