// SPDX-License-Identifier: Apache-2.0
/**
 * @file backend_selection_test.cpp
 * @brief Verifies that microHAL compiles when only a subset of backends is enabled.
 * @details The other test translation units name every backend tag, so they can only build in the
 *          default "all backends" configuration. This one is deliberately backend-agnostic: it
 *          includes the full core surface and every peripheral facade, but never spells a concrete
 *          backend tag. It therefore builds under any value of `MICROHAL_BACKENDS`, which is what
 *          catches headers that only compile because some other backend was included first.
 */

#include "microhal/core/backend_config.hpp"
#include "microhal/core/backend_selector.hpp"
#include "microhal/core/capabilities.hpp"
#include "microhal/core/concepts.hpp"
#include "microhal/core/status.hpp"
#include "microhal/core/traits.hpp"

#include "microhal/adc/adc.hpp"
#include "microhal/can/can.hpp"
#include "microhal/crc/crc.hpp"
#include "microhal/dac/dac.hpp"
#include "microhal/dma/dma.hpp"
#include "microhal/ethernet/ethernet.hpp"
#include "microhal/flash/flash.hpp"
#include "microhal/gpio/gpio.hpp"
#include "microhal/i2c/i2c.hpp"
#include "microhal/i2s/i2s.hpp"
#include "microhal/pwm/pwm.hpp"
#include "microhal/qspi/qspi.hpp"
#include "microhal/rcc/rcc.hpp"
#include "microhal/rtc/rtc.hpp"
#include "microhal/sd/sd.hpp"
#include "microhal/spi/spi.hpp"
#include "microhal/timers/timer.hpp"
#include "microhal/uart/uart.hpp"
#include "microhal/usb/usb.hpp"
#include "microhal/watchdog/watchdog.hpp"

// The registry only contains the backends actually compiled in, so its size tracks the CMake
// MICROHAL_BACKENDS selection.
static_assert(microhal::backend_registry.size() == MICROHAL_EXPECTED_BACKEND_COUNT,
              "backend_registry must contain exactly the backends selected at configure time");
static_assert(!microhal::backend_registry.empty(), "at least one backend must always be compiled in");

// Every registered backend must carry a distinct, non-empty name.
consteval bool names_are_unique() noexcept
{
  for (std::size_t outer = 0U; outer < microhal::backend_registry.size(); ++outer)
  {
    if (microhal::backend_registry[outer].name.empty())
    {
      return false;
    }
    for (std::size_t inner = outer + 1U; inner < microhal::backend_registry.size(); ++inner)
    {
      if (microhal::backend_registry[outer].name == microhal::backend_registry[inner].name)
      {
        return false;
      }
    }
  }
  return true;
}
static_assert(names_are_unique(), "backend descriptors must have unique, non-empty names");

// The status vocabulary must be usable without any backend at all.
static_assert(microhal::is_ok(microhal::status::ok));
static_assert(!microhal::is_ok(microhal::status::timeout));
static_assert(microhal::to_string(microhal::status::io_error) == "io_error");
static_assert(microhal::result<std::uint32_t>{microhal::status::busy}.value_or(9U) == 9U);
static_assert(microhal::result<std::uint32_t>{std::uint32_t{3U}}.value_or(9U) == 3U);

int main()
{
  return 0;
}
