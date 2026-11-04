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

  [[nodiscard]] static constexpr bool is_valid(const date_time value) noexcept
  {
    return (value.hour < 24U) && (value.minute < 60U) && (value.second < 60U);
  }

  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return rtc_api_t<Backend>::template initialize<Config>(); });
  }

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

  [[nodiscard]] static inline result<date_time> read() noexcept
  {
    return rtc_api_t<Backend>::template get_date_time<Config>();
  }

  [[nodiscard]] static result<std::uint32_t> read_subsecond_us() noexcept
  {
    return rtc_api_t<Backend>::template read_subsecond_us<Config>();
  }

  template <auto Handler>
  [[nodiscard]] static status enable_periodic_irq(std::uint32_t frequency_hz) noexcept
    requires IsrCapableConcept<Backend, peripheral_kind::rtc>
  {
    static_assert(valid_serial_completion_handler<Handler>());
    if (frequency_hz == 0U)
      return status::invalid_argument;
    return rtc_api_t<Backend>::template enable_periodic_irq<Config, Handler>(frequency_hz);
  }

  [[nodiscard]] static status disable_periodic_irq() noexcept
  {
    return rtc_api_t<Backend>::template disable_periodic_irq<Config>();
  }

  [[nodiscard]] static status handle_interrupt() noexcept
  {
    return rtc_api_t<Backend>::template handle_interrupt<Config>();
  }

  [[nodiscard]] static inline date_time now() noexcept
  {
    std::uint8_t hours = 0U;
    std::uint8_t minutes = 0U;
    std::uint8_t seconds = 0U;
    rtc_api_t<Backend>::template get_time<Config>(hours, minutes, seconds);
    return {.hour = hours, .minute = minutes, .second = seconds};
  }

  template <auto Handler>
  [[nodiscard]] static inline status enable_alarm(const date_time alarm_time) noexcept
    requires IsrCapableConcept<Backend, peripheral_kind::rtc>
  {
    return detail::as_status([alarm_time]
                             { return rtc_api_t<Backend>::template enable_alarm<Config, Handler>(alarm_time); });
  }
};

} // namespace microhal::rtc
