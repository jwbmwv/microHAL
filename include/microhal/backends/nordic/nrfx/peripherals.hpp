// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <array>
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <span>

#include "microhal/core/traits.hpp"

#if MICROHAL_NORDIC_NRFX_PRESENT &&                                                                                    \
    (defined(NRF52840_XXAA) || defined(NRF5340_XXAA_APPLICATION) || defined(NRF54L15_XXAA))
#define MICROHAL_NORDIC_NATIVE_TARGET 1
#else
#define MICROHAL_NORDIC_NATIVE_TARGET 0
#endif

#if MICROHAL_NORDIC_NATIVE_TARGET && defined(NRFX_SPIM_ENABLED) && NRFX_SPIM_ENABLED
#include <nrfx_spim.h>
#define MICROHAL_NORDIC_HAS_SPIM 1
#else
#define MICROHAL_NORDIC_HAS_SPIM 0
#endif
#if MICROHAL_NORDIC_NATIVE_TARGET && defined(NRFX_UARTE_ENABLED) && NRFX_UARTE_ENABLED
#include <nrfx_uarte.h>
#define MICROHAL_NORDIC_HAS_UARTE 1
#else
#define MICROHAL_NORDIC_HAS_UARTE 0
#endif
#if MICROHAL_NORDIC_NATIVE_TARGET && defined(NRFX_TIMER_ENABLED) && NRFX_TIMER_ENABLED
#include <nrfx_timer.h>
#define MICROHAL_NORDIC_HAS_TIMER 1
#else
#define MICROHAL_NORDIC_HAS_TIMER 0
#endif
#if MICROHAL_NORDIC_NATIVE_TARGET && defined(NRFX_PWM_ENABLED) && NRFX_PWM_ENABLED
#include <nrfx_pwm.h>
#define MICROHAL_NORDIC_HAS_PWM 1
#else
#define MICROHAL_NORDIC_HAS_PWM 0
#endif
#if MICROHAL_NORDIC_NATIVE_TARGET && defined(NRFX_WDT_ENABLED) && NRFX_WDT_ENABLED
#include <nrfx_wdt.h>
#define MICROHAL_NORDIC_HAS_WDT 1
#else
#define MICROHAL_NORDIC_HAS_WDT 0
#endif
#if MICROHAL_NORDIC_NATIVE_TARGET && defined(NRFX_SAADC_ENABLED) && NRFX_SAADC_ENABLED
#include <nrfx_saadc.h>
#define MICROHAL_NORDIC_HAS_SAADC 1
#else
#define MICROHAL_NORDIC_HAS_SAADC 0
#endif
#if MICROHAL_NORDIC_NATIVE_TARGET && defined(NRFX_GPIOTE_ENABLED) && NRFX_GPIOTE_ENABLED
#include <nrfx_gpiote.h>
#define MICROHAL_NORDIC_HAS_GPIOTE 1
#else
#define MICROHAL_NORDIC_HAS_GPIOTE 0
#endif
#if MICROHAL_NORDIC_NATIVE_TARGET && !defined(NRF54L15_XXAA) && defined(NRFX_QSPI_ENABLED) && NRFX_QSPI_ENABLED
#include <nrfx_qspi.h>
#define MICROHAL_NORDIC_HAS_QSPI 1
#else
#define MICROHAL_NORDIC_HAS_QSPI 0
#endif
#if MICROHAL_NORDIC_NATIVE_TARGET && defined(NRFX_I2S_ENABLED) && NRFX_I2S_ENABLED
#include <nrfx_i2s.h>
#define MICROHAL_NORDIC_HAS_I2S 1
#else
#define MICROHAL_NORDIC_HAS_I2S 0
#endif
#if MICROHAL_NORDIC_NATIVE_TARGET && !defined(NRF54L15_XXAA) && defined(NRFX_RTC_ENABLED) && NRFX_RTC_ENABLED
#if NRFX_API_VER_AT_LEAST(4, 1, 0)
#include <nrfx_rtc.h>
#define MICROHAL_NORDIC_HAS_RTC 1
#endif
#endif
#ifndef MICROHAL_NORDIC_HAS_RTC
#define MICROHAL_NORDIC_HAS_RTC 0
#endif

namespace microhal::backends::nordic::nrfx
{
namespace detail
{
constexpr status driver_result(const int error) noexcept
{
  switch (error)
  {
  case 0:
    return status::ok;
  case -EBUSY:
    return status::busy;
  case -EINVAL:
  case -E2BIG:
  case -EACCES:
    return status::invalid_argument;
  case -ETIMEDOUT:
    return status::timeout;
  case -ENOTSUP:
  case -EINPROGRESS:
    return status::unsupported;
  default:
    return status::io_error;
  }
}

template <typename Config> constexpr std::uint32_t timeout_ms() noexcept
{
  if constexpr (requires { Config::timeout_ms; })
    return Config::timeout_ms;
  return 1000U;
}

template <auto Handler> void notify(const status value) noexcept
{
  if constexpr (requires { Handler(value); })
    Handler(value);
  else if constexpr (requires { Handler(); })
    Handler();
}

struct pwm_timing
{
  std::uint32_t prescaler;
  std::uint16_t top;
};
constexpr result<pwm_timing> pwm_clock(const std::uint32_t frequency) noexcept
{
  if (frequency == 0U)
    return status::invalid_argument;
  std::uint32_t clock = 16'000'000U;
  std::uint32_t prescaler = 0U;
  while (prescaler < 7U && clock / frequency > 32767U)
  {
    clock /= 2U;
    ++prescaler;
  }
  if (clock % frequency != 0U || clock / frequency < 1U || clock / frequency > 32767U)
    return status::invalid_argument;
  return pwm_timing{prescaler, static_cast<std::uint16_t>(clock / frequency)};
}

constexpr result<std::uint32_t> saadc_channel_mask(const std::span<const std::uint8_t> channels) noexcept
{
  if (channels.empty() || channels.size() > 8U)
    return status::invalid_argument;
  std::uint32_t mask = 0U;
  for (std::size_t i = 0U; i < channels.size(); ++i)
  {
    if (channels[i] >= 8U || (i > 0U && channels[i] <= channels[i - 1U]))
      return status::invalid_argument;
    mask |= 1U << channels[i];
  }
  return mask;
}

constexpr bool qspi_range_valid(const std::uint32_t address, const std::size_t size, const std::uint8_t address_bytes,
                                const std::size_t max_count) noexcept
{
  if ((address_bytes != 3U && address_bytes != 4U) || size == 0U || size > max_count)
    return false;
  const std::uint64_t limit = std::uint64_t{1U} << (address_bytes * 8U);
  return address < limit && size <= limit - address;
}

constexpr bool i2s_count_valid(const std::size_t samples, const std::size_t max_words) noexcept
{
  return samples >= 4U && samples <= 65534U && samples % 2U == 0U && samples / 2U <= max_words;
}

constexpr bool serial_count_valid(const std::size_t bytes) noexcept
{
  return bytes <= UINT16_MAX;
}

struct transfer_state
{
  std::atomic<bool> active{false};
  std::atomic<bool> complete{false};
  status result{status::error};
  void (*completion)(status) noexcept {nullptr};

