// SPDX-License-Identifier: Apache-2.0
#include "microhal/backends/nordic/nrfx/traits.hpp"

#include <array>
#include <cstdio>

namespace
{
using microhal::status;
namespace nordic = microhal::backends::nordic::nrfx;
namespace detail = nordic::detail;
int failures{};
int callbacks{};
status last_status{status::error};
detail::transfer_state transfer{};

void check(const bool condition, const char* message)
{
  if (!condition)
  {
    std::printf("FAIL: %s\n", message);
    ++failures;
  }
}
void completion(const status result) noexcept
{
  ++callbacks;
  last_status = result;
  check(!transfer.active.load(), "release transfer before callback");
  check(transfer.complete.load(), "publish completion before callback");
}
struct config
{
};
struct zero_timeout
{
  static constexpr std::uint32_t timeout_ms{0U};
};
static_assert(detail::timeout_ms<config>() == 1000U);
static_assert(detail::timeout_ms<zero_timeout>() == 0U);
static_assert(detail::pwm_clock(0U).error() == status::invalid_argument);
static_assert(detail::pwm_clock(16'000'000U).value().top == 1U);
static_assert(detail::pwm_clock(50U).value().top == 20000U);
static_assert(detail::pwm_clock(50U).value().prescaler == 4U);
static_assert(detail::pwm_clock(1U).error() == status::invalid_argument);
static_assert(detail::pwm_clock(16'000'001U).error() == status::invalid_argument);
static_assert(detail::pwm_clock(73U).error() == status::invalid_argument);
} // namespace

int main()
{
  check(detail::driver_result(0) == status::ok, "nrfx success mapping");
  check(detail::driver_result(-EBUSY) == status::busy, "nrfx busy mapping");
  check(detail::driver_result(-EINVAL) == status::invalid_argument, "nrfx invalid mapping");
  check(detail::driver_result(-ETIMEDOUT) == status::timeout, "nrfx timeout mapping");
  check(detail::driver_result(-ENOTSUP) == status::unsupported, "nrfx unsupported mapping");
  check(detail::driver_result(-EIO) == status::io_error, "nrfx I/O error mapping");
  check(transfer.begin(completion), "first transfer accepted");
  check(!transfer.begin(nullptr), "overlapping transfer rejected");
  transfer.finish(status::io_error);
  check(callbacks == 1 && last_status == status::io_error, "preserve callback on busy rejection");
  check(transfer.begin(completion), "transfer reusable after failure");
  check(!transfer.complete.load(), "clear stale completion");
  transfer.finish(status::ok);
  check(callbacks == 2 && last_status == status::ok, "deliver success callback");
  check(transfer.begin(nullptr), "blocking transfer accepted");
  transfer.finish(status::timeout);
  check(callbacks == 2 && transfer.result == status::timeout, "timeout without stale callback");

  constexpr std::array<std::uint8_t, 3U> channels{0U, 2U, 7U};
  constexpr std::array<std::uint8_t, 2U> duplicate{2U, 2U};
  constexpr std::array<std::uint8_t, 2U> reversed{2U, 1U};
  constexpr std::array<std::uint8_t, 1U> invalid{8U};
  check(detail::saadc_channel_mask(channels).value() == 0x85U, "scan channel mask");
  check(detail::saadc_channel_mask(duplicate).error() == status::invalid_argument, "reject duplicate channels");
  check(detail::saadc_channel_mask(reversed).error() == status::invalid_argument, "reject reordered channels");
  check(detail::saadc_channel_mask(invalid).error() == status::invalid_argument, "reject invalid channel");
  check(detail::saadc_channel_mask({}).error() == status::invalid_argument, "reject empty scan");
  check(detail::qspi_range_valid(0xFFFFFFU, 1U, 3U, 0x3FFFFU), "last 24-bit QSPI byte fits");
  check(!detail::qspi_range_valid(0xFFFFFFU, 2U, 3U, 0x3FFFFU), "reject QSPI address wrap");
  check(!detail::qspi_range_valid(0x1000000U, 1U, 3U, 0x3FFFFU), "reject oversized 24-bit address");
  check(detail::qspi_range_valid(UINT32_MAX, 1U, 4U, 0x3FFFFU), "last 32-bit QSPI byte fits");
  check(!detail::qspi_range_valid(UINT32_MAX, 2U, 4U, 0x3FFFFU), "reject 32-bit QSPI wrap");
  check(detail::qspi_range_valid(0U, 0x3FFFFU, 3U, 0x3FFFFU), "QSPI maximum count fits");
  check(!detail::qspi_range_valid(0U, 0x40000U, 3U, 0x3FFFFU), "reject QSPI count truncation");
  check(!detail::qspi_range_valid(0U, 0U, 3U, 0x3FFFFU), "reject empty QSPI transfer");
  check(detail::i2s_count_valid(32766U, 0x3FFFU), "I2S maximum word count fits");
  check(!detail::i2s_count_valid(32768U, 0x3FFFU), "reject I2S count truncation");
  check(!detail::i2s_count_valid(5U, 0x3FFFU), "reject incomplete I2S stereo frame");
  check(!detail::i2s_count_valid(2U, 0x3FFFU), "reject undersized I2S block");
  check(detail::serial_count_valid(0U), "zero-length serial phase fits for address probes");
  check(detail::serial_count_valid(65535U), "serial DMA maximum count fits");
  check(!detail::serial_count_valid(65536U), "reject serial DMA count truncation");
  for (std::uint32_t frequency = 1U; frequency <= 16000U; ++frequency)
  {
    const auto timing = detail::pwm_clock(frequency);
    if (timing)
    {
      const auto clock = 16'000'000U >> timing.value().prescaler;
      check(timing.value().top > 0U && timing.value().top <= 32767U, "PWM top fits hardware");
      check(clock == timing.value().top * frequency, "PWM frequency is exact");
    }
  }

  check(nordic::spi_api::initialize<config>() == status::unsupported, "no SDK SPI remains unsupported");
  check(nordic::uart_api::initialize<config>() == status::unsupported, "no SDK UART remains unsupported");
  check(nordic::adc_api::read_blocking<config>(0U).error() == status::unsupported, "ADC fallback is not a zero sample");
  check(nordic::qspi_api::initialize<config>() == status::unsupported, "no SDK QSPI remains unsupported");
  check(nordic::rtc_api::initialize<config>() == status::unsupported, "no SDK RTC remains unsupported");
  check(nordic::i2s_api::initialize<config>() == status::unsupported, "no SDK I2S remains unsupported");
  check(nordic::rcc_api::configure<config>() == status::unsupported, "no silent clock configuration success");
  return failures == 0 ? 0 : 1;
}
