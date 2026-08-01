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
          bool EnableDma = false, bool EnableInterrupts = false, std::uint32_t TimeoutMs = 1000U>
struct master_config final
{
  static constexpr std::uint8_t instance_index{InstanceIndex};
  static constexpr std::uint32_t clock_hz{ClockHz};
  static constexpr spi_duplex_mode duplex_mode{DuplexMode};
  static constexpr spi_clock_polarity clock_polarity{ClockPolarity};
  static constexpr spi_clock_phase clock_phase{ClockPhase};
  static constexpr spi_bit_order bit_order{BitOrder};
  static constexpr bool enable_dma{EnableDma};
  static constexpr bool enable_interrupts{EnableInterrupts};
  static constexpr std::uint32_t timeout_ms{TimeoutMs};
};

/**
 * @brief Zero-overhead SPI master facade.
 */
template <typename Backend, typename Config>
  requires SpiMasterConcept<Backend, Config>
struct master final
{
  using backend_tag = Backend;
  using config_type = Config;

  static constexpr peripheral_kind kind{peripheral_kind::spi};

  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return spi_api_t<Backend>::template initialize<Config>(); });
  }

  [[nodiscard]] static inline status transfer(const std::span<const std::uint8_t> tx_payload,
                                              const std::span<std::uint8_t> rx_payload) noexcept
  {
    return detail::as_status([tx_payload, rx_payload]
                             { return spi_api_t<Backend>::template transfer<Config>(tx_payload, rx_payload); });
  }

  template <auto CompletionHandler>
  [[nodiscard]] static inline status transfer_async(const std::span<const std::uint8_t> tx_payload,
                                                    const std::span<std::uint8_t> rx_payload) noexcept
    requires IsrCapableConcept<Backend, peripheral_kind::spi>
  {
    return detail::as_status(
        [tx_payload, rx_payload]
        { return spi_api_t<Backend>::template transfer_async<Config, CompletionHandler>(tx_payload, rx_payload); });
  }
};

} // namespace microhal::spi
