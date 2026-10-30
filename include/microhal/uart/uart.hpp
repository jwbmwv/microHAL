// SPDX-License-Identifier: Apache-2.0
/**
 * @file uart.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <span>

#include "microhal/core/concepts.hpp"

namespace microhal::uart
{

/**
 * @brief Compile-time UART, USART, or LPUART configuration.
 * @tparam TimeoutMs Bound applied to blocking transfers. Timeout policy belongs to the application,
 *         not the backend, so it is part of the configuration type.
 */
template <std::uint8_t InstanceIndex, std::uint32_t BaudRate, uart_parity Parity = uart_parity::none,
          uart_stop_bits StopBits = uart_stop_bits::one, bool EnableDma = false, bool EnableInterrupts = false,
          bool RequireFifo = false, bool RequireLpuart = false, std::uint32_t TimeoutMs = 1000U,
          transfer_mode Mode = transfer_mode::automatic>
struct port_config final
{
  static constexpr std::uint8_t instance_index{InstanceIndex};
  static constexpr std::uint32_t baud_rate{BaudRate};
  static constexpr uart_parity parity{Parity};
  static constexpr uart_stop_bits stop_bits{StopBits};
  static constexpr transfer_mode transfer_mode_value{Mode};
  static constexpr bool enable_dma{Mode == transfer_mode::dma || (Mode == transfer_mode::automatic && EnableDma)};
  static constexpr bool enable_interrupts{Mode == transfer_mode::interrupt ||
                                          (Mode == transfer_mode::automatic && EnableInterrupts)};
  static constexpr bool require_fifo{RequireFifo};
  static constexpr bool require_lpuart{RequireLpuart};
  static constexpr std::uint32_t timeout_ms{TimeoutMs};
};

/**
 * @brief Zero-overhead UART facade with compile-time FIFO, DMA, and ISR validation.
 */
template <typename Backend, typename Config>
  requires UartConcept<Backend, Config>
struct port final
{
  using backend_tag = Backend;
  using config_type = Config;

  static constexpr peripheral_kind kind{peripheral_kind::uart};
  static constexpr transfer_mode mode{configured_transfer_mode<Config>()};
  using mode_api = serial_mode_api<Backend, peripheral_kind::uart>;
  static constexpr bool has_fifo{backend_traits<Backend>::uart_capability().fifo};
  static constexpr bool has_lpuart{backend_traits<Backend>::uart_capability().lpuart};
  static constexpr bool has_generic_autobaud{backend_traits<Backend>::uart_capability().autobaud};

  /** @brief Initializes the selected UART/USART/LPUART instance. */
  [[nodiscard]] static inline status initialize() noexcept
  {
    if constexpr (mode != transfer_mode::automatic)
      return mode_api::template initialize<Config>();
    else
      return detail::as_status([] { return uart_api_t<Backend>::template initialize<Config>(); });
  }

  /**
   * @brief Writes bytes using a blocking transmit path.
   * @param payload Bytes to send.
   */
  [[nodiscard]] static inline status write_blocking(const std::span<const std::uint8_t> payload) noexcept
  {
    if constexpr (mode != transfer_mode::automatic)
      return mode_api::template write<Config, nullptr>(payload);
    else
      return detail::as_status([payload] { return uart_api_t<Backend>::template write_blocking<Config>(payload); });
  }

  /**
   * @brief Reads bytes using a blocking receive path.
   * @param payload Destination buffer to fill.
   */
  [[nodiscard]] static inline status read_blocking(const std::span<std::uint8_t> payload) noexcept
  {
    if constexpr (mode != transfer_mode::automatic)
      return mode_api::template read<Config, nullptr>(payload);
    else
      return detail::as_status([payload] { return uart_api_t<Backend>::template read_blocking<Config>(payload); });
  }

  /** @brief Writes one byte using the configured blocking transmit path. */
  [[nodiscard]] static inline status write_byte(const std::uint8_t byte) noexcept
  {
    return write_blocking(std::span<const std::uint8_t>{&byte, 1U});
  }

  /** @brief Reads one byte using the configured blocking receive path. */
  [[nodiscard]] static inline status read_byte(std::uint8_t& byte) noexcept
  {
    return read_blocking(std::span<std::uint8_t>{&byte, 1U});
  }

  /**
   * @brief Starts receive using the selected asynchronous engine (legacy interrupt path in automatic mode).
   * @tparam CompletionHandler Callback notified by the backend on completion.
   * @param payload Destination buffer used by the backend.
   */
  template <auto CompletionHandler>
  [[nodiscard]] static inline status start_async_rx(const std::span<std::uint8_t> payload) noexcept
    requires IsrCapableConcept<Backend, peripheral_kind::uart>
  {
    static_assert(mode == transfer_mode::automatic || valid_serial_completion_handler<CompletionHandler>());
    if constexpr (mode != transfer_mode::automatic)
      return mode_api::template read<Config, CompletionHandler>(payload);
    else
      return detail::as_status(
          [payload] { return uart_api_t<Backend>::template start_async_rx<Config, CompletionHandler>(payload); });
  }

  /**
   * @brief Starts transmit using the selected asynchronous engine (legacy interrupt path in automatic mode).
   * @tparam CompletionHandler Callback notified by the backend on completion.
   * @param payload Bytes to send.
   */
  template <auto CompletionHandler>
  [[nodiscard]] static inline status start_async_tx(const std::span<const std::uint8_t> payload) noexcept
    requires IsrCapableConcept<Backend, peripheral_kind::uart>
  {
    static_assert(mode == transfer_mode::automatic || valid_serial_completion_handler<CompletionHandler>());
    if constexpr (mode != transfer_mode::automatic)
      return mode_api::template write<Config, CompletionHandler>(payload);
    else
      return detail::as_status(
          [payload] { return uart_api_t<Backend>::template start_async_tx<Config, CompletionHandler>(payload); });
  }
  [[nodiscard]] static status abort() noexcept
  {
    if constexpr (mode == transfer_mode::automatic)
      return status::unsupported;
    else
      return mode_api::template abort<Config>();
  }
  [[nodiscard]] static status transfer_status() noexcept
  {
    if constexpr (mode == transfer_mode::automatic)
      return status::unsupported;
    else
      return mode_api::template transfer_status<Config>();
  }
  [[nodiscard]] static status handle_interrupt() noexcept
  {
    if constexpr (mode == transfer_mode::automatic)
      return status::unsupported;
    else
      return detail::as_status([] { return mode_api::template handle_interrupt<Config>(); });
  }
};

} // namespace microhal::uart
