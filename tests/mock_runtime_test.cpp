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
#include "microhal/can/can.hpp"
#include "microhal/core/backend_selector.hpp"
#include "microhal/crc/crc.hpp"
#include "microhal/dac/dac.hpp"
#include "microhal/dma/dma.hpp"
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

#include <array>
#include <cstdint>
#include <cstdio>
#include <limits>
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
using host_timer_channel = microhal::timers::channel_config<1U, true, false>;
using host_dac = microhal::dac::dac_config<0U>;
using host_qspi = microhal::qspi::controller_config<0U, 80'000'000U>;
using host_dma = microhal::dma::dma_config<0U, 0U, 0U>;
using host_memcpy_dma = microhal::dma::dma_config<0U, 0U, 0U, microhal::dma::dma_direction::memory_to_memory>;
using host_sd = microhal::sd::host_config<0U>;
using host_i2s = microhal::i2s::i2s_config<0U>;
using host_usb = microhal::usb::usb_config<0U>;
using host_crc = microhal::crc::crc_config<0U, true>;
using host_can = microhal::can::can_config<0U>;
using host_clock = microhal::rcc::clock_config<microhal::rcc::clock_source::hsi, 48'000'000U>;

bool dma_completed = false;
bool i2c_completed = false;
status i2c_completion_status = status::error;

void on_dma_complete() noexcept
{
  dma_completed = true;
}

void on_i2c_complete(const status result) noexcept
{
  i2c_completed = true;
  i2c_completion_status = result;
}

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

