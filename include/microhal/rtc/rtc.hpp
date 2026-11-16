// SPDX-License-Identifier: Apache-2.0
/**
 * @file rtc.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <cstdint>

#include "microhal/core/concepts.hpp"

namespace microhal::rtc
{

/**
 * @brief Compile-time RTC feature selection.
 */
template <bool EnablePeriodicIrq = false, bool EnableAlarmIrq = false> struct rtc_config final
{
  static constexpr bool enable_periodic_irq{EnablePeriodicIrq};
  static constexpr bool enable_alarm_irq{EnableAlarmIrq};
};

/**
 * @brief Zero-overhead RTC facade with compile-time alarm and periodic IRQ validation.
 */
template <typename Backend, typename Config>
  requires RtcConcept<Backend, Config>
struct controller final
{
  using backend_tag = Backend;
  using config_type = Config;

  static constexpr peripheral_kind kind{peripheral_kind::rtc};

  /**
   * @brief Checks whether the hour, minute, and second fields are within clock ranges.
   * @param[in] value Time value to validate.
   * @return True when hour is less than 24 and minute and second are less than 60.
   * @note Calendar fields are not checked by this helper.
   */
  [[nodiscard]] static constexpr bool is_valid(const date_time value) noexcept
  {
    return (value.hour < 24U) && (value.minute < 60U) && (value.second < 60U);
  }

  /**
   * @brief Initializes the configured real-time clock.
   * @return Backend initialization status.
   */
  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return rtc_api_t<Backend>::template initialize<Config>(); });
  }

  /**
   * @brief Sets the RTC time, and sets calendar fields when any are nonzero.
   * @param[in] value Time and optional date; hour, minute, and second must be in range.
   * @return @ref microhal::status::invalid_argument for an out-of-range time,
   *         @ref microhal::status::unsupported when date fields are requested without backend calendar support,
   *         or the backend set status.
   * @note Calendar-field validation is delegated to the backend.
   */
  [[nodiscard]] static inline status set(const date_time value) noexcept
  {
    if (!is_valid(value))
    {
      return status::invalid_argument;
    }
    if ((value.year != 0U) || (value.month != 0U) || (value.day != 0U))
    {
      if constexpr (backend_traits<Backend>::rtc_capability().calendar)
        return rtc_api_t<Backend>::template set_date_time<Config>(value);
      else
        return status::unsupported;
    }
    return detail::as_status(
        [value] { return rtc_api_t<Backend>::template set_time<Config>(value.hour, value.minute, value.second); });
  }

  /**
   * @brief Reads the RTC date and time.
   * @return Date/time value on success, or the backend read error.
   */
  [[nodiscard]] static inline result<date_time> read() noexcept
  {
    return rtc_api_t<Backend>::template get_date_time<Config>();
  }

  /**
   * @brief Reads the RTC subsecond position in microseconds.
   * @return Subsecond value on success, or the backend error when the read is unavailable or fails.
   */
  [[nodiscard]] static result<std::uint32_t> read_subsecond_us() noexcept
  {
    return rtc_api_t<Backend>::template read_subsecond_us<Config>();
  }

  /**
   * @brief Enables a periodic RTC interrupt at the requested frequency.
   * @tparam Handler Backend-compatible periodic callback, invoked from interrupt context.
   * @param[in] frequency_hz Interrupt frequency in hertz; zero is invalid.
   * @return @ref microhal::status::invalid_argument for zero frequency, or the backend setup status.
   * @note Requires an ISR-capable RTC backend. Keep the handler bounded, non-blocking, and noexcept; do not call
   *       blocking facade operations from it.
   */
  template <auto Handler>
  [[nodiscard]] static status enable_periodic_irq(std::uint32_t frequency_hz) noexcept
    requires IsrCapableConcept<Backend, peripheral_kind::rtc>
  {
    static_assert(valid_serial_completion_handler<Handler>());
    if (frequency_hz == 0U)
      return status::invalid_argument;
    return rtc_api_t<Backend>::template enable_periodic_irq<Config, Handler>(frequency_hz);
  }

  /**
   * @brief Disables the configured periodic RTC interrupt.
   * @return Backend interrupt-disable status.
   */
  [[nodiscard]] static status disable_periodic_irq() noexcept
  {
    return rtc_api_t<Backend>::template disable_periodic_irq<Config>();
  }

  /**
   * @brief Dispatches RTC interrupt handling to the backend.
   * @return Backend interrupt-handling status.
   * @note Call from the RTC interrupt path when required by the backend integration; the callback contract is
   *       interrupt-context, bounded, and non-blocking.
   */
  [[nodiscard]] static status handle_interrupt() noexcept
  {
    return rtc_api_t<Backend>::template handle_interrupt<Config>();
  }

  /**
   * @brief Reads the current RTC time-of-day.
   * @return Current hour, minute, and second; date fields are zero-initialized.
   * @note This convenience query does not expose backend read errors.
   */
  [[nodiscard]] static inline date_time now() noexcept
  {
    std::uint8_t hours = 0U;
    std::uint8_t minutes = 0U;
    std::uint8_t seconds = 0U;
    rtc_api_t<Backend>::template get_time<Config>(hours, minutes, seconds);
    return {.hour = hours, .minute = minutes, .second = seconds};
  }

  /**
   * @brief Enables an RTC alarm at the requested time.
   * @tparam Handler Backend-compatible alarm callback, invoked from interrupt context.
   * @param[in] alarm_time Alarm date/time passed to the backend.
   * @return Backend alarm-setup status.
   * @note Requires an ISR-capable RTC backend. Keep the handler bounded, non-blocking, and noexcept; do not call
   *       blocking facade operations from it. Validation of the alarm value is backend-defined.
   */
  template <auto Handler>
  [[nodiscard]] static inline status enable_alarm(const date_time alarm_time) noexcept
    requires IsrCapableConcept<Backend, peripheral_kind::rtc>
  {
    return detail::as_status([alarm_time]
                             { return rtc_api_t<Backend>::template enable_alarm<Config, Handler>(alarm_time); });
  }
};

} // namespace microhal::rtc
