// SPDX-License-Identifier: Apache-2.0
/**
 * @file traits.hpp
 * @brief Host-side recording backend for unit-testing microHAL drivers without hardware.
 * @details Every other backend needs a vendor SDK and a real MCU, which makes the behaviour of
 *          application code untestable on a build machine. This backend records each call into a
 *          fixed-capacity log and lets a test script the return value of the next call, so drivers
 *          written against microHAL facades can be verified for call sequence, arguments, and error
 *          handling on the host.
 *
 *          The recorder uses static storage and no allocation, so it is usable from a freestanding
 *          build as well. It is deliberately **not** thread-safe or ISR-safe: it is a test double.
 * @copyright Copyright (c) 2026 James Baldwin
 * @author microHAL contributors
 * @ingroup backend_support
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "microhal/core/traits.hpp"

namespace microhal::backends::mock
{

/// @brief Operations the recorder can distinguish.
enum class operation : std::uint8_t
{
  configure,
  initialize,
  write,
  read,
  transfer,
  erase,
  program,
  start,
  stop,
  refresh,
  connect,
  disconnect,
  abort,
  set_filter,
  enable_interrupt,
  set_duty_cycle
};

/// @brief One recorded backend call.
struct call_record final
{
  peripheral_kind peripheral{};
  operation op{};
  std::uint32_t argument{0U}; ///< Address, channel, duty cycle, or transfer length depending on @ref op.
  std::size_t length{0U};     ///< Payload length in bytes, when the call carries a buffer.
};

/**
 * @brief Static call log plus scripted failure injection.
 * @details All state is `inline static` so the mock can be driven from free functions without the
 *          test having to thread an instance through the facade's `Config` type.
 */
struct recorder final
{
  static constexpr std::size_t capacity = 64U;

  inline static std::array<call_record, capacity> calls{};
  inline static std::size_t call_count{0U};
  inline static bool overflowed{false};

  /// @brief When non-zero, the next @c fail_after calls succeed and every call after that fails.
  inline static std::size_t fail_after{0U};
  inline static bool failure_armed{false};

  /// @brief Byte pattern handed back by read-style operations.
  inline static std::uint8_t read_fill{0xA5U};

  /// @brief Level reported by @ref gpio_api::read.
  inline static bool gpio_level{false};

  /// @brief Monotonic counter returned by @ref timer_api::counter.
  inline static std::uint32_t timer_count{0U};

  /// @brief Synthetic RTC state used by @ref rtc_api.
  inline static std::uint8_t rtc_hours{0U};
  inline static std::uint8_t rtc_minutes{0U};
  inline static std::uint8_t rtc_seconds{0U};

  /// @brief Last MAC address set through ethernet API.
  inline static std::array<std::uint8_t, 6U> mac_address{0U, 0U, 0U, 0U, 0U, 0U};

  /// @brief Simulated SD geometry and backing store.
  static constexpr std::uint32_t sd_block_size_bytes{512U};
  static constexpr std::uint32_t sd_block_count{64U};
  inline static std::array<std::uint8_t, sd_block_size_bytes * sd_block_count> sd_storage{};

  static void reset() noexcept
  {
    call_count = 0U;
    overflowed = false;
    fail_after = 0U;
    failure_armed = false;
    read_fill = 0xA5U;
    gpio_level = false;
    timer_count = 0U;
    rtc_hours = 0U;
    rtc_minutes = 0U;
    rtc_seconds = 0U;
    mac_address = {0U, 0U, 0U, 0U, 0U, 0U};
    for (std::uint8_t& byte : sd_storage)
    {
      byte = 0U;
    }
  }

  /// @brief Makes every call after the first @p successes calls report failure.
  static void fail_after_calls(const std::size_t successes) noexcept
  {
    fail_after = successes;
    failure_armed = true;
  }

  static void record(const peripheral_kind peripheral, const operation op, const std::uint32_t argument = 0U,
                     const std::size_t length = 0U) noexcept
  {
    if (call_count >= capacity)
    {
      overflowed = true;
      return;
    }
    calls[call_count] = call_record{peripheral, op, argument, length};
    ++call_count;
  }

