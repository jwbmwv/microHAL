// SPDX-License-Identifier: Apache-2.0
/**
 * @file spi.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <span>

#include "microhal/core/concepts.hpp"

namespace microhal::spi
{

/**
 * @brief Compile-time SPI master configuration.
 */
template <std::uint8_t InstanceIndex, std::uint32_t ClockHz, spi_duplex_mode DuplexMode = spi_duplex_mode::full,
          spi_clock_polarity ClockPolarity = spi_clock_polarity::idle_low,
          spi_clock_phase ClockPhase = spi_clock_phase::first_edge, spi_bit_order BitOrder = spi_bit_order::msb_first,
          bool EnableDma = false, bool EnableInterrupts = false, std::uint32_t TimeoutMs = 1000U,
          transfer_mode Mode = transfer_mode::automatic>
struct master_config final
{
  static constexpr std::uint8_t instance_index{InstanceIndex};
  static constexpr std::uint32_t clock_hz{ClockHz};
  static constexpr spi_duplex_mode duplex_mode{DuplexMode};
  static constexpr spi_clock_polarity clock_polarity{ClockPolarity};
  static constexpr spi_clock_phase clock_phase{ClockPhase};
  static constexpr spi_bit_order bit_order{BitOrder};
  static constexpr transfer_mode transfer_mode_value{Mode};
  static constexpr bool enable_dma{Mode == transfer_mode::dma || (Mode == transfer_mode::automatic && EnableDma)};
  static constexpr bool enable_interrupts{Mode == transfer_mode::interrupt ||
                                          (Mode == transfer_mode::automatic && EnableInterrupts)};
  static constexpr std::uint32_t timeout_ms{TimeoutMs};
};

/**
 * @brief Zero-overhead SPI master facade.
 */
template <typename Backend, typename Config>
  requires SpiMasterConcept<Backend, Config>
struct master
{
  using backend_tag = Backend;
  using config_type = Config;

  static constexpr peripheral_kind kind{peripheral_kind::spi};
  static constexpr transfer_mode mode{configured_transfer_mode<Config>()};
  using mode_api = serial_mode_api<Backend, peripheral_kind::spi>;

  [[nodiscard]] static status handle_dma_interrupt() noexcept
  {
    return mode_api::template handle_dma_interrupt<Config>();
  }

  [[nodiscard]] static inline status initialize() noexcept
  {
    if constexpr (mode != transfer_mode::automatic || serial_slave_enabled<Config>())
      return mode_api::template initialize<Config>();
    else
      return detail::as_status([] { return spi_api_t<Backend>::template initialize<Config>(); });
  }

  [[nodiscard]] static inline status transfer(const std::span<const std::uint8_t> tx_payload,
                                              const std::span<std::uint8_t> rx_payload) noexcept
  {
    if (tx_payload.size() != rx_payload.size())
    {
      return status::invalid_argument;
    }
    if (tx_payload.empty())
    {
      return status::ok;
    }

    if constexpr (mode != transfer_mode::automatic || serial_slave_enabled<Config>())
      return mode_api::template transfer<Config, nullptr>(tx_payload, rx_payload);
    else
      return detail::as_status([tx_payload, rx_payload]
                               { return spi_api_t<Backend>::template transfer<Config>(tx_payload, rx_payload); });
  }

  /**
   * @brief Transmits bytes without requiring a receive buffer.
   * @details Backends that cannot provide a transmit-only operation report @ref status::unsupported.
   */
  [[nodiscard]] static inline status write_blocking(const std::span<const std::uint8_t> payload) noexcept
  {
    if (payload.empty())
    {
      return status::ok;
    }

    if constexpr (mode != transfer_mode::automatic || serial_slave_enabled<Config>())
      return mode_api::template write<Config, nullptr>(payload);
    else
      return detail::as_status([payload] { return spi_api_t<Backend>::template write_blocking<Config>(payload); });
  }

  /**
   * @brief Receives bytes without requiring a transmit buffer.
   * @details The backend supplies its documented dummy transmit value; unsupported paths report
   *          @ref status::unsupported.
   */
  [[nodiscard]] static inline status read_blocking(const std::span<std::uint8_t> payload) noexcept
  {
    if (payload.empty())
    {
      return status::ok;
    }

    if constexpr (mode != transfer_mode::automatic || serial_slave_enabled<Config>())
      return mode_api::template read<Config, nullptr>(payload);
    else
      return detail::as_status([payload] { return spi_api_t<Backend>::template read_blocking<Config>(payload); });
  }

