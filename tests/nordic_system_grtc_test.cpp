// SPDX-License-Identifier: Apache-2.0
// Exercise the real GRTC engine, including a 52-bit SYSCOUNTER wrap.
#include "microhal/backends/nordic/nrfx/calendar.hpp"
#include "microhal/core/rtc_types.hpp"
#include "microhal/core/status.hpp"
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <iterator>

#define MICROHAL_NORDIC_HAS_RTC 0
#define MICROHAL_NORDIC_HAS_GRTC 1
#define NRFY_GRTC_HAS_EXTENDED 1
#define NRF_GRTC_SYSCOUNTER_CCH_MASK 0xFFFFFU
struct nrfx_grtc_channel_t
{
  void (*handler)(std::int32_t, std::uint64_t, void*){};
  void* p_context{};
  std::uint8_t channel{};
};
static constexpr std::uint64_t mask{(1ULL << 52U) - 1U};
static std::uint64_t counter{mask - 499999U};
static bool initialized{}, active{};
static std::uint8_t allocated{};
static std::uint64_t compare[2]{};
static nrfx_grtc_channel_t callbacks[2]{};
static bool enabled[2]{};
static std::uint32_t __get_PRIMASK()
{
  return 0U;
}
static void __disable_irq() {}
static void __set_PRIMASK(std::uint32_t) {}
static bool nrfx_grtc_init_check()
{
  return initialized;
}
static int nrfx_grtc_init(std::uint8_t)
{
  initialized = true;
  return 0;
}
static int nrfx_grtc_syscounter_start(bool, std::uint8_t*)
{
  return -EALREADY;
}
static void nrfx_grtc_active_request_set(bool value)
{
  active = value;
}
static bool nrfx_grtc_ready_check()
{
  return true;
}
static std::uint64_t nrfx_grtc_syscounter_get()
{
  return counter;
}
static int nrfx_grtc_channel_alloc(std::uint8_t* output)
{
  if (allocated == 2U)
    return -ENOMEM;
  *output = allocated++;
  return 0;
}
static int nrfx_grtc_channel_free(std::uint8_t)
{
  return 0;
}
static int nrfx_grtc_syscounter_cc_absolute_set(nrfx_grtc_channel_t* channel, std::uint64_t when, bool irq)
{
  assert(when <= mask);
  compare[channel->channel] = when;
  callbacks[channel->channel] = *channel;
  enabled[channel->channel] = irq;
  return 0;
}
static int nrfx_grtc_syscounter_cc_disable(std::uint8_t channel)
{
  enabled[channel] = false;
  return 0;
}
static void nrfx_grtc_irq_handler() {}
namespace microhal::backends::nordic::nrfx
{
namespace detail
{
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
};
static int notifications{};
static void handler(microhal::status value) noexcept
{
  assert(value == microhal::status::ok);
  ++notifications;
}
int main()
{
  using microhal::status;
  using api = nordic::rtc_api;
  assert(api::initialize<config>() == status::ok && active && allocated == 2U);
  assert(api::set_time<config>(23U, 59U, 59U) == status::ok);
  counter = (counter + 1000000U) & mask;
  const auto time = api::get_date_time<config>();
  assert(time && time.value().hour == 0U && time.value().second == 0U);
  assert(api::read_subsecond_us<config>().value() == 0U);
  assert((api::enable_periodic_irq<config, handler>(10U) == status::ok));
  assert(compare[1] == counter + 100000U);
  counter = compare[1];
  callbacks[1].handler(1, counter, callbacks[1].p_context);
  assert(notifications == 1 && compare[1] == counter + 100000U);
  assert(api::disable_periodic_irq<config>() == status::ok && !enabled[1]);
  assert(api::set_date_time<config>({2024U, 2U, 29U, 23U, 59U, 59U}) == status::ok);
  counter += 1000000U;
  const auto tomorrow = api::get_date_time<config>().value();
  assert(tomorrow.year == 2024U && tomorrow.month == 3U && tomorrow.day == 1U);
  assert((api::enable_alarm<config, handler>(microhal::rtc::date_time{.hour = 0U, .minute = 0U, .second = 1U}) ==
          status::ok));
  counter = compare[0];
  callbacks[0].handler(0, counter, callbacks[0].p_context);
  assert(notifications == 2 && !enabled[0]);
}
