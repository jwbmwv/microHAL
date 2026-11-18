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
  toggle,
  initialize,
  write,
  read,
  probe,
  write_read,
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
  set_duty_cycle,
  memory_map,
  set_period,
  capture,
  query,
  reset_counter,
  period_read
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
  inline static status i2c_result{status::ok};

  /// @brief Level reported by @ref gpio_api::read.
  inline static bool gpio_level{false};

  /// @brief Monotonic counter returned by @ref timer_api::counter.
  inline static std::uint32_t timer_count{0U};
  inline static std::uint32_t timer_capture{0U};
  inline static std::uint32_t timer_period{0U};

  inline static bool qspi_memory_mapped{false};
  inline static dma_transfer_state dma_state{dma_transfer_state::idle};

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

  /// @brief Resets the associated backend state.
  static void reset() noexcept
  {
    call_count = 0U;
    overflowed = false;
    fail_after = 0U;
    failure_armed = false;
    read_fill = 0xA5U;
    i2c_result = status::ok;
    gpio_level = false;
    timer_count = 0U;
    timer_capture = 0U;
    timer_period = 0U;
    qspi_memory_mapped = false;
    dma_state = dma_transfer_state::idle;
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

  /// @brief Records a backend operation and its arguments.
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

  /// @brief Returns the recorded backend calls.
  [[nodiscard]] static std::span<const call_record> log() noexcept
  {
    return std::span<const call_record>{calls.data(), call_count};
  }

  /// @brief Checks whether the recorder contains the requested operation.
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
  /// @brief Configures the selected backend resource.
  template <typename PinConfig> static bool configure() noexcept
  {
    return recorder::step(peripheral_kind::gpio, operation::configure, PinConfig::pin_index);
  }

  /// @brief Writes data using the configured backend.
  template <typename PinConfig> static void write(const bool level) noexcept
  {
    recorder::gpio_level = level;
    recorder::record(peripheral_kind::gpio, operation::write, PinConfig::pin_index, level ? 1U : 0U);
  }

  /// @copybrief microhal::gpio::pin::read
  template <typename PinConfig> static bool read() noexcept
  {
    recorder::record(peripheral_kind::gpio, operation::read, PinConfig::pin_index);
    return recorder::gpio_level;
  }

  /// @copybrief microhal::gpio::pin::toggle
  template <typename PinConfig> static status toggle() noexcept
  {
    if (!recorder::step(peripheral_kind::gpio, operation::toggle, PinConfig::pin_index))
    {
      return status::error;
    }
    recorder::gpio_level = !recorder::gpio_level;
    return status::ok;
  }

  /// @brief Enables the interrupt for the GPIO pin.
  template <typename PinConfig, auto Handler> static bool enable_interrupt() noexcept
  {
    return recorder::step(peripheral_kind::gpio, operation::enable_interrupt, PinConfig::pin_index);
  }
};

struct uart_api final
{
  /// @copybrief microhal::uart::port::initialize
  template <typename Config> static bool initialize() noexcept
  {
    return recorder::step(peripheral_kind::uart, operation::initialize, Config::instance_index);
  }

  /// @copybrief microhal::uart::port::write_blocking
  template <typename Config> static bool write_blocking(const std::span<const std::uint8_t> payload) noexcept
  {
    return recorder::step(peripheral_kind::uart, operation::write, Config::instance_index, payload.size());
  }

  /// @copybrief microhal::uart::port::read_blocking
  template <typename Config> static bool read_blocking(const std::span<std::uint8_t> payload) noexcept
  {
    for (std::uint8_t& byte : payload)
    {
      byte = recorder::read_fill;
    }
    return recorder::step(peripheral_kind::uart, operation::read, Config::instance_index, payload.size());
  }

  /// @brief Starts an asynchronous receive operation.
  template <typename Config, auto Handler> static bool start_async_rx(const std::span<std::uint8_t> payload) noexcept
  {
    return recorder::step(peripheral_kind::uart, operation::start, Config::instance_index, payload.size());
  }