  /// @brief Records a call and returns whether the backend should report success.
  [[nodiscard]] static bool step(const peripheral_kind peripheral, const operation op,
                                 const std::uint32_t argument = 0U, const std::size_t length = 0U) noexcept
  {
    const std::size_t index = call_count;
    record(peripheral, op, argument, length);
    if (!failure_armed)
    {
      return true;
    }
    return index < fail_after;
  }

  [[nodiscard]] static std::span<const call_record> log() noexcept
  {
    return std::span<const call_record>{calls.data(), call_count};
  }

  [[nodiscard]] static bool called(const peripheral_kind peripheral, const operation op) noexcept
  {
    for (std::size_t index = 0U; index < call_count; ++index)
    {
      if (calls[index].peripheral == peripheral && calls[index].op == op)
      {
        return true;
      }
    }
    return false;
  }
};

struct gpio_api final
{
  template <typename PinConfig> static bool configure() noexcept
  {
    return recorder::step(peripheral_kind::gpio, operation::configure, PinConfig::pin_index);
  }

  template <typename PinConfig> static void write(const bool level) noexcept
  {
    recorder::gpio_level = level;
    recorder::record(peripheral_kind::gpio, operation::write, PinConfig::pin_index, level ? 1U : 0U);
  }

  template <typename PinConfig> static bool read() noexcept
  {
    recorder::record(peripheral_kind::gpio, operation::read, PinConfig::pin_index);
    return recorder::gpio_level;
  }

  template <typename PinConfig, auto Handler> static bool enable_interrupt() noexcept
  {
    return recorder::step(peripheral_kind::gpio, operation::enable_interrupt, PinConfig::pin_index);
  }
};

struct uart_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    return recorder::step(peripheral_kind::uart, operation::initialize, Config::instance_index);
  }

  template <typename Config> static bool write_blocking(const std::span<const std::uint8_t> payload) noexcept
  {
    return recorder::step(peripheral_kind::uart, operation::write, Config::instance_index, payload.size());
  }

  template <typename Config> static bool read_blocking(const std::span<std::uint8_t> payload) noexcept
  {
    for (std::uint8_t& byte : payload)
    {
      byte = recorder::read_fill;
    }
    return recorder::step(peripheral_kind::uart, operation::read, Config::instance_index, payload.size());
  }

  template <typename Config, auto Handler> static bool start_async_rx(const std::span<std::uint8_t> payload) noexcept
  {
    return recorder::step(peripheral_kind::uart, operation::start, Config::instance_index, payload.size());
  }

  template <typename Config, auto Handler>
  static bool start_async_tx(const std::span<const std::uint8_t> payload) noexcept
  {
    return recorder::step(peripheral_kind::uart, operation::start, Config::instance_index, payload.size());
  }

  template <typename UartConfig, typename AutobaudConfig> static bool arm_autobaud() noexcept
  {
    (void)sizeof(UartConfig);
    (void)sizeof(AutobaudConfig);
    return false;
  }

  template <typename UartConfig> static bool apply_autobaud(const std::uint32_t baud_rate) noexcept
  {
    (void)sizeof(UartConfig);
    (void)baud_rate;
    return false;
  }
};

struct i2c_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    return recorder::step(peripheral_kind::i2c, operation::initialize, Config::instance_index);
  }

  template <typename Config>
  static bool write_blocking(const std::span<const std::uint8_t> payload, const std::uint16_t address) noexcept
  {
    return recorder::step(peripheral_kind::i2c, operation::write, address, payload.size());
  }

  template <typename Config>
  static bool read_blocking(const std::span<std::uint8_t> payload, const std::uint16_t address) noexcept
  {
    for (std::uint8_t& byte : payload)
    {
      byte = recorder::read_fill;
    }
    return recorder::step(peripheral_kind::i2c, operation::read, address, payload.size());
  }

  template <typename Config, auto Handler>
  static bool start_transfer(const std::span<std::uint8_t> payload, const std::uint16_t address) noexcept
  {
    return recorder::step(peripheral_kind::i2c, operation::start, address, payload.size());
  }
};

struct spi_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    return recorder::step(peripheral_kind::spi, operation::initialize, Config::instance_index);
  }

  template <typename Config>
  static bool transfer(const std::span<const std::uint8_t> tx_payload, const std::span<std::uint8_t> rx_payload) noexcept
  {
    for (std::uint8_t& byte : rx_payload)
    {
      byte = recorder::read_fill;
    }
    return recorder::step(peripheral_kind::spi, operation::transfer, Config::instance_index, tx_payload.size());
  }

  template <typename Config, auto Handler>
  static bool transfer_async(const std::span<const std::uint8_t> tx_payload,
                             const std::span<std::uint8_t> rx_payload) noexcept
  {
    (void)rx_payload;
    return recorder::step(peripheral_kind::spi, operation::start, Config::instance_index, tx_payload.size());
  }
};

