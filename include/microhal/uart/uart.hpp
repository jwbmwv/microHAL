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
 * @tparam DataBits  Requested data width; the backend capability model rejects unsupported widths.
 */
template <std::uint8_t InstanceIndex, std::uint32_t BaudRate, uart_parity Parity = uart_parity::none,
          uart_stop_bits StopBits = uart_stop_bits::one, bool EnableDma = false, bool EnableInterrupts = false,
          bool RequireFifo = false, bool RequireLpuart = false, std::uint32_t TimeoutMs = 1000U,
          transfer_mode Mode = transfer_mode::automatic, uart_data_bits DataBits = uart_data_bits::eight>
struct port_config final
{
  static constexpr std::uint8_t instance_index{InstanceIndex};
  static constexpr std::uint32_t baud_rate{BaudRate};
  static constexpr uart_parity parity{Parity};
  static constexpr uart_stop_bits stop_bits{StopBits};
  static constexpr uart_data_bits data_bits{DataBits};
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

  /**
   * @brief Starts a finite receive that completes on UART IDLE or when the buffer is full.
   * @tparam Handler  No-throw callback accepting status and the received mutable byte span.
   * @param[out] buffer  Receive storage; keep it valid until the completion callback or successful abort.
   * @return Submission status from the selected backend mode.
   * @note This API is available only when supported by the selected mode/backend. The receive callback
   *       normally runs in interrupt context and must not block.
   */
  template <auto Handler> [[nodiscard]] static status start_receive_to_idle(std::span<std::uint8_t> buffer) noexcept
  {
    static_assert(valid_uart_receive_handler<Handler>());
    return mode_api::template start_receive_to_idle<Config, Handler>(buffer);
  }
  /**
   * @brief Starts continuous circular-DMA reception.
   * @tparam Handler  No-throw callback accepting status and the newly received mutable byte span.
   * @param[out] buffer  Circular receive storage; keep it valid until successful abort.
   * @return Submission status from the selected backend mode.
   * @note Requires backend DMA support for circular RX. Callback spans refer to newly received bytes
   *       and are valid only until DMA overwrites that range. The callback normally runs in interrupt
   *       context; consume or copy the data there without blocking.
   */
  template <auto Handler> [[nodiscard]] static status start_circular_rx(std::span<std::uint8_t> buffer) noexcept
  {
    static_assert(valid_uart_receive_handler<Handler>());
    return mode_api::template start_circular_rx<Config, Handler>(buffer);
  }
  /**
   * @brief Attempts to cancel UART transmission without changing receive activity.
   * @return Backend cancellation status. A failed abort retains TX buffer ownership; recover the
   *         hardware and retry before reusing that storage.
   */
  [[nodiscard]] static status abort_tx() noexcept
  {
    if constexpr (mode == transfer_mode::automatic && !std::same_as<Backend, nordic_nrf_tag>)
      return status::unsupported;
    else
      return mode_api::template abort_tx<Config>();
  }
  /**
   * @brief Attempts to cancel UART reception, including a circular stream, without changing TX.
   * @return Backend cancellation status. A failed abort retains RX buffer ownership; recover the
   *         hardware and retry before reusing that storage.
   */
  [[nodiscard]] static status abort_rx() noexcept
  {
    if constexpr (mode == transfer_mode::automatic && !std::same_as<Backend, nordic_nrf_tag>)
      return status::unsupported;
    else
      return mode_api::template abort_rx<Config>();
  }
  /**
   * @brief Queries the current or most recently completed TX operation.
   * @return Backend TX status, typically @ref status::busy while active.
   */
  [[nodiscard]] static status tx_status() noexcept
  {
    return mode_api::template tx_status<Config>();
  }
  /**
   * @brief Queries the current or most recently completed RX operation.
   * @return Backend RX status, typically @ref status::busy while active.
   */
  [[nodiscard]] static status rx_status() noexcept
  {
    return mode_api::template rx_status<Config>();
  }
  /**
   * @brief Dispatches the configured UART DMA interrupt.
   * @return Backend interrupt-dispatch status.
   * @note Call from the DMA ISR when required by the backend.
   */
  [[nodiscard]] static status handle_dma_interrupt() noexcept
  {
    return mode_api::template handle_dma_interrupt<Config>();
  }
  /**
   * @brief Dispatches a backend-specific UART RX timeout/IDLE interrupt.
   * @return Backend interrupt-dispatch status, or @ref status::unsupported if no such path exists.
   * @note Call from the RX timeout ISR when required by the backend.
   */
  [[nodiscard]] static status handle_rx_timeout_interrupt() noexcept
  {
    if constexpr (requires { mode_api::template handle_rx_timeout_interrupt<Config>(); })
      return detail::as_status([] { return mode_api::template handle_rx_timeout_interrupt<Config>(); });
    else
      return status::unsupported;
  }