  bool begin(void (*callback)(status) noexcept) noexcept
  {
    if (active.exchange(true))
      return false;
    complete.store(false);
    result = status::error;
    completion = callback;
    return true;
  }
  void finish(const status value) noexcept
  {
    result = value;
    const auto callback = completion;
    completion = nullptr;
    complete.store(true, std::memory_order_release);
    active.store(false);
    if (callback != nullptr)
      callback(value);
  }
};

template <typename Config, typename Predicate> bool wait_until(Predicate ready) noexcept
{
#if MICROHAL_NORDIC_SYSTEM_CORE_CLOCK
  if (SystemCoreClock == 0U)
    return false;
  std::uint64_t remaining = static_cast<std::uint64_t>(timeout_ms<Config>()) * 1000U;
  while (!ready())
  {
    if (remaining == 0U)
      return false;
    --remaining;
    nrfx_coredep_delay_us(1U);
  }
  return true;
#else
  (void)ready;
  return false;
#endif
}

#if MICROHAL_NORDIC_HAS_SPIM
inline nrfx_spim_t spim_instances[]{
#if defined(NRF54L15_XXAA)
    {NRF_SPIM00, {}}, {NRF_SPIM20, {}}, {NRF_SPIM21, {}}, {NRF_SPIM22, {}}, {NRF_SPIM30, {}}
#else
    {NRF_SPIM0, {}},
    {NRF_SPIM1, {}},
    {NRF_SPIM2, {}},
    {NRF_SPIM3, {}}
#if defined(NRF5340_XXAA_APPLICATION)
    ,
    {NRF_SPIM4, {}}
#endif
#endif
};
inline std::array<transfer_state, std::size(spim_instances)> spim_states{};
inline void spim_handler(nrfx_spim_event_t const*, void* context) noexcept
{
  static_cast<transfer_state*>(context)->finish(status::ok);
}
#endif

#if MICROHAL_NORDIC_HAS_UARTE
inline nrfx_uarte_t uarte_instances[]{
#if defined(NRF54L15_XXAA)
    {NRF_UARTE00, {}}, {NRF_UARTE20, {}}, {NRF_UARTE21, {}}, {NRF_UARTE22, {}}, {NRF_UARTE30, {}}
#else
    {NRF_UARTE0, {}},
    {NRF_UARTE1, {}}
#if defined(NRF5340_XXAA_APPLICATION)
    ,
    {NRF_UARTE2, {}},
    {NRF_UARTE3, {}}
#endif
#endif
};
struct uarte_state
{
  nrfx_uarte_t* instance{};
  transfer_state tx{};
  transfer_state rx{};
  std::size_t tx_size{};
  std::size_t rx_size{};
  bool rx_error{};
  status rx_result{status::io_error};
};
inline std::array<uarte_state, std::size(uarte_instances)> uarte_states{};
inline void uarte_handler(nrfx_uarte_event_t const* event, void* context) noexcept
{
  auto& state = *static_cast<uarte_state*>(context);
  if (event->type == NRFX_UARTE_EVT_TX_DONE)
    state.tx.finish(event->data.tx.length == state.tx_size ? status::ok : status::io_error);
  else if (event->type == NRFX_UARTE_EVT_RX_DONE && !state.rx_error)
    state.rx_result = event->data.rx.length == state.rx_size ? status::ok : status::io_error;
  else if (event->type == NRFX_UARTE_EVT_ERROR && state.rx.active.load())
  {
    state.rx_error = true;
    (void)nrfx_uarte_rx_abort(state.instance, true, false);
  }
  else if (event->type == NRFX_UARTE_EVT_RX_DISABLED && state.rx.active.load())
    state.rx.finish(state.rx_error ? status::io_error : state.rx_result);
}

constexpr nrf_uarte_baudrate_t uarte_baudrate(const std::uint32_t baud) noexcept
{
  switch (baud)
  {
  case 1200U:
    return NRF_UARTE_BAUDRATE_1200;
  case 2400U:
    return NRF_UARTE_BAUDRATE_2400;
  case 4800U:
    return NRF_UARTE_BAUDRATE_4800;
  case 9600U:
    return NRF_UARTE_BAUDRATE_9600;
  case 14400U:
    return NRF_UARTE_BAUDRATE_14400;
  case 19200U:
    return NRF_UARTE_BAUDRATE_19200;
  case 28800U:
    return NRF_UARTE_BAUDRATE_28800;
  case 31250U:
    return NRF_UARTE_BAUDRATE_31250;
  case 38400U:
    return NRF_UARTE_BAUDRATE_38400;
  case 56000U:
    return NRF_UARTE_BAUDRATE_56000;
  case 57600U:
    return NRF_UARTE_BAUDRATE_57600;
  case 76800U:
    return NRF_UARTE_BAUDRATE_76800;
  case 115200U:
    return NRF_UARTE_BAUDRATE_115200;
  case 230400U:
    return NRF_UARTE_BAUDRATE_230400;
  case 250000U:
    return NRF_UARTE_BAUDRATE_250000;
  case 460800U:
    return NRF_UARTE_BAUDRATE_460800;
  case 921600U:
    return NRF_UARTE_BAUDRATE_921600;
  case 1000000U:
    return NRF_UARTE_BAUDRATE_1000000;
  default:
    return static_cast<nrf_uarte_baudrate_t>(0U);
  }
}
#endif

#if MICROHAL_NORDIC_HAS_TIMER
inline nrfx_timer_t timer_instances[]{
#if defined(NRF54L15_XXAA)
    {NRF_TIMER00, {}}, {NRF_TIMER10, {}}, {NRF_TIMER20, {}}, {NRF_TIMER21, {}},
    {NRF_TIMER22, {}}, {NRF_TIMER23, {}}, {NRF_TIMER24, {}}
#else
    {NRF_TIMER0, {}},
    {NRF_TIMER1, {}},
    {NRF_TIMER2, {}}
#if defined(NRF52840_XXAA)
    ,
    {NRF_TIMER3, {}},
    {NRF_TIMER4, {}}
#endif
#endif
};
struct timer_state
{
  std::uint32_t period{};
  std::array<void (*)() noexcept, 2U> callbacks{};
};
inline std::array<timer_state, std::size(timer_instances)> timer_states{};
inline void timer_handler(const nrf_timer_event_t event, void* context) noexcept
{
  auto& state = *static_cast<timer_state*>(context);
  const auto channel = (static_cast<std::uint32_t>(event) - NRF_TIMER_EVENT_COMPARE0) / 4U;
  if (channel > 0U && channel <= state.callbacks.size() && state.callbacks[channel - 1U] != nullptr)
    state.callbacks[channel - 1U]();
}
#endif

#if MICROHAL_NORDIC_HAS_PWM
inline nrfx_pwm_t pwm_instances[]{
#if defined(NRF54L15_XXAA)
    {NRF_PWM20, {}}, {NRF_PWM21, {}}, {NRF_PWM22, {}}
#else
    {NRF_PWM0, {}}, {NRF_PWM1, {}}, {NRF_PWM2, {}}, {NRF_PWM3, {}}
#endif
};
struct pwm_state
{
  alignas(4) nrf_pwm_values_individual_t values{};
  std::uint16_t top{};
  std::uint8_t channel{};
  bool inverted{};
  bool initialized{};
  bool running{};
};
inline std::array<pwm_state, std::size(pwm_instances)> pwm_states{};
#endif

#if MICROHAL_NORDIC_HAS_WDT
inline nrfx_wdt_t wdt_instances[]{
#if defined(NRF54L15_XXAA)
#if defined(NRF_WDT30)
    {NRF_WDT30, {}},
#endif
    {NRF_WDT31, {}}
#elif defined(NRF52840_XXAA)
    {NRF_WDT, {}}
#else
    {NRF_WDT0, {}}, {NRF_WDT1, {}}
#endif
};
inline std::array<bool, std::size(wdt_instances)> wdt_started{};
#endif

#if MICROHAL_NORDIC_HAS_SAADC
inline std::atomic<bool> saadc_active{false};
inline std::atomic<bool> saadc_complete{false};
inline nrf_saadc_value_t saadc_sample{};
inline void (*saadc_completion)(status) noexcept {nullptr};
inline std::span<std::uint16_t> saadc_scan{};
constexpr nrf_saadc_resolution_t saadc_resolution(const adc_resolution resolution) noexcept
{
  switch (resolution)
  {
  case adc_resolution::bits_8:
    return NRF_SAADC_RESOLUTION_8BIT;
  case adc_resolution::bits_10:
    return NRF_SAADC_RESOLUTION_10BIT;
  case adc_resolution::bits_12:
    return NRF_SAADC_RESOLUTION_12BIT;
  case adc_resolution::bits_14:
    return NRF_SAADC_RESOLUTION_14BIT;
  default:
    return static_cast<nrf_saadc_resolution_t>(UINT32_MAX);
  }
}
#endif

#if MICROHAL_NORDIC_HAS_GPIOTE
inline nrfx_gpiote_t gpiote_instance = []
{
  nrfx_gpiote_t instance{};
#if defined(NRF54L15_XXAA)
  instance.p_reg = NRF_GPIOTE20;
  instance.cb.drv_inst_idx = NRFX_REG_TO_INSTANCE(GPIOTE, NRF_GPIOTE20);
#else
  instance.p_reg = NRF_GPIOTE;
  instance.cb.drv_inst_idx = NRFX_REG_TO_INSTANCE(GPIOTE, NRF_GPIOTE);
#endif
  return instance;
}();
#endif

#if MICROHAL_NORDIC_HAS_QSPI
inline nrfx_qspi_config_t qspi_configuration{};
inline std::atomic<bool> qspi_active{false};

inline bool qspi_memory_command(const qspi_command& command, const bool write) noexcept
{
  const auto& config = qspi_configuration;
  const auto address_bytes = config.prot_if.addrmode == NRF_QSPI_ADDRMODE_24BIT ? 3U : 4U;
  if (command.instruction_mode != qspi_io_mode::single || command.address_mode != qspi_io_mode::single ||
      command.address_size_bytes != address_bytes || (address_bytes == 3U && command.address > 0xFFFFFFU))
    return false;
  if (write)
  {
    if (command.dummy_cycles != 0U)
      return false;
    switch (config.prot_if.writeoc)
    {
    case NRF_QSPI_WRITEOC_PP:
      return command.instruction == 0x02U && command.data_mode == qspi_io_mode::single;
    case NRF_QSPI_WRITEOC_PP2O:
      return command.instruction == 0xA2U && command.data_mode == qspi_io_mode::dual;
    case NRF_QSPI_WRITEOC_PP4O:
      return command.instruction == 0x32U && command.data_mode == qspi_io_mode::quad;
    default:
      return false;
    }
  }
  if (command.dummy_cycles != 8U)
    return false;
  switch (config.prot_if.readoc)
  {
  case NRF_QSPI_READOC_FASTREAD:
    return command.instruction == 0x0BU && command.data_mode == qspi_io_mode::single;
  case NRF_QSPI_READOC_READ2O:
    return command.instruction == 0x3BU && command.data_mode == qspi_io_mode::dual;
  case NRF_QSPI_READOC_READ4O:
    return command.instruction == 0x6BU && command.data_mode == qspi_io_mode::quad;
  default:
    return false;
  }
}
#endif

#if MICROHAL_NORDIC_HAS_I2S
inline nrfx_i2s_t i2s_instance{
#if defined(NRF54L15_XXAA)
    NRF_I2S20,
#elif defined(NRF5340_XXAA_APPLICATION)
    NRF_I2S0,
#else
    NRF_I2S,
#endif
    {}};
inline transfer_state i2s_transfer{};
inline const std::uint32_t* i2s_tx_buffer{};
inline std::uint32_t* i2s_rx_buffer{};
inline std::array<std::uint32_t, 16U> i2s_silence{};
inline std::array<std::uint32_t, 16U> i2s_discard{};
inline bool i2s_transmit_mode{};
inline bool i2s_stopping{};
inline status i2s_result{status::ok};

inline void i2s_handler(nrfx_i2s_buffers_t const* released, const std::uint32_t flags) noexcept
{
  if (!i2s_transfer.active.load())
    return;
  if ((flags & NRFX_I2S_STATUS_TRANSFER_STOPPED) != 0U)
  {
    i2s_transfer.finish(i2s_result);
    return;
  }
  if (i2s_stopping)
    return;
  if (released == nullptr || (released->p_tx_buffer != nullptr && released->p_tx_buffer == i2s_tx_buffer) ||
      (released->p_rx_buffer != nullptr && released->p_rx_buffer == i2s_rx_buffer))
  {
    if (released == nullptr)
      i2s_result = status::io_error;
    i2s_stopping = true;
    nrfx_i2s_stop(&i2s_instance);
  }
  else if ((flags & NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED) != 0U)
  {
    // Keep DMA off the user's buffer while the final block drains to the pins.
    nrfx_i2s_buffers_t tail{};
    tail.p_tx_buffer = i2s_tx_buffer != nullptr ? i2s_silence.data() : nullptr;
    tail.p_rx_buffer = i2s_rx_buffer != nullptr ? i2s_discard.data() : nullptr;
    tail.buffer_size = i2s_silence.size();
    const int error = nrfx_i2s_next_buffers_set(&i2s_instance, &tail);
    if (error != 0)
    {
      i2s_result = driver_result(error);
      i2s_stopping = true;
      nrfx_i2s_stop(&i2s_instance);
    }
  }
}
#endif

#if MICROHAL_NORDIC_HAS_RTC
inline nrfx_rtc_t rtc_instances[]{{NRF_RTC0, {}},
                                  {NRF_RTC1, {}}
#if defined(NRF52840_XXAA)
                                  ,
                                  {NRF_RTC2, {}}
#endif
};
struct rtc_state
{
  nrfx_rtc_t* instance{};
  std::uint32_t seconds{};
  void (*alarm)() noexcept {nullptr};
  bool initialized{};
};
inline std::array<rtc_state, std::size(rtc_instances)> rtc_states{};
inline void rtc_handler(const nrf_rtc_event_t event, void* context) noexcept
{
  auto& state = *static_cast<rtc_state*>(context);
  if (event == NRF_RTC_EVENT_OVERFLOW)
    state.seconds = (state.seconds + (1UL << 21U)) % 86400U;
  else if (event == NRF_RTC_EVENT_COMPARE_0)
  {
    (void)nrfx_rtc_cc_disable(state.instance, 0U);
    const auto alarm = state.alarm;
    state.alarm = nullptr;
    if (alarm != nullptr)
      alarm();
  }
}

template <typename Config> constexpr std::uint8_t rtc_index() noexcept
{
  if constexpr (requires { Config::instance_index; })
    return Config::instance_index;
  return 0U;
}

struct rtc_snapshot
{
  std::uint32_t seconds;
  std::uint32_t ticks;
};
inline rtc_snapshot rtc_now(const std::uint8_t index) noexcept
{
  const auto saved = __get_PRIMASK();
  __disable_irq();
  auto& instance = rtc_instances[index];
  auto seconds = rtc_states[index].seconds;
  auto ticks = nrfx_rtc_counter_get(&instance);
  if (nrf_rtc_event_check(instance.p_reg, NRF_RTC_EVENT_OVERFLOW))
  {
    seconds += (1UL << 21U);
    ticks = nrfx_rtc_counter_get(&instance);
  }
  seconds = (seconds + ticks / 8U) % 86400U;
  __set_PRIMASK(saved);
  return {seconds, ticks};
}
#endif
} // namespace detail

struct spi_api final
{
  template <typename Config> static status initialize() noexcept
  {
#if MICROHAL_NORDIC_HAS_SPIM
    if constexpr (requires {
                    Config::binding::sck_pin;
                    Config::binding::mosi_pin;
                    Config::binding::miso_pin;
                  })
    {
      if (Config::instance_index >= std::size(detail::spim_instances))
        return status::invalid_argument;
      if (Config::duplex_mode == spi_duplex_mode::half)
        return status::unsupported;
      auto& instance = detail::spim_instances[Config::instance_index];
      if (nrfx_spim_init_check(&instance))
        return status::busy;
      nrfx_spim_config_t config{};
      config.sck_pin = Config::binding::sck_pin;
      config.mosi_pin = Config::binding::mosi_pin;
      config.miso_pin = Config::binding::miso_pin;
      config.ss_pin = NRF_SPIM_PIN_NOT_CONNECTED;
      if constexpr (requires { Config::binding::ss_pin; })
        config.ss_pin = Config::binding::ss_pin;
      config.irq_priority = 7U;
      config.orc = 0xFFU;
      config.frequency = Config::clock_hz;
      const auto mode = (Config::clock_polarity == spi_clock_polarity::idle_high ? 2U : 0U) |
                        (Config::clock_phase == spi_clock_phase::second_edge ? 1U : 0U);
      config.mode = static_cast<nrf_spim_mode_t>(mode);
      config.bit_order =
          Config::bit_order == spi_bit_order::msb_first ? NRF_SPIM_BIT_ORDER_MSB_FIRST : NRF_SPIM_BIT_ORDER_LSB_FIRST;
      config.miso_pull = NRF_GPIO_PIN_NOPULL;
#if NRF_SPIM_HAS_DCX
      config.dcx_pin = NRF_SPIM_PIN_NOT_CONNECTED;
#endif
      return detail::driver_result(
          nrfx_spim_init(&instance, &config, detail::spim_handler, &detail::spim_states[Config::instance_index]));
    }
#endif
    return status::unsupported;
  }