struct qspi_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::qspi, operation::initialize);
  }
};

struct rtc_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::rtc, operation::initialize);
  }

  template <typename Config>
  static bool set_time(const std::uint8_t hours, const std::uint8_t minutes, const std::uint8_t seconds) noexcept
  {
    recorder::rtc_hours = hours;
    recorder::rtc_minutes = minutes;
    recorder::rtc_seconds = seconds;
    return recorder::step(peripheral_kind::rtc, operation::write, (static_cast<std::uint32_t>(hours) << 16U) |
                                                                  (static_cast<std::uint32_t>(minutes) << 8U) |
                                                                  static_cast<std::uint32_t>(seconds));
  }

  template <typename Config>
  static bool get_time(std::uint8_t& hours, std::uint8_t& minutes, std::uint8_t& seconds) noexcept
  {
    hours = recorder::rtc_hours;
    minutes = recorder::rtc_minutes;
    seconds = recorder::rtc_seconds;
    return recorder::step(peripheral_kind::rtc, operation::read);
  }

  template <typename Config, auto Handler, typename AlarmTime>
  static bool enable_alarm(const AlarmTime&) noexcept
  {
    (void)Handler;
    return recorder::step(peripheral_kind::rtc, operation::enable_interrupt);
  }
};

struct timer_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::timer, operation::initialize);
  }

  template <typename Config> static std::uint32_t counter() noexcept
  {
    recorder::record(peripheral_kind::timer, operation::read);
    return recorder::timer_count++;
  }
};

struct adc_api final
{
  /// @brief Sample value returned by @ref read_blocking.
  inline static std::uint16_t sample{0U};

  template <typename Config> static bool initialize() noexcept
  {
    return recorder::step(peripheral_kind::adc, operation::initialize, Config::instance_index);
  }

  template <typename Config> static std::uint16_t read_blocking(const std::uint8_t channel) noexcept
  {
    recorder::record(peripheral_kind::adc, operation::read, channel);
    return sample;
  }

  template <typename Config> static bool start_continuous(const std::span<const std::uint8_t> channels) noexcept
  {
    return recorder::step(peripheral_kind::adc, operation::start, Config::instance_index, channels.size());
  }

  template <typename Config, auto Handler> static bool start_async(const std::uint8_t channel) noexcept
  {
    (void)Handler;
    return recorder::step(peripheral_kind::adc, operation::start, channel);
  }

  template <typename Config, auto Handler>
  static bool start_dma_scan(const std::span<const std::uint8_t> channels,
                             const std::span<std::uint16_t> buffer) noexcept
  {
    (void)Handler;
    (void)buffer;
    return recorder::step(peripheral_kind::adc, operation::transfer, Config::instance_index, channels.size());
  }
};

struct dac_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::dac, operation::initialize);
  }

  template <typename Config> static bool write_blocking(const std::uint16_t value) noexcept
  {
    return recorder::step(peripheral_kind::dac, operation::write, value);
  }

  template <typename Config> static bool write_dual(const std::uint16_t a, const std::uint16_t b) noexcept
  {
    return recorder::step(peripheral_kind::dac, operation::write,
                          (static_cast<std::uint32_t>(a) << 16U) | static_cast<std::uint32_t>(b));
  }

  template <typename Config, auto Handler>
  static bool start_dma_waveform(const std::span<const std::uint16_t> waveform) noexcept
  {
    (void)Handler;
    return recorder::step(peripheral_kind::dac, operation::start, 0U, waveform.size());
  }
};

