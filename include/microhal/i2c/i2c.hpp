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
 * @tparam TimeoutMs Deadline applied when the backend exposes a configurable timeout; otherwise the
 *         backend's native blocking-transfer timeout applies.
 */
template <std::uint8_t InstanceIndex, std::uint32_t ClockHz, i2c_address_mode AddressMode = i2c_address_mode::seven_bit,
          bool EnableDma = false, bool EnableInterrupts = false, std::uint32_t TimeoutMs = 1000U,
          transfer_mode Mode = transfer_mode::automatic>
struct master_config final
{
  static_assert(TimeoutMs > 0U, "microHAL: I2C timeout must be non-zero.");

  static constexpr std::uint8_t instance_index{InstanceIndex};
  static constexpr std::uint32_t clock_hz{ClockHz};
  static constexpr i2c_address_mode address_mode{AddressMode};
  static constexpr transfer_mode transfer_mode_value{Mode};
  static constexpr bool enable_dma{Mode == transfer_mode::dma || (Mode == transfer_mode::automatic && EnableDma)};
  static constexpr bool enable_interrupts{Mode == transfer_mode::interrupt ||
                                          (Mode == transfer_mode::automatic && EnableInterrupts)};
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
  static constexpr bool is_dma_capable{serial_dma_enabled<Config>()};
  static constexpr bool is_isr_capable{serial_interrupt_enabled<Config>()};
  static constexpr transfer_mode mode{configured_transfer_mode<Config>()};
  using mode_api = serial_mode_api<Backend, peripheral_kind::i2c>;

  [[nodiscard]] static status handle_dma_interrupt() noexcept
  {
    return mode_api::template handle_dma_interrupt<Config>();
  }

  /** @brief Initializes the selected I2C controller instance. */
  [[nodiscard]] static inline status initialize() noexcept
  {
    if constexpr (mode != transfer_mode::automatic)
      return mode_api::template initialize<Config>();
    else
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
    const std::uint16_t max_address = (Config::address_mode == i2c_address_mode::seven_bit) ? 0x7FU : 0x3FFU;
    if (address > max_address)
    {
      return status::invalid_argument;
    }

    if constexpr (mode != transfer_mode::automatic)
      return mode_api::template write<Config, nullptr>(payload, address);
    else
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
    const std::uint16_t max_address = (Config::address_mode == i2c_address_mode::seven_bit) ? 0x7FU : 0x3FFU;
    if (address > max_address)
    {
      return status::invalid_argument;
    }

    if constexpr (mode != transfer_mode::automatic)
      return mode_api::template read<Config, nullptr>(payload, address);
    else
      return detail::as_status([address, payload]
                               { return i2c_api_t<Backend>::template read_blocking<Config>(payload, address); });
  }

  /**
   * @brief Checks whether a slave acknowledges its address without transferring payload bytes.
   * @param address 7-bit or 10-bit slave address, as defined by @ref Config::address_mode.
   * @return @ref status::ok on ACK, @ref status::io_error when the address is not acknowledged,
   *         or @ref status::unsupported if the backend cannot perform an address-only probe.
   */
  [[nodiscard]] static inline status probe(const std::uint16_t address) noexcept
  {
    const std::uint16_t max_address = (Config::address_mode == i2c_address_mode::seven_bit) ? 0x7FU : 0x3FFU;
    if (address > max_address)
    {
      return status::invalid_argument;
    }

    if constexpr (mode != transfer_mode::automatic)
      return mode_api::template probe<Config>(address);
    else
      return detail::as_status([address] { return i2c_api_t<Backend>::template probe<Config>(address); });
  }