  template <typename Config, auto Handler>
  static status transfer_async(const std::span<const std::uint8_t> tx, const std::span<std::uint8_t> rx) noexcept
  {
#if MICROHAL_NORDIC_HAS_SPIM
    if (Config::instance_index >= std::size(detail::spim_instances) || (tx.empty() && rx.empty()) ||
        !detail::serial_count_valid(tx.size()) || !detail::serial_count_valid(rx.size()))
      return status::invalid_argument;
    auto& instance = detail::spim_instances[Config::instance_index];
    if (!nrfx_spim_init_check(&instance))
      return status::unsupported;
    auto& state = detail::spim_states[Config::instance_index];
    if (!state.begin(detail::notify<Handler>))
      return status::busy;
    nrfx_spim_xfer_desc_t descriptor{};
    descriptor.p_tx_buffer = tx.data();
    descriptor.tx_length = tx.size();
    descriptor.p_rx_buffer = rx.data();
    descriptor.rx_length = rx.size();
    const auto result = detail::driver_result(nrfx_spim_xfer(&instance, &descriptor, 0U));
    if (result != status::ok)
    {
      state.completion = nullptr;
      state.finish(result);
    }
    return result;
#else
    (void)tx;
    (void)rx;
    return status::unsupported;
#endif
  }

