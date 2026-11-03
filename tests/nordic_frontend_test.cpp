// SPDX-License-Identifier: Apache-2.0
#include "microhal/adc/adc.hpp"
#include "microhal/crc/crc.hpp"
#include "microhal/rtc/rtc.hpp"
#include "microhal/usb/usb.hpp"

#include <cstdio>
#include <limits>

namespace
{
using microhal::status;
struct calendar_tag
{
};
struct calendar_api
{
  static inline microhal::rtc::date_time current{};
  static inline status read_status{status::ok};
  static inline unsigned interrupts{}, callbacks{};
  template <typename Config> static status initialize() noexcept
  {
    return status::ok;
  }
  template <typename Config> static status set_date_time(microhal::rtc::date_time value) noexcept
  {
    current = value;
    return status::ok;
  }
  template <typename Config> static microhal::result<microhal::rtc::date_time> get_date_time() noexcept
  {
    if (read_status != status::ok)
      return read_status;
    return current;
  }
  template <typename Config>
  static status set_time(std::uint8_t hour, std::uint8_t minute, std::uint8_t second) noexcept
  {
    current.hour = hour;
    current.minute = minute;
    current.second = second;
    return status::ok;
  }
  template <typename Config> static microhal::result<std::uint32_t> read_subsecond_us() noexcept
  {
    return 125000U;
  }
  template <typename Config, auto Handler> static status enable_periodic_irq(std::uint32_t frequency) noexcept
  {
    if (frequency != 8U)
      return status::invalid_argument;
    Handler(status::ok);
    return status::ok;
  }
  template <typename Config> static status disable_periodic_irq() noexcept
  {
    return status::ok;
  }
  static void handle_interrupt(std::uint8_t instance) noexcept
  {
    interrupts += instance + 1U;
  }
};
struct usb_tag
{
};
struct usb_api
{
  static inline status state{status::busy};
  static inline bool failed_abort{}, stalled{};
  static inline unsigned controls{};
  template <typename Config> static status endpoint_status(std::uint8_t) noexcept
  {
    return state;
  }
  template <typename Config> static status endpoint_abort(std::uint8_t) noexcept
  {
    ++controls;
    if (failed_abort)
      return status::timeout;
    state = status::ok;
    return status::ok;
  }
  template <typename Config> static status endpoint_stall(std::uint8_t, bool enabled) noexcept
  {
    ++controls;
    stalled = enabled;
    return status::ok;
  }
  static void handle_interrupt() noexcept
  {
    ++controls;
  }
};
struct differential_tag
{
};
struct differential_api
{
  static inline std::int16_t sample{};
  static inline status error{status::ok};
  static inline unsigned reads{};
  template <typename Config> static microhal::result<std::uint16_t> read_blocking(std::uint8_t) noexcept
  {
    ++reads;
    return status::error;
  }
  template <typename Config> static microhal::result<std::int16_t> read_differential_blocking(std::uint8_t) noexcept
  {
    ++reads;
    if (error != status::ok)
      return error;
    return sample;
  }
};
struct entropy_tag
{
};
struct entropy_api
{
  static inline status error{status::ok};
  template <typename Config> static microhal::result<std::uint32_t> read_random() noexcept
  {
    if (error != status::ok)
      return error;
    return 0U;
  }
};
} // namespace