  template <auto CompletionHandler>
  [[nodiscard]] static inline status transfer_async(const std::span<const std::uint8_t> tx_payload,
                                                    const std::span<std::uint8_t> rx_payload) noexcept
    requires IsrCapableConcept<Backend, peripheral_kind::spi>
  {
    static_assert(mode == transfer_mode::automatic || valid_serial_completion_handler<CompletionHandler>());
    if (tx_payload.size() != rx_payload.size())
    {
      return status::invalid_argument;
    }
    if (tx_payload.empty())
    {
      if constexpr (mode != transfer_mode::automatic)
        return status::invalid_argument;
      return status::ok;
    }

    if constexpr (mode != transfer_mode::automatic || serial_slave_enabled<Config>())
      return mode_api::template transfer<Config, CompletionHandler>(tx_payload, rx_payload);
    else
      return detail::as_status(
          [tx_payload, rx_payload]
          { return spi_api_t<Backend>::template transfer_async<Config, CompletionHandler>(tx_payload, rx_payload); });
  }

  template <auto CompletionHandler>
  [[nodiscard]] static status write_async(const std::span<const std::uint8_t> payload) noexcept
  {
    static_assert(valid_serial_completion_handler<CompletionHandler>());
    if (payload.empty())
      return status::invalid_argument;
    return mode_api::template write<Config, CompletionHandler>(payload);
  }
  template <auto CompletionHandler>
  [[nodiscard]] static status read_async(const std::span<std::uint8_t> payload) noexcept
  {
    static_assert(valid_serial_completion_handler<CompletionHandler>());
    if (payload.empty())
      return status::invalid_argument;
    return mode_api::template read<Config, CompletionHandler>(payload);
  }
  [[nodiscard]] static status abort() noexcept
  {
    if constexpr (mode == transfer_mode::automatic && !serial_slave_enabled<Config>() &&
                  !std::same_as<Backend, nordic_nrf_tag>)
      return status::unsupported;
    else
      return mode_api::template abort<Config>();
  }
  [[nodiscard]] static status transfer_status() noexcept
  {
    if constexpr (mode == transfer_mode::automatic && !serial_slave_enabled<Config>() &&
                  !std::same_as<Backend, nordic_nrf_tag>)
      return status::unsupported;
    else
      return mode_api::template transfer_status<Config>();
  }
  [[nodiscard]] static status handle_interrupt() noexcept
  {
    if constexpr (mode == transfer_mode::automatic && !serial_slave_enabled<Config>() &&
                  !std::same_as<Backend, nordic_nrf_tag>)
      return status::unsupported;
    else
      return detail::as_status([] { return mode_api::template handle_interrupt<Config>(); });
  }
};

/** @brief Hardware-NSS SPI slave; the external master supplies the clock. */
template <std::uint8_t InstanceIndex, spi_duplex_mode DuplexMode = spi_duplex_mode::full,
          spi_clock_polarity ClockPolarity = spi_clock_polarity::idle_low,
          spi_clock_phase ClockPhase = spi_clock_phase::first_edge, spi_bit_order BitOrder = spi_bit_order::msb_first,
          bool EnableDma = false, bool EnableInterrupts = false, std::uint32_t TimeoutMs = 1000U,
          transfer_mode Mode = transfer_mode::interrupt>
struct slave_config final
{
  static constexpr bool is_slave = true;
  static constexpr std::uint8_t instance_index = InstanceIndex;
  // Satisfies shared facade validation; slave hardware never generates this clock.
  static constexpr std::uint32_t clock_hz = 1U;
  static constexpr auto duplex_mode = DuplexMode;
  static constexpr auto clock_polarity = ClockPolarity;
  static constexpr auto clock_phase = ClockPhase;
  static constexpr auto bit_order = BitOrder;
  static constexpr auto transfer_mode_value = Mode;
  static constexpr bool enable_dma = Mode == transfer_mode::dma || (Mode == transfer_mode::automatic && EnableDma);
  static constexpr bool enable_interrupts =
      Mode == transfer_mode::interrupt || (Mode == transfer_mode::automatic && EnableInterrupts);
  static constexpr auto timeout_ms = TimeoutMs;
};

template <typename Backend, typename Config> struct slave final : master<Backend, Config>
{
  static_assert(serial_slave_enabled<Config>());
  template <auto Handler> [[nodiscard]] static status transmit_async(std::span<const std::uint8_t> bytes) noexcept
  {
    return master<Backend, Config>::template write_async<Handler>(bytes);
  }
  template <auto Handler> [[nodiscard]] static status receive_async(std::span<std::uint8_t> bytes) noexcept
  {
    return master<Backend, Config>::template read_async<Handler>(bytes);
  }
};
} // namespace microhal::spi