  template <typename Config>
  static status transfer(const std::span<const std::uint8_t> tx, const std::span<std::uint8_t> rx) noexcept
  {
#if MICROHAL_NORDIC_HAS_SPIM && MICROHAL_NORDIC_SYSTEM_CORE_CLOCK
    if (SystemCoreClock == 0U)
      return status::error;
    const auto started = transfer_async<Config, nullptr>(tx, rx);
    if (started != status::ok)
      return started;
    auto& state = detail::spim_states[Config::instance_index];
    if (!detail::wait_until<Config>([&] { return state.complete.load(std::memory_order_acquire); }))
    {
      nrfx_spim_abort(&detail::spim_instances[Config::instance_index]);
      state.finish(status::timeout);
      return status::timeout;
    }
    return state.result;
#else
    (void)tx;
    (void)rx;
    return status::unsupported;
#endif
  }

  static void handle_interrupt(const std::uint8_t index) noexcept
  {
#if MICROHAL_NORDIC_HAS_SPIM
    if (index < std::size(detail::spim_instances))
      nrfx_spim_irq_handler(&detail::spim_instances[index]);
#else
    (void)index;
#endif
  }
};

struct uart_api final
{
  template <typename Config> static status initialize() noexcept
  {
#if MICROHAL_NORDIC_HAS_UARTE
    if constexpr (requires {
                    Config::binding::tx_pin;
                    Config::binding::rx_pin;
                  })
    {
      if (Config::instance_index >= std::size(detail::uarte_instances) ||
          static_cast<std::uint32_t>(detail::uarte_baudrate(Config::baud_rate)) == 0U)
        return status::invalid_argument;
      if (Config::stop_bits != uart_stop_bits::one || Config::parity == uart_parity::odd)
        return status::unsupported;
      auto& instance = detail::uarte_instances[Config::instance_index];
      if (nrfx_uarte_init_check(&instance))
        return status::busy;
      if constexpr (requires { Config::require_fifo; })
        if (Config::require_fifo)
          return status::unsupported;
      if constexpr (requires { Config::require_lpuart; })
        if (Config::require_lpuart)
          return status::unsupported;
      nrfx_uarte_config_t config{};
      config.txd_pin = Config::binding::tx_pin;
      config.rxd_pin = Config::binding::rx_pin;
      config.rts_pin = NRF_UARTE_PSEL_DISCONNECTED;
      config.cts_pin = NRF_UARTE_PSEL_DISCONNECTED;
      config.p_context = &detail::uarte_states[Config::instance_index];
      detail::uarte_states[Config::instance_index].instance = &instance;
      config.baudrate = detail::uarte_baudrate(Config::baud_rate);
      config.config.parity =
          Config::parity == uart_parity::even ? NRF_UARTE_PARITY_INCLUDED : NRF_UARTE_PARITY_EXCLUDED;
      config.config.hwfc = NRF_UARTE_HWFC_DISABLED;
      config.interrupt_priority = 7U;
      return detail::driver_result(nrfx_uarte_init(&instance, &config, detail::uarte_handler));
    }
#endif
    return status::unsupported;
  }

  template <typename Config, auto Handler>
  static status start_async_tx(const std::span<const std::uint8_t> payload) noexcept
  {
#if MICROHAL_NORDIC_HAS_UARTE
    if (payload.empty() || !detail::serial_count_valid(payload.size()) ||
        Config::instance_index >= std::size(detail::uarte_instances))
      return status::invalid_argument;
    auto& instance = detail::uarte_instances[Config::instance_index];
    if (!nrfx_uarte_init_check(&instance))
      return status::unsupported;
    auto& state = detail::uarte_states[Config::instance_index];
    if (!state.tx.begin(detail::notify<Handler>))
      return status::busy;
    state.tx_size = payload.size();
    const auto result = detail::driver_result(nrfx_uarte_tx(&instance, payload.data(), payload.size(), 0U));
    if (result != status::ok)
    {
      state.tx.completion = nullptr;
      state.tx.finish(result);
    }
    return result;
#else
    (void)payload;
    return status::unsupported;
#endif
  }

  template <typename Config, auto Handler> static status start_async_rx(const std::span<std::uint8_t> payload) noexcept
  {
#if MICROHAL_NORDIC_HAS_UARTE
    if (payload.empty() || !detail::serial_count_valid(payload.size()) ||
        Config::instance_index >= std::size(detail::uarte_instances))
      return status::invalid_argument;
    auto& instance = detail::uarte_instances[Config::instance_index];
    if (!nrfx_uarte_init_check(&instance))
      return status::unsupported;
    auto& state = detail::uarte_states[Config::instance_index];
    if (!state.rx.begin(detail::notify<Handler>))
      return status::busy;
    state.rx_size = payload.size();
    state.rx_error = false;
    state.rx_result = status::io_error;
    int error = nrfx_uarte_rx_buffer_set(&instance, payload.data(), payload.size());
    if (error == 0)
      error = nrfx_uarte_rx_enable(&instance, NRFX_UARTE_RX_ENABLE_STOP_ON_END);
    const auto result = detail::driver_result(error);
    if (result != status::ok)
    {
      state.rx.completion = nullptr;
      (void)nrfx_uarte_rx_abort(&instance, true, true);
      state.rx.finish(result);
    }
    return result;
#else
    (void)payload;
    return status::unsupported;
#endif
  }

  template <typename Config> static status write_blocking(const std::span<const std::uint8_t> payload) noexcept
  {
#if MICROHAL_NORDIC_HAS_UARTE && MICROHAL_NORDIC_SYSTEM_CORE_CLOCK
    if (SystemCoreClock == 0U)
      return status::error;
    const auto result = start_async_tx<Config, nullptr>(payload);
    if (result != status::ok)
      return result;
    auto& state = detail::uarte_states[Config::instance_index].tx;
    if (!detail::wait_until<Config>([&] { return state.complete.load(std::memory_order_acquire); }))
    {
      (void)nrfx_uarte_tx_abort(&detail::uarte_instances[Config::instance_index], true);
      state.finish(status::timeout);
      return status::timeout;
    }
    return state.result;
#else
    (void)payload;
    return status::unsupported;
#endif
  }

  template <typename Config> static status read_blocking(const std::span<std::uint8_t> payload) noexcept
  {
#if MICROHAL_NORDIC_HAS_UARTE && MICROHAL_NORDIC_SYSTEM_CORE_CLOCK
    if (SystemCoreClock == 0U)
      return status::error;
    const auto result = start_async_rx<Config, nullptr>(payload);
    if (result != status::ok)
      return result;
    auto& state = detail::uarte_states[Config::instance_index].rx;
    auto& instance = detail::uarte_instances[Config::instance_index];
    if (!detail::wait_until<Config>([&] { return state.complete.load(std::memory_order_acquire); }))
    {
      (void)nrfx_uarte_rx_abort(&instance, true, true);
      state.finish(status::timeout);
      return status::timeout;
    }
    const auto received = state.result;
    (void)nrfx_uarte_rx_abort(&instance, true, true);
    return received;
#else
    (void)payload;
    return status::unsupported;
#endif
  }