  /**
   * @brief Writes a command or register address, then reads data without releasing the bus.
   * @details The backend must issue a repeated START between the write and read phases. A backend
   *          timeout is reported as @ref status::timeout, bus/address failures as @ref status::io_error,
   *          and absent support as @ref status::unsupported. Backends with configurable deadlines use
   *          @c Config::timeout_ms; drivers with only native blocking calls use their native timeout.
   */
  [[nodiscard]] static inline status write_read_blocking(const std::uint16_t address,
                                                         const std::span<const std::uint8_t> tx_payload,
                                                         const std::span<std::uint8_t> rx_payload) noexcept
  {
    const std::uint16_t max_address = (Config::address_mode == i2c_address_mode::seven_bit) ? 0x7FU : 0x3FFU;
    if ((address > max_address) || tx_payload.empty() || rx_payload.empty())
    {
      return status::invalid_argument;
    }

    if constexpr (mode != transfer_mode::automatic)
      return mode_api::template write_read<Config, nullptr>(tx_payload, rx_payload, address);
    else
      return detail::as_status(
          [address, tx_payload, rx_payload]
          { return i2c_api_t<Backend>::template write_read_blocking<Config>(tx_payload, rx_payload, address); });
  }

  /**
   * @brief Starts a write followed by a repeated-START read.
   * @details The completion handler is called after the read finishes or an error occurs. Backends
   *          without asynchronous combined-transfer support return @ref status::unsupported. Both
   *          buffers must remain valid until completion; on interrupt-driven backends, the handler
   *          runs in the completion interrupt context.
   */
  template <auto CompletionHandler>
  [[nodiscard]] static inline status write_read_async(const std::uint16_t address,
                                                      const std::span<const std::uint8_t> tx_payload,
                                                      const std::span<std::uint8_t> rx_payload) noexcept
    requires IsrCapableConcept<Backend, peripheral_kind::i2c>
  {
    static_assert(mode == transfer_mode::automatic || valid_serial_completion_handler<CompletionHandler>());
    const std::uint16_t max_address = (Config::address_mode == i2c_address_mode::seven_bit) ? 0x7FU : 0x3FFU;
    if ((address > max_address) || tx_payload.empty() || rx_payload.empty())
    {
      return status::invalid_argument;
    }

    if constexpr (mode != transfer_mode::automatic)
      return mode_api::template write_read<Config, CompletionHandler>(tx_payload, rx_payload, address);
    else
      return detail::as_status(
          [address, tx_payload, rx_payload] {
            return i2c_api_t<Backend>::template write_read_async<Config, CompletionHandler>(tx_payload, rx_payload,
                                                                                            address);
          });
  }

  /**
   * @brief Starts transmit using the selected asynchronous engine (legacy interrupt path in automatic mode).
   * @tparam CompletionHandler Callback notified by the backend when the transfer completes.
   * @param address 7-bit or 10-bit slave address, as defined by @ref Config::address_mode.
   * @param payload Transfer buffer used by the backend.
   */
  template <auto CompletionHandler>
  [[nodiscard]] static inline status start_transfer(const std::uint16_t address,
                                                    const std::span<std::uint8_t> payload) noexcept
    requires IsrCapableConcept<Backend, peripheral_kind::i2c>
  {
    static_assert(mode == transfer_mode::automatic || valid_serial_completion_handler<CompletionHandler>());
    const std::uint16_t max_address = (Config::address_mode == i2c_address_mode::seven_bit) ? 0x7FU : 0x3FFU;
    if ((address > max_address) || payload.empty())
    {
      return status::invalid_argument;
    }

    if constexpr (mode != transfer_mode::automatic)
      return mode_api::template write<Config, CompletionHandler>(payload, address);
    else
      return detail::as_status(
          [address, payload]
          { return i2c_api_t<Backend>::template start_transfer<Config, CompletionHandler>(payload, address); });
  }

  template <auto CompletionHandler>
  [[nodiscard]] static status write_async(const std::uint16_t address,
                                          const std::span<const std::uint8_t> payload) noexcept
  {
    static_assert(valid_serial_completion_handler<CompletionHandler>());
    if (address > (Config::address_mode == i2c_address_mode::seven_bit ? 0x7FU : 0x3FFU) || payload.empty())
      return status::invalid_argument;
    return mode_api::template write<Config, CompletionHandler>(payload, address);
  }