namespace microhal
{
template <> struct backend_traits<calendar_tag>
{
  using backend_tag = calendar_tag;
  using rtc_api = calendar_api;
  static constexpr std::string_view name{"calendar-fixture"};
  static constexpr vendor_kind vendor{vendor_kind::mock};
  static constexpr backend_kind kind{backend_traits<mock_tag>::kind};
  static consteval struct rtc_capability rtc_capability() noexcept
  {
    return {.supported = true, .isr = true, .periodic = true, .calendar = true, .subsecond = true};
  }
};
template <> struct backend_traits<usb_tag>
{
  using backend_tag = usb_tag;
  using usb_api = ::usb_api;
  static constexpr std::string_view name{"usb-fixture"};
  static constexpr vendor_kind vendor{vendor_kind::mock};
  static constexpr backend_kind kind{backend_traits<mock_tag>::kind};
  static consteval struct usb_capability usb_capability() noexcept
  {
    return {.supported = true,
            .isr = true,
            .instance_count = 1U,
            .device = true,
            .endpoint_count = 4U,
            .endpoint_transfer = true,
            .max_packet_size_bytes = 64U};
  }
};
template <> struct backend_traits<differential_tag>
{
  using backend_tag = differential_tag;
  using adc_api = differential_api;
  static constexpr std::string_view name{"differential-fixture"};
  static constexpr vendor_kind vendor{vendor_kind::mock};
  static constexpr backend_kind kind{backend_traits<mock_tag>::kind};
  static consteval struct adc_capability adc_capability() noexcept
  {
    return {.supported = true, .blocking = true, .channel_count = 1U, .differential = true, .voltage_conversion = true};
  }
};
template <> struct backend_traits<entropy_tag>
{
  using backend_tag = entropy_tag;
  using crc_api = entropy_api;
  static constexpr std::string_view name{"entropy-fixture"};
  static constexpr vendor_kind vendor{vendor_kind::mock};
  static constexpr backend_kind kind{backend_traits<mock_tag>::kind};
  static consteval struct crc_capability crc_capability() noexcept
  {
    return {.supported = true, .instance_count = 1U, .random_number_generator = true};
  }
};
} // namespace microhal