  template <typename Config, typename AutobaudConfig> static status arm_autobaud() noexcept
  {
    return status::unsupported;
  }
  template <typename Config> static status apply_autobaud(std::uint32_t) noexcept
  {
    return status::unsupported;
  }
  static void handle_interrupt(const std::uint8_t index) noexcept
  {
#if MICROHAL_NORDIC_HAS_UARTE
    if (index < std::size(detail::uarte_instances))
      nrfx_uarte_irq_handler(&detail::uarte_instances[index]);
#else
    (void)index;
#endif
  }
};

struct timer_api final
{
  template <typename Config> static status initialize() noexcept
  {
#if MICROHAL_NORDIC_HAS_TIMER
    if (Config::instance_index >= std::size(detail::timer_instances) || Config::period == 0U || Config::clock_hz == 0U)
      return status::invalid_argument;
    if (Config::counter_mode != timer_counter_mode::up || Config::clock_source != timer_clock_source::internal ||
        Config::enable_dma)
      return status::unsupported;
    auto& instance = detail::timer_instances[Config::instance_index];
    if (nrfx_timer_init_check(&instance))
      return status::busy;
    const std::uint64_t divisor = static_cast<std::uint64_t>(Config::prescaler) + 1U;
    if (Config::clock_hz % divisor != 0U)
      return status::invalid_argument;
    nrfx_timer_config_t config{};
    config.frequency = static_cast<std::uint32_t>(Config::clock_hz / divisor);
    config.mode = NRF_TIMER_MODE_TIMER;
    config.bit_width = NRF_TIMER_BIT_WIDTH_32;
    config.interrupt_priority = 7U;
    config.p_context = &detail::timer_states[Config::instance_index];
    const auto result = detail::driver_result(nrfx_timer_init(&instance, &config, detail::timer_handler));
    if (result != status::ok)
      return result;
    return set_period<Config>(Config::period);
#else
    return status::unsupported;
#endif
  }
  template <typename Config> static status start() noexcept
  {
#if MICROHAL_NORDIC_HAS_TIMER
    if (Config::instance_index >= std::size(detail::timer_instances))
      return status::invalid_argument;
    auto& instance = detail::timer_instances[Config::instance_index];
    if (!nrfx_timer_init_check(&instance))
      return status::unsupported;
    if (nrfx_timer_is_enabled(&instance))
      return status::busy;
    nrfx_timer_enable(&instance);
    return status::ok;
#else
    return status::unsupported;
#endif
  }
  template <typename Config> static status stop() noexcept
  {
#if MICROHAL_NORDIC_HAS_TIMER
    if (Config::instance_index >= std::size(detail::timer_instances))
      return status::invalid_argument;
    auto& instance = detail::timer_instances[Config::instance_index];
    if (!nrfx_timer_init_check(&instance))
      return status::unsupported;
    nrfx_timer_disable(&instance);
    return status::ok;
#else
    return status::unsupported;
#endif
  }
  template <typename Config> static status reset_counter() noexcept
  {
#if MICROHAL_NORDIC_HAS_TIMER
    if (Config::instance_index >= std::size(detail::timer_instances))
      return status::invalid_argument;
    auto& instance = detail::timer_instances[Config::instance_index];
    if (!nrfx_timer_init_check(&instance))
      return status::unsupported;
    nrfx_timer_clear(&instance);
    return status::ok;
#else
    return status::unsupported;
#endif
  }
  template <typename Config> static status set_period(const std::uint32_t ticks) noexcept
  {
#if MICROHAL_NORDIC_HAS_TIMER
    if (ticks == 0U || Config::instance_index >= std::size(detail::timer_instances))
      return status::invalid_argument;
    auto& instance = detail::timer_instances[Config::instance_index];
    if (!nrfx_timer_init_check(&instance))
      return status::unsupported;
    const bool running = nrfx_timer_is_enabled(&instance);
    if (running)
      nrfx_timer_pause(&instance);
    nrfx_timer_clear(&instance);
    nrfx_timer_extended_compare(&instance, NRF_TIMER_CC_CHANNEL0, ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);
    auto& state = detail::timer_states[Config::instance_index];
    state.period = ticks;
    for (std::size_t channel = 0U; channel < state.callbacks.size(); ++channel)
      if (state.callbacks[channel] != nullptr)
        nrfx_timer_compare(&instance, static_cast<nrf_timer_cc_channel_t>(channel + 1U), ticks, true);
    if (running)
      nrfx_timer_resume(&instance);
    return status::ok;
#else
    (void)ticks;
    return status::unsupported;
#endif
  }
  template <typename Config> static result<std::uint32_t> period() noexcept
  {
#if MICROHAL_NORDIC_HAS_TIMER
    if (Config::instance_index >= std::size(detail::timer_instances))
      return status::invalid_argument;
    if (!nrfx_timer_init_check(&detail::timer_instances[Config::instance_index]))
      return status::unsupported;
    return detail::timer_states[Config::instance_index].period;
#else
    return status::unsupported;
#endif
  }
  template <typename Config> static std::uint32_t counter() noexcept
  {
#if MICROHAL_NORDIC_HAS_TIMER
    if (Config::instance_index < std::size(detail::timer_instances) &&
        nrfx_timer_init_check(&detail::timer_instances[Config::instance_index]))
      return nrfx_timer_capture(&detail::timer_instances[Config::instance_index], NRF_TIMER_CC_CHANNEL3);
#endif
    return 0U;
  }
  template <typename Config, typename Channel, auto Handler> static status enable_capture_compare() noexcept
  {
#if MICROHAL_NORDIC_HAS_TIMER
    if (Config::instance_index >= std::size(detail::timer_instances) || Channel::channel_index < 1U ||
        Channel::channel_index > 2U)
      return status::invalid_argument;
    if (Channel::enable_capture || !Channel::enable_compare)
      return status::unsupported;
    auto& instance = detail::timer_instances[Config::instance_index];
    if (!nrfx_timer_init_check(&instance))
      return status::unsupported;
    detail::timer_states[Config::instance_index].callbacks[Channel::channel_index - 1U] = []() noexcept
    { detail::notify<Handler>(status::ok); };
    nrfx_timer_compare(&instance, static_cast<nrf_timer_cc_channel_t>(Channel::channel_index),
                       detail::timer_states[Config::instance_index].period, true);
    return status::ok;
#else
    return status::unsupported;
#endif
  }
  template <typename Config, typename Channel> static result<std::uint32_t> capture_value() noexcept
  {
    return status::unsupported;
  }
  static void handle_interrupt(const std::uint8_t index) noexcept
  {
#if MICROHAL_NORDIC_HAS_TIMER
    if (index < std::size(detail::timer_instances))
      nrfx_timer_irq_handler(&detail::timer_instances[index]);
#else
    (void)index;
#endif
  }
};

struct pwm_api final
{
  template <typename Config> static status initialize() noexcept
  {
#if MICROHAL_NORDIC_HAS_PWM
    if constexpr (requires { Config::binding::output_pin; })
    {
      if (Config::timer_index >= std::size(detail::pwm_instances) || Config::channel < 1U || Config::channel > 4U ||
          Config::frequency_hz == 0U)
        return status::invalid_argument;
      if (Config::enable_dead_time)
        return status::unsupported;
      auto& instance = detail::pwm_instances[Config::timer_index];
      if (nrfx_pwm_init_check(&instance))
        return status::busy;
      const auto timing = detail::pwm_clock(Config::frequency_hz);
      if (!timing)
        return timing.error();
      nrfx_pwm_config_t config{};
      for (auto& pin : config.output_pins)
        pin = NRF_PWM_PIN_NOT_CONNECTED;
      config.output_pins[Config::channel - 1U] = Config::binding::output_pin;
      config.irq_priority = 7U;
      config.base_clock = static_cast<nrf_pwm_clk_t>(timing.value().prescaler);
      config.count_mode = NRF_PWM_MODE_UP;
      config.top_value = timing.value().top;
      config.load_mode = NRF_PWM_LOAD_INDIVIDUAL;
      config.step_mode = NRF_PWM_STEP_AUTO;
      const auto result = detail::driver_result(nrfx_pwm_init(&instance, &config, nullptr, nullptr));
      if (result != status::ok)
        return result;
      auto& state = detail::pwm_states[Config::timer_index];
      state.top = config.top_value;
      state.channel = Config::channel;
      state.inverted = Config::polarity == pwm_polarity::active_low;
      state.initialized = true;
      return set_duty_cycle<Config>(0U);
    }
#endif
    return status::unsupported;
  }
  template <typename Config> static status set_duty_cycle(const std::uint16_t permille) noexcept
  {
#if MICROHAL_NORDIC_HAS_PWM
    if (permille > 1000U || Config::timer_index >= std::size(detail::pwm_instances))
      return status::invalid_argument;
    auto& state = detail::pwm_states[Config::timer_index];
    if (!state.initialized || state.channel != Config::channel)
      return status::unsupported;
    const bool running = state.running;
    if (running)
    {
      const auto stopped = stop<Config>();
      if (stopped != status::ok)
        return stopped;
    }
    const auto value = static_cast<std::uint16_t>((static_cast<std::uint32_t>(state.top) * permille / 1000U) |
                                                  (state.inverted ? 0U : 0x8000U));
    switch (state.channel)
    {
    case 1U:
      state.values.channel_0 = value;
      break;
    case 2U:
      state.values.channel_1 = value;
      break;
    case 3U:
      state.values.channel_2 = value;
      break;
    case 4U:
      state.values.channel_3 = value;
      break;
    default:
      return status::invalid_argument;
    }
    return running ? start<Config>() : status::ok;
#else
    (void)permille;
    return status::unsupported;
#endif
  }
  template <typename Config> static status start() noexcept
  {
#if MICROHAL_NORDIC_HAS_PWM
    if (Config::timer_index >= std::size(detail::pwm_instances))
      return status::invalid_argument;
    auto& state = detail::pwm_states[Config::timer_index];
    if (!state.initialized || state.channel != Config::channel)
      return status::unsupported;
    if (state.running)
      return status::busy;
    nrf_pwm_sequence_t sequence{};
    sequence.values.p_individual = &state.values;
    sequence.length = NRF_PWM_CHANNEL_COUNT;
    (void)nrfx_pwm_simple_playback(&detail::pwm_instances[Config::timer_index], &sequence, 1U, NRFX_PWM_FLAG_LOOP);
    state.running = true;
    return status::ok;
#else
    return status::unsupported;
#endif
  }
  template <typename Config> static status stop() noexcept
  {
#if MICROHAL_NORDIC_HAS_PWM && MICROHAL_NORDIC_SYSTEM_CORE_CLOCK
    if (Config::timer_index >= std::size(detail::pwm_instances))
      return status::invalid_argument;
    auto& state = detail::pwm_states[Config::timer_index];
    if (!state.initialized || state.channel != Config::channel)
      return status::unsupported;
    auto& instance = detail::pwm_instances[Config::timer_index];
    if (!nrfx_pwm_stop(&instance, false) &&
        !detail::wait_until<Config>([&] { return nrfx_pwm_stopped_check(&instance); }))
      return status::timeout;
    state.running = false;
    return status::ok;
#else
    return status::unsupported;
#endif
  }
};

struct watchdog_api final
{
  template <typename Config> static status initialize() noexcept
  {
#if MICROHAL_NORDIC_HAS_WDT
    if (Config::instance_index >= std::size(detail::wdt_instances) || Config::timeout_ms == 0U ||
        Config::timeout_ms > 131'071'999U)
      return status::invalid_argument;
    if (Config::window_mode || !Config::independent_mode)
      return status::unsupported;
    auto& instance = detail::wdt_instances[Config::instance_index];
    if (nrfx_wdt_init_check(&instance))
      return status::busy;
    nrfx_wdt_config_t config{};
    config.behaviour = NRF_WDT_BEHAVIOUR_RUN_SLEEP_MASK;
    config.reload_value = Config::timeout_ms;
#if !NRFX_CHECK(NRFX_WDT_CONFIG_NO_IRQ)
    config.interrupt_priority = 7U;
#endif
    const auto result = detail::driver_result(nrfx_wdt_init(&instance, &config, nullptr, nullptr));
    if (result != status::ok)
      return result;
    nrfx_wdt_channel_id channel{};
    const auto allocated = detail::driver_result(nrfx_wdt_channel_alloc(&instance, &channel));
    if (allocated != status::ok)
      nrfx_wdt_uninit(&instance);
    return allocated;
#else
    return status::unsupported;
#endif
  }
  template <typename Config> static status start() noexcept
  {
#if MICROHAL_NORDIC_HAS_WDT
    if (Config::instance_index >= std::size(detail::wdt_instances))
      return status::invalid_argument;
    auto& instance = detail::wdt_instances[Config::instance_index];
    if (!nrfx_wdt_init_check(&instance))
      return status::unsupported;
    if (detail::wdt_started[Config::instance_index])
      return status::busy;
    nrfx_wdt_enable(&instance);
    detail::wdt_started[Config::instance_index] = true;
    return status::ok;
#else
    return status::unsupported;
#endif
  }
  template <typename Config> static status refresh() noexcept
  {
#if MICROHAL_NORDIC_HAS_WDT
    if (Config::instance_index >= std::size(detail::wdt_instances))
      return status::invalid_argument;
    if (!detail::wdt_started[Config::instance_index])
      return status::unsupported;
    nrfx_wdt_feed(&detail::wdt_instances[Config::instance_index]);
    return status::ok;
#else
    return status::unsupported;
#endif
  }
};

struct adc_api final
{
  template <typename Config> static status initialize() noexcept
  {
#if MICROHAL_NORDIC_HAS_SAADC
    if (Config::instance_index != 0U ||
        static_cast<std::uint32_t>(detail::saadc_resolution(Config::resolution)) == UINT32_MAX)
      return status::invalid_argument;
    if (Config::differential_mode)
      return status::unsupported;
    if constexpr (requires { Config::binding::channels; })
    {
      if (Config::binding::channels.empty() || Config::binding::channels.size() > 8U)
        return status::invalid_argument;
      for (const auto& channel : Config::binding::channels)
        if (channel.channel_config.mode != NRF_SAADC_MODE_SINGLE_ENDED)
          return status::unsupported;
      if (nrfx_saadc_init_check())
        return status::busy;
      const auto result = detail::driver_result(nrfx_saadc_init(7U));
      if (result != status::ok)
        return result;
      const auto configured = detail::driver_result(
          nrfx_saadc_channels_config(Config::binding::channels.data(), Config::binding::channels.size()));
      if (configured != status::ok)
        nrfx_saadc_uninit();
      return configured;
    }
#endif
    return status::unsupported;
  }

