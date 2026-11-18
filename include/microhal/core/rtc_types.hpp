// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace microhal::rtc
{
struct date_time final
{
  std::uint16_t year{0U};
  std::uint8_t month{0U};
  std::uint8_t day{0U};
  std::uint8_t hour{0U};
  std::uint8_t minute{0U};
  std::uint8_t second{0U};
};
} // namespace microhal::rtc