  /// @brief Starts an asynchronous transmit operation.
  template <typename Config, auto Handler>
  static bool start_async_tx(const std::span<const std::uint8_t> payload) noexcept
  {
    return recorder::step(peripheral_kind::uart, operation::start, Config::instance_index, payload.size());
  }

  /// @brief Returns false because this backend does not implement the operation.
  template <typename UartConfig, typename AutobaudConfig> static bool arm_autobaud() noexcept
  {
    (void)sizeof(UartConfig);
    (void)sizeof(AutobaudConfig);
    return false;
  }

  /// @brief Returns false because this backend does not implement the operation.
  template <typename UartConfig> static bool apply_autobaud(const std::uint32_t baud_rate) noexcept
  {
    (void)sizeof(UartConfig);
    (void)baud_rate;
    return false;
  }
};

struct i2c_api final
{
  /// @copybrief microhal::i2c::master::initialize
  template <typename Config> static bool initialize() noexcept
  {
    return recorder::step(peripheral_kind::i2c, operation::initialize, Config::instance_index);
  }

  /// @copybrief microhal::i2c::master::write_blocking
  template <typename Config>
  static bool write_blocking(const std::span<const std::uint8_t> payload, const std::uint16_t address) noexcept
  {
    return recorder::step(peripheral_kind::i2c, operation::write, address, payload.size());
  }

  /// @copybrief microhal::i2c::master::read_blocking
  template <typename Config>
  static bool read_blocking(const std::span<std::uint8_t> payload, const std::uint16_t address) noexcept
  {
    for (std::uint8_t& byte : payload)
    {
      byte = recorder::read_fill;
    }
    return recorder::step(peripheral_kind::i2c, operation::read, address, payload.size());
  }

  /// @copybrief microhal::i2c::master::probe
  template <typename Config> static status probe(const std::uint16_t address) noexcept
  {
    (void)sizeof(Config);
    if (!recorder::step(peripheral_kind::i2c, operation::probe, address))
    {
      return status::error;
    }
    return recorder::i2c_result;
  }

  /// @brief Writes data and then reads data using the blocking transfer path.
  template <typename Config>
  static status write_read_blocking(const std::span<const std::uint8_t> tx_payload,
                                    const std::span<std::uint8_t> rx_payload, const std::uint16_t address) noexcept
  {
    for (std::uint8_t& byte : rx_payload)
    {
      byte = recorder::read_fill;
    }

    if (!recorder::step(peripheral_kind::i2c, operation::write_read, address, tx_payload.size() + rx_payload.size()))
    {
      return status::error;
    }
    return recorder::i2c_result;
  }

  /// @brief Starts an asynchronous write followed by a read.
  template <typename Config, auto Handler>
  static status write_read_async(const std::span<const std::uint8_t> tx_payload,
                                 const std::span<std::uint8_t> rx_payload, const std::uint16_t address) noexcept
  {
    const status result = write_read_blocking<Config>(tx_payload, rx_payload, address);
    if constexpr (requires { Handler(result); })
    {
      Handler(result);
    }
    else if constexpr (requires { Handler(); })
    {
      Handler();
    }
    return result;
  }

  /// @brief Starts the transfer for the I2C peripheral.
  template <typename Config, auto Handler>
  static bool start_transfer(const std::span<std::uint8_t> payload, const std::uint16_t address) noexcept
  {
    return recorder::step(peripheral_kind::i2c, operation::start, address, payload.size());
  }
};

struct spi_api final
{
  /// @copybrief microhal::spi::master::initialize
  template <typename Config> static bool initialize() noexcept
  {
    return recorder::step(peripheral_kind::spi, operation::initialize, Config::instance_index);
  }

  /// @brief Transfers data using the configured peripheral.
  template <typename Config>
  static bool transfer(const std::span<const std::uint8_t> tx_payload,
                       const std::span<std::uint8_t> rx_payload) noexcept
  {
    for (std::uint8_t& byte : rx_payload)
    {
      byte = recorder::read_fill;
    }
    return recorder::step(peripheral_kind::spi, operation::transfer, Config::instance_index, tx_payload.size());
  }