  template <typename Config> static result<std::uint16_t> read_blocking(const std::uint8_t channel) noexcept
  {
#if MICROHAL_NORDIC_HAS_SAADC && MICROHAL_NORDIC_SYSTEM_CORE_CLOCK
    if (SystemCoreClock == 0U)
      return status::error;
    const auto started = start_async<Config, nullptr>(channel);
    if (started != status::ok)
      return started;
    if (!detail::wait_until<Config>([] { return detail::saadc_complete.load(std::memory_order_acquire); }))
    {
      detail::saadc_completion = nullptr;
      detail::saadc_active.store(false);
      nrfx_saadc_uninit();
      return status::timeout;
    }
    return static_cast<std::uint16_t>(detail::saadc_sample < 0 ? 0 : detail::saadc_sample);
#else
    (void)channel;
    return status::unsupported;
#endif
  }
  template <typename Config> static status start_continuous(std::span<const std::uint8_t>) noexcept
  {
    return status::unsupported;
  }
  template <typename Config, auto Handler> static status start_async(const std::uint8_t channel) noexcept
  {
#if MICROHAL_NORDIC_HAS_SAADC
    if (channel >= 8U || Config::instance_index != 0U)
      return status::invalid_argument;
    if (!nrfx_saadc_init_check())
      return status::unsupported;
    if (detail::saadc_active.exchange(true))
      return status::busy;
    detail::saadc_complete.store(false);
    detail::saadc_completion = detail::notify<Handler>;
    const auto callback = +[](nrfx_saadc_evt_t const* event)
    {
      if (event->type != NRFX_SAADC_EVT_DONE || !detail::saadc_active.load())
        return;
      const auto completion = detail::saadc_completion;
      detail::saadc_completion = nullptr;
      detail::saadc_complete.store(true, std::memory_order_release);
      detail::saadc_active.store(false);
      if constexpr (requires { Handler(std::uint16_t{}); })
        Handler(static_cast<std::uint16_t>(detail::saadc_sample < 0 ? 0 : detail::saadc_sample));
      else if (completion != nullptr)
        completion(status::ok);
    };
    int error = nrfx_saadc_simple_mode_set(1UL << channel, detail::saadc_resolution(Config::resolution),
                                           NRF_SAADC_OVERSAMPLE_DISABLED, callback);
    if (error == 0)
      error = nrfx_saadc_buffer_set(&detail::saadc_sample, 1U);
    if (error == 0)
      error = nrfx_saadc_mode_trigger();
    if (error != 0)
    {
      detail::saadc_completion = nullptr;
      detail::saadc_active.store(false);
      nrfx_saadc_uninit();
    }
    return detail::driver_result(error);
#else
    (void)channel;
    return status::unsupported;
#endif
  }
  template <typename Config, auto Handler>
  static status start_dma_scan(const std::span<const std::uint8_t> channels,
                               const std::span<std::uint16_t> samples) noexcept
  {
#if MICROHAL_NORDIC_HAS_SAADC
    if (Config::instance_index != 0U || channels.empty() || channels.size() > 8U || samples.size() != channels.size())
      return status::invalid_argument;
    const auto mask = detail::saadc_channel_mask(channels);
    if (!mask)
      return mask.error();
    if (!nrfx_saadc_init_check())
      return status::unsupported;
    if (detail::saadc_active.exchange(true))
      return status::busy;
    detail::saadc_scan = samples;
    const auto callback = +[](nrfx_saadc_evt_t const* event)
    {
      if (event->type != NRFX_SAADC_EVT_DONE || !detail::saadc_active.load())
        return;
      for (auto& sample : detail::saadc_scan)
        if (sample > 0x7FFFU)
          sample = 0U;
      detail::saadc_scan = {};
      detail::saadc_active.store(false);
      detail::notify<Handler>(status::ok);
    };
    int error = nrfx_saadc_simple_mode_set(mask.value(), detail::saadc_resolution(Config::resolution),
                                           NRF_SAADC_OVERSAMPLE_DISABLED, callback);
    if (error == 0)
      error = nrfx_saadc_buffer_set(reinterpret_cast<nrf_saadc_value_t*>(samples.data()),
                                    static_cast<std::uint16_t>(samples.size()));
    if (error == 0)
      error = nrfx_saadc_mode_trigger();
    if (error != 0)
    {
      detail::saadc_scan = {};
      detail::saadc_active.store(false);
      nrfx_saadc_uninit();
    }
    return detail::driver_result(error);
#else
    (void)channels;
    (void)samples;
    return status::unsupported;
#endif
  }
  static void handle_interrupt() noexcept
  {
#if MICROHAL_NORDIC_HAS_SAADC
    nrfx_saadc_irq_handler();
#endif
  }
};

struct qspi_api final
{
  template <typename Config> static status initialize() noexcept
  {
#if MICROHAL_NORDIC_HAS_QSPI
    if constexpr (requires { Config::binding::configuration; })
    {
      if (nrfx_qspi_init_check())
        return status::busy;
      auto config = Config::binding::configuration;
#if defined(NRF5340_XXAA_APPLICATION)
      constexpr std::uint32_t base_clock = 96'000'000U;
#else
      constexpr std::uint32_t base_clock = 32'000'000U;
#endif
      const auto divider = static_cast<std::uint32_t>(config.phy_if.sck_freq);
      if (divider > 15U || Config::clock_hz != base_clock / (divider + 1U))
        return status::invalid_argument;
      config.timeout = detail::timeout_ms<Config>();
      const auto result = detail::driver_result(nrfx_qspi_init(&config, nullptr, nullptr));
      if (result == status::ok)
        detail::qspi_configuration = config;
      return result;
    }
#endif
    return status::unsupported;
  }