  template <auto CompletionHandler>
  [[nodiscard]] static status read_async(const std::uint16_t address, const std::span<std::uint8_t> payload) noexcept
  {
    static_assert(valid_serial_completion_handler<CompletionHandler>());
    if (address > (Config::address_mode == i2c_address_mode::seven_bit ? 0x7FU : 0x3FFU) || payload.empty())
      return status::invalid_argument;
    return mode_api::template read<Config, CompletionHandler>(payload, address);
  }

  [[nodiscard]] static status abort() noexcept
  {
    if constexpr (mode == transfer_mode::automatic && !std::same_as<Backend, nordic_nrf_tag>)
      return status::unsupported;
    else
      return mode_api::template abort<Config>();
  }
  [[nodiscard]] static status transfer_status() noexcept
  {
    if constexpr (mode == transfer_mode::automatic && !std::same_as<Backend, nordic_nrf_tag>)
      return status::unsupported;
    else
      return mode_api::template transfer_status<Config>();
  }
  [[nodiscard]] static status handle_interrupt() noexcept
  {
    if constexpr (mode == transfer_mode::automatic && !std::same_as<Backend, nordic_nrf_tag>)
      return status::unsupported;
    else
      return detail::as_status([] { return mode_api::template handle_interrupt<Config>(); });
  }
};

/** @brief Fixed-length I2C slave configuration. OwnAddress is unshifted. */
template <std::uint8_t InstanceIndex, std::uint16_t OwnAddress,
          i2c_address_mode AddressMode = i2c_address_mode::seven_bit, std::uint32_t ClockHz = 100000U,
          bool EnableDma = false, bool EnableInterrupts = false, std::uint32_t TimeoutMs = 1000U,
          transfer_mode Mode = transfer_mode::interrupt>
struct slave_config final
{
  static_assert(OwnAddress <= (AddressMode == i2c_address_mode::seven_bit ? 0x7FU : 0x3FFU));
  static_assert(TimeoutMs > 0U);
  static constexpr bool is_slave = true;
  static constexpr auto instance_index = InstanceIndex;
  static constexpr auto own_address = OwnAddress;
  static constexpr auto address_mode = AddressMode;
  static constexpr auto clock_hz = ClockHz;
  static constexpr auto transfer_mode_value = Mode;
  static constexpr bool enable_dma = Mode == transfer_mode::dma || (Mode == transfer_mode::automatic && EnableDma);
  static constexpr bool enable_interrupts =
      Mode == transfer_mode::interrupt || (Mode == transfer_mode::automatic && EnableInterrupts);
  static constexpr auto timeout_ms = TimeoutMs;
};

template <typename Backend, typename Config>
  requires I2cMasterConcept<Backend, Config>
struct slave final
{
  // Async buffers remain owned until completion or a successful abort.
  static_assert(serial_slave_enabled<Config>());
  using backend_tag = Backend;
  using config_type = Config;
  using mode_api = serial_mode_api<Backend, peripheral_kind::i2c>;
  static constexpr auto kind = peripheral_kind::i2c;
  static constexpr auto mode = configured_transfer_mode<Config>();
  [[nodiscard]] static status initialize() noexcept
  {
    return mode_api::template initialize<Config>();
  }
  template <auto Handler> [[nodiscard]] static status receive_async(std::span<std::uint8_t> bytes) noexcept
  {
    static_assert(valid_serial_completion_handler<Handler>());
    return mode_api::template read<Config, Handler>(bytes);
  }
  template <auto Handler> [[nodiscard]] static status transmit_async(std::span<const std::uint8_t> bytes) noexcept
  {
    static_assert(valid_serial_completion_handler<Handler>());
    return mode_api::template write<Config, Handler>(bytes);
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
    return mode_api::template handle_interrupt<Config>();
  }
  [[nodiscard]] static status handle_dma_interrupt() noexcept
  {
    return mode_api::template handle_dma_interrupt<Config>();
  }
};
} // namespace microhal::i2c
