// SPDX-License-Identifier: Apache-2.0
/**
 * @file stm32_sd_block_io.cpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

// STM32 SD/MMC Host Example
// Demonstrates: host initialization and block-oriented read/write/erase calls.

#include "microhal/sd/sd.hpp"

#include <array>
#include <cstdint>

namespace
{

using backend = microhal::stm32_hal_tag;

struct sd_binding final
{
  [[maybe_unused]] static constexpr std::uintptr_t base_address{0x40012C00U};
  [[maybe_unused]] static constexpr std::uint32_t clock_divider{0U};
  [[maybe_unused]] static constexpr bool hardware_flow_control{false};
};

using sd_cfg = microhal::sd::bound_host_config<sd_binding, 25'000'000U, microhal::sd_bus_width::four,
                                               microhal::sd_timing_mode::default_speed, true, true>;
using sd_card = microhal::sd::card<backend, sd_cfg>;

std::array<std::uint8_t, 512U> read_buffer{};
std::array<std::uint8_t, 512U> write_buffer{{0xAAU, 0x55U}};

} // namespace

int main()
{
  static_assert(microhal::SdConcept<backend, sd_cfg>);

  if (!microhal::is_ok(sd_card::initialize()))
  {
    return 1;
  }

  if (!microhal::is_ok(sd_card::read_blocks(0U, read_buffer)) ||
      !microhal::is_ok(sd_card::write_blocks(1U, write_buffer)) || !microhal::is_ok(sd_card::erase_blocks(2U, 1U)))
  {
    return 1;
  }

  (void)sd_card::capacity_blocks();

  return 0;
}