  template <typename Config>
  static status command_write(const qspi_command& request, const std::span<const std::uint8_t> payload) noexcept
  {
#if MICROHAL_NORDIC_HAS_QSPI
    if (!nrfx_qspi_init_check())
      return status::unsupported;
    if (detail::qspi_active.exchange(true))
      return status::busy;
    status result = status::unsupported;
    if (detail::qspi_memory_command(request, true))
      result = detail::qspi_range_valid(request.address, payload.size(), request.address_size_bytes,
                                        QSPI_WRITE_CNT_CNT_Msk >> QSPI_WRITE_CNT_CNT_Pos)
                   ? detail::driver_result(nrfx_qspi_write(payload.data(), payload.size(), request.address))
                   : status::invalid_argument;
    else
      result = custom_command(request, payload.data(), nullptr, payload.size());
    if (result == status::timeout)
      nrfx_qspi_uninit();
    detail::qspi_active.store(false);
    return result;
#else
    (void)request;
    (void)payload;
    return status::unsupported;
#endif
  }
  template <typename Config>
  static status command_read(const qspi_command& request, const std::span<std::uint8_t> payload) noexcept
  {
#if MICROHAL_NORDIC_HAS_QSPI
    if (!nrfx_qspi_init_check())
      return status::unsupported;
    if (payload.empty())
      return status::invalid_argument;
    if (detail::qspi_active.exchange(true))
      return status::busy;
    status result;
    if (detail::qspi_memory_command(request, false))
      result = detail::qspi_range_valid(request.address, payload.size(), request.address_size_bytes,
                                        QSPI_READ_CNT_CNT_Msk >> QSPI_READ_CNT_CNT_Pos)
                   ? detail::driver_result(nrfx_qspi_read(payload.data(), payload.size(), request.address))
                   : status::invalid_argument;
    else
      result = custom_command(request, nullptr, payload.data(), payload.size());
    if (result == status::timeout)
      nrfx_qspi_uninit();
    detail::qspi_active.store(false);
    return result;
#else
    (void)request;
    (void)payload;
    return status::unsupported;
#endif
  }
  template <typename Config> static status enter_memory_mapped(const qspi_command&) noexcept
  {
    return status::unsupported;
  }
  template <typename Config> static status exit_memory_mapped() noexcept
  {
    return status::unsupported;
  }

private:
#if MICROHAL_NORDIC_HAS_QSPI
  static status custom_command(const qspi_command& request, const std::uint8_t* tx, std::uint8_t* rx,
                               const std::size_t size) noexcept
  {
    if (request.instruction_mode != qspi_io_mode::single || request.address_size_bytes != 0U ||
        request.address_mode != qspi_io_mode::none || request.dummy_cycles != 0U || size > 8U ||
        request.data_mode != (size == 0U ? qspi_io_mode::none : qspi_io_mode::single))
      return status::unsupported;
    nrf_qspi_cinstr_conf_t config{};
    config.opcode = request.instruction;
    config.length = static_cast<nrf_qspi_cinstr_len_t>(size + 1U);
    config.io2_level = true;
    config.io3_level = true;
    return detail::driver_result(nrfx_qspi_cinstr_xfer(&config, tx, rx));
  }
#endif
};

struct i2s_api final
{
  template <typename Config> static status initialize() noexcept
  {
#if MICROHAL_NORDIC_HAS_I2S
    if constexpr (requires {
                    Config::binding::sck_pin;
                    Config::binding::lrck_pin;
                    Config::binding::sdout_pin;
                    Config::binding::sdin_pin;
                  })
    {
      if constexpr (requires { Config::instance_index; })
        if (Config::instance_index != 0U)
          return status::invalid_argument;
      if (Config::sample_rate < 8000U || Config::sample_rate > 48000U)
        return status::invalid_argument;
      if (Config::standard == i2s_standard::pcm_short || Config::standard == i2s_standard::pcm_long)
        return status::unsupported;
      if (nrfx_i2s_init_check(&detail::i2s_instance))
        return status::busy;
      nrfx_i2s_config_t config{};
      config.sck_pin = Config::binding::sck_pin;
      config.lrck_pin = Config::binding::lrck_pin;
      config.sdout_pin = Config::binding::sdout_pin;
      config.sdin_pin = Config::binding::sdin_pin;
      config.mck_pin = NRF_I2S_PIN_NOT_CONNECTED;
      if constexpr (Config::enable_master_clock)
      {
        if constexpr (requires { Config::binding::mck_pin; })
          config.mck_pin = Config::binding::mck_pin;
        else
          return status::unsupported;
      }
      config.irq_priority = 7U;
      config.mode = Config::mode == i2s_mode::master_tx || Config::mode == i2s_mode::master_rx ? NRF_I2S_MODE_MASTER
                                                                                               : NRF_I2S_MODE_SLAVE;
      config.format = Config::standard == i2s_standard::philips ? NRF_I2S_FORMAT_I2S : NRF_I2S_FORMAT_ALIGNED;
      config.alignment = Config::standard == i2s_standard::lsb_justified ? NRF_I2S_ALIGN_RIGHT : NRF_I2S_ALIGN_LEFT;
      config.sample_width = NRF_I2S_SWIDTH_16BIT;
      config.channels = NRF_I2S_CHANNELS_STEREO;
#if NRF_I2S_HAS_CLKCONFIG
      config.clksrc = NRF_I2S_CLKSRC_PCLK32M;
#endif
      nrfx_i2s_clk_params_t clocks{32'000'000U, Config::sample_rate, NRF_I2S_SWIDTH_16BIT, false};
      const auto calculated = detail::driver_result(nrfx_i2s_prescalers_calc(&clocks, &config.prescalers));
      if (calculated != status::ok)
        return calculated;
      const auto initialized =
          detail::driver_result(nrfx_i2s_init(&detail::i2s_instance, &config, detail::i2s_handler));
      if (initialized == status::ok)
        detail::i2s_transmit_mode = Config::mode == i2s_mode::master_tx || Config::mode == i2s_mode::slave_tx;
      return initialized;
    }
#endif
    return status::unsupported;
  }