  /**
   * @brief Initializes the configured UART, USART, or LPUART instance.
   * @return Initialization status, including backend errors or @ref status::unsupported when the
   *         selected configuration has no initialization path.
   */
  [[nodiscard]] static inline status initialize() noexcept
  {
    if constexpr (mode != transfer_mode::automatic)
      return mode_api::template initialize<Config>();
    else
      return detail::as_status([] { return uart_api_t<Backend>::template initialize<Config>(); });
  }

  /**
   * @brief Writes bytes using a blocking transmit path.
   * @param[in] payload  Bytes to transmit; storage is used only for the duration of this call.
   * @return Backend transfer status, including timeout where the selected backend enforces
   *         @c Config::timeout_ms.
   * @note Blocking calls may require live interrupt service and must not be called from an ISR.
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
   * @param[out] payload  Destination bytes; storage is used only for the duration of this call.
   * @return Backend transfer status, including timeout where the selected backend enforces
   *         @c Config::timeout_ms.
   * @note Blocking calls may require live interrupt service and must not be called from an ISR.
   */
  [[nodiscard]] static inline status read_blocking(const std::span<std::uint8_t> payload) noexcept
  {
    if constexpr (mode != transfer_mode::automatic)
      return mode_api::template read<Config, nullptr>(payload);
    else
      return detail::as_status([payload] { return uart_api_t<Backend>::template read_blocking<Config>(payload); });
  }

  /**
   * @brief Writes one byte and returns only after the configured blocking transfer completes.
   * @param[in] byte  Byte to transmit.
   * @return Result of the one-byte blocking write, or @ref status::unsupported when the configured
   *         mode cannot complete the operation synchronously.
   */
  [[nodiscard]] static inline status write_byte(const std::uint8_t byte) noexcept
  {
    if constexpr ((mode != transfer_mode::automatic) && (mode != transfer_mode::polled))
    {
      return status::unsupported;
    }
    else
    {
      return write_blocking(std::span<const std::uint8_t>{&byte, 1U});
    }
  }

  /**
   * @brief Reads one byte using the configured blocking receive path.
   * @param[out] byte  Receives the byte on successful completion.
   * @return Backend receive status, including timeout where the selected backend enforces the
   *         configured timeout.
   * @note Blocking calls may require live interrupt service and must not be called from an ISR.
   */
  [[nodiscard]] static inline status read_byte(std::uint8_t& byte) noexcept
  {
    return read_blocking(std::span<std::uint8_t>{&byte, 1U});
  }

  /**
   * @brief Starts receive using the selected asynchronous engine (legacy interrupt path in automatic mode).
   * @tparam CompletionHandler  No-throw callback accepting status or no arguments, as supported by
   *                            the selected serial mode.
   * @param[out] payload  Destination buffer; keep it valid until completion or successful abort.
   * @return Submission status from the selected mode/backend.
   * @note Requires ISR-capable UART support. The callback normally runs in interrupt context and
   *       must not block.
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
   * @tparam CompletionHandler  No-throw callback accepting status or no arguments, as supported by
   *                            the selected serial mode.
   * @param[in] payload  Bytes to transmit; keep the buffer valid until completion or successful abort.
   * @return Submission status from the selected mode/backend.
   * @note Requires ISR-capable UART support. The callback normally runs in interrupt context and
   *       must not block.
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
  /**
   * @brief Attempts to cancel both outstanding UART directions.
   * @return Backend cancellation status. Retain active buffers unless cancellation succeeds;
   *         @ref status::unsupported is returned where automatic mode has no abort path.
   */
  [[nodiscard]] static status abort() noexcept
  {
    if constexpr (mode == transfer_mode::automatic && !std::same_as<Backend, nordic_nrf_tag>)
      return status::unsupported;
    else
      return mode_api::template abort<Config>();
  }
  /**
   * @brief Queries the configured-mode transfer status.
   * @return Backend status, or @ref status::unsupported in automatic mode.
   */
  [[nodiscard]] static status transfer_status() noexcept
  {
    if constexpr (mode == transfer_mode::automatic)
      return status::unsupported;
    else
      return mode_api::template transfer_status<Config>();
  }
  /**
   * @brief Dispatches the configured UART peripheral interrupt.
   * @return Backend interrupt-dispatch status, or @ref status::unsupported in automatic mode.
   * @note Call from the UART ISR when required by the backend.
   */
  [[nodiscard]] static status handle_interrupt() noexcept
  {
    if constexpr (mode == transfer_mode::automatic)
      return status::unsupported;
    else
      return detail::as_status([] { return mode_api::template handle_interrupt<Config>(); });
  }
};

} // namespace microhal::uart
