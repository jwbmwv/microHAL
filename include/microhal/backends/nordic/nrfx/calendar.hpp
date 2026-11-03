// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>

namespace microhal::backends::nordic::nrfx::calendar
{
constexpr bool leap(std::uint32_t year) noexcept
{
  return year % 4U == 0U && (year % 100U != 0U || year % 400U == 0U);
}
constexpr std::uint8_t month_days(std::uint32_t year, std::uint8_t month) noexcept
{
  constexpr std::uint8_t days[]{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  return month >= 1U && month <= 12U ? days[month - 1U] + (month == 2U && leap(year) ? 1U : 0U) : 0U;
}
template <typename Date> constexpr bool valid(const Date& date) noexcept
{
  return date.year >= 1970U && date.year <= 9999U && date.month >= 1U && date.month <= 12U && date.day >= 1U &&
         date.day <= month_days(date.year, date.month) && date.hour < 24U && date.minute < 60U && date.second < 60U;
}
template <typename Date> constexpr std::uint64_t seconds(const Date& date) noexcept
{
  std::uint64_t days{};
  for (std::uint32_t year = 1970U; year < date.year; ++year)
    days += leap(year) ? 366U : 365U;
  for (std::uint8_t month = 1U; month < date.month; ++month)
    days += month_days(date.year, month);
  days += date.day - 1U;
  return days * 86400U + date.hour * 3600U + date.minute * 60U + date.second;
}
template <typename Date> constexpr void split(std::uint64_t seconds, Date& date) noexcept
{
  auto days = seconds / 86400U;
  date.hour = static_cast<std::uint8_t>((seconds / 3600U) % 24U);
  date.minute = static_cast<std::uint8_t>((seconds / 60U) % 60U);
  date.second = static_cast<std::uint8_t>(seconds % 60U);
  std::uint32_t year = 1970U;
  while (days >= (leap(year) ? 366U : 365U))
  {
    days -= leap(year) ? 366U : 365U;
    ++year;
  }
  date.year = static_cast<std::uint16_t>(year);
  date.month = 1U;
  while (days >= month_days(year, date.month))
  {
    days -= month_days(year, date.month);
    ++date.month;
  }
  date.day = static_cast<std::uint8_t>(days + 1U);
}
} // namespace microhal::backends::nordic::nrfx::calendar