struct can_api final
{
  inline static microhal::can::message last_message{};

  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::can, operation::initialize);
  }

  template <typename Config> static bool transmit(const microhal::can::message& message) noexcept
  {
    last_message = message;
    return recorder::step(peripheral_kind::can, operation::write, message.id, message.dlc);
  }

  template <typename Config> static bool receive(microhal::can::message& message) noexcept
  {
    message = last_message;
    return recorder::step(peripheral_kind::can, operation::read, message.id, message.dlc);
  }

  template <typename Config>
  static bool transmit(const std::uint8_t* frame, const std::size_t length) noexcept
  {
    (void)frame;
    return recorder::step(peripheral_kind::can, operation::write, 0U, length);
  }

  template <typename Config>
  static bool receive(std::uint8_t* frame, std::size_t& length) noexcept
  {
    for (std::size_t index = 0U; index < length; ++index)
    {
      frame[index] = recorder::read_fill;
    }
    return recorder::step(peripheral_kind::can, operation::read, 0U, length);
  }

  template <typename Config>
  static bool configure_filter(const std::uint32_t filter_id, const std::uint32_t mask,
                               const std::uint32_t fifo) noexcept
  {
    return recorder::step(peripheral_kind::can, operation::set_filter,
                          filter_id ^ mask ^ (fifo << 24U));
  }

  template <typename Config, auto Handler> static bool enable_rx_interrupt() noexcept
  {
    (void)Handler;
    return recorder::step(peripheral_kind::can, operation::enable_interrupt);
  }
};

struct usb_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::usb, operation::initialize);
  }

  template <typename Config> static bool connect() noexcept
  {
    return recorder::step(peripheral_kind::usb, operation::connect);
  }

  template <typename Config> static bool disconnect() noexcept
  {
    return recorder::step(peripheral_kind::usb, operation::disconnect);
  }

  template <typename Config>
  static bool endpoint_transmit(const std::uint8_t endpoint, const std::span<const std::uint8_t> data) noexcept
  {
    return recorder::step(peripheral_kind::usb, operation::write, endpoint, data.size());
  }

  template <typename Config>
  static std::uint32_t endpoint_receive(const std::uint8_t endpoint, const std::span<std::uint8_t> buffer) noexcept
  {
    for (std::uint8_t& byte : buffer)
    {
      byte = recorder::read_fill;
    }
    if (!recorder::step(peripheral_kind::usb, operation::read, endpoint, buffer.size()))
    {
      return 0U;
    }
    return static_cast<std::uint32_t>(buffer.size());
  }
};

struct ethernet_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::ethernet, operation::initialize);
  }

  template <typename Config> static bool start() noexcept
  {
    return recorder::step(peripheral_kind::ethernet, operation::start);
  }

  template <typename Config> static bool stop() noexcept
  {
    return recorder::step(peripheral_kind::ethernet, operation::stop);
  }

  template <typename Config> static bool transmit_frame(const std::span<const std::uint8_t> frame) noexcept
  {
    return recorder::step(peripheral_kind::ethernet, operation::write, 0U, frame.size());
  }

  template <typename Config> static std::uint32_t receive_frame(const std::span<std::uint8_t> buffer) noexcept
  {
    for (std::uint8_t& byte : buffer)
    {
      byte = recorder::read_fill;
    }
    if (!recorder::step(peripheral_kind::ethernet, operation::read, 0U, buffer.size()))
    {
      return 0U;
    }
    return static_cast<std::uint32_t>(buffer.size());
  }

  template <typename Config> static bool set_mac_address(const std::array<std::uint8_t, 6U>& address) noexcept
  {
    recorder::mac_address = address;
    return recorder::step(peripheral_kind::ethernet, operation::configure);
  }
};

