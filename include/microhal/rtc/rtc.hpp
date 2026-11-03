// SPDX-License-Identifier: Apache-2.0
/**
 * @file rtc.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <cstdint>
#include <type_traits>

#include "microhal/core/concepts.hpp"

namespace microhal::rtc
{

/**
 * @brief Unified calendar representation used by the RTC abstraction.
 */
struct date_time final
{
  std::uint16_t year{0U};
  std::uint8_t month{0U};
  std::uint8_t day{0U};
  std::uint8_t hour{0U};
  std::uint8_t minute{0U};
  std::uint8_t second{0U};
};

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
      return status::unsupported;
    }
    return detail::as_status(
        [value] { return rtc_api_t<Backend>::template set_time<Config>(value.hour, value.minute, value.second); });
  }

  [[nodiscard]] static inline result<date_time> read() noexcept
  {
    std::uint8_t hours = 0U;
    std::uint8_t minutes = 0U;
    std::uint8_t seconds = 0U;
    using return_type = decltype(rtc_api_t<Backend>::template get_time<Config>(hours, minutes, seconds));
    if constexpr (std::is_same_v<std::remove_cv_t<return_type>, status>)
    {
      const status read_status = rtc_api_t<Backend>::template get_time<Config>(hours, minutes, seconds);
      if (read_status != status::ok)
      {
        return read_status;
      }
    }
    else if constexpr (std::is_same_v<std::remove_cv_t<return_type>, bool>)
    {
      if (!rtc_api_t<Backend>::template get_time<Config>(hours, minutes, seconds))
      {
        return status::io_error;
      }
    }
    else
    {
      rtc_api_t<Backend>::template get_time<Config>(hours, minutes, seconds);
    }
    return date_time{.hour = hours, .minute = minutes, .second = seconds};
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
