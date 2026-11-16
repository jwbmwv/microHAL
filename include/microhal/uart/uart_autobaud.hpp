// SPDX-License-Identifier: Apache-2.0
/**
 * @file uart_autobaud.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <cstdint>

#include "microhal/gpio/gpio.hpp"
#include "microhal/timers/timer.hpp"
#include "microhal/uart/uart.hpp"

namespace microhal::uart
{

/**
 * @brief Compile-time description of the resources used by the generic autobaud helper.
 */
template <typename GpioPin, typename TimerConfig, std::uint32_t MinBaud, std::uint32_t MaxBaud,
          bool UseDedicatedSensePin = false, bool ReconfigureRxPinAsGpio = true>
struct autobaud_config final
{
  using gpio_pin = GpioPin;
  using timer_config = TimerConfig;

  static constexpr std::uint32_t min_baud{MinBaud};
  static constexpr std::uint32_t max_baud{MaxBaud};
  static constexpr bool use_dedicated_sense_pin{UseDedicatedSensePin};
  static constexpr bool reconfigure_rx_pin_as_gpio{ReconfigureRxPinAsGpio};
};

/**
 * @brief Result of timing a UART RX signal edge interval.
 */
struct autobaud_measurement final
{
  std::uint32_t bit_period_ticks{0U};
  std::uint32_t baud_rate{0U};
  bool stable{false};
};

/**
 * @brief ISR-oriented generic autobaud helper backed by GPIO edge sensing and timer capture.
 */
template <typename Backend, typename UartConfig, typename AutobaudConfig>
  requires AutobaudCapableConcept<Backend, UartConfig, AutobaudConfig>
struct autobaud_helper final
{
  using backend_tag = Backend;
  using uart_config = UartConfig;
  using gpio_pin = typename AutobaudConfig::gpio_pin;
  using timer_config = typename AutobaudConfig::timer_config;

  static constexpr bool uses_dedicated_sense_pin{AutobaudConfig::use_dedicated_sense_pin};
  static constexpr bool reconfigures_rx_pin{AutobaudConfig::reconfigure_rx_pin_as_gpio};

  /**
   * @brief Computes the minimum accepted timer interval for the configured maximum baud rate.
   * @return Integer timer ticks calculated as timer clock divided by @c AutobaudConfig::max_baud.
   */
  static consteval std::uint32_t minimum_ticks() noexcept
  {
    return timer_config::clock_hz / AutobaudConfig::max_baud;
  }

  /**
   * @brief Computes the maximum accepted timer interval for the configured minimum baud rate.
   * @return Integer timer ticks calculated as timer clock divided by @c AutobaudConfig::min_baud.
   */
  static consteval std::uint32_t maximum_ticks() noexcept
  {
    return timer_config::clock_hz / AutobaudConfig::min_baud;
  }

  /**
   * @brief Arms the backend's generic UART autobaud measurement path.
   * @return Backend arm status, including capability or hardware errors reported by the backend.
   * @note Requires @ref AutobaudCapableConcept support; call according to the backend's documented
   *       peripheral/interrupt setup.
   */
  [[nodiscard]] static inline status arm() noexcept
  {
    return detail::as_status([] { return uart_api_t<Backend>::template arm_autobaud<UartConfig, AutobaudConfig>(); });
  }

  /**
   * @brief Converts a captured edge interval to a baud estimate and range-stability result.
   * @param[in] delta_ticks  Timer ticks between the two edge captures.
   * @return Measurement containing the input interval, integer baud estimate (zero for a zero
   *         interval), and whether both interval and estimate lie within the configured baud range.
   */
  [[nodiscard]] static constexpr autobaud_measurement decode_from_ticks(const std::uint32_t delta_ticks) noexcept
  {
    const std::uint32_t baud = (delta_ticks == 0U) ? 0U : (timer_config::clock_hz / delta_ticks);
    const bool stable = (delta_ticks >= minimum_ticks()) && (delta_ticks <= maximum_ticks()) &&
                        (baud >= AutobaudConfig::min_baud) && (baud <= AutobaudConfig::max_baud);
    return autobaud_measurement{
        .bit_period_ticks = delta_ticks,
        .baud_rate = baud,
        .stable = stable,
    };
  }

  /**
   * @brief Decodes a timer capture edge interval and immediately notifies the supplied handler.
   * @tparam CompletionHandler  No-throw callable accepting one @ref autobaud_measurement.
   * @param[in] previous_capture_ticks  Timer value at the preceding capture.
   * @param[in] current_capture_ticks   Timer value at the current capture.
   * @note The unsigned difference is decoded and the handler is invoked inline in the caller's
   *       context (normally a timer capture ISR). This timer ISR path must remain bounded and
   *       the handler must be safe and non-blocking for that context.
   */
  template <auto CompletionHandler>
  static inline void on_edge(const std::uint32_t previous_capture_ticks,
                             const std::uint32_t current_capture_ticks) noexcept
    requires IsrCapableConcept<Backend, peripheral_kind::timer>
  {
    const auto measurement = decode_from_ticks(current_capture_ticks - previous_capture_ticks);
    CompletionHandler(measurement);
  }

  /**
   * @brief Passes the measured baud rate to the backend for application to the UART configuration.
   * @param[in] measurement  Measurement whose @c baud_rate is passed to the backend. The caller is
   *                         responsible for checking @c stable before applying it.
   * @return Backend application status.
   */
  [[nodiscard]] static inline status apply(const autobaud_measurement measurement) noexcept
  {
    return detail::as_status(
        [measurement] { return uart_api_t<Backend>::template apply_autobaud<UartConfig>(measurement.baud_rate); });
  }
};

} // namespace microhal::uart