int main()
{
  int failures{};
  const auto check = [&failures](bool condition, const char* message)
  {
    if (!condition)
    {
      std::printf("FAIL: %s\n", message);
      ++failures;
    }
  };
  using rtc = microhal::rtc::controller<calendar_tag, microhal::rtc::rtc_config<true, false>>;
  const microhal::rtc::date_time leap{.year = 2024U, .month = 2U, .day = 29U, .hour = 12U};
  check(rtc::initialize() == status::ok, "initialize RTC facade");
  check(rtc::set(leap) == status::ok, "calendar reaches backend rather than being discarded");
  const auto read = rtc::read();
  check(read && read.value().year == 2024U && read.value().month == 2U && read.value().day == 29U,
        "read preserves calendar fields");
  check(rtc::set({.hour = 14U}) == status::ok && calendar_api::current.day == 29U,
        "time-only set preserves backend calendar");
  calendar_api::read_status = status::io_error;
  check(rtc::read().error() == status::io_error, "read propagates backend errors");
  check(rtc::read_subsecond_us().value() == 125000U, "subseconds have explicit microsecond units");
  constexpr auto callback = [](status) noexcept { ++calendar_api::callbacks; };
  check(rtc::enable_periodic_irq<callback>(0U) == status::invalid_argument, "reject zero periodic frequency");
  check(rtc::enable_periodic_irq<callback>(8U) == status::ok && calendar_api::callbacks == 1U,
        "dispatch periodic handler");
  check(rtc::disable_periodic_irq() == status::ok, "dispatch periodic cancellation");
  check(rtc::handle_interrupt() == status::ok && calendar_api::interrupts == 1U, "dispatch default instance IRQ");
  using legacy = microhal::rtc::controller<microhal::mock_tag, microhal::rtc::rtc_config<>>;
  check(legacy::set(leap) == status::unsupported, "legacy calendar limitation remains explicit");
  check(legacy::set({.hour = 11U, .minute = 30U}) == status::ok, "legacy time-only set remains supported");
  const auto legacy_read = legacy::read();
  check(legacy_read && legacy_read.value().hour == 11U && legacy_read.value().year == 0U,
        "legacy time-only read remains supported");
  check(legacy::read_subsecond_us().error() == status::unsupported, "unsupported subsecond returns an error");
  using usb = microhal::usb::device<usb_tag, microhal::usb::usb_config<0U>>;
  check(usb::endpoint_status(0x81U) == status::busy, "USB reports asynchronous ownership");
  usb_api::failed_abort = true;
  check(usb::endpoint_abort(0x81U) == status::timeout && usb::endpoint_status(0x81U) == status::busy,
        "USB abort failure is not disguised as completion");
  usb_api::failed_abort = false;
  check(usb::endpoint_abort(0x81U) == status::ok && usb::endpoint_status(0x81U) == status::ok,
        "USB abort dispatches cancellation");
  const auto calls = usb_api::controls;
  check(usb::endpoint_abort(0x84U) == status::invalid_argument &&
            usb::endpoint_stall(0x90U) == status::invalid_argument && usb_api::controls == calls,
        "USB rejects invalid endpoints before touching the driver");
  check(usb::endpoint_stall(0x81U) == status::ok && usb_api::stalled, "USB dispatches endpoint stall");
  check(usb::endpoint_stall(0x81U, false) == status::ok && !usb_api::stalled, "USB dispatches endpoint clear-stall");
  check(usb::handle_interrupt() == status::ok, "USB dispatches non-template driver IRQ");
  using adc_config = microhal::adc::adc_config<0U, microhal::adc_resolution::bits_12,
                                               microhal::adc_sample_time::cycles_15, false, false, true>;
  using adc = microhal::adc::converter<differential_tag, adc_config>;
  check(adc::read_millivolts(0U, 3300U).error() == status::unsupported,
        "unsigned conversion does not misreport a differential voltage");
  check(differential_api::reads == 0U, "invalid conversion does not start hardware");
  differential_api::sample = -2048;
  check(adc::read_differential_blocking(0U).value() == -2048, "signed counts preserve negative full scale");
  check(adc::read_differential_millivolts(0U, 3300U).value() == -3300, "negative full-scale voltage");
  differential_api::sample = 0;
  check(adc::read_differential_millivolts(0U, 3300U).value() == 0, "differential zero remains zero volts");
  differential_api::sample = 2047;
  check(adc::read_differential_millivolts(0U, 3300U).value() == 3298, "positive signed full-scale rounding");
  differential_api::sample = -1024;
  check(adc::read_differential_millivolts(0U, 3300U).value() == -1650, "negative half-scale voltage");
  differential_api::sample = 4096;
  check(adc::read_differential_millivolts(0U, 3300U).error() == status::error,
        "reject out-of-resolution signed counts");
  check(adc::read_differential_millivolts(0U, 0U).error() == status::invalid_argument,
        "reject zero differential voltage reference");
  check(adc::read_differential_millivolts(0U, std::numeric_limits<std::uint32_t>::max()).error() ==
            status::invalid_argument,
        "reject a reference outside signed output range");
  differential_api::sample = -2048;
  const auto largest_reference =
      adc::read_differential_millivolts(0U, static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()));
  check(largest_reference && largest_reference.value() == -std::numeric_limits<std::int32_t>::max(),
        "largest supported reference does not overflow signed conversion");
  differential_api::error = status::io_error;
  check(adc::read_differential_millivolts(0U, 3300U).error() == status::io_error,
        "signed voltage conversion preserves acquisition errors");
  using entropy = microhal::crc::accelerator<entropy_tag, microhal::crc::crc_config<0U, true>>;
  const auto zero_entropy = entropy::read_random();
  check(zero_entropy && zero_entropy.value() == 0U, "zero entropy is valid data, not an error sentinel");
  entropy_api::error = status::io_error;
  check(entropy::read_random().error() == status::io_error, "checked RNG propagates hardware errors");
  entropy_api::error = status::busy;
  check(entropy::read_random().error() == status::busy, "checked RNG preserves provider availability");
  return failures == 0 ? 0 : 1;
}