  /// @copybrief microhal::spi::master::write_blocking
  template <typename Config> static bool write_blocking(const std::span<const std::uint8_t> payload) noexcept
  {
    return recorder::step(peripheral_kind::spi, operation::write, Config::instance_index, payload.size());
  }

  /// @copybrief microhal::spi::master::read_blocking
  template <typename Config> static bool read_blocking(const std::span<std::uint8_t> payload) noexcept
  {
    for (std::uint8_t& byte : payload)
    {
      byte = 0xFFU;
    }
    return recorder::step(peripheral_kind::spi, operation::read, Config::instance_index, payload.size());
  }

  /// @brief Starts an asynchronous transfer using the configured peripheral.
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
  /// @brief Initializes the configured backend resource.
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::qspi, operation::initialize);
  }

  /// @copybrief microhal::qspi::controller::command_write
  template <typename Config>
  static bool command_write(const qspi_command& request, const std::span<const std::uint8_t> payload) noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::qspi, operation::write, request.instruction, payload.size());
  }

  /// @copybrief microhal::qspi::controller::command_read
  template <typename Config>
  static bool command_read(const qspi_command& request, const std::span<std::uint8_t> payload) noexcept
  {
    (void)sizeof(Config);
    if (!recorder::step(peripheral_kind::qspi, operation::read, request.instruction, payload.size()))
    {
      return false;
    }
    for (std::uint8_t& byte : payload)
    {
      byte = recorder::read_fill;
    }
    return true;
  }

  /// @brief Enters the configured QSPI memory-mapped mode.
  template <typename Config> static bool enter_memory_mapped(const qspi_command& request) noexcept
  {
    (void)sizeof(Config);
    (void)request;
    const bool success = recorder::step(peripheral_kind::qspi, operation::memory_map, 1U);
    recorder::qspi_memory_mapped = success;
    return success;
  }

  /// @copybrief microhal::qspi::controller::exit_memory_mapped
  template <typename Config> static bool exit_memory_mapped() noexcept
  {
    (void)sizeof(Config);
    const bool success = recorder::step(peripheral_kind::qspi, operation::memory_map, 0U);
    recorder::qspi_memory_mapped = !success;
    return success;
  }
};

struct rtc_api final
{
  /// @copybrief microhal::rtc::controller::initialize
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::rtc, operation::initialize);
  }

  /// @brief Sets the time for the RTC.
  template <typename Config>
  static bool set_time(const std::uint8_t hours, const std::uint8_t minutes, const std::uint8_t seconds) noexcept
  {
    recorder::rtc_hours = hours;
    recorder::rtc_minutes = minutes;
    recorder::rtc_seconds = seconds;
    return recorder::step(peripheral_kind::rtc, operation::write,
                          (static_cast<std::uint32_t>(hours) << 16U) | (static_cast<std::uint32_t>(minutes) << 8U) |
                              static_cast<std::uint32_t>(seconds));
  }

  /// @brief Returns the time for the RTC.
  template <typename Config>
  static bool get_time(std::uint8_t& hours, std::uint8_t& minutes, std::uint8_t& seconds) noexcept
  {
    hours = recorder::rtc_hours;
    minutes = recorder::rtc_minutes;
    seconds = recorder::rtc_seconds;
    return recorder::step(peripheral_kind::rtc, operation::read);
  }

  /// @copybrief microhal::rtc::controller::enable_alarm
  template <typename Config, auto Handler, typename AlarmTime> static bool enable_alarm(const AlarmTime&) noexcept
  {
    (void)Handler;
    return recorder::step(peripheral_kind::rtc, operation::enable_interrupt);
  }
};

