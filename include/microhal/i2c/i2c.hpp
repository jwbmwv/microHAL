// SPDX-License-Identifier: Apache-2.0
/**
 * @file i2c.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <span>

#include "microhal/core/concepts.hpp"

namespace microhal::i2c
{

/**
 * @brief Compile-time I2C controller configuration.
 * @tparam TimeoutMs Bound applied to blocking transfers.
 */
template <std::uint8_t InstanceIndex, std::uint32_t ClockHz, i2c_address_mode AddressMode = i2c_address_mode::seven_bit,
          bool EnableDma = false, bool EnableInterrupts = false, std::uint32_t TimeoutMs = 1000U>
struct master_config final
{
  static constexpr std::uint8_t instance_index{InstanceIndex};
  static constexpr std::uint32_t clock_hz{ClockHz};
  static constexpr i2c_address_mode address_mode{AddressMode};
  static constexpr bool enable_dma{EnableDma};
  static constexpr bool enable_interrupts{EnableInterrupts};
  static constexpr std::uint32_t timeout_ms{TimeoutMs};
};

/**
 * @brief Zero-overhead I2C master facade with compile-time capability validation.
 */
template <typename Backend, typename Config>
  requires I2cMasterConcept<Backend, Config>
struct master final
{
  using backend_tag = Backend;
  using config_type = Config;

  static constexpr peripheral_kind kind{peripheral_kind::i2c};
  static constexpr bool is_dma_capable{Config::enable_dma};
  static constexpr bool is_isr_capable{Config::enable_interrupts};

  /** @brief Initializes the selected I2C controller instance. */
  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return i2c_api_t<Backend>::template initialize<Config>(); });
  }

  /**
   * @brief Writes a payload to a slave using a blocking transfer.
   * @param address 7-bit or 10-bit slave address, as defined by @ref Config::address_mode.
   * @param payload Bytes to transmit.
   */
  [[nodiscard]] static inline status write_blocking(const std::uint16_t address,
                                                    const std::span<const std::uint8_t> payload) noexcept
  {
    return detail::as_status([address, payload]
                             { return i2c_api_t<Backend>::template write_blocking<Config>(payload, address); });
  }

  /**
   * @brief Reads bytes from a slave using a blocking transfer.
   * @param address 7-bit or 10-bit slave address, as defined by @ref Config::address_mode.
   * @param payload Destination buffer to fill.
   */
  [[nodiscard]] static inline status read_blocking(const std::uint16_t address,
                                                   const std::span<std::uint8_t> payload) noexcept
  {
    return detail::as_status([address, payload]
                             { return i2c_api_t<Backend>::template read_blocking<Config>(payload, address); });
  }

  /**
   * @brief Starts an interrupt-driven transfer.
   * @tparam CompletionHandler Callback notified by the backend when the transfer completes.
   * @param address 7-bit or 10-bit slave address, as defined by @ref Config::address_mode.
   * @param payload Transfer buffer used by the backend.
   */
  template <auto CompletionHandler>
  [[nodiscard]] static inline status start_transfer(const std::uint16_t address,
                                                    const std::span<std::uint8_t> payload) noexcept
    requires IsrCapableConcept<Backend, peripheral_kind::i2c>
  {
    return detail::as_status(
        [address, payload]
        { return i2c_api_t<Backend>::template start_transfer<Config, CompletionHandler>(payload, address); });
  }
};

} // namespace microhal::i2c