  template <typename Config, auto Handler>
  static status start_dma_tx(const std::span<const std::uint16_t> samples) noexcept
  {
    return start<Config, Handler>(samples.data(), nullptr, samples.size());
  }
  template <typename Config> static status transmit_blocking(const std::span<const std::uint16_t> samples) noexcept
  {
#if MICROHAL_NORDIC_SYSTEM_CORE_CLOCK
    if (SystemCoreClock == 0U)
      return status::error;
    const auto result = start<Config, nullptr>(samples.data(), nullptr, samples.size());
    return result == status::ok ? wait<Config>() : result;
#else
    (void)samples;
    return status::unsupported;
#endif
  }
  template <typename Config> static status receive_blocking(const std::span<std::uint16_t> samples) noexcept
  {
#if MICROHAL_NORDIC_SYSTEM_CORE_CLOCK
    if (SystemCoreClock == 0U)
      return status::error;
    const auto result = start<Config, nullptr>(nullptr, samples.data(), samples.size());
    return result == status::ok ? wait<Config>() : result;
#else
    (void)samples;
    return status::unsupported;
#endif
  }
  static void handle_interrupt() noexcept
  {
#if MICROHAL_NORDIC_HAS_I2S
    nrfx_i2s_irq_handler(&detail::i2s_instance);
#endif
  }

private:
  template <typename Config, auto Handler>
  static status start(const std::uint16_t* tx, std::uint16_t* rx, const std::size_t count) noexcept
  {
#if MICROHAL_NORDIC_HAS_I2S
    constexpr std::size_t max_count = I2S_RXTXD_MAXCNT_MAXCNT_Msk >> I2S_RXTXD_MAXCNT_MAXCNT_Pos;
#if defined(DMA_BUFFER_UNIFIED_BYTE_ACCESS)
    constexpr auto max_words = max_count / sizeof(std::uint32_t);
#else
    constexpr auto max_words = max_count;
#endif
    if (!detail::i2s_count_valid(count, max_words) || reinterpret_cast<std::uintptr_t>(tx) % 4U != 0U ||
        reinterpret_cast<std::uintptr_t>(rx) % 4U != 0U)
      return status::invalid_argument;
    if (!nrfx_i2s_init_check(&detail::i2s_instance))
      return status::unsupported;
    if ((tx != nullptr) != detail::i2s_transmit_mode)
      return status::unsupported;
    if (!detail::i2s_transfer.begin(detail::notify<Handler>))
      return status::busy;
    detail::i2s_tx_buffer = reinterpret_cast<const std::uint32_t*>(tx);
    detail::i2s_rx_buffer = reinterpret_cast<std::uint32_t*>(rx);
    detail::i2s_stopping = false;
    detail::i2s_result = status::ok;
    nrfx_i2s_buffers_t buffers{};
    buffers.p_tx_buffer = detail::i2s_tx_buffer;
    buffers.p_rx_buffer = detail::i2s_rx_buffer;
    buffers.buffer_size = static_cast<std::uint16_t>(count / 2U);
    const auto result = detail::driver_result(nrfx_i2s_start(&detail::i2s_instance, &buffers, 0U));
    if (result != status::ok)
    {
      detail::i2s_transfer.completion = nullptr;
      detail::i2s_transfer.finish(result);
    }
    return result;
#else
    (void)tx;
    (void)rx;
    (void)count;
    return status::unsupported;
#endif
  }
  template <typename Config> static status wait() noexcept
  {
#if MICROHAL_NORDIC_HAS_I2S
    if (!detail::wait_until<Config>([] { return detail::i2s_transfer.complete.load(std::memory_order_acquire); }))
    {
      detail::i2s_transfer.completion = nullptr;
      nrfx_i2s_uninit(&detail::i2s_instance);
      detail::i2s_transfer.finish(status::timeout);
      return status::timeout;
    }
    return detail::i2s_transfer.result;
#else
    return status::unsupported;
#endif
  }
};

struct rtc_api final
{
  template <typename Config> static status initialize() noexcept
  {
#if MICROHAL_NORDIC_HAS_RTC
    constexpr auto index = detail::rtc_index<Config>();
    if (index >= std::size(detail::rtc_instances))
      return status::invalid_argument;
    if (Config::enable_periodic_irq)
      return status::unsupported;
    auto& instance = detail::rtc_instances[index];
    auto& state = detail::rtc_states[index];
    if (state.initialized)
      return status::busy;
    nrfx_rtc_config_t config{};
    config.prescaler = 4095U;
    config.interrupt_priority = 7U;
    config.p_context = &state;
    const auto result = detail::driver_result(nrfx_rtc_init(&instance, &config, detail::rtc_handler));
    if (result != status::ok)
      return result;
    state.instance = &instance;
    state.initialized = true;
    nrfx_rtc_overflow_enable(&instance, true);
    nrfx_rtc_enable(&instance);
    return status::ok;
#else
    return status::unsupported;
#endif
  }
  template <typename Config>
  static status set_time(const std::uint8_t hours, const std::uint8_t minutes, const std::uint8_t seconds) noexcept
  {
#if MICROHAL_NORDIC_HAS_RTC
    constexpr auto index = detail::rtc_index<Config>();
    if (index >= std::size(detail::rtc_instances) || hours >= 24U || minutes >= 60U || seconds >= 60U)
      return status::invalid_argument;
    auto& state = detail::rtc_states[index];
    if (!state.initialized)
      return status::unsupported;
    const auto saved = __get_PRIMASK();
    __disable_irq();
    auto& instance = detail::rtc_instances[index];
    nrfx_rtc_disable(&instance);
    (void)nrfx_rtc_cc_disable(&instance, 0U);
    state.alarm = nullptr;
    nrfx_rtc_counter_clear(&instance);
    nrf_rtc_event_clear(instance.p_reg, NRF_RTC_EVENT_OVERFLOW);
    state.seconds = static_cast<std::uint32_t>(hours) * 3600U + minutes * 60U + seconds;
    nrfx_rtc_enable(&instance);
    __set_PRIMASK(saved);
    return status::ok;
#else
    (void)hours;
    (void)minutes;
    (void)seconds;
    return status::unsupported;
#endif
  }
  template <typename Config>
  static status get_time(std::uint8_t& hours, std::uint8_t& minutes, std::uint8_t& seconds) noexcept
  {
#if MICROHAL_NORDIC_HAS_RTC
    constexpr auto index = detail::rtc_index<Config>();
    if (index >= std::size(detail::rtc_instances))
      return status::invalid_argument;
    if (!detail::rtc_states[index].initialized)
      return status::unsupported;
    const auto now = detail::rtc_now(index).seconds;
    hours = static_cast<std::uint8_t>(now / 3600U);
    minutes = static_cast<std::uint8_t>((now / 60U) % 60U);
    seconds = static_cast<std::uint8_t>(now % 60U);
    return status::ok;
#else
    (void)hours;
    (void)minutes;
    (void)seconds;
    return status::unsupported;
#endif
  }
  template <typename Config, auto Handler, typename Alarm> static status enable_alarm(const Alarm& alarm) noexcept
  {
#if MICROHAL_NORDIC_HAS_RTC
    constexpr auto index = detail::rtc_index<Config>();
    if (index >= std::size(detail::rtc_instances) || alarm.hour >= 24U || alarm.minute >= 60U || alarm.second >= 60U)
      return status::invalid_argument;
    if (alarm.year != 0U || alarm.month != 0U || alarm.day != 0U)
      return status::unsupported;
    if (!detail::rtc_states[index].initialized)
      return status::unsupported;
    const auto saved = __get_PRIMASK();
    __disable_irq();
    const auto now = detail::rtc_now(index);
    const auto target = static_cast<std::uint32_t>(alarm.hour) * 3600U + alarm.minute * 60U + alarm.second;
    auto delta = (target + 86400U - now.seconds) % 86400U;
    if (delta == 0U)
      delta = 86400U;
    const auto ticks = (now.ticks + delta * 8U - now.ticks % 8U) & 0xFFFFFFU;
    const auto result = detail::driver_result(nrfx_rtc_cc_set(&detail::rtc_instances[index], 0U, ticks, true));
    if (result == status::ok)
      detail::rtc_states[index].alarm = []() noexcept { detail::notify<Handler>(status::ok); };
    __set_PRIMASK(saved);
    return result;
#else
    (void)alarm;
    return status::unsupported;
#endif
  }
  static void handle_interrupt(const std::uint8_t index) noexcept
  {
#if MICROHAL_NORDIC_HAS_RTC
    if (index < std::size(detail::rtc_instances))
      nrfx_rtc_irq_handler(&detail::rtc_instances[index]);
#else
    (void)index;
#endif
  }
};
} // namespace microhal::backends::nordic::nrfx