struct timer_api final
{
  /// @brief Initializes the configured backend resource.
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    if (!recorder::step(peripheral_kind::timer, operation::initialize))
    {
      return false;
    }
    recorder::timer_period = Config::period;
    return true;
  }

  /// @copybrief microhal::timers::timer::start
  template <typename Config> static bool start() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::timer, operation::start);
  }

  /// @copybrief microhal::timers::timer::stop
  template <typename Config> static bool stop() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::timer, operation::stop);
  }

  /// @copybrief microhal::timers::timer::set_period
  template <typename Config> static bool set_period(const std::uint32_t period) noexcept
  {
    (void)sizeof(Config);
    if (!recorder::step(peripheral_kind::timer, operation::set_period, period))
    {
      return false;
    }
    recorder::timer_period = period;
    return true;
  }

  /// @brief Resets the counter for the timer.
  template <typename Config> static bool reset_counter() noexcept
  {
    (void)sizeof(Config);
    if (!recorder::step(peripheral_kind::timer, operation::reset_counter))
    {
      return false;
    }
    recorder::timer_count = 0U;
    return true;
  }

  /// @copybrief microhal::timers::timer::period
  template <typename Config> static result<std::uint32_t> period() noexcept
  {
    (void)sizeof(Config);
    if (!recorder::step(peripheral_kind::timer, operation::period_read))
    {
      return status::error;
    }
    return recorder::timer_period;
  }

  /// @copybrief microhal::timers::timer::counter
  template <typename Config> static std::uint32_t counter() noexcept
  {
    recorder::record(peripheral_kind::timer, operation::read);
    return recorder::timer_count++;
  }

  /// @copybrief microhal::timers::timer::capture_value
  template <typename Config, typename ChannelConfig> static result<std::uint32_t> capture_value() noexcept
  {
    (void)sizeof(Config);
    if (!recorder::step(peripheral_kind::timer, operation::capture, ChannelConfig::channel_index))
    {
      return status::error;
    }
    return recorder::timer_capture;
  }
};

struct adc_api final
{
  /// @brief Sample value returned by @ref read_blocking.
  inline static std::uint16_t sample{0U};

  /// @copybrief microhal::adc::converter::initialize
  template <typename Config> static bool initialize() noexcept
  {
    return recorder::step(peripheral_kind::adc, operation::initialize, Config::instance_index);
  }

  /// @copybrief microhal::adc::converter::read_blocking
  template <typename Config> static result<std::uint16_t> read_blocking(const std::uint8_t channel) noexcept
  {
    if (!recorder::step(peripheral_kind::adc, operation::read, channel))
    {
      return status::error;
    }
    return sample;
  }

  /// @copybrief microhal::adc::converter::start_continuous
  template <typename Config> static bool start_continuous(const std::span<const std::uint8_t> channels) noexcept
  {
    return recorder::step(peripheral_kind::adc, operation::start, Config::instance_index, channels.size());
  }

  /// @copybrief microhal::adc::converter::start_async
  template <typename Config, auto Handler> static bool start_async(const std::uint8_t channel) noexcept
  {
    (void)Handler;
    return recorder::step(peripheral_kind::adc, operation::start, channel);
  }