void test_uart_byte_helpers() noexcept
{
  recorder::reset();

  std::uint8_t rx = 0U;
  check_status(microhal::uart::port<backend, console_uart>::write_byte(0x5AU), status::ok, "uart byte write succeeds");
  check_status(microhal::uart::port<backend, console_uart>::read_byte(rx), status::ok, "uart byte read succeeds");
  check(rx == recorder::read_fill, "uart byte read returns the backend-provided value");

  const auto log = recorder::log();
  check(log.size() == 2U && log[0].length == 1U && log[1].length == 1U,
        "uart byte helpers forward exactly one byte per call");
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

void test_i2c_combined_write_read_reports_status() noexcept
{
  recorder::reset();

  const std::array<std::uint8_t, 2U> tx{0x10U, 0x20U};
  std::array<std::uint8_t, 3U> rx{};
  check_status(microhal::i2c::master<backend, sensor_i2c>::write_read_blocking(0x48U, tx, rx), status::ok,
               "combined i2c write/read succeeds");
  check(rx[0] == 0xA5U && rx[2] == 0xA5U, "combined i2c read fills the caller's buffer");

  auto log = recorder::log();
  check(log.size() == 1U && log[0].op == operation::write_read, "combined transfer is a single bus operation");
  check(log[0].argument == 0x48U && log[0].length == (tx.size() + rx.size()),
        "combined transfer forwards its address and both payload lengths");

  recorder::i2c_result = status::timeout;
  check_status(microhal::i2c::master<backend, sensor_i2c>::write_read_blocking(0x48U, tx, rx), status::timeout,
               "combined i2c transfer preserves timeout status");
  check_status(microhal::i2c::master<backend, sensor_i2c>::write_read_blocking(0x80U, tx, rx), status::invalid_argument,
               "seven-bit i2c rejects an out-of-range address");
  const std::span<const std::uint8_t> empty_tx{};
  check_status(microhal::i2c::master<backend, sensor_i2c>::write_read_blocking(0x48U, empty_tx, rx),
               status::invalid_argument, "combined i2c rejects an empty write phase");
}

void test_i2c_combined_async_reports_completion() noexcept
{
  recorder::reset();
  i2c_completed = false;
  i2c_completion_status = status::error;

  const std::array<std::uint8_t, 1U> tx{0x10U};
  std::array<std::uint8_t, 2U> rx{};
  check_status(microhal::i2c::master<backend, sensor_i2c>::write_read_async<on_i2c_complete>(0x48U, tx, rx), status::ok,
               "combined async i2c transfer starts");
  check(i2c_completed && i2c_completion_status == status::ok, "combined async i2c completion handler receives success");
  check(rx[0] == 0xA5U && rx[1] == 0xA5U, "combined async i2c mock fills the receive buffer");
  check(recorder::log().size() == 1U && recorder::log()[0].op == operation::write_read,
        "combined async i2c transfer records one bus operation");

  i2c_completed = false;
  recorder::i2c_result = status::timeout;
  check_status(microhal::i2c::master<backend, sensor_i2c>::write_read_async<on_i2c_complete>(0x48U, tx, rx),
               status::timeout, "combined async i2c preserves timeout status");
  check(i2c_completed && i2c_completion_status == status::timeout,
        "combined async i2c completion handler receives timeout");

  i2c_completed = false;
  check_status(microhal::i2c::master<backend, sensor_i2c>::write_read_async<on_i2c_complete>(0x80U, tx, rx),
               status::invalid_argument, "combined async i2c rejects an invalid address");
  check(!i2c_completed, "invalid async i2c requests do not invoke the completion handler");
}

void test_i2c_probe_status() noexcept
{
  recorder::reset();
  check_status(microhal::i2c::master<backend, sensor_i2c>::probe(0x48U), status::ok,
               "i2c probe succeeds when the mock acknowledges");
  recorder::i2c_result = status::io_error;
  check_status(microhal::i2c::master<backend, sensor_i2c>::probe(0x48U), status::io_error,
               "i2c probe preserves a NACK/error result");
  check_status(microhal::i2c::master<backend, sensor_i2c>::probe(0x80U), status::invalid_argument,
               "seven-bit i2c rejects an out-of-range probe address");
  check(recorder::log().size() == 2U, "invalid i2c probe addresses do not reach the backend");
}

void test_spi_fills_rx_buffer() noexcept
{
  recorder::reset();

  const std::array<std::uint8_t, 2U> tx{0xDEU, 0xADU};
  std::array<std::uint8_t, 2U> rx{};
  check_status(microhal::spi::master<backend, sensor_spi>::transfer(tx, rx), status::ok, "spi transfer succeeds");
  check(rx[0] == 0xA5U && rx[1] == 0xA5U, "spi transfer populated the rx buffer");
  check(recorder::called(peripheral_kind::spi, operation::transfer), "spi transfer reached the backend");

  std::array<std::uint8_t, 1U> short_rx{};
  check_status(microhal::spi::master<backend, sensor_spi>::transfer(tx, short_rx), status::invalid_argument,
               "spi rejects mismatched transfer lengths instead of truncating");
  std::array<std::uint8_t, 2U> read_data{};
  check_status(microhal::spi::master<backend, sensor_spi>::read_blocking(read_data), status::ok,
               "spi receive-only operation succeeds");
  check(read_data[0] == 0xFFU && read_data[1] == 0xFFU, "mock SPI receive uses documented idle-high fill bytes");
  check_status(microhal::spi::master<backend, sensor_spi>::write_blocking(tx), status::ok,
               "spi transmit-only operation succeeds");
}

void test_gpio_round_trip() noexcept
{
  recorder::reset();

  check_status(microhal::gpio::pin<backend, led_pin>::configure(), status::ok, "gpio configure succeeds");
  microhal::gpio::pin<backend, led_pin>::set();
  check(microhal::gpio::pin<backend, led_pin>::read(), "gpio reads back the level written by set()");
  microhal::gpio::pin<backend, led_pin>::clear();
  check(!microhal::gpio::pin<backend, led_pin>::read(), "gpio reads back the level written by clear()");
  check_status(microhal::gpio::pin<backend, led_pin>::toggle(), status::ok, "gpio atomic toggle succeeds");
  check(microhal::gpio::pin<backend, led_pin>::read(), "gpio toggle changes the output level");

  const auto log = recorder::log();
  check(log[0].argument == 5U, "gpio calls carry the configured pin index");
}

void test_flash_forwards_address() noexcept
{
  recorder::reset();

  const auto region = microhal::flash::controller<backend, log_flash>::erase_region(0x0800'4001U);
  check(region.has_value(), "flash reports physical erase geometry");
  check(region.value().start_address == 0x0800'4000U && region.value().size_bytes == 2'048U,
        "flash erase geometry identifies the containing block");

  check_status(microhal::flash::controller<backend, log_flash>::erase(0x0800'4000U, 2'048U), status::ok,
               "flash erase succeeds");

  const std::array<std::uint8_t, 8U> payload{};
  check_status(microhal::flash::controller<backend, log_flash>::program(0x0800'4000U, payload), status::ok,
               "flash program succeeds");

  const auto log = recorder::log();
  check(log[0].op == operation::erase && log[0].argument == 0x0800'4000U, "flash erase forwarded the address");
  check(log[1].op == operation::program && log[1].length == 8U, "flash program forwarded the payload length");

  check_status(microhal::flash::controller<backend, log_flash>::erase(0x0800'4001U, 2'048U), status::invalid_argument,
               "flash erase requires an aligned block address");
  check_status(microhal::flash::controller<backend, log_flash>::program(0x0800'4001U, payload),
               status::invalid_argument, "flash program requires an aligned address");
  const std::array<std::uint8_t, 2U> short_payload{};
  check_status(microhal::flash::controller<backend, log_flash>::program(0x0800'4000U, short_payload),
               status::invalid_argument, "flash program requires a whole programming unit");
  check(recorder::log().size() == 2U, "invalid flash requests do not reach the backend");

  std::array<std::uint8_t, 4U> verify_buffer{};
  const std::array<std::uint8_t, 4U> expected_readback{recorder::read_fill, recorder::read_fill, recorder::read_fill,
                                                       recorder::read_fill};
  check_status(microhal::flash::controller<backend, log_flash>::program_and_verify(0x0800'4000U, expected_readback,
                                                                                   verify_buffer),
               status::ok, "flash program-and-verify succeeds when readback matches");
  check(verify_buffer == expected_readback, "flash verification reads into caller-owned storage");
  check_status(microhal::flash::controller<backend, log_flash>::program_and_verify(
                   0x0800'4000U, expected_readback, std::span<std::uint8_t>{verify_buffer.data(), 3U}),
               status::invalid_argument, "flash verification rejects mismatched buffer lengths");
  const std::array<std::uint8_t, 4U> mismatched_readback{1U, 2U, 3U, 4U};
  check_status(microhal::flash::controller<backend, log_flash>::program_and_verify(0x0800'4000U, mismatched_readback,
                                                                                   verify_buffer),
               status::io_error, "flash verification reports mismatched readback");
}

void test_pwm_and_watchdog_sequence() noexcept
{
  recorder::reset();

  check_status(microhal::pwm::output<backend, motor_pwm>::initialize(), status::ok, "pwm initialize succeeds");
  check_status(microhal::pwm::output<backend, motor_pwm>::set_duty_cycle(750U), status::ok, "pwm set duty succeeds");
  check_status(microhal::pwm::output<backend, motor_pwm>::start(), status::ok, "pwm start succeeds");
  check_status(microhal::pwm::output<backend, motor_pwm>::set_duty_cycle_percent(50U), status::ok,
               "pwm percentage helper succeeds");
  check_status(microhal::pwm::output<backend, motor_pwm>::set_enabled(false), status::ok,
               "pwm can be stopped through the enable helper");
  check_status(microhal::pwm::output<backend, motor_pwm>::set_duty_cycle_percent(101U), status::invalid_argument,
               "pwm rejects percentages outside zero to one hundred");
  check_status(microhal::watchdog::timer<backend, system_watchdog>::initialize(), status::ok,
               "watchdog initialize succeeds");
  check_status(microhal::watchdog::timer<backend, system_watchdog>::refresh(), status::ok, "watchdog refresh succeeds");

  const auto log = recorder::log();
  check(log.size() == 7U, "pwm/watchdog produced seven backend calls");
  check(log[1].op == operation::set_duty_cycle && log[1].argument == 750U, "pwm forwarded the duty cycle in permille");
  check(log[3].op == operation::set_duty_cycle && log[3].argument == 500U, "pwm percentage helper forwards permille");
  check(log[5].peripheral == peripheral_kind::watchdog, "watchdog calls are attributed to the watchdog peripheral");
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

  std::array<std::uint8_t, 511U> partial_block{};
  const std::size_t calls_before_invalid = recorder::log().size();
  check_status(microhal::sd::card<backend, host_sd>::read_blocks(0U, partial_block), status::invalid_argument,
               "sd rejects partial blocks before calling the backend");
  check(recorder::log().size() == calls_before_invalid, "invalid sd requests do not reach the backend");
  check(!microhal::sd::card<backend, host_sd>::is_valid_range(recorder::sd_block_count, 1U),
        "sd range validation rejects blocks beyond capacity");
  check_status(microhal::sd::card<backend, host_sd>::erase_blocks(UINT32_MAX, 2U), status::invalid_argument,
               "sd rejects overflowing block ranges");
}

void test_i2s_transfer_validation() noexcept
{
  recorder::reset();
  using interface = microhal::i2s::interface<backend, host_i2s>;

  check_status(interface::initialize(), status::ok, "i2s initializes");
  const std::array<std::uint16_t, 2U> tx{0x1234U, 0x5678U};
  std::array<std::uint16_t, 2U> rx{};
  check_status(interface::transmit_blocking(tx), status::ok, "i2s transmits samples");
  check_status(interface::receive_blocking(rx), status::ok, "i2s receives samples");
  check(rx[0] == recorder::read_fill, "i2s receive populates its output");
  dma_completed = false;
  check_status(interface::template start_dma_tx<&on_dma_complete>(tx), status::ok, "i2s starts dma transmit");
  check(dma_completed, "i2s mock signals completed dma transmit");

  const std::span<const std::uint16_t> empty{};
  check_status(interface::transmit_blocking(empty), status::invalid_argument,
               "i2s rejects empty transfers before calling the backend");
  check(interface::is_valid_transfer(std::numeric_limits<std::uint16_t>::max()),
        "i2s accepts the largest representable transfer size");
  check(!interface::is_valid_transfer(static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()) + 1U),
        "i2s rejects transfer sizes that a backend would truncate");
}

void test_rtc_set_and_now() noexcept
{
  recorder::reset();

  check_status(microhal::rtc::controller<backend, host_rtc>::initialize(), status::ok, "rtc initialize succeeds");

  const microhal::rtc::date_time expected{.hour = 12U, .minute = 34U, .second = 56U};
  check_status(microhal::rtc::controller<backend, host_rtc>::set(expected), status::ok, "rtc set succeeds");

  const auto read = microhal::rtc::controller<backend, host_rtc>::read();
  check(read.has_value(), "rtc read preserves backend failure status");
  const microhal::rtc::date_time now = read.value();
  check(now.hour == expected.hour && now.minute == expected.minute && now.second == expected.second,
        "rtc read reflects the value written through set()");
  check_status(microhal::rtc::controller<backend, host_rtc>::set({.hour = 24U}), status::invalid_argument,
               "rtc rejects out-of-range time values");
  check_status(microhal::rtc::controller<backend, host_rtc>::set({.year = 2026U, .hour = 12U}), status::unsupported,
               "rtc does not silently discard unsupported calendar fields");
}

void test_timer_counter_monotonic() noexcept
{
  recorder::reset();

  check_status(microhal::timers::timer<backend, host_timer>::initialize(), status::ok, "timer initialize succeeds");

  const std::uint32_t first = microhal::timers::timer<backend, host_timer>::counter();
  const std::uint32_t second = microhal::timers::timer<backend, host_timer>::counter();
  check(second == (first + 1U), "timer counter increments monotonically");
}

void test_qspi_transactions_and_memory_map() noexcept
{
  recorder::reset();
  using qspi = microhal::qspi::controller<backend, host_qspi>;

  const microhal::qspi::command read_command{.instruction = 0x0BU,
                                             .instruction_mode = microhal::qspi::io_mode::single,
                                             .address = 0x123456U,
                                             .address_size_bytes = 3U,
                                             .address_mode = microhal::qspi::io_mode::quad,
                                             .dummy_cycles = 8U,
                                             .data_mode = microhal::qspi::io_mode::quad};
  std::array<std::uint8_t, 4U> rx{};
  check_status(qspi::initialize(), status::ok, "qspi initialize succeeds");
  check_status(qspi::command_read(read_command, rx), status::ok, "qspi command read succeeds");
  check(rx[0] == recorder::read_fill && rx[3] == recorder::read_fill, "qspi read fills caller buffer");

  const std::array<std::uint8_t, 2U> tx{0xAAU, 0x55U};
  check_status(qspi::command_write(read_command, tx), status::ok, "qspi command write succeeds");
  check_status(qspi::enter_memory_mapped(read_command), status::ok, "qspi enters memory-mapped mode");
  check(recorder::qspi_memory_mapped, "qspi mock records memory-mapped state");
  check_status(qspi::exit_memory_mapped(), status::ok, "qspi exits memory-mapped mode");
  check(!recorder::qspi_memory_mapped, "qspi mock clears memory-mapped state");
  check(qspi::is_valid_command(read_command), "qspi command helper accepts a valid request");
  check(qspi::supports_command_mode(read_command), "qspi mode helper recognizes supported bus modes");

  const microhal::qspi::command invalid_command{
      .instruction = 0x03U, .address_size_bytes = 5U, .address_mode = microhal::qspi::io_mode::single};
  check_status(qspi::command_read(invalid_command, rx), status::invalid_argument,
               "qspi rejects addresses wider than four bytes");
  auto octo_command = read_command;
  octo_command.data_mode = microhal::qspi::io_mode::octo;
  check_status(qspi::command_read(octo_command, rx), status::unsupported,
               "qspi rejects modes absent from backend capabilities");

  const auto log = recorder::log();
  check(log[1].op == operation::read && log[1].argument == 0x0BU && log[1].length == rx.size(),
        "qspi read forwards opcode and payload length");
  check(log[2].op == operation::write && log[2].argument == 0x0BU && log[2].length == tx.size(),
        "qspi write forwards opcode and payload length");
}

void test_usb_endpoint_validation() noexcept
{
  recorder::reset();
  using device = microhal::usb::device<backend, host_usb>;
  const std::array<std::uint8_t, 3U> tx{1U, 2U, 3U};
  check_status(device::initialize(), status::ok, "usb initializes");
  check(device::is_valid_endpoint(0U), "usb accepts control endpoint zero");
  check(!device::is_valid_endpoint(static_cast<std::uint8_t>(device::endpoint_count)),
        "usb rejects endpoint indexes outside the backend range");
  check_status(device::endpoint_transmit(0U, tx), status::ok, "usb transfers endpoint data");
  check_status(device::endpoint_transmit(static_cast<std::uint8_t>(device::endpoint_count), tx),
               status::invalid_argument, "usb rejects invalid endpoint writes");
  std::array<std::uint8_t, 2U> rx{};
  const auto received = device::endpoint_receive(0U, rx);
  check(received && received.value() == rx.size(), "usb receives endpoint data");
  check_status(device::endpoint_receive(static_cast<std::uint8_t>(device::endpoint_count), rx).error(),
               status::invalid_argument, "usb rejects invalid endpoint reads");

  std::array<std::uint8_t, 1U> byte{};
  const std::span<const std::uint8_t> oversized{
      byte.data(), static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()) + 1U};
  check_status(device::endpoint_transmit(0U, oversized), status::invalid_argument,
               "usb rejects endpoint lengths that exceed backend transfer limits");
  const std::span<std::uint8_t> oversized_rx{byte.data(),
                                             static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()) + 1U};
  check_status(device::endpoint_receive(0U, oversized_rx).error(), status::invalid_argument,
               "usb rejects receive lengths that exceed backend transfer limits");
}

void test_timer_controls_and_capture() noexcept
{
  recorder::reset();
  using timer = microhal::timers::timer<backend, host_timer>;
  recorder::timer_capture = 0x1234U;

  check_status(timer::initialize(), status::ok, "timer initialize succeeds");
  check_status(timer::start(), status::ok, "timer starts");
  const auto initial_period = timer::period();
  check(initial_period && initial_period.value() == host_timer::period, "timer reports its configured period");
  check_status(timer::set_period(10'000U), status::ok, "timer period updates");
  check(recorder::timer_period == 10'000U, "timer period reaches backend");
  const auto updated_period = timer::period();
  check(updated_period && updated_period.value() == 10'000U, "timer period readback reflects updates");
  static_cast<void>(timer::counter());
  check_status(timer::reset_counter(), status::ok, "timer counter resets");
  check(timer::counter() == 0U, "timer counter reset takes effect");
  const auto captured = timer::capture_value<host_timer_channel>();
  check(captured.has_value() && captured.value() == 0x1234U, "timer returns captured channel value");
  check_status(timer::set_period(0U), status::invalid_argument, "timer rejects zero period");
  using non_capture_channel = microhal::timers::channel_config<1U>;
  check_status(timer::capture_value<non_capture_channel>().error(), status::invalid_argument,
               "timer rejects capture reads on channels not configured for capture");
  check_status(timer::stop(), status::ok, "timer stops");
}

void test_adc_and_dac_helpers() noexcept
{
  recorder::reset();
  microhal::backends::mock::adc_api::sample = 2'048U;
  using adc = microhal::adc::converter<backend, sensor_adc>;
  using dac = microhal::dac::converter<backend, host_dac>;

  constexpr std::array<std::uint8_t, 3U> channels{0U, 1U, 2U};
  std::array<std::uint16_t, channels.size()> samples{};
  check_status(adc::read_sequence_blocking(channels, samples), status::ok, "ADC scans channels in order");
  check(samples == std::array<std::uint16_t, 3U>{2'048U, 2'048U, 2'048U}, "ADC sequence returns each channel sample");
  const auto millivolts = adc::read_millivolts(0U, 3'300U);
  check(millivolts && millivolts.value() == 1'650U, "ADC converts raw codes to rounded millivolts");

  std::array<std::uint16_t, 2U> partial_samples{7U, 9U};
  recorder::reset();
  recorder::fail_after_calls(1U);
  constexpr std::array<std::uint8_t, 2U> failure_channels{0U, 1U};
  check_status(adc::read_sequence_blocking(failure_channels, partial_samples), status::error,
               "ADC sequence propagates the first backend error");
  check(partial_samples[0] == 2'048U && partial_samples[1] == 9U,
        "ADC sequence preserves unread output values after a failure");

  recorder::reset();
  constexpr std::array<std::uint16_t, 3U> dac_samples{1U, 2U, 3U};
  check_status(dac::write_sequence_blocking(dac_samples), status::ok, "DAC writes a sequence in order");
  check_status(dac::write_millivolts(1'650U, 3'300U), status::ok, "DAC converts millivolts to an output code");
  const auto writes = recorder::log();
  check(writes.size() == 4U && writes[0].argument == 1U && writes[2].argument == 3U,
        "DAC sequence preserves sample order");
  check(writes[3].argument == 2'048U, "DAC voltage conversion rounds to the nearest code");
  check_status(dac::write_millivolts(3'301U, 3'300U), status::invalid_argument,
               "DAC rejects voltages above its reference");
}

void test_dma_completion_and_status() noexcept
{
  recorder::reset();
  using dma = microhal::dma::controller<backend, host_dma>;
  dma_completed = false;

  check_status(dma::initialize(), status::ok, "dma initialize succeeds");
  const auto initial = dma::transfer_status();
  check(initial.has_value() && initial.value() == microhal::dma::transfer_state::idle, "DMA starts idle");
  check_status(dma::start_transfer<on_dma_complete>(0x1000U, 0x2000U, 16U), status::ok, "DMA transfer starts");
  check(dma_completed, "DMA completion callback runs");
  const auto completed = dma::transfer_status();
  check(completed.has_value() && completed.value() == microhal::dma::transfer_state::complete,
        "DMA reports completion");
  check(dma::transfer_complete().has_value() && dma::transfer_complete().value(),
        "DMA completion helper reports the completed state");
  check_status(dma::abort(), status::ok, "DMA abort succeeds");
  const auto aborted = dma::transfer_status();
  check(aborted.has_value() && aborted.value() == microhal::dma::transfer_state::idle, "DMA abort returns to idle");
  recorder::fail_after_calls(0U);
  check_status(dma::transfer_status().error(), status::error, "DMA status query propagates backend failure");

  recorder::reset();
  using memcpy_dma = microhal::dma::controller<backend, host_memcpy_dma>;
  check_status(memcpy_dma::start_memcpy<on_dma_complete>(0x1000U, 0x2000U, 16U), status::ok,
               "DMA memory-copy helper starts a transfer");
  check_status(memcpy_dma::start_memcpy<on_dma_complete>(0x1000U, 0x2000U, 0U), status::invalid_argument,
               "DMA rejects zero-length transfers before reaching the backend");
  check_status(memcpy_dma::start_memcpy<on_dma_complete>(UINT32_MAX, 0x2000U, 2U), status::invalid_argument,
               "DMA rejects address ranges that overflow");
}

void test_unimplemented_backend_operations_are_unsupported() noexcept
{
  using stm32_hal = microhal::stm32_hal_tag;
  using timer = microhal::timers::timer<stm32_hal, host_timer>;
  using qspi = microhal::qspi::controller<stm32_hal, host_qspi>;
  using dma = microhal::dma::controller<stm32_hal, host_dma>;
  using i2c = microhal::i2c::master<stm32_hal, sensor_i2c>;
  using stm32_flash_config = microhal::flash::flash_config<8U, 2'048U>;
  using flash = microhal::flash::controller<stm32_hal, stm32_flash_config>;

  const microhal::qspi::command request{.instruction = 0x03U};
  std::array<std::uint8_t, 1U> rx{};
  const std::array<std::uint8_t, 1U> tx{};
  check_status(i2c::write_read_blocking(0x48U, tx, rx), status::unsupported,
               "I2C reports unsupported without a combined-transfer backend method");
  check_status(flash::erase_region(0x0800'4000U).error(), status::unsupported,
               "flash reports unsupported when erase geometry is unavailable");
  check_status(qspi::command_read(request, rx), status::unsupported,
               "QSPI reports unsupported without a hardware implementation");
  check_status(timer::start(), status::unsupported, "timer reports unsupported without a hardware implementation");
  check_status(dma::transfer_status().error(), status::unsupported,
               "DMA status reports unsupported without a hardware implementation");
}

void test_crc_calculate_nonzero() noexcept
{
  recorder::reset();

  check_status(microhal::crc::accelerator<backend, host_crc>::initialize(), status::ok, "crc initialize succeeds");

  const std::array<std::uint8_t, 4U> payload{1U, 2U, 3U, 4U};
  const std::uint32_t crc = microhal::crc::accelerator<backend, host_crc>::calculate(payload);
  check(crc != 0U, "crc calculate returns a non-zero digest for non-empty input");

  constexpr std::array<std::uint8_t, 9U> check_vector{'1', '2', '3', '4', '5', '6', '7', '8', '9'};
  constexpr std::uint32_t crc_mpeg2 = microhal::crc::accelerator<backend, host_crc>::calculate_software(check_vector);
  check(crc_mpeg2 == 0x0376'E6E7U, "software CRC uses the configured polynomial and CRC-32/MPEG-2 parameters");
}

void test_can_validation_and_batch_operations() noexcept
{
  recorder::reset();
  using can = microhal::can::bus<backend, host_can>;
  microhal::can::message standard{};
  standard.id = 0x7FFU;
  standard.dlc = 1U;
  microhal::can::message extended{};
  extended.id = 0x1FFF'FFFFU;
  extended.extended_id = true;
  const std::array<microhal::can::message, 2U> messages{standard, extended};

  check_status(can::transmit_many(messages), status::ok, "CAN transmits a batch of valid messages");
  auto invalid = standard;
  invalid.id = 0x800U;
  check_status(can::transmit(invalid), status::invalid_argument, "CAN rejects standard identifiers above 11 bits");
  invalid = extended;
  invalid.dlc = 9U;
  check_status(can::transmit(invalid), status::invalid_argument, "CAN rejects payload lengths above eight bytes");
  check(recorder::log().size() == 2U, "invalid CAN frames are rejected before reaching the backend");
}

void test_rcc_configuration_readback() noexcept
{
  recorder::reset();
  using rcc = microhal::rcc::clock_controller<backend, host_clock>;
  check_status(rcc::configure_and_verify(), status::ok, "RCC verifies the configured system clock");
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
  check_status(microhal::flash::controller<backend, log_flash>::erase(0x0800'4000U, 2'048U), status::error,
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
  static_assert(microhal::backend_traits<backend>::gpio_capability().output_toggle,
                "the mock GPIO backend advertises native toggle support");
  static_assert(microhal::backend_traits<backend>::i2c_capability().repeated_start &&
                    microhal::backend_traits<backend>::i2c_capability().address_probe,
                "the mock I2C backend advertises combined transactions and address probing");
  static_assert(microhal::backend_traits<backend>::spi_capability().transmit_only &&
                    microhal::backend_traits<backend>::spi_capability().receive_only,
                "the mock SPI backend advertises single-direction transfers");
  static_assert(microhal::backend_traits<backend>::uart_capability().parity &&
                    microhal::backend_traits<backend>::uart_capability().two_stop_bits,
                "the mock UART backend advertises framing capabilities");
  static_assert(microhal::backend_traits<backend>::timer_capability().period_readback &&
                    microhal::backend_traits<backend>::timer_capability().counter_reset,
                "the mock timer advertises period readback and counter reset");
  static_assert(microhal::backend_traits<backend>::adc_capability().sequential_scan &&
                    microhal::backend_traits<backend>::adc_capability().voltage_conversion,
                "the mock ADC advertises sequence reads and voltage conversion");
  static_assert(microhal::backend_traits<backend>::dac_capability().sequence_write &&
                    microhal::backend_traits<backend>::dac_capability().voltage_conversion,
                "the mock DAC advertises sequence writes and voltage conversion");
  static_assert(
      std::is_same_v<microhal::backend_tag_from_kind_t<microhal::backend_kind::mock_recording>, microhal::mock_tag>,
      "backend_kind::mock_recording maps back to mock_tag");
}

} // namespace

int main()
{
  test_uart_forwards_calls();
  test_uart_byte_helpers();
  test_i2c_forwards_address_and_length();
  test_i2c_combined_write_read_reports_status();
  test_i2c_combined_async_reports_completion();
  test_i2c_probe_status();
  test_spi_fills_rx_buffer();
  test_gpio_round_trip();
  test_flash_forwards_address();
  test_pwm_and_watchdog_sequence();
  test_sd_round_trip_and_capacity();
  test_i2s_transfer_validation();
  test_usb_endpoint_validation();
  test_rtc_set_and_now();
  test_timer_counter_monotonic();
  test_qspi_transactions_and_memory_map();
  test_timer_controls_and_capture();
  test_adc_and_dac_helpers();
  test_dma_completion_and_status();
  test_unimplemented_backend_operations_are_unsupported();
  test_crc_calculate_nonzero();
  test_can_validation_and_batch_operations();
  test_rcc_configuration_readback();
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