struct sd_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::sd, operation::initialize);
  }

  template <typename Config>
  static bool read_blocks(const std::uint32_t start_block, const std::span<std::uint8_t> buffer) noexcept
  {
    constexpr std::uint32_t block_size = recorder::sd_block_size_bytes;
    if ((buffer.size() % block_size) != 0U)
    {
      return false;
    }

    const std::uint32_t block_count = static_cast<std::uint32_t>(buffer.size() / block_size);
    if ((start_block + block_count) > recorder::sd_block_count)
    {
      return false;
    }

    const std::size_t offset = static_cast<std::size_t>(start_block) * block_size;
    for (std::size_t index = 0U; index < buffer.size(); ++index)
    {
      buffer[index] = recorder::sd_storage[offset + index];
    }
    return recorder::step(peripheral_kind::sd, operation::read, start_block, buffer.size());
  }

  template <typename Config>
  static bool write_blocks(const std::uint32_t start_block, const std::span<const std::uint8_t> buffer) noexcept
  {
    constexpr std::uint32_t block_size = recorder::sd_block_size_bytes;
    if ((buffer.size() % block_size) != 0U)
    {
      return false;
    }

    const std::uint32_t block_count = static_cast<std::uint32_t>(buffer.size() / block_size);
    if ((start_block + block_count) > recorder::sd_block_count)
    {
      return false;
    }

    const std::size_t offset = static_cast<std::size_t>(start_block) * block_size;
    for (std::size_t index = 0U; index < buffer.size(); ++index)
    {
      recorder::sd_storage[offset + index] = buffer[index];
    }
    return recorder::step(peripheral_kind::sd, operation::write, start_block, buffer.size());
  }

  template <typename Config>
  static bool erase_blocks(const std::uint32_t start_block, const std::uint32_t block_count) noexcept
  {
    if ((start_block + block_count) > recorder::sd_block_count)
    {
      return false;
    }

    const std::size_t offset = static_cast<std::size_t>(start_block) * recorder::sd_block_size_bytes;
    const std::size_t bytes = static_cast<std::size_t>(block_count) * recorder::sd_block_size_bytes;
    for (std::size_t index = 0U; index < bytes; ++index)
    {
      recorder::sd_storage[offset + index] = 0U;
    }
    return recorder::step(peripheral_kind::sd, operation::erase, start_block, block_count);
  }

  template <typename Config> static std::uint32_t capacity_blocks() noexcept
  {
    recorder::record(peripheral_kind::sd, operation::read);
    return recorder::sd_block_count;
  }
};

struct dma_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::dma, operation::initialize);
  }

  template <typename Config, auto Handler>
  static bool start_transfer(const std::uint32_t src_addr, const std::uint32_t dst_addr,
                             const std::uint32_t length) noexcept
  {
    (void)Handler;
    return recorder::step(peripheral_kind::dma, operation::transfer, src_addr ^ dst_addr, length);
  }

  template <typename Config> static bool abort() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::dma, operation::abort);
  }
};

struct flash_api final
{
  template <typename Config> static bool erase(const std::uint32_t address, const std::uint32_t bytes) noexcept
  {
    return recorder::step(peripheral_kind::flash, operation::erase, address, bytes);
  }

  template <typename Config>
  static bool program(const std::uint32_t address, const std::span<const std::uint8_t> data) noexcept
  {
    return recorder::step(peripheral_kind::flash, operation::program, address, data.size());
  }

  template <typename Config> static bool read(const std::uint32_t address, const std::span<std::uint8_t> data) noexcept
  {
    for (std::uint8_t& byte : data)
    {
      byte = recorder::read_fill;
    }
    return recorder::step(peripheral_kind::flash, operation::read, address, data.size());
  }
};

struct watchdog_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    return recorder::step(peripheral_kind::watchdog, operation::initialize);
  }

  template <typename Config> static bool start() noexcept
  {
    return recorder::step(peripheral_kind::watchdog, operation::start);
  }

  template <typename Config> static bool refresh() noexcept
  {
    return recorder::step(peripheral_kind::watchdog, operation::refresh);
  }
};

struct pwm_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    return recorder::step(peripheral_kind::pwm, operation::initialize, Config::timer_index);
  }

  template <typename Config> static bool set_duty_cycle(const std::uint16_t duty_permille) noexcept
  {
    return recorder::step(peripheral_kind::pwm, operation::set_duty_cycle, duty_permille);
  }

  template <typename Config> static bool start() noexcept
  {
    return recorder::step(peripheral_kind::pwm, operation::start, Config::timer_index);
  }

  template <typename Config> static bool stop() noexcept
  {
    return recorder::step(peripheral_kind::pwm, operation::stop, Config::timer_index);
  }
};

struct i2s_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::i2s, operation::initialize);
  }

  template <typename Config> static bool transmit_blocking(const std::span<const std::uint16_t> samples) noexcept
  {
    return recorder::step(peripheral_kind::i2s, operation::write, 0U, samples.size());
  }

  template <typename Config> static bool receive_blocking(const std::span<std::uint16_t> samples) noexcept
  {
    for (std::uint16_t& sample : samples)
    {
      sample = static_cast<std::uint16_t>(recorder::read_fill);
    }
    return recorder::step(peripheral_kind::i2s, operation::read, 0U, samples.size());
  }

  template <typename Config, auto Handler>
  static bool start_dma_tx(const std::span<const std::uint16_t> samples) noexcept
  {
    (void)Handler;
    return recorder::step(peripheral_kind::i2s, operation::start, 0U, samples.size());
  }
};