  /// @copybrief microhal::adc::converter::start_dma_scan
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
  /// @copybrief microhal::dac::converter::initialize
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::dac, operation::initialize);
  }

  /// @copybrief microhal::dac::converter::write_blocking
  template <typename Config> static bool write_blocking(const std::uint16_t value) noexcept
  {
    return recorder::step(peripheral_kind::dac, operation::write, value);
  }

  /// @copybrief microhal::dac::converter::write_dual
  template <typename Config> static bool write_dual(const std::uint16_t a, const std::uint16_t b) noexcept
  {
    return recorder::step(peripheral_kind::dac, operation::write,
                          (static_cast<std::uint32_t>(a) << 16U) | static_cast<std::uint32_t>(b));
  }

  /// @copybrief microhal::dac::converter::start_dma_waveform
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

  /// @copybrief microhal::can::bus::initialize
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::can, operation::initialize);
  }

  /// @copybrief microhal::can::bus::transmit
  template <typename Config> static bool transmit(const microhal::can::message& message) noexcept
  {
    last_message = message;
    return recorder::step(peripheral_kind::can, operation::write, message.id, message.dlc);
  }

  /// @copybrief microhal::can::bus::receive
  template <typename Config> static bool receive(microhal::can::message& message) noexcept
  {
    message = last_message;
    return recorder::step(peripheral_kind::can, operation::read, message.id, message.dlc);
  }

  /// @copybrief microhal::can::bus::transmit
  template <typename Config> static bool transmit(const std::uint8_t* frame, const std::size_t length) noexcept
  {
    (void)frame;
    return recorder::step(peripheral_kind::can, operation::write, 0U, length);
  }

  /// @copybrief microhal::can::bus::receive
  template <typename Config> static bool receive(std::uint8_t* frame, std::size_t& length) noexcept
  {
    for (std::size_t index = 0U; index < length; ++index)
    {
      frame[index] = recorder::read_fill;
    }
    return recorder::step(peripheral_kind::can, operation::read, 0U, length);
  }

  /// @copybrief microhal::can::bus::configure_filter
  template <typename Config>
  static bool configure_filter(const std::uint32_t filter_id, const std::uint32_t mask,
                               const std::uint32_t fifo) noexcept
  {
    return recorder::step(peripheral_kind::can, operation::set_filter, filter_id ^ mask ^ (fifo << 24U));
  }

  /// @brief Enables the receive interrupt for the CAN peripheral.
  template <typename Config, auto Handler> static bool enable_rx_interrupt() noexcept
  {
    (void)Handler;
    return recorder::step(peripheral_kind::can, operation::enable_interrupt);
  }
};

struct usb_api final
{
  /// @copybrief microhal::usb::device::initialize
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::usb, operation::initialize);
  }

  /// @copybrief microhal::usb::device::connect
  template <typename Config> static bool connect() noexcept
  {
    return recorder::step(peripheral_kind::usb, operation::connect);
  }

  /// @copybrief microhal::usb::device::disconnect
  template <typename Config> static bool disconnect() noexcept
  {
    return recorder::step(peripheral_kind::usb, operation::disconnect);
  }

  /// @copybrief microhal::usb::device::endpoint_transmit
  template <typename Config>
  static bool endpoint_transmit(const std::uint8_t endpoint, const std::span<const std::uint8_t> data) noexcept
  {
    return recorder::step(peripheral_kind::usb, operation::write, endpoint, data.size());
  }

  /// @copybrief microhal::usb::device::endpoint_receive
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
  /// @copybrief microhal::ethernet::mac::initialize
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::ethernet, operation::initialize);
  }

  /// @copybrief microhal::ethernet::mac::start
  template <typename Config> static bool start() noexcept
  {
    return recorder::step(peripheral_kind::ethernet, operation::start);
  }

  /// @copybrief microhal::ethernet::mac::stop
  template <typename Config> static bool stop() noexcept
  {
    return recorder::step(peripheral_kind::ethernet, operation::stop);
  }

  /// @copybrief microhal::ethernet::mac::transmit_frame
  template <typename Config> static bool transmit_frame(const std::span<const std::uint8_t> frame) noexcept
  {
    return recorder::step(peripheral_kind::ethernet, operation::write, 0U, frame.size());
  }

  /// @copybrief microhal::ethernet::mac::receive_frame
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

  /// @copybrief microhal::ethernet::mac::set_mac_address
  template <typename Config> static bool set_mac_address(const std::array<std::uint8_t, 6U>& address) noexcept
  {
    recorder::mac_address = address;
    return recorder::step(peripheral_kind::ethernet, operation::configure);
  }
};

struct sd_api final
{
  /// @copybrief microhal::sd::card::initialize
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::sd, operation::initialize);
  }

  /// @copybrief microhal::sd::card::read_blocks
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

  /// @copybrief microhal::sd::card::write_blocks
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

  /// @copybrief microhal::sd::card::erase_blocks
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

  /// @copybrief microhal::sd::card::capacity_blocks
  template <typename Config> static std::uint32_t capacity_blocks() noexcept
  {
    recorder::record(peripheral_kind::sd, operation::read);
    return recorder::sd_block_count;
  }
};

