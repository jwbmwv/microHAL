// SPDX-License-Identifier: Apache-2.0
#include "microhal/backends/nordic/nrfx/calendar.hpp"
#include <cassert>

namespace calendar = microhal::backends::nordic::nrfx::calendar;
struct date
{
  std::uint16_t year{};
  std::uint8_t month{}, day{}, hour{}, minute{}, second{};
};
static_assert(calendar::valid(date{2024, 2, 29}));
static_assert(!calendar::valid(date{2100, 2, 29}));
static_assert(calendar::valid(date{2000, 2, 29}));
static_assert(!calendar::valid(date{2024, 4, 31}));
static_assert(!calendar::valid(date{1969, 12, 31}));
static_assert(calendar::seconds(date{1970, 1, 1}) == 0U);
static_assert(calendar::seconds(date{1970, 1, 2}) == 86400U);
int main()
{
  for (std::uint16_t year = 1970U; year < 2400U; ++year)
    for (std::uint8_t month = 1U; month <= 12U; ++month)
      for (std::uint8_t day = 1U; day <= calendar::month_days(year, month); ++day)
      {
        const date input{year, month, day, 23U, 59U, 59U};
        date output{};
        calendar::split(calendar::seconds(input), output);
        assert(output.year == year && output.month == month && output.day == day);
        assert(output.hour == 23U && output.minute == 59U && output.second == 59U);
        date next{};
        calendar::split(calendar::seconds(input) + 1U, next);
        assert(calendar::valid(next) && next.hour == 0U && next.minute == 0U && next.second == 0U);
      }
}
