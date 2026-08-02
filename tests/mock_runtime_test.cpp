// SPDX-License-Identifier: Apache-2.0
/**
 * @file mock_runtime_test.cpp
 * @brief Runtime behavioural tests for the microHAL facades using the host recording backend.
 * @details The other test translation units are `static_assert`-only: they prove that the type
 *          machinery compiles, but they never observe what a facade actually *does*. This suite
 *          runs on the host and asserts the real call sequence, the arguments forwarded to the
 *          backend, and — most importantly — that backend failures are propagated to the caller as
 *          @ref microhal::status instead of being discarded.
 */

#include "microhal/adc/adc.hpp"
#include "microhal/core/backend_selector.hpp"
#include "microhal/crc/crc.hpp"
#include "microhal/flash/flash.hpp"
#include "microhal/gpio/gpio.hpp"
#include "microhal/i2c/i2c.hpp"
#include "microhal/pwm/pwm.hpp"
#include "microhal/rtc/rtc.hpp"
#include "microhal/sd/sd.hpp"
#include "microhal/spi/spi.hpp"
#include "microhal/timers/timer.hpp"
#include "microhal/uart/uart.hpp"
#include "microhal/watchdog/watchdog.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <span>

namespace
{

using backend = microhal::mock_tag;
using microhal::peripheral_kind;
using microhal::status;
using microhal::backends::mock::operation;
using microhal::backends::mock::recorder;

int failures = 0;

void check(const bool condition, const char* const what) noexcept
{
  if (!condition)
  {
    ++failures;
    std::printf("FAIL: %s\n", what);
  }
}

void check_status(const status actual, const status expected, const char* const what) noexcept
{
  if (actual != expected)
  {
    ++failures;
    std::printf("FAIL: %s (expected %.*s, got %.*s)\n", what, static_cast<int>(microhal::to_string(expected).size()),
                microhal::to_string(expected).data(), static_cast<int>(microhal::to_string(actual).size()),
                microhal::to_string(actual).data());
  }
}

using console_uart = microhal::uart::port_config<2U, 115'200U>;
using sensor_i2c = microhal::i2c::master_config<1U, 400'000U>;
using sensor_spi = microhal::spi::master_config<1U, 8'000'000U>;
using led_pin = microhal::gpio::pin_config<0U, 5U, microhal::gpio_mode::output>;
using sensor_adc = microhal::adc::adc_config<1U>;
using log_flash = microhal::flash::flash_config<4U, 2'048U>;
using system_watchdog = microhal::watchdog::watchdog_config<1U, 1'000U>;
using motor_pwm = microhal::pwm::pwm_config<3U, 1U, 20'000U>;
using host_rtc = microhal::rtc::rtc_config<false, true>;
using host_timer = microhal::timers::timer_config<0U, 1'000'000U>;
using host_sd = microhal::sd::host_config<0U>;
using host_crc = microhal::crc::crc_config<0U, true>;

// ---------------------------------------------------------------------------
// Call forwarding: the facade must reach the backend with the right arguments.
// ---------------------------------------------------------------------------
void test_uart_forwards_calls() noexcept
{
  recorder::reset();

  check_status(microhal::uart::port<backend, console_uart>::initialize(), status::ok, "uart initialize succeeds");

  const std::array<std::uint8_t, 4U> tx{1U, 2U, 3U, 4U};
  check_status(microhal::uart::port<backend, console_uart>::write_blocking(tx), status::ok, "uart write succeeds");

  const auto log = recorder::log();
  check(log.size() == 2U, "uart produced exactly two backend calls");
  check(log[0].peripheral == peripheral_kind::uart && log[0].op == operation::initialize,
        "first uart call is initialize");
  check(log[1].op == operation::write, "second uart call is write");
  check(log[1].argument == 2U, "uart write targeted instance 2");
  check(log[1].length == 4U, "uart write forwarded the 4-byte payload length");
}

void test_i2c_forwards_address_and_length() noexcept
{
  recorder::reset();

  check_status(microhal::i2c::master<backend, sensor_i2c>::initialize(), status::ok, "i2c initialize succeeds");

  std::array<std::uint8_t, 3U> rx{};
  check_status(microhal::i2c::master<backend, sensor_i2c>::read_blocking(0x48U, rx), status::ok, "i2c read succeeds");

  const auto log = recorder::log();
  check(log.size() == 2U, "i2c produced exactly two backend calls");
  check(log[1].op == operation::read, "second i2c call is read");
  check(log[1].argument == 0x48U, "i2c read forwarded the 7-bit device address");
  check(log[1].length == 3U, "i2c read forwarded the buffer length");
  check(rx[0] == 0xA5U && rx[2] == 0xA5U, "i2c read filled the caller's buffer");
}

void test_spi_fills_rx_buffer() noexcept
{
  recorder::reset();

  const std::array<std::uint8_t, 2U> tx{0xDEU, 0xADU};
  std::array<std::uint8_t, 2U> rx{};
  check_status(microhal::spi::master<backend, sensor_spi>::transfer(tx, rx), status::ok, "spi transfer succeeds");
  check(rx[0] == 0xA5U && rx[1] == 0xA5U, "spi transfer populated the rx buffer");
  check(recorder::called(peripheral_kind::spi, operation::transfer), "spi transfer reached the backend");
}

void test_gpio_round_trip() noexcept
{
  recorder::reset();

  check_status(microhal::gpio::pin<backend, led_pin>::configure(), status::ok, "gpio configure succeeds");
  microhal::gpio::pin<backend, led_pin>::set();
  check(microhal::gpio::pin<backend, led_pin>::read(), "gpio reads back the level written by set()");
  microhal::gpio::pin<backend, led_pin>::clear();
  check(!microhal::gpio::pin<backend, led_pin>::read(), "gpio reads back the level written by clear()");

  const auto log = recorder::log();
  check(log[0].argument == 5U, "gpio calls carry the configured pin index");
}

void test_flash_forwards_address() noexcept
{
  recorder::reset();

  check_status(microhal::flash::controller<backend, log_flash>::erase(0x0800'4000U, 2'048U), status::ok,
               "flash erase succeeds");

  const std::array<std::uint8_t, 8U> payload{};
  check_status(microhal::flash::controller<backend, log_flash>::program(0x0800'4000U, payload), status::ok,
               "flash program succeeds");

  const auto log = recorder::log();
  check(log[0].op == operation::erase && log[0].argument == 0x0800'4000U, "flash erase forwarded the address");
  check(log[1].op == operation::program && log[1].length == 8U, "flash program forwarded the payload length");
}

void test_pwm_and_watchdog_sequence() noexcept
{
  recorder::reset();

  check_status(microhal::pwm::output<backend, motor_pwm>::initialize(), status::ok, "pwm initialize succeeds");
  check_status(microhal::pwm::output<backend, motor_pwm>::set_duty_cycle(750U), status::ok, "pwm set duty succeeds");
  check_status(microhal::pwm::output<backend, motor_pwm>::start(), status::ok, "pwm start succeeds");
  check_status(microhal::watchdog::timer<backend, system_watchdog>::initialize(), status::ok,
               "watchdog initialize succeeds");
  check_status(microhal::watchdog::timer<backend, system_watchdog>::refresh(), status::ok, "watchdog refresh succeeds");

  const auto log = recorder::log();
  check(log.size() == 5U, "pwm/watchdog produced five backend calls");
  check(log[1].op == operation::set_duty_cycle && log[1].argument == 750U, "pwm forwarded the duty cycle in permille");
  check(log[3].peripheral == peripheral_kind::watchdog, "watchdog calls are attributed to the watchdog peripheral");
}

void test_sd_round_trip_and_capacity() noexcept
{
  recorder::reset();

  check_status(microhal::sd::card<backend, host_sd>::initialize(), status::ok, "sd initialize succeeds");

  std::array<std::uint8_t, 512U> tx{};
  for (std::size_t index = 0U; index < tx.size(); ++index)
  {
    tx[index] = static_cast<std::uint8_t>(index & 0xFFU);
  }
  check_status(microhal::sd::card<backend, host_sd>::write_blocks(2U, tx), status::ok, "sd write succeeds");

  std::array<std::uint8_t, 512U> rx{};
  check_status(microhal::sd::card<backend, host_sd>::read_blocks(2U, rx), status::ok, "sd read succeeds");
  check(rx == tx, "sd read returns the previously written block");

  const auto capacity = microhal::sd::card<backend, host_sd>::capacity_blocks();
  check(static_cast<bool>(capacity), "sd capacity returns a valid result");
  check(capacity.value() == recorder::sd_block_count, "sd capacity matches mock storage geometry");
}

void test_rtc_set_and_now() noexcept
{
  recorder::reset();

  check_status(microhal::rtc::controller<backend, host_rtc>::initialize(), status::ok, "rtc initialize succeeds");

  const microhal::rtc::date_time expected{.hour = 12U, .minute = 34U, .second = 56U};
  check_status(microhal::rtc::controller<backend, host_rtc>::set(expected), status::ok, "rtc set succeeds");

  const microhal::rtc::date_time now = microhal::rtc::controller<backend, host_rtc>::now();
  check(now.hour == expected.hour && now.minute == expected.minute && now.second == expected.second,
        "rtc now reflects the value written through set()");
}

void test_timer_counter_monotonic() noexcept
{
  recorder::reset();

  check_status(microhal::timers::timer<backend, host_timer>::initialize(), status::ok, "timer initialize succeeds");

  const std::uint32_t first = microhal::timers::timer<backend, host_timer>::counter();
  const std::uint32_t second = microhal::timers::timer<backend, host_timer>::counter();
  check(second == (first + 1U), "timer counter increments monotonically");
}

void test_crc_calculate_nonzero() noexcept
{
  recorder::reset();

  check_status(microhal::crc::accelerator<backend, host_crc>::initialize(), status::ok, "crc initialize succeeds");

  const std::array<std::uint8_t, 4U> payload{1U, 2U, 3U, 4U};
  const std::uint32_t crc = microhal::crc::accelerator<backend, host_crc>::calculate(payload);
  check(crc != 0U, "crc calculate returns a non-zero digest for non-empty input");
}

// ---------------------------------------------------------------------------
// Error propagation: this is what the status refactor exists to guarantee.
// ---------------------------------------------------------------------------
void test_backend_failure_reaches_caller() noexcept
{
  recorder::reset();
  recorder::fail_after_calls(0U); // Fail immediately.

  check_status(microhal::uart::port<backend, console_uart>::initialize(), status::error,
               "uart initialize reports backend failure");

  const std::array<std::uint8_t, 1U> tx{0U};
  check_status(microhal::uart::port<backend, console_uart>::write_blocking(tx), status::error,
               "uart write reports backend failure");
  check_status(microhal::i2c::master<backend, sensor_i2c>::initialize(), status::error,
               "i2c initialize reports backend failure");
  check_status(microhal::spi::master<backend, sensor_spi>::initialize(), status::error,
               "spi initialize reports backend failure");
  check_status(microhal::gpio::pin<backend, led_pin>::configure(), status::error,
               "gpio configure reports backend failure");
  check_status(microhal::flash::controller<backend, log_flash>::erase(0U, 4U), status::error,
               "flash erase reports backend failure");
  check_status(microhal::watchdog::timer<backend, system_watchdog>::refresh(), status::error,
               "watchdog refresh reports backend failure");
}

void test_failure_injection_boundary() noexcept
{
  recorder::reset();
  recorder::fail_after_calls(1U); // First call succeeds, everything after fails.

  check_status(microhal::uart::port<backend, console_uart>::initialize(), status::ok,
               "first call still succeeds before the injected failure point");

  const std::array<std::uint8_t, 1U> tx{0U};
  check_status(microhal::uart::port<backend, console_uart>::write_blocking(tx), status::error,
               "second call fails after the injected failure point");
}

// ---------------------------------------------------------------------------
// result<T>: value-returning operations must carry both a value and a status.
// ---------------------------------------------------------------------------
void test_result_carries_value_and_error() noexcept
{
  recorder::reset();
  microhal::backends::mock::adc_api::sample = 2'048U;

  const auto reading = microhal::adc::converter<backend, sensor_adc>::read_blocking(3U);
  check(static_cast<bool>(reading), "successful adc read converts to true");
  check(reading.value() == 2'048U, "adc result carries the sampled value");
  check(reading.error() == status::ok, "successful adc result reports status::ok");

  const auto log = recorder::log();
  check(log[0].argument == 3U, "adc read forwarded the channel index");
}

void test_result_value_or_default() noexcept
{
  constexpr microhal::result<std::uint16_t> failed{status::timeout};
  static_assert(!failed.has_value(), "a result constructed from an error has no value");
  static_assert(failed.value_or(7U) == 7U, "value_or returns the fallback for an error result");
  static_assert(failed.error() == status::timeout, "the original error code is preserved");

  constexpr microhal::result<std::uint16_t> succeeded{std::uint16_t{42U}};
  static_assert(succeeded.has_value(), "a result constructed from a value has one");
  static_assert(succeeded.value_or(7U) == 42U, "value_or returns the value when present");
}

// ---------------------------------------------------------------------------
// Capability wiring for the mock backend itself.
// ---------------------------------------------------------------------------
void test_mock_capabilities() noexcept
{
  static_assert(microhal::peripheral_supported<backend, peripheral_kind::uart>(),
                "the mock backend advertises uart support");
  static_assert(microhal::peripheral_supported<backend, peripheral_kind::ethernet>(),
                "the mock backend now advertises ethernet support");
  static_assert(microhal::peripheral_available<backend, peripheral_kind::gpio>(),
                "the pure predicate agrees with the capability query");
  static_assert(microhal::peripheral_available<backend, peripheral_kind::can>(),
                "the pure predicate now reports can support on the completed mock backend");

  static_assert(microhal::backend_traits<backend>::kind == microhal::backend_kind::mock_recording,
                "the mock backend reports its own backend_kind");
  static_assert(
      std::is_same_v<microhal::backend_tag_from_kind_t<microhal::backend_kind::mock_recording>, microhal::mock_tag>,
      "backend_kind::mock_recording maps back to mock_tag");
}

} // namespace

int main()
{
  test_uart_forwards_calls();
  test_i2c_forwards_address_and_length();
  test_spi_fills_rx_buffer();
  test_gpio_round_trip();
  test_flash_forwards_address();
  test_pwm_and_watchdog_sequence();
  test_sd_round_trip_and_capacity();
  test_rtc_set_and_now();
  test_timer_counter_monotonic();
  test_crc_calculate_nonzero();
  test_backend_failure_reaches_caller();
  test_failure_injection_boundary();
  test_result_carries_value_and_error();
  test_result_value_or_default();
  test_mock_capabilities();

  if (failures == 0)
  {
    std::printf("mock runtime tests: all checks passed\n");
    return 0;
  }
  std::printf("mock runtime tests: %d check(s) failed\n", failures);
  return 1;
}