struct dma_api final
{
  /// @copybrief microhal::dma::controller::initialize
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::dma, operation::initialize);
  }

  /// @copybrief microhal::dma::controller::start_transfer
  template <typename Config, auto Handler>
  static bool start_transfer(const std::uint32_t src_addr, const std::uint32_t dst_addr,
                             const std::uint32_t length) noexcept
  {
    const bool success = recorder::step(peripheral_kind::dma, operation::transfer, src_addr ^ dst_addr, length);
    recorder::dma_state = success ? dma_transfer_state::complete : dma_transfer_state::error;
    if constexpr (requires { Handler(); })
    {
      if (success)
      {
        Handler();
      }
    }
    return success;
  }

  /// @copybrief microhal::dma::controller::abort
  template <typename Config> static bool abort() noexcept
  {
    (void)sizeof(Config);
    const bool success = recorder::step(peripheral_kind::dma, operation::abort);
    if (success)
    {
      recorder::dma_state = dma_transfer_state::idle;
    }
    return success;
  }

  /// @copybrief microhal::dma::controller::transfer_status
  template <typename Config> static result<dma_transfer_state> transfer_status() noexcept
  {
    (void)sizeof(Config);
    if (!recorder::step(peripheral_kind::dma, operation::query))
    {
      return status::error;
    }
    return recorder::dma_state;
  }
};

struct flash_api final
{
  /// @brief Erases the requested storage range.
  template <typename Config> static bool erase(const std::uint32_t address, const std::uint32_t bytes) noexcept
  {
    return recorder::step(peripheral_kind::flash, operation::erase, address, bytes);
  }

  /// @brief Programs the supplied data into storage.
  template <typename Config>
  static bool program(const std::uint32_t address, const std::span<const std::uint8_t> data) noexcept
  {
    return recorder::step(peripheral_kind::flash, operation::program, address, data.size());
  }

  /// @copybrief microhal::flash::controller::read
  template <typename Config> static bool read(const std::uint32_t address, const std::span<std::uint8_t> data) noexcept
  {
    for (std::uint8_t& byte : data)
    {
      byte = recorder::read_fill;
    }
    return recorder::step(peripheral_kind::flash, operation::read, address, data.size());
  }

  /// @copybrief microhal::flash::controller::erase_region
  template <typename Config> static result<flash_erase_region> erase_region(const std::uint32_t address) noexcept
  {
    constexpr std::uint32_t erase_size = Config::erase_size_bytes;
    const std::uint32_t start_address = address - (address % erase_size);
    return flash_erase_region{.start_address = start_address, .size_bytes = erase_size};
  }
};

struct watchdog_api final
{
  /// @copybrief microhal::watchdog::timer::initialize
  template <typename Config> static bool initialize() noexcept
  {
    return recorder::step(peripheral_kind::watchdog, operation::initialize);
  }

  /// @copybrief microhal::watchdog::timer::start
  template <typename Config> static bool start() noexcept
  {
    return recorder::step(peripheral_kind::watchdog, operation::start);
  }

  /// @copybrief microhal::watchdog::timer::refresh
  template <typename Config> static bool refresh() noexcept
  {
    return recorder::step(peripheral_kind::watchdog, operation::refresh);
  }
};

struct pwm_api final
{
  /// @copybrief microhal::pwm::output::initialize
  template <typename Config> static bool initialize() noexcept
  {
    return recorder::step(peripheral_kind::pwm, operation::initialize, Config::timer_index);
  }

  /// @copybrief microhal::pwm::output::set_duty_cycle
  template <typename Config> static bool set_duty_cycle(const std::uint16_t duty_permille) noexcept
  {
    return recorder::step(peripheral_kind::pwm, operation::set_duty_cycle, duty_permille);
  }

  /// @copybrief microhal::pwm::output::start
  template <typename Config> static bool start() noexcept
  {
    return recorder::step(peripheral_kind::pwm, operation::start, Config::timer_index);
  }