struct rcc_api final
{
  template <typename Config> static bool configure() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::rcc, operation::configure);
  }

  template <typename Config> static std::uint32_t get_system_clock_hz() noexcept
  {
    recorder::record(peripheral_kind::rcc, operation::read);
    return 48'000'000U;
  }

  template <typename Config>
  static void enable_peripheral_clock(const peripheral_kind peripheral, const std::uint8_t instance) noexcept
  {
    recorder::record(peripheral, operation::start, instance);
  }

  template <typename Config>
  static void disable_peripheral_clock(const peripheral_kind peripheral, const std::uint8_t instance) noexcept
  {
    recorder::record(peripheral, operation::stop, instance);
  }
};

struct crc_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::crc, operation::initialize);
  }

  template <typename Config> static std::uint32_t calculate(const std::span<const std::uint8_t> data) noexcept
  {
    std::uint32_t crc = 0xFFFFFFFFU;
    for (const std::uint8_t byte : data)
    {
      crc ^= static_cast<std::uint32_t>(byte);
      for (std::uint8_t bit = 0U; bit < 8U; ++bit)
      {
        const std::uint32_t lsb = crc & 1U;
        crc >>= 1U;
        if (lsb != 0U)
        {
          crc ^= 0xEDB88320U;
        }
      }
    }
    recorder::record(peripheral_kind::crc, operation::transfer, 0U, data.size());
    return ~crc;
  }

  template <typename Config> static void reset() noexcept
  {
    (void)sizeof(Config);
    recorder::record(peripheral_kind::crc, operation::configure);
  }

  template <typename Config> static std::uint32_t get_random() noexcept
  {
    recorder::timer_count = recorder::timer_count * 1664525U + 1013904223U;
    recorder::record(peripheral_kind::crc, operation::read);
    return recorder::timer_count;
  }
};

} // namespace microhal::backends::mock

