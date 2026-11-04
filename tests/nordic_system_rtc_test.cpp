// SPDX-License-Identifier: Apache-2.0
// Exercise the real RTC engine against a 24-bit, 32768 Hz counter model.
#include "microhal/backends/nordic/nrfx/calendar.hpp"
#include "microhal/core/rtc_types.hpp"
#include "microhal/core/status.hpp"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

#define MICROHAL_NORDIC_HAS_RTC 1
#define MICROHAL_NORDIC_HAS_GRTC 0
enum nrf_rtc_event_t
{
  NRF_RTC_EVENT_OVERFLOW,
  NRF_RTC_EVENT_COMPARE_0,
  NRF_RTC_EVENT_COMPARE_1,
  NRF_RTC_EVENT_TICK
};
struct registers
{
  std::uint32_t ticks{};
  bool overflow{};
  std::uint32_t compare[2]{};
  bool enabled[2]{};
  bool tick_enabled{};
};
struct nrfx_rtc_t
{
  registers* p_reg;
};
struct nrfx_rtc_config_t
{
  std::uint32_t prescaler{};
  std::uint8_t interrupt_priority{};
  void* p_context{};
};
static registers hardware{};
static std::uint32_t interrupt_mask{};
static std::uint32_t __get_PRIMASK()
{
  return interrupt_mask;
}
static void __disable_irq()
{
  interrupt_mask = 1U;
}
static void __set_PRIMASK(std::uint32_t value)
{
  interrupt_mask = value;
}
static std::uint32_t nrfx_rtc_counter_get(nrfx_rtc_t* rtc)
{
  return rtc->p_reg->ticks;
}
static bool nrf_rtc_event_check(registers* rtc, nrf_rtc_event_t)
{
  return rtc->overflow;
}
static int nrfx_rtc_cc_set(nrfx_rtc_t* rtc, std::uint8_t slot, std::uint32_t ticks, bool enable)
{
  rtc->p_reg->compare[slot] = ticks;
  rtc->p_reg->enabled[slot] = enable;
  return 0;
}
static int nrfx_rtc_cc_disable(nrfx_rtc_t* rtc, std::uint8_t slot)
{
  rtc->p_reg->enabled[slot] = false;
  return 0;
}
static int nrfx_rtc_init(nrfx_rtc_t*, nrfx_rtc_config_t const* config, void (*)(nrf_rtc_event_t, void*))
{
  assert(config->prescaler == 0U);
  return 0;
}
static void nrfx_rtc_overflow_enable(nrfx_rtc_t*, bool) {}
static void nrfx_rtc_enable(nrfx_rtc_t*) {}
static void nrfx_rtc_tick_enable(nrfx_rtc_t*, bool)
{
  hardware.tick_enabled = true;
}
static void nrfx_rtc_tick_disable(nrfx_rtc_t*)
{
  hardware.tick_enabled = false;
}
static void nrfx_rtc_irq_handler(nrfx_rtc_t*) {}
namespace microhal::backends::nordic::nrfx
{
namespace detail
{
inline nrfx_rtc_t rtc_instances[]{{&hardware}};
constexpr status driver_result(int value)
{
  return value == 0 ? status::ok : status::error;
}
template <auto Handler> void notify(status value)
{
  Handler(value);
}
} // namespace detail
#include "microhal/backends/nordic/nrfx/rtc_engine.inc"
} // namespace microhal::backends::nordic::nrfx
namespace nordic = microhal::backends::nordic::nrfx;
struct config
{
  static constexpr std::uint8_t instance_index{0U};
};
static int alarms{}, periodic{};
static void alarm_handler(microhal::status result) noexcept
{
  assert(result == microhal::status::ok);
  ++alarms;
}
static void periodic_handler(microhal::status result) noexcept
{
  assert(result == microhal::status::ok);
  ++periodic;
}
static void compare(std::uint8_t slot)
{
  assert(hardware.enabled[slot]);
  hardware.ticks = hardware.compare[slot];
  // nrfx disables compare interrupts before invoking its client callback.
  hardware.enabled[slot] = false;
  nordic::system_detail::event(0U, slot);
}
int main()
{
  using microhal::status;
  using api = nordic::rtc_api;
  assert(api::get_date_time<config>().error() == status::unsupported);
  assert(api::initialize<config>() == status::ok);
  assert(api::initialize<config>() == status::busy);
  assert(api::set_date_time<config>({2024, 2, 30}) == status::invalid_argument);
  assert(api::set_date_time<config>({2024, 1, 1}) == status::ok);
  constexpr microhal::rtc::date_time alarm{2024, 2, 1};
  assert((api::enable_alarm<config, alarm_handler>(alarm) == status::ok));
  compare(0U);
  assert(alarms == 0 && hardware.enabled[0]);
  // An early modulo compare must be rearmed, not lose an alarm beyond the 512-second wrap.
  const auto target_ticks = 31ULL * 86400U * 32768U;
  nordic::system_detail::clocks[0].high_ticks = (target_ticks & ~0xFFFFFFULL) - (1ULL << 24U);
  hardware.ticks = 0U;
  hardware.overflow = true;
  const auto pending = api::get_date_time<config>();
  assert(pending && pending.value().year == 2024U);
  hardware.overflow = false;
  nordic::system_detail::rtc_event(NRF_RTC_EVENT_OVERFLOW, &nordic::system_detail::clocks[0]);
  compare(0U);
  assert(alarms == 1 && !hardware.enabled[0]);
  const auto date = api::get_date_time<config>().value();
  assert(date.year == 2024U && date.month == 2U && date.day == 1U);
  assert((api::enable_periodic_irq<config, periodic_handler>(3U) == status::invalid_argument));
  assert((api::enable_periodic_irq<config, periodic_handler>(8U) == status::ok));
  compare(1U);
  assert(periodic == 1 && hardware.enabled[1]);
  assert(api::read_subsecond_us<config>().value() == 125000U);
  assert(api::disable_periodic_irq<config>() == status::ok && !hardware.enabled[1]);
  assert((api::enable_periodic_irq<config, periodic_handler>(32768U) == status::ok && hardware.tick_enabled));
  ++hardware.ticks;
  nordic::system_detail::rtc_event(NRF_RTC_EVENT_TICK, &nordic::system_detail::clocks[0]);
  assert(periodic == 2);
  assert(api::disable_periodic_irq<config>() == status::ok && !hardware.tick_enabled);
  assert(api::set_time<config>(23U, 59U, 59U) == status::ok);
  hardware.ticks += 32768U;
  const auto midnight = api::get_date_time<config>().value();
  assert(midnight.year == 0U && midnight.month == 0U && midnight.day == 0U);
  assert(midnight.hour == 0U && midnight.minute == 0U && midnight.second == 0U);
  assert(interrupt_mask == 0U);
}