  /// @copybrief microhal::pwm::output::stop
  template <typename Config> static bool stop() noexcept
  {
    return recorder::step(peripheral_kind::pwm, operation::stop, Config::timer_index);
  }
};

struct i2s_api final
{
  /// @copybrief microhal::i2s::interface::initialize
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::i2s, operation::initialize);
  }

  /// @copybrief microhal::i2s::interface::transmit_blocking
  template <typename Config> static bool transmit_blocking(const std::span<const std::uint16_t> samples) noexcept
  {
    return recorder::step(peripheral_kind::i2s, operation::write, 0U, samples.size());
  }

  /// @copybrief microhal::i2s::interface::receive_blocking
  template <typename Config> static bool receive_blocking(const std::span<std::uint16_t> samples) noexcept
  {
    for (std::uint16_t& sample : samples)
    {
      sample = static_cast<std::uint16_t>(recorder::read_fill);
    }
    return recorder::step(peripheral_kind::i2s, operation::read, 0U, samples.size());
  }

  /// @copybrief microhal::i2s::interface::start_dma_tx
  template <typename Config, auto Handler>
  static bool start_dma_tx(const std::span<const std::uint16_t> samples) noexcept
  {
    const bool completed = recorder::step(peripheral_kind::i2s, operation::start, 0U, samples.size());
    if (completed)
    {
      if constexpr (requires { Handler(); })
      {
        Handler();
      }
    }
    return completed;
  }
};

struct rcc_api final
{
  /// @copybrief microhal::rcc::clock_controller::configure
  template <typename Config> static bool configure() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::rcc, operation::configure);
  }

  /// @brief Returns the system-clock frequency in hertz.
  template <typename Config> static std::uint32_t get_system_clock_hz() noexcept
  {
    recorder::record(peripheral_kind::rcc, operation::read);
    return 48'000'000U;
  }

  /// @copybrief microhal::rcc::clock_controller::enable_peripheral_clock
  template <typename Config>
  static void enable_peripheral_clock(const peripheral_kind peripheral, const std::uint8_t instance) noexcept
  {
    recorder::record(peripheral, operation::start, instance);
  }

  /// @copybrief microhal::rcc::clock_controller::disable_peripheral_clock
  template <typename Config>
  static void disable_peripheral_clock(const peripheral_kind peripheral, const std::uint8_t instance) noexcept
  {
    recorder::record(peripheral, operation::stop, instance);
  }
};