namespace microhal
{

/**
 * @brief Capability profile of the host recording backend.
 * @details Only the peripherals the recorder implements are advertised as supported, so the same
 *          capability machinery that guards real hardware also guards the mock. A driver that
 *          compiles against `mock_tag` therefore only uses peripherals the mock can actually
 *          record.
 */
template <> struct backend_traits<mock_tag> final
{
  using backend_tag = mock_tag;
  using gpio_api = backends::mock::gpio_api;
  using i2c_api = backends::mock::i2c_api;
  using spi_api = backends::mock::spi_api;
  using qspi_api = backends::mock::qspi_api;
  using uart_api = backends::mock::uart_api;
  using rtc_api = backends::mock::rtc_api;
  using flash_api = backends::mock::flash_api;
  using timer_api = backends::mock::timer_api;
  using adc_api = backends::mock::adc_api;
  using dac_api = backends::mock::dac_api;
  using can_api = backends::mock::can_api;
  using usb_api = backends::mock::usb_api;
  using ethernet_api = backends::mock::ethernet_api;
  using sd_api = backends::mock::sd_api;
  using dma_api = backends::mock::dma_api;
  using watchdog_api = backends::mock::watchdog_api;
  using pwm_api = backends::mock::pwm_api;
  using i2s_api = backends::mock::i2s_api;
  using rcc_api = backends::mock::rcc_api;
  using crc_api = backends::mock::crc_api;

  static constexpr vendor_kind vendor = vendor_kind::mock;
  static constexpr backend_kind kind = backend_kind::mock_recording;
  static constexpr std::string_view name{"mock-recording"};
  static constexpr std::uint32_t gpio_port_count{8U};
  static constexpr std::uint32_t gpio_pins_per_port{16U};
  static constexpr std::uint32_t gpio_alternate_function_count{16U};

  static consteval struct gpio_capability gpio_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = true,
            .dma = false,
            .instance_count = gpio_port_count,
            .max_frequency_hz = 0U,
            .alternate_function = true,
            .alternate_function_count = gpio_alternate_function_count,
            .port_count = gpio_port_count,
            .pins_per_port = gpio_pins_per_port};
  }

  static consteval struct i2c_capability i2c_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = false,
            .instance_count = 4U,
            .max_frequency_hz = 1'000'000U};
  }

  static consteval struct spi_capability spi_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = false,
            .instance_count = 4U,
            .max_frequency_hz = 50'000'000U};
  }

  static consteval struct qspi_capability qspi_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 80'000'000U,
            .quad_mode = true,
            .octo_mode = false,
            .ddr_mode = false};
  }

  static consteval struct uart_capability uart_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = false,
            .instance_count = 8U,
            .max_frequency_hz = 10'000'000U,
            .fifo = true,
            .lpuart = false,
            .autobaud = false,
            .fifo_depth = 16U};
  }

  static consteval struct rtc_capability rtc_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .periodic = true,
            .alarm = true};
  }

  static consteval struct flash_capability flash_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .alignment_bytes = 4U,
            .erase_bytes = 2'048U};
  }

  static consteval struct timer_capability timer_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = false,
            .dma = false,
            .instance_count = 4U,
            .max_frequency_hz = 100'000'000U,
            .capture_compare = true,
            .periodic = true,
            .pwm = true,
            .channel_count = 4U};
  }

  static consteval struct adc_capability adc_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 1'000'000U,
            .resolution_bits = 12U,
            .channel_count = 16U,
            .differential = false,
            .oversampling = false};
  }

  static consteval struct dac_capability dac_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 1U,
            .max_frequency_hz = 1'000'000U,
            .resolution_bits = 12U,
            .channel_count = 2U,
            .dual_mode = true,
            .waveform_generation = true};
  }

  static consteval struct can_capability can_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = false,
            .instance_count = 2U,
            .max_frequency_hz = 1'000'000U,
            .can_fd = true,
            .time_triggered = true,
            .mailbox_count = 8U,
            .filter_banks = 8U};
  }

  static consteval struct usb_capability usb_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 1U,
            .max_frequency_hz = 12'000'000U,
            .device = true,
            .host = true,
            .otg = true,
            .high_speed = true,
            .endpoint_count = 8U};
  }

  static consteval struct ethernet_capability ethernet_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 1U,
            .max_frequency_hz = 100'000'000U,
            .checksum_offload = true,
            .ptp_timestamp = true,
            .vlan_filter = true,
            .gigabit = true,
            .mtu_bytes = 1'518U};
  }

  static consteval struct sd_capability sd_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 1U,
            .max_frequency_hz = 50'000'000U,
            .high_speed = true,
            .uhs = true,
            .bus_width_4 = true,
            .bus_width_8 = true,
            .block_size_bytes = 512U};
  }

  static consteval struct dma_capability dma_capability() noexcept
  {
    return {.supported = true,
            .blocking = false,
            .non_blocking = true,
            .isr = true,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .stream_count = 8U,
            .channel_count = 8U,
            .mem_to_mem = true,
            .fifo = true,
            .double_buffer = true};
  }

  static consteval struct pwm_capability pwm_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 4U,
            .max_frequency_hz = 1'000'000U,
            .channel_count = 4U,
            .complementary_output = false,
            .dead_time = false};
  }

  static consteval struct watchdog_capability watchdog_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .independent = true,
            .window = false,
            .min_timeout_ms = 1U,
            .max_timeout_ms = 32'000U};
  }

  static consteval struct i2s_capability i2s_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 2U,
            .max_frequency_hz = 24'000'000U,
            .full_duplex = true,
            .master_clock_output = true,
            .max_sample_rate = 192'000U};
  }

  static consteval struct rcc_capability rcc_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 100'000'000U,
            .pll_config = true,
            .hse = true,
            .lse = true,
            .max_system_clock_hz = 100'000'000U};
  }

  static consteval struct crc_capability crc_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = false,
            .dma = true,
            .instance_count = 1U,
            .max_frequency_hz = 100'000'000U,
            .hardware_crc32 = true,
            .programmable_polynomial = true,
            .random_number_generator = true};
  }

  template <typename PinConfig> static consteval bool is_valid_gpio_pin() noexcept
  {
    return (PinConfig::port_index < gpio_port_count) && (PinConfig::pin_index < gpio_pins_per_port);
  }

  template <typename PinConfig> static consteval bool is_valid_alternate_function() noexcept
  {
    return (PinConfig::alternate_function == gpio_no_alternate) ||
           (PinConfig::alternate_function < gpio_alternate_function_count);
  }
};

} // namespace microhal