struct crc_api final
{
  /// @copybrief microhal::crc::accelerator::initialize
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return recorder::step(peripheral_kind::crc, operation::initialize);
  }

  /// @brief Calculates the configured checksum.
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

  /// @copybrief microhal::crc::accelerator::reset
  template <typename Config> static void reset() noexcept
  {
    (void)sizeof(Config);
    recorder::record(peripheral_kind::crc, operation::configure);
  }

  /// @copybrief microhal::crc::accelerator::get_random
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

  /// @brief Reports the backend's GPIO capabilities.
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
            .pins_per_port = gpio_pins_per_port,
            .output_toggle = true};
  }

  /// @brief Reports the backend's I2C capabilities.
  static consteval struct i2c_capability i2c_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = false,
            .instance_count = 4U,
            .max_frequency_hz = 1'000'000U,
            .ten_bit_addressing = true,
            .repeated_start = true,
            .address_probe = true,
            .clock_stretching = true,
            .smbus_pec = true};
  }

  /// @brief Reports the backend's SPI capabilities.
  static consteval struct spi_capability spi_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = false,
            .instance_count = 4U,
            .max_frequency_hz = 50'000'000U,
            .transmit_only = true,
            .receive_only = true,
            .async_transfer = true};
  }

  /// @brief Reports the backend's QSPI capabilities.
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
            .dual_mode = true,
            .octo_mode = false,
            .ddr_mode = false,
            .command_transfer = true,
            .memory_mapped = true,
            .max_address_bytes = 4U};
  }

  /// @brief Reports the backend's UART capabilities.
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
            .fifo_depth = 16U,
            .parity = true,
            .two_stop_bits = true,
            .one_and_half_stop_bits = true,
            .hardware_flow_control = false,
            .max_baud_rate = 10'000'000U,
            .data_bits_mask = static_cast<std::uint16_t>(
                uart_data_bits_mask(uart_data_bits::five) | uart_data_bits_mask(uart_data_bits::six) |
                uart_data_bits_mask(uart_data_bits::seven) | uart_data_bits_mask(uart_data_bits::eight) |
                uart_data_bits_mask(uart_data_bits::nine))};
  }

  /// @brief Reports the backend's RTC capabilities.
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
            .alarm = true,
            .calendar = false,
            .subsecond = false};
  }

  /// @brief Reports the backend's flash capabilities.
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
            .erase_bytes = 2'048U,
            .erase_region_query = true,
            .program_readback = true};
  }

  /// @brief Reports the backend's timer capabilities.
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
            .channel_count = 4U,
            .period_readback = true,
            .counter_reset = true};
  }

  /// @brief Reports the backend's ADC capabilities.
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
            .oversampling = false,
            .sequential_scan = true,
            .voltage_conversion = true};
  }

  /// @brief Reports the backend's DAC capabilities.
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
            .waveform_generation = true,
            .output_buffer = true,
            .voltage_conversion = true,
            .sequence_write = true};
  }

  /// @brief Reports the backend's CAN capabilities.
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
            .filter_banks = 8U,
            .extended_ids = true,
            .remote_frames = true,
            .rx_interrupt = true};
  }

  /// @brief Reports the backend's USB capabilities.
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
            .endpoint_count = 8U,
            .endpoint_transfer = true,
            .endpoint_stall = false,
            .max_packet_size_bytes = 64U};
  }

  /// @brief Reports the backend's Ethernet capabilities.
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

  /// @brief Reports the backend's SD capabilities.
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
            .block_size_bytes = 512U,
            .block_io = true,
            .erase = true,
            .capacity_query = true,
            .bus_width_1 = true};
  }

  /// @brief Reports the backend's DMA capabilities.
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
            .double_buffer = true,
            .transfer_status = true,
            .abort = true,
            .completion_callback = true,
            .address_width_bits = 32U};
  }

  /// @brief Reports the backend's PWM capabilities.
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
            .dead_time = false,
            .runtime_duty_update = true,
            .runtime_start_stop = true};
  }

  /// @brief Reports the backend's watchdog capabilities.
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
            .max_timeout_ms = 32'000U,
            .runtime_refresh = true,
            .reset_on_timeout = true};
  }

  /// @brief Reports the backend's I2S capabilities.
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
            .master_mode = true,
            .slave_mode = true,
            .master_clock_output = true,
            .max_sample_rate = 192'000U,
            .transmit_dma = true,
            .receive_dma = false,
            .sample_bits = 16U};
  }

  /// @brief Reports the backend's RCC capabilities.
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
            .max_system_clock_hz = 100'000'000U,
            .clock_configuration = true,
            .peripheral_clock_gating = true,
            .system_clock_readback = true};
  }

  /// @brief Reports the backend's CRC capabilities.
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
            .random_number_generator = true,
            .seed_configurable = false,
            .output_xor_configurable = false,
            .checksum_width_bits = 32U};
  }

  /// @brief Checks whether the GPIO pin is valid for the backend.
  template <typename PinConfig> static consteval bool is_valid_gpio_pin() noexcept
  {
    return (PinConfig::port_index < gpio_port_count) && (PinConfig::pin_index < gpio_pins_per_port);
  }

  /// @brief Checks whether the alternate function is valid for the backend.
  template <typename PinConfig> static consteval bool is_valid_alternate_function() noexcept
  {
    return (PinConfig::alternate_function == gpio_no_alternate) ||
           (PinConfig::alternate_function < gpio_alternate_function_count);
  }
};

} // namespace microhal
