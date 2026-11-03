// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <array>
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <span>
#include <type_traits>

#include "microhal/backends/nordic/nrfx/calendar.hpp"
#include "microhal/core/rtc_types.hpp"
#include "microhal/core/traits.hpp"

#if MICROHAL_NORDIC_NRFX_PRESENT &&                                                                                    \
    (defined(NRF52840_XXAA) || defined(NRF5340_XXAA_APPLICATION) || defined(NRF54L15_XXAA) || defined(NRF54H20_XXAA))
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
#if MICROHAL_NORDIC_NATIVE_TARGET && defined(NRFX_SPIS_ENABLED) && NRFX_SPIS_ENABLED
#include <nrfx_spis.h>
#define MICROHAL_NORDIC_HAS_SPIS 1
#else
#define MICROHAL_NORDIC_HAS_SPIS 0
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
#if MICROHAL_NORDIC_HAS_UARTE && MICROHAL_NORDIC_HAS_TIMER && __has_include(<helpers/nrfx_gppi.h>)
#include <helpers/nrfx_gppi.h>
#if defined(NRF54H20_XXAA)
#include <hal/nrf_dppi.h>
#endif
#define MICROHAL_NORDIC_HAS_SERIAL_TIMEOUT 1
#else
#define MICROHAL_NORDIC_HAS_SERIAL_TIMEOUT 0
#endif
#if MICROHAL_NORDIC_NATIVE_TARGET && defined(NRFX_PWM_ENABLED) && NRFX_PWM_ENABLED
#include <hal/nrf_gpio.h>
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
#include "microhal/core/adc_types.hpp"
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
#if MICROHAL_NORDIC_NATIVE_TARGET && defined(NRF_QSPI) && defined(NRFX_QSPI_ENABLED) && NRFX_QSPI_ENABLED
#include <nrfx_qspi.h>
#define MICROHAL_NORDIC_HAS_QSPI 1
#if defined(NRF_CACHE) && __has_include(<hal/nrf_cache.h>)
#include <hal/nrf_cache.h>
#define MICROHAL_NORDIC_QSPI_HAS_CACHE 1
#else
#define MICROHAL_NORDIC_QSPI_HAS_CACHE 0
#endif
#else
#define MICROHAL_NORDIC_HAS_QSPI 0
#define MICROHAL_NORDIC_QSPI_HAS_CACHE 0
#endif
#if MICROHAL_NORDIC_NATIVE_TARGET && !defined(NRF54H20_XXAA) && defined(NRFX_I2S_ENABLED) && NRFX_I2S_ENABLED
#include <nrfx_i2s.h>
#define MICROHAL_NORDIC_HAS_I2S 1
#else
#define MICROHAL_NORDIC_HAS_I2S 0
#endif
#if MICROHAL_NORDIC_NATIVE_TARGET && defined(NRF_RTC0) && defined(NRFX_RTC_ENABLED) && NRFX_RTC_ENABLED
#if NRFX_API_VER_AT_LEAST(4, 1, 0)
#include <nrfx_rtc.h>
#define MICROHAL_NORDIC_HAS_RTC 1
#endif
#endif
#ifndef MICROHAL_NORDIC_HAS_RTC
#define MICROHAL_NORDIC_HAS_RTC 0
#endif

#if MICROHAL_NORDIC_NATIVE_TARGET && defined(NRF_GRTC) && defined(NRFX_GRTC_ENABLED) && NRFX_GRTC_ENABLED &&           \
    __has_include(<nrfx_grtc.h>)
#include <nrfx_grtc.h>
#define MICROHAL_NORDIC_HAS_GRTC 1
#else
#define MICROHAL_NORDIC_HAS_GRTC 0
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

#include "microhal/backends/nordic/nrfx/qspi_state.inc"

constexpr bool i2s_count_valid(const std::size_t samples, const std::size_t max_words) noexcept
{
  return samples >= 4U && samples <= 65534U && samples % 2U == 0U && samples / 2U <= max_words;
}

constexpr bool serial_count_valid(const std::size_t bytes) noexcept
{
  return bytes <= UINT16_MAX;
}

#include "microhal/backends/nordic/nrfx/serial_transfer_state.inc"

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

#include "microhal/backends/nordic/nrfx/serial_resource.inc"

#if MICROHAL_NORDIC_HAS_SPIM
inline nrfx_spim_t spim_instances[]{
#if defined(NRF54L15_XXAA)
    {NRF_SPIM00, {}}, {NRF_SPIM20, {}}, {NRF_SPIM21, {}}, {NRF_SPIM22, {}}, {NRF_SPIM30, {}}
#elif defined(NRF54H20_XXAA)
    {NRF_SPIM130, {}}, {NRF_SPIM131, {}}, {NRF_SPIM132, {}}, {NRF_SPIM133, {}}, {NRF_SPIM134, {}}, {NRF_SPIM135, {}},
    {NRF_SPIM136, {}}, {NRF_SPIM137, {}}, {NRF_SPIM120, {}}, {NRF_SPIM121, {}}
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
  auto& state = *static_cast<transfer_state*>(context);
  if (state.active.load())
    state.finish(status::ok);
}
#endif

#include "microhal/backends/nordic/nrfx/serial_spis_state.inc"

#if MICROHAL_NORDIC_HAS_UARTE
inline nrfx_uarte_t uarte_instances[]{
#if defined(NRF54L15_XXAA)
    {NRF_UARTE00, {}}, {NRF_UARTE20, {}}, {NRF_UARTE21, {}}, {NRF_UARTE22, {}}, {NRF_UARTE30, {}}
#elif defined(NRF54H20_XXAA)
    {NRF_UARTE130, {}}, {NRF_UARTE131, {}}, {NRF_UARTE132, {}}, {NRF_UARTE133, {}}, {NRF_UARTE134, {}},
    {NRF_UARTE135, {}}, {NRF_UARTE136, {}}, {NRF_UARTE137, {}}, {NRF_UARTE120, {}}
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
#include "microhal/backends/nordic/nrfx/serial_uarte_config.inc"
#include "microhal/backends/nordic/nrfx/serial_uarte_state.inc"

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
  case 2000000U:
    return static_cast<nrf_uarte_baudrate_t>(0x20000000U);
  case 4000000U:
    return static_cast<nrf_uarte_baudrate_t>(0x40000000U);
  case 8000000U:
    return static_cast<nrf_uarte_baudrate_t>(0x80000000U);
  case 16000000U:
  case 20000000U:
    // nrfx normalizes its baud argument to 16 MHz, so these need a native value after init.
    return NRF_UARTE_BAUDRATE_1000000;
  default:
    return static_cast<nrf_uarte_baudrate_t>(0U);
  }
}
#endif

#if MICROHAL_NORDIC_HAS_TIMER
inline nrfx_timer_t timer_instances[]{
#if defined(NRF54H20_XXAA)
    {NRF_TIMER120, {}}, {NRF_TIMER121, {}}, {NRF_TIMER130, {}}, {NRF_TIMER131, {}},
    {NRF_TIMER132, {}}, {NRF_TIMER133, {}}, {NRF_TIMER134, {}}, {NRF_TIMER135, {}},
    {NRF_TIMER136, {}}, {NRF_TIMER137, {}}
#elif defined(NRF54L15_XXAA)
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
  std::array<void (*)() noexcept, 6U> callbacks{};
  std::array<std::uint32_t, 6U> compare_ticks{};
  std::uint8_t captures{};
  std::uint8_t periodic{};
};
inline std::array<timer_state, std::size(timer_instances)> timer_states{};
constexpr std::uint8_t timer_cc_count(const std::size_t index) noexcept
{
#if defined(NRF52840_XXAA)
  return index < 3U ? 4U : 6U;
#elif defined(NRF54L15_XXAA)
  return index == 1U ? 8U : 6U;
#else
  (void)index;
  return 6U;
#endif
}
inline void timer_handler(const nrf_timer_event_t event, void* context) noexcept
{
  auto& state = *static_cast<timer_state*>(context);
  const auto channel = (static_cast<std::uint32_t>(event) - NRF_TIMER_EVENT_COMPARE0) / 4U;
  if (channel > 0U && channel <= state.callbacks.size() && (state.captures & (1U << (channel - 1U))) == 0U &&
      state.callbacks[channel - 1U] != nullptr)
    state.callbacks[channel - 1U]();
}
#endif

#if MICROHAL_NORDIC_HAS_PWM
inline nrfx_pwm_t pwm_instances[]{
#if defined(NRF54H20_XXAA)
    {NRF_PWM120, {}}, {NRF_PWM130, {}}, {NRF_PWM131, {}}, {NRF_PWM132, {}}, {NRF_PWM133, {}}
#elif defined(NRF54L15_XXAA)
    {NRF_PWM20, {}}, {NRF_PWM21, {}}, {NRF_PWM22, {}}
#else
    {NRF_PWM0, {}}, {NRF_PWM1, {}}, {NRF_PWM2, {}}, {NRF_PWM3, {}}
#endif
};
struct pwm_state
{
  alignas(4) nrf_pwm_values_individual_t values{};
  std::uint16_t top{};
  std::uint8_t clock{};
  std::uint8_t owned{};
  std::uint8_t enabled{};
  std::array<std::uint16_t, 4U> duty{};
  std::array<std::uint32_t, 4U> pins{};
  std::uint8_t inverted{};
  bool initialized{};
  bool running{};
};
inline std::array<pwm_state, std::size(pwm_instances)> pwm_states{};
#endif

#if MICROHAL_NORDIC_HAS_WDT
inline nrfx_wdt_t wdt_instances[]{
#if defined(NRF54H20_XXAA)
#if defined(NRF_WDT010)
    {NRF_WDT010, {}},
#endif
#if defined(NRF_WDT011)
    {NRF_WDT011, {}},
#endif
#if defined(NRF_WDT020)
    {NRF_WDT020, {}},
#endif
#if defined(NRF_WDT030)
    {NRF_WDT030, {}},
#endif
#if defined(NRF_WDT120)
    {NRF_WDT120, {}},
#endif
#if defined(NRF_WDT121)
    {NRF_WDT121, {}},
#endif
#elif defined(NRF54L15_XXAA)
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
inline std::uint32_t saadc_configured{};
inline status saadc_result{status::ok};
inline bool saadc_streaming{};
inline bool saadc_continuous{};
inline bool saadc_circular{};
inline bool saadc_in_callback{};
inline bool saadc_reinitialize{};
inline bool saadc_differential{};
inline std::uint8_t saadc_bits{};
inline std::uint8_t saadc_next{};
inline std::uint8_t saadc_blocks{};
inline std::span<std::uint16_t> saadc_storage{};
inline void (*saadc_block_completion)(status, std::span<std::uint16_t>) noexcept {};
inline void (*saadc_trigger_start)() noexcept {};
inline void (*saadc_trigger_stop)() noexcept {};
inline void (*saadc_trigger_disconnect)() noexcept {};
inline std::array<std::uint16_t, 16U> saadc_continuous_storage{};
inline std::array<std::uint8_t, 8U> saadc_continuous_channels{};
inline std::uint8_t saadc_continuous_count{};
inline std::array<std::atomic<std::uint16_t>, 8U> saadc_latest{};
inline std::atomic<std::uint32_t> saadc_latest_valid{};
inline std::uint16_t saadc_unsigned(const nrf_saadc_value_t sample) noexcept
{
  if (saadc_differential)
    return static_cast<std::uint16_t>(static_cast<std::int32_t>(sample) + (1L << (saadc_bits - 1U)));
  return static_cast<std::uint16_t>(sample < 0 ? 0 : sample);
}
inline void saadc_release_trigger() noexcept
{
  if (saadc_trigger_stop != nullptr)
    saadc_trigger_stop();
  if (saadc_trigger_disconnect != nullptr)
    saadc_trigger_disconnect();
  saadc_trigger_start = nullptr;
  saadc_trigger_stop = nullptr;
  saadc_trigger_disconnect = nullptr;
}
inline void saadc_finish(const status result) noexcept
{
  saadc_release_trigger();
  saadc_result = result;
  const auto completion = saadc_completion;
  saadc_completion = nullptr;
  saadc_block_completion = nullptr;
  saadc_scan = {};
  saadc_storage = {};
  saadc_continuous = false;
  saadc_complete.store(true, std::memory_order_release);
  saadc_active.store(false, std::memory_order_release);
  if (completion != nullptr)
    completion(result);
}
inline void saadc_stream_handler(const nrfx_saadc_evt_t* event) noexcept
{
  if (!saadc_active.load(std::memory_order_acquire) || !saadc_streaming)
    return;
  saadc_in_callback = true;
  if (event->type == NRFX_SAADC_EVT_READY && saadc_trigger_start != nullptr)
    saadc_trigger_start();
  else if (event->type == NRFX_SAADC_EVT_BUF_REQ && saadc_circular)
  {
    const auto half = saadc_storage.size() / 2U;
    const auto error =
        nrfx_saadc_buffer_set(reinterpret_cast<nrf_saadc_value_t*>(saadc_storage.data() + saadc_next * half),
                              static_cast<std::uint16_t>(half));
    saadc_next ^= 1U;
    if (error != 0)
    {
      // Teardown flushes DMA and pending events before caller storage is released.
      nrfx_saadc_uninit();
      saadc_reinitialize = true;
      const auto callback = saadc_block_completion;
      saadc_finish(driver_result(error));
      if (callback != nullptr)
        callback(driver_result(error), {});
    }
  }
  else if (event->type == NRFX_SAADC_EVT_DONE)
  {
    const auto half = saadc_storage.size() / 2U;
    auto* const expected = reinterpret_cast<nrf_saadc_value_t*>(saadc_storage.data() + (saadc_blocks % 2U) * half);
    if (event->data.done.p_buffer == expected && event->data.done.size == half)
    {
      for (std::size_t sample = 0U; sample < half; ++sample)
        reinterpret_cast<std::uint16_t*>(expected)[sample] = saadc_unsigned(expected[sample]);
      ++saadc_blocks;
      if (saadc_block_completion != nullptr)
        saadc_block_completion(status::ok, {reinterpret_cast<std::uint16_t*>(expected), half});
    }
  }
  else if (event->type == NRFX_SAADC_EVT_FINISHED)
  {
    const auto result = !saadc_circular && saadc_blocks == 2U ? status::ok : status::io_error;
    const auto callback = saadc_block_completion;
    saadc_finish(result);
    if (result != status::ok && callback != nullptr)
      callback(result, {});
  }
  saadc_in_callback = false;
}
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
#if defined(NRF54H20_XXAA)
  instance.p_reg = NRF_GPIOTE130;
  instance.cb.drv_inst_idx = NRFX_REG_TO_INSTANCE(GPIOTE, NRF_GPIOTE130);
#elif defined(NRF54L15_XXAA)
  instance.p_reg = NRF_GPIOTE20;
  instance.cb.drv_inst_idx = NRFX_REG_TO_INSTANCE(GPIOTE, NRF_GPIOTE20);
#else
  instance.p_reg = NRF_GPIOTE;
  instance.cb.drv_inst_idx = NRFX_REG_TO_INSTANCE(GPIOTE, NRF_GPIOTE);
#endif
  return instance;
}();
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
inline bool i2s_reinitialize{};
inline bool i2s_streaming{};
inline bool i2s_circular{};
inline bool i2s_in_callback{};
inline std::uint8_t i2s_next{};
inline std::uint8_t i2s_blocks{};
inline std::size_t i2s_half_samples{};
inline void (*i2s_block_completion)(status, std::span<const std::uint16_t>, std::span<std::uint16_t>) noexcept {};

inline void i2s_handler(nrfx_i2s_buffers_t const* released, const std::uint32_t flags) noexcept
{
  if (!i2s_transfer.active.load())
    return;
  if ((flags & NRFX_I2S_STATUS_TRANSFER_STOPPED) != 0U)
  {
    if (!i2s_stopping)
      return;
    const auto callback = i2s_block_completion;
    i2s_block_completion = nullptr;
    const auto result = i2s_result;
    i2s_transfer.finish(result);
    if (callback != nullptr && result != status::ok)
      callback(result, {}, {});
    return;
  }
  if (i2s_stopping)
    return;
  if (i2s_streaming)
  {
    if (released == nullptr)
    {
      i2s_result = status::io_error;
      i2s_stopping = true;
      nrfx_i2s_stop(&i2s_instance);
      return;
    }
    const auto offset = (i2s_blocks % 2U) * (i2s_half_samples / 2U);
    const bool populated = released->p_tx_buffer != nullptr || released->p_rx_buffer != nullptr;
    const bool user_buffer = populated &&
                             (i2s_tx_buffer == nullptr || released->p_tx_buffer == i2s_tx_buffer + offset) &&
                             (i2s_rx_buffer == nullptr || released->p_rx_buffer == i2s_rx_buffer + offset);
    if (populated && !user_buffer)
      return;
    if (user_buffer)
    {
      ++i2s_blocks;
      i2s_in_callback = true;
      if (i2s_block_completion != nullptr)
        i2s_block_completion(
            status::ok,
            i2s_tx_buffer != nullptr
                ? std::span<const std::uint16_t>{reinterpret_cast<const std::uint16_t*>(released->p_tx_buffer),
                                                 i2s_half_samples}
                : std::span<const std::uint16_t>{},
            i2s_rx_buffer != nullptr
                ? std::span<std::uint16_t>{reinterpret_cast<std::uint16_t*>(released->p_rx_buffer), i2s_half_samples}
                : std::span<std::uint16_t>{});
      i2s_in_callback = false;
      if (!i2s_transfer.active.load() || i2s_stopping)
        return;
      if (!i2s_circular && i2s_blocks == 2U)
      {
        i2s_stopping = true;
        nrfx_i2s_stop(&i2s_instance);
        return;
      }
    }
    if ((flags & NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED) != 0U)
    {
      nrfx_i2s_buffers_t next{};
      if (!i2s_circular && i2s_next == 0U)
      {
        next.p_tx_buffer = i2s_tx_buffer != nullptr ? i2s_silence.data() : nullptr;
        next.p_rx_buffer = i2s_rx_buffer != nullptr ? i2s_discard.data() : nullptr;
        next.buffer_size = i2s_silence.size();
      }
      else
      {
        const auto next_offset = i2s_next * (i2s_half_samples / 2U);
        next.p_tx_buffer = i2s_tx_buffer != nullptr ? i2s_tx_buffer + next_offset : nullptr;
        next.p_rx_buffer = i2s_rx_buffer != nullptr ? i2s_rx_buffer + next_offset : nullptr;
        next.buffer_size = static_cast<std::uint16_t>(i2s_half_samples / 2U);
        i2s_next ^= 1U;
      }
      const auto error = nrfx_i2s_next_buffers_set(&i2s_instance, &next);
      if (error != 0)
      {
        i2s_result = driver_result(error);
        i2s_stopping = true;
        nrfx_i2s_stop(&i2s_instance);
      }
    }
    return;
  }
  if (released != nullptr && (released->p_tx_buffer != nullptr || released->p_rx_buffer != nullptr) &&
      ((i2s_tx_buffer != nullptr && released->p_tx_buffer != i2s_tx_buffer) ||
       (i2s_rx_buffer != nullptr && released->p_rx_buffer != i2s_rx_buffer)))
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
#endif
} // namespace detail

struct spi_api final
{
  template <typename Config> static status initialize() noexcept
  {
#if MICROHAL_NORDIC_HAS_SPIM
    if constexpr (requires { Config::binding::sck_pin; })
    {
      if (Config::instance_index >= std::size(detail::spim_instances))
        return status::invalid_argument;
      constexpr auto mosi = []() -> std::uint32_t
      {
        if constexpr (requires { Config::binding::mosi_pin; })
          return Config::binding::mosi_pin;
        return NRF_SPIM_PIN_NOT_CONNECTED;
      }();
      constexpr auto miso = []() -> std::uint32_t
      {
        if constexpr (requires { Config::binding::miso_pin; })
          return Config::binding::miso_pin;
        return NRF_SPIM_PIN_NOT_CONNECTED;
      }();
      if ((Config::duplex_mode != spi_duplex_mode::simplex_rx && mosi == NRF_SPIM_PIN_NOT_CONNECTED) ||
          ((Config::duplex_mode == spi_duplex_mode::full || Config::duplex_mode == spi_duplex_mode::simplex_rx) &&
           miso == NRF_SPIM_PIN_NOT_CONNECTED))
        return status::invalid_argument;
      auto& instance = detail::spim_instances[Config::instance_index];
      if (nrfx_spim_init_check(&instance))
        return status::busy;
      nrfx_spim_config_t config{};
      config.sck_pin = Config::binding::sck_pin;
      config.mosi_pin = Config::duplex_mode == spi_duplex_mode::simplex_rx ? NRF_SPIM_PIN_NOT_CONNECTED : mosi;
      config.miso_pin =
          Config::duplex_mode == spi_duplex_mode::simplex_tx || Config::duplex_mode == spi_duplex_mode::half
              ? NRF_SPIM_PIN_NOT_CONNECTED
              : miso;
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
      return detail::serial_initialize(instance.p_reg, 1U,
                                       [&] {
                                         return nrfx_spim_init(&instance, &config, detail::spim_handler,
                                                               &detail::spim_states[Config::instance_index]);
                                       });
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
    if constexpr (Config::duplex_mode == spi_duplex_mode::simplex_tx ||
                  Config::duplex_mode == spi_duplex_mode::simplex_rx || Config::duplex_mode == spi_duplex_mode::half)
    {
      if ((Config::duplex_mode == spi_duplex_mode::simplex_tx && !rx.empty()) ||
          (Config::duplex_mode == spi_duplex_mode::simplex_rx && !tx.empty()) ||
          (Config::duplex_mode == spi_duplex_mode::half && !tx.empty() && !rx.empty()))
      {
        state.completion = nullptr;
        state.finish(status::unsupported);
        return status::unsupported;
      }
      if constexpr (Config::duplex_mode == spi_duplex_mode::half && requires { Config::binding::mosi_pin; })
      {
        const auto pin = Config::binding::mosi_pin;
        if (tx.empty())
          nrf_gpio_cfg_input(pin, NRF_GPIO_PIN_NOPULL);
        else
          nrf_gpio_cfg_output(pin);
        nrf_spim_pins_set(instance.p_reg, Config::binding::sck_pin, tx.empty() ? NRF_SPIM_PIN_NOT_CONNECTED : pin,
                          rx.empty() ? NRF_SPIM_PIN_NOT_CONNECTED : pin);
      }
    }
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
    if (!detail::wait_until<Config>(
            [&]
            {
              if constexpr (configured_transfer_mode<Config>() == transfer_mode::polled)
                nrfx_spim_irq_handler(&detail::spim_instances[Config::instance_index]);
              return state.complete.load(std::memory_order_acquire);
            }))
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
#include "microhal/backends/nordic/nrfx/serial_spis_api.inc"
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
#if !NRF_UARTE_HAS_STOP_MODES
      if (Config::stop_bits != uart_stop_bits::one)
        return status::unsupported;
#else
      if (Config::stop_bits != uart_stop_bits::one && Config::stop_bits != uart_stop_bits::two)
        return status::unsupported;
#endif
#if !NRF_UARTE_HAS_PARITY_TYPES
      if (Config::parity == uart_parity::odd)
        return status::unsupported;
#endif
      auto& instance = detail::uarte_instances[Config::instance_index];
      if (Config::baud_rate > NRF_UARTE_BASE_FREQUENCY_GET(instance.p_reg) / 16U)
        return status::unsupported;
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
      config.tx_cache.p_buffer = detail::uarte_states[Config::instance_index].tx_cache.data();
      config.tx_cache.length = detail::uarte_states[Config::instance_index].tx_cache.size();
      detail::uarte_states[Config::instance_index].instance = &instance;
      config.baudrate = detail::uarte_baudrate(Config::baud_rate);
      config.config.parity =
          Config::parity != uart_parity::none ? NRF_UARTE_PARITY_INCLUDED : NRF_UARTE_PARITY_EXCLUDED;
#if NRF_UARTE_HAS_PARITY_TYPES
      config.config.paritytype =
          Config::parity == uart_parity::odd ? NRF_UARTE_PARITYTYPE_ODD : NRF_UARTE_PARITYTYPE_EVEN;
#endif
#if NRF_UARTE_HAS_STOP_MODES
      config.config.stop = Config::stop_bits == uart_stop_bits::two ? NRF_UARTE_STOP_TWO : NRF_UARTE_STOP_ONE;
#endif
      config.interrupt_priority = 7U;
      const auto result = detail::uarte_initialize_driver<Config>(instance, config, detail::uarte_handler);
      if constexpr (Config::baud_rate >= 16'000'000U)
        if (result == status::ok)
          nrf_uarte_baudrate_set(
              instance.p_reg, static_cast<nrf_uarte_baudrate_t>((static_cast<std::uint64_t>(Config::baud_rate) << 32U) /
                                                                NRF_UARTE_BASE_FREQUENCY_GET(instance.p_reg)));
      return result;
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
    state.tx_cancelled = false;
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
    state.rx_cancelled = false;
    state.idle = false;
    state.circular = false;
    state.receive = nullptr;
    state.received = {};
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
    if (!detail::wait_until<Config>(
            [&]
            {
              if constexpr (configured_transfer_mode<Config>() == transfer_mode::polled)
                nrfx_uarte_irq_handler(&detail::uarte_instances[Config::instance_index]);
              return state.complete.load(std::memory_order_acquire);
            }))
    {
      const auto error = nrfx_uarte_tx_abort(&detail::uarte_instances[Config::instance_index], true);
      if (error != 0 && error != -EINPROGRESS)
        return detail::driver_result(error);
      if (state.active.load())
        state.finish(status::timeout);
      else
        state.result = status::timeout;
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
    if (!detail::wait_until<Config>(
            [&]
            {
              if constexpr (configured_transfer_mode<Config>() == transfer_mode::polled)
                nrfx_uarte_irq_handler(&instance);
              return state.complete.load(std::memory_order_acquire);
            }))
    {
      const auto error = nrfx_uarte_rx_abort(&instance, true, true);
      if (error != 0 && error != -EINPROGRESS)
        return detail::driver_result(error);
      if (state.active.load())
        state.finish(status::timeout);
      else
        state.result = status::timeout;
      return status::timeout;
    }
    return state.result;
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
#include "microhal/backends/nordic/nrfx/serial_uarte_api.inc"
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
    detail::timer_states[Config::instance_index] = {};
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
    auto& state = detail::timer_states[Config::instance_index];
    for (std::size_t channel = 0U; channel < state.callbacks.size(); ++channel)
      if (state.callbacks[channel] != nullptr && (state.captures & (1U << channel)) == 0U &&
          (state.periodic & (1U << channel)) == 0U && state.compare_ticks[channel] > ticks)
        return status::invalid_argument;
    if (running)
      nrfx_timer_pause(&instance);
    nrfx_timer_clear(&instance);
    nrfx_timer_extended_compare(&instance, NRF_TIMER_CC_CHANNEL0, ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);
    state.period = ticks;
    for (std::size_t channel = 0U; channel < state.callbacks.size(); ++channel)
      if (state.callbacks[channel] != nullptr && (state.captures & (1U << channel)) == 0U)
        nrfx_timer_compare(&instance, static_cast<nrf_timer_cc_channel_t>(channel + 1U),
                           (state.periodic & (1U << channel)) != 0U ? ticks : state.compare_ticks[channel], true);
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
      return nrfx_timer_capture(
          &detail::timer_instances[Config::instance_index],
          static_cast<nrf_timer_cc_channel_t>(detail::timer_cc_count(Config::instance_index) - 1U));
#endif
    return 0U;
  }
  template <typename Config, typename Channel, auto Handler> static status enable_capture_compare() noexcept
  {
#if MICROHAL_NORDIC_HAS_TIMER
    if (Config::instance_index >= std::size(detail::timer_instances) || Channel::channel_index < 1U ||
        Channel::channel_index > detail::timer_cc_count(Config::instance_index) - 2U)
      return status::invalid_argument;
    if (Channel::enable_capture == Channel::enable_compare)
      return status::unsupported;
    auto& instance = detail::timer_instances[Config::instance_index];
    if (!nrfx_timer_init_check(&instance))
      return status::unsupported;
    auto& state = detail::timer_states[Config::instance_index];
    constexpr auto slot = Channel::channel_index - 1U;
    if (state.callbacks[slot] != nullptr)
      return status::busy;
    const auto callback = +[]() noexcept { detail::notify<Handler>(status::ok); };
    if constexpr (Channel::enable_capture)
    {
      if constexpr (requires {
                      Config::binding::capture_connect(Channel::channel_index, std::uintptr_t{}, callback);
                      Config::binding::capture_disconnect(Channel::channel_index);
                    })
      {
        const auto task =
            nrfx_timer_capture_task_address_get(&instance, static_cast<nrf_timer_cc_channel_t>(Channel::channel_index));
        const auto connected = Config::binding::capture_connect(Channel::channel_index, task, callback);
        if (connected != status::ok)
          return connected;
        state.captures |= 1U << slot;
        state.callbacks[slot] = callback;
        return status::ok;
      }
      return status::unsupported;
    }
    std::uint32_t ticks = 0U;
    if constexpr (requires { Channel::compare_ticks; })
      ticks = Channel::compare_ticks;
    else
      state.periodic |= 1U << slot;
    if (ticks > state.period)
      return status::invalid_argument;
    state.callbacks[slot] = callback;
    state.compare_ticks[slot] = ticks;
    nrfx_timer_compare(&instance, static_cast<nrf_timer_cc_channel_t>(Channel::channel_index),
                       (state.periodic & (1U << slot)) != 0U ? state.period : ticks, true);
    return status::ok;
#else
    return status::unsupported;
#endif
  }
  template <typename Config, typename Channel> static result<std::uint32_t> capture_value() noexcept
  {
#if MICROHAL_NORDIC_HAS_TIMER
    if (Config::instance_index >= std::size(detail::timer_instances) || Channel::channel_index < 1U ||
        Channel::channel_index > detail::timer_cc_count(Config::instance_index) - 2U)
      return status::invalid_argument;
    if (!nrfx_timer_init_check(&detail::timer_instances[Config::instance_index]) ||
        (detail::timer_states[Config::instance_index].captures & (1U << (Channel::channel_index - 1U))) == 0U)
      return status::unsupported;
    return nrfx_timer_capture_get(&detail::timer_instances[Config::instance_index],
                                  static_cast<nrf_timer_cc_channel_t>(Channel::channel_index));
#endif
    return status::unsupported;
  }
  template <typename Config, typename Channel> static status disable_capture_compare() noexcept
  {
#if MICROHAL_NORDIC_HAS_TIMER
    if (Config::instance_index >= std::size(detail::timer_instances) || Channel::channel_index < 1U ||
        Channel::channel_index > detail::timer_cc_count(Config::instance_index) - 2U)
      return status::invalid_argument;
    auto& state = detail::timer_states[Config::instance_index];
    if (!nrfx_timer_init_check(&detail::timer_instances[Config::instance_index]))
      return status::unsupported;
    if ((state.captures & (1U << (Channel::channel_index - 1U))) != 0U)
    {
      if constexpr (requires { Config::binding::capture_disconnect(Channel::channel_index); })
        Config::binding::capture_disconnect(Channel::channel_index);
    }
    nrfx_timer_compare_int_disable(&detail::timer_instances[Config::instance_index], Channel::channel_index);
    state.captures &= ~(1U << (Channel::channel_index - 1U));
    state.periodic &= ~(1U << (Channel::channel_index - 1U));
    state.callbacks[Channel::channel_index - 1U] = nullptr;
    return status::ok;
#else
    return status::unsupported;
#endif
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
      const auto timing = detail::pwm_clock(Config::frequency_hz);
      if (!timing)
        return timing.error();
      auto& state = detail::pwm_states[Config::timer_index];
      constexpr auto bit = 1U << (Config::channel - 1U);
      if (nrfx_pwm_init_check(&instance))
      {
        if (!state.initialized || (state.owned & bit) != 0U)
          return status::busy;
        if (state.top != timing.value().top || state.clock != timing.value().prescaler)
          return status::invalid_argument;
        for (std::size_t channel = 0U; channel < state.pins.size(); ++channel)
          if ((state.owned & (1U << channel)) != 0U && state.pins[channel] == Config::binding::output_pin)
            return status::invalid_argument;
        nrf_gpio_pin_write(Config::binding::output_pin, Config::polarity == pwm_polarity::active_low);
        nrf_gpio_cfg_output(Config::binding::output_pin);
        nrf_pwm_pin_set(instance.p_reg, Config::channel - 1U, Config::binding::output_pin);
        state.owned |= bit;
        state.pins[Config::channel - 1U] = Config::binding::output_pin;
        if (Config::polarity == pwm_polarity::active_low)
          state.inverted |= bit;
        return set_duty_cycle<Config>(0U);
      }
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
      state = {};
      state.values = {0x8000U, 0x8000U, 0x8000U, 0x8000U};
      state.top = config.top_value;
      state.clock = timing.value().prescaler;
      state.owned = bit;
      state.pins[Config::channel - 1U] = Config::binding::output_pin;
      state.inverted = Config::polarity == pwm_polarity::active_low ? bit : 0U;
      state.initialized = true;
      return set_duty_cycle<Config>(0U);
    }
#endif
    return status::unsupported;
  }
  template <typename Config> static status set_duty_cycle(const std::uint16_t permille) noexcept
  {
#if MICROHAL_NORDIC_HAS_PWM
    if (permille > 1000U || Config::timer_index >= std::size(detail::pwm_instances) || Config::channel < 1U ||
        Config::channel > 4U)
      return status::invalid_argument;
    auto& state = detail::pwm_states[Config::timer_index];
    if (!state.initialized || (state.owned & (1U << (Config::channel - 1U))) == 0U)
      return status::unsupported;
    state.duty[Config::channel - 1U] = permille;
    const auto duty = (state.enabled & (1U << (Config::channel - 1U))) != 0U ? permille : 0U;
    const auto value =
        static_cast<std::uint16_t>((static_cast<std::uint32_t>(state.top) * duty / 1000U) |
                                   ((state.inverted & (1U << (Config::channel - 1U))) != 0U ? 0U : 0x8000U));
    switch (Config::channel)
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
    std::atomic_thread_fence(std::memory_order_release);
    return status::ok;
#else
    (void)permille;
    return status::unsupported;
#endif
  }
  template <typename Config> static status start() noexcept
  {
#if MICROHAL_NORDIC_HAS_PWM
    if (Config::timer_index >= std::size(detail::pwm_instances) || Config::channel < 1U || Config::channel > 4U)
      return status::invalid_argument;
    auto& state = detail::pwm_states[Config::timer_index];
    constexpr auto bit = 1U << (Config::channel - 1U);
    if (!state.initialized || (state.owned & bit) == 0U)
      return status::unsupported;
    if ((state.enabled & bit) != 0U)
      return status::busy;
    state.enabled |= bit;
    (void)set_duty_cycle<Config>(state.duty[Config::channel - 1U]);
    if (state.running)
      return status::ok;
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
    if (Config::timer_index >= std::size(detail::pwm_instances) || Config::channel < 1U || Config::channel > 4U)
      return status::invalid_argument;
    auto& state = detail::pwm_states[Config::timer_index];
    constexpr auto bit = 1U << (Config::channel - 1U);
    if (!state.initialized || (state.owned & bit) == 0U)
      return status::unsupported;
    if ((state.enabled & bit) == 0U)
      return status::ok;
    if ((state.enabled & ~bit) != 0U)
    {
      state.enabled &= ~bit;
      return set_duty_cycle<Config>(state.duty[Config::channel - 1U]);
    }
    auto* const instance = &detail::pwm_instances[Config::timer_index];
    if (!nrfx_pwm_stop(instance, false) &&
        !detail::wait_until<Config>([instance] { return nrfx_pwm_stopped_check(instance); }))
      return status::timeout;
    state.running = false;
    state.enabled = 0U;
    return status::ok;
#else
    return status::unsupported;
#endif
  }
  template <typename Config> static status set_frequency(const std::uint32_t frequency) noexcept
  {
#if MICROHAL_NORDIC_HAS_PWM
    if (Config::timer_index >= std::size(detail::pwm_instances))
      return status::invalid_argument;
    auto& state = detail::pwm_states[Config::timer_index];
    if (!state.initialized)
      return status::unsupported;
    if (state.running)
      return status::busy;
    const auto timing = detail::pwm_clock(frequency);
    if (!timing)
      return timing.error();
    state.top = timing.value().top;
    state.clock = timing.value().prescaler;
    nrf_pwm_configure(detail::pwm_instances[Config::timer_index].p_reg, static_cast<nrf_pwm_clk_t>(state.clock),
                      NRF_PWM_MODE_UP, state.top);
    std::array<std::uint16_t*, 4U> values{&state.values.channel_0, &state.values.channel_1, &state.values.channel_2,
                                          &state.values.channel_3};
    for (std::size_t channel = 0U; channel < state.duty.size(); ++channel)
      *values[channel] = (state.inverted & (1U << channel)) != 0U ? 0U : 0x8000U;
    return status::ok;
#else
    (void)frequency;
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
    if (detail::saadc_in_callback)
      return status::busy;
    if constexpr (requires { Config::binding::channels; })
    {
      if (Config::binding::channels.empty() || Config::binding::channels.size() > 8U)
        return status::invalid_argument;
      std::uint32_t configured_mask = 0U;
      for (const auto& channel : Config::binding::channels)
      {
        if (channel.channel_index >= 8U || (configured_mask & (1U << channel.channel_index)) != 0U)
          return status::invalid_argument;
        if ((channel.channel_config.mode == NRF_SAADC_MODE_DIFFERENTIAL) != Config::differential_mode)
          return status::invalid_argument;
        configured_mask |= 1U << channel.channel_index;
      }
      if (nrfx_saadc_init_check())
        return status::busy;
      const auto result = detail::driver_result(nrfx_saadc_init(7U));
      if (result != status::ok)
        return result;
      const auto configured = detail::driver_result(
          nrfx_saadc_channels_config(Config::binding::channels.data(), Config::binding::channels.size()));
      if (configured != status::ok)
        nrfx_saadc_uninit();
      else
      {
        detail::saadc_configured = 0U;
        for (const auto& channel : Config::binding::channels)
          detail::saadc_configured |= 1U << channel.channel_index;
        detail::saadc_differential = Config::differential_mode;
        detail::saadc_bits = static_cast<std::uint8_t>(Config::resolution);
        detail::saadc_reinitialize = false;
      }
      return configured;
    }
#endif
    return status::unsupported;
  }

  template <typename Config> static result<std::uint16_t> read_blocking(const std::uint8_t channel) noexcept
  {
#if MICROHAL_NORDIC_HAS_SAADC && MICROHAL_NORDIC_SYSTEM_CORE_CLOCK
    if (Config::instance_index != 0U || channel >= 8U)
      return status::invalid_argument;
    if (SystemCoreClock == 0U)
      return status::error;
    if (detail::saadc_active.load(std::memory_order_acquire) && detail::saadc_continuous)
    {
      if (channel >= 8U)
        return status::invalid_argument;
      bool selected = false;
      for (std::size_t index = 0U; index < detail::saadc_continuous_count; ++index)
        selected |= detail::saadc_continuous_channels[index] == channel;
      if (!selected)
        return status::invalid_argument;
      if (!detail::wait_until<Config>(
              [channel]
              {
                return (detail::saadc_latest_valid.load(std::memory_order_acquire) & (1U << channel)) != 0U ||
                       !detail::saadc_active.load(std::memory_order_acquire);
              }))
      {
        (void)cancel<Config>(status::timeout, false);
        return status::timeout;
      }
      if ((detail::saadc_latest_valid.load(std::memory_order_acquire) & (1U << channel)) == 0U)
        return detail::saadc_result == status::ok ? status::error : detail::saadc_result;
      return detail::saadc_latest[channel].load(std::memory_order_acquire);
    }
    const auto started = start_async<Config, nullptr>(channel);
    if (started != status::ok)
      return started;
    if (!detail::wait_until<Config>([] { return detail::saadc_complete.load(std::memory_order_acquire); }))
    {
      (void)cancel<Config>(status::timeout, false);
      return status::timeout;
    }
    if (detail::saadc_result != status::ok)
      return detail::saadc_result;
    return detail::saadc_unsigned(detail::saadc_sample);
#else
    (void)channel;
    return status::unsupported;
#endif
  }
  template <typename Config> static result<std::int16_t> read_differential_blocking(const std::uint8_t channel) noexcept
  {
    constexpr auto bits = static_cast<std::uint32_t>(Config::resolution);
    if constexpr (!Config::differential_mode || (bits != 8U && bits != 10U && bits != 12U && bits != 14U))
      return status::unsupported;
    else
    {
      const auto sample = read_blocking<Config>(channel);
      if (!sample)
        return sample.error();
      return static_cast<std::int16_t>(static_cast<std::int32_t>(sample.value()) -
                                       static_cast<std::int32_t>(1U << (bits - 1U)));
    }
  }
  template <typename Config> static status start_continuous(const std::span<const std::uint8_t> channels) noexcept
  {
#if MICROHAL_NORDIC_HAS_SAADC
    const auto mask = detail::saadc_channel_mask(channels);
    if (!mask)
      return mask.error();
    if (detail::saadc_active.load() || detail::saadc_in_callback)
      return status::busy;
    detail::saadc_latest_valid.store(0U);
    detail::saadc_continuous_count = static_cast<std::uint8_t>(channels.size());
    for (std::size_t index = 0U; index < channels.size(); ++index)
      detail::saadc_continuous_channels[index] = channels[index];
    constexpr auto callback = [](const status value, const std::span<std::uint16_t> block) noexcept
    {
      if (value != status::ok)
        return;
      std::uint32_t valid = 0U;
      for (std::size_t index = 0U; index < detail::saadc_continuous_count; ++index)
      {
        const auto channel = detail::saadc_continuous_channels[index];
        detail::saadc_latest[channel].store(block[index], std::memory_order_relaxed);
        valid |= 1U << channel;
      }
      detail::saadc_latest_valid.store(valid, std::memory_order_release);
    };
    const auto result = start_dma_stream<Config, callback>(
        channels, std::span<std::uint16_t>{detail::saadc_continuous_storage}.first(channels.size() * 2U), true);
    if (result == status::ok)
      detail::saadc_continuous = true;
    return result;
#else
    (void)channels;
    return status::unsupported;
#endif
  }
  template <typename Config, auto Handler> static status start_async(const std::uint8_t channel) noexcept
  {
#if MICROHAL_NORDIC_HAS_SAADC
    if (channel >= 8U || Config::instance_index != 0U)
      return status::invalid_argument;
    if constexpr (requires { Config::trigger_edge; })
      if (Config::trigger_edge != adc_trigger_edge::software)
        return status::unsupported;
    if (detail::saadc_in_callback)
      return status::busy;
    if (detail::saadc_reinitialize)
    {
      if (nrfx_saadc_init_check())
        nrfx_saadc_uninit();
      const auto initialized = initialize<Config>();
      if (initialized != status::ok)
        return initialized;
    }
    if ((detail::saadc_configured & (1U << channel)) == 0U)
      return status::invalid_argument;
    if (!nrfx_saadc_init_check())
      return status::unsupported;
    if (detail::saadc_active.exchange(true))
      return status::busy;
    detail::saadc_complete.store(false);
    detail::saadc_result = status::ok;
    detail::saadc_streaming = false;
    detail::saadc_completion = +[](const status value) noexcept
    {
      if constexpr (requires { Handler(value, std::uint16_t{}); })
        Handler(value, std::uint16_t{});
      else
        detail::notify<Handler>(value);
    };
    const auto callback = +[](nrfx_saadc_evt_t const* event)
    {
      if (event->type != NRFX_SAADC_EVT_DONE || !detail::saadc_active.load())
        return;
      if (event->data.done.p_buffer != &detail::saadc_sample || event->data.done.size != 1U)
        return;
      detail::saadc_completion = nullptr;
      detail::saadc_finish(status::ok);
      const auto sample = detail::saadc_unsigned(detail::saadc_sample);
      if constexpr (requires { Handler(status::ok, sample); })
        Handler(status::ok, sample);
      else if constexpr (requires { Handler(sample); })
        Handler(sample);
      else
        detail::notify<Handler>(status::ok);
    };
    int error = nrfx_saadc_simple_mode_set(1UL << channel, detail::saadc_resolution(Config::resolution),
                                           NRF_SAADC_OVERSAMPLE_DISABLED, callback);
    if (error == 0)
      error = nrfx_saadc_buffer_set(&detail::saadc_sample, 1U);
    if (error == 0)
      error = nrfx_saadc_mode_trigger();
    if (error != 0)
    {
      (void)cancel<Config>(detail::driver_result(error), false);
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
    if constexpr (requires { Config::trigger_edge; })
      if (Config::trigger_edge != adc_trigger_edge::software)
        return status::unsupported;
    if (detail::saadc_in_callback)
      return status::busy;
    if (detail::saadc_reinitialize)
    {
      if (nrfx_saadc_init_check())
        nrfx_saadc_uninit();
      const auto initialized = initialize<Config>();
      if (initialized != status::ok)
        return initialized;
    }
    if ((mask.value() & ~detail::saadc_configured) != 0U)
      return status::invalid_argument;
    if (!nrfx_saadc_init_check())
      return status::unsupported;
    if (detail::saadc_active.exchange(true))
      return status::busy;
    detail::saadc_scan = samples;
    detail::saadc_complete.store(false);
    detail::saadc_result = status::ok;
    detail::saadc_streaming = false;
    detail::saadc_completion = detail::notify<Handler>;
    const auto callback = +[](nrfx_saadc_evt_t const* event)
    {
      if (event->type != NRFX_SAADC_EVT_DONE || !detail::saadc_active.load())
        return;
      if (event->data.done.p_buffer != reinterpret_cast<nrf_saadc_value_t*>(detail::saadc_scan.data()) ||
          event->data.done.size != detail::saadc_scan.size())
        return;
      for (auto& sample : detail::saadc_scan)
        sample = detail::saadc_unsigned(static_cast<nrf_saadc_value_t>(sample));
      detail::saadc_finish(status::ok);
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
      (void)cancel<Config>(detail::driver_result(error), false);
    }
    return detail::driver_result(error);
#else
    (void)channels;
    (void)samples;
    return status::unsupported;
#endif
  }
  template <typename Config, auto Handler>
  static status start_dma_stream(const std::span<const std::uint8_t> channels, const std::span<std::uint16_t> samples,
                                 const bool circular = true) noexcept
  {
#if MICROHAL_NORDIC_HAS_SAADC
    const auto mask = detail::saadc_channel_mask(channels);
    if (!mask || Config::instance_index != 0U || samples.empty() || samples.size() % 2U != 0U ||
        samples.size() / 2U > UINT16_MAX || (samples.size() / 2U) % channels.size() != 0U)
      return status::invalid_argument;
    if (detail::saadc_in_callback)
      return status::busy;
    if (detail::saadc_reinitialize)
    {
      if (nrfx_saadc_init_check())
        nrfx_saadc_uninit();
      const auto initialized = initialize<Config>();
      if (initialized != status::ok)
        return initialized;
    }
    if (!nrfx_saadc_init_check())
      return status::unsupported;
    if ((mask.value() & ~detail::saadc_configured) != 0U)
      return status::invalid_argument;
    if (detail::saadc_in_callback || detail::saadc_active.exchange(true))
      return status::busy;
    nrfx_saadc_adv_config_t config{};
    config.oversampling = NRF_SAADC_OVERSAMPLE_DISABLED;
    config.burst = NRF_SAADC_BURST_DISABLED;
    config.start_on_end = true;
    if constexpr (requires { Config::binding::saadc_internal_timer_cc; })
    {
      const auto cc = Config::binding::saadc_internal_timer_cc;
      if (channels.size() != 1U || cc < NRF_SAADC_SAMPLERATE_CC_MIN || cc > NRF_SAADC_SAMPLERATE_CC_MAX)
      {
        detail::saadc_active.store(false);
        return status::invalid_argument;
      }
      config.internal_timer_cc = cc;
      if constexpr (requires { Config::trigger_edge; })
        if (Config::trigger_edge != adc_trigger_edge::software)
        {
          detail::saadc_active.store(false);
          return status::unsupported;
        }
    }
    else if constexpr (requires {
                         Config::binding::saadc_trigger_connect(std::uintptr_t{}, std::uint32_t{}, adc_trigger_edge{});
                         Config::binding::saadc_trigger_start();
                         Config::binding::saadc_trigger_stop();
                         Config::binding::saadc_trigger_disconnect();
                       })
    {
      constexpr auto trigger = []
      {
        if constexpr (requires { Config::external_trigger; })
          return Config::external_trigger;
        return std::uint32_t{};
      }();
      constexpr auto edge = []
      {
        if constexpr (requires { Config::trigger_edge; })
          return Config::trigger_edge;
        return adc_trigger_edge::software;
      }();
      const auto connected = Config::binding::saadc_trigger_connect(
          nrf_saadc_task_address_get(NRF_SAADC, NRF_SAADC_TASK_SAMPLE), trigger, edge);
      if (connected != status::ok)
      {
        detail::saadc_active.store(false);
        return connected;
      }
      detail::saadc_trigger_start = Config::binding::saadc_trigger_start;
      detail::saadc_trigger_stop = Config::binding::saadc_trigger_stop;
      detail::saadc_trigger_disconnect = Config::binding::saadc_trigger_disconnect;
    }
    else
    {
      detail::saadc_active.store(false);
      return status::unsupported;
    }
    detail::saadc_complete.store(false);
    detail::saadc_result = status::ok;
    detail::saadc_storage = samples;
    detail::saadc_streaming = true;
    detail::saadc_continuous = false;
    detail::saadc_circular = circular;
    detail::saadc_next = 0U;
    detail::saadc_blocks = 0U;
    detail::saadc_completion = nullptr;
    detail::saadc_block_completion = +[](const status value, const std::span<std::uint16_t> block) noexcept
    {
      if constexpr (requires { Handler(value, block); })
        Handler(value, block);
      else
        detail::notify<Handler>(value);
    };
    auto error = nrfx_saadc_advanced_mode_set(mask.value(), detail::saadc_resolution(Config::resolution), &config,
                                              detail::saadc_stream_handler);
    const auto half = static_cast<std::uint16_t>(samples.size() / 2U);
    if (error == 0)
      error = nrfx_saadc_buffer_set(reinterpret_cast<nrf_saadc_value_t*>(samples.data()), half);
    if (error == 0)
      error = nrfx_saadc_buffer_set(reinterpret_cast<nrf_saadc_value_t*>(samples.data() + half), half);
    if (error == 0)
      error = nrfx_saadc_mode_trigger();
    if (error != 0)
      (void)cancel<Config>(detail::driver_result(error), false);
    return detail::driver_result(error);
#else
    (void)channels;
    (void)samples;
    (void)circular;
    return status::unsupported;
#endif
  }
  template <typename Config> static status stop() noexcept
  {
    return cancel<Config>(status::ok, false);
  }
  template <typename Config> static status abort() noexcept
  {
    return cancel<Config>(status::error, true);
  }
  template <typename Config> static result<dma_transfer_state> transfer_status() noexcept
  {
#if MICROHAL_NORDIC_HAS_SAADC
    if (Config::instance_index != 0U)
      return status::invalid_argument;
    if (detail::saadc_active.load(std::memory_order_acquire))
      return dma_transfer_state::active;
    if (!detail::saadc_complete.load(std::memory_order_acquire))
      return dma_transfer_state::idle;
    return detail::saadc_result == status::ok ? dma_transfer_state::complete : dma_transfer_state::error;
#else
    return status::unsupported;
#endif
  }
  static void handle_interrupt() noexcept
  {
#if MICROHAL_NORDIC_HAS_SAADC
    if (nrfx_saadc_init_check())
      nrfx_saadc_irq_handler();
#endif
  }
  template <typename Config> static void handle_interrupt() noexcept
  {
    handle_interrupt();
  }
  template <typename Config> static void handle_dma_interrupt() noexcept
  {
    handle_interrupt();
  }

private:
  template <typename Config> static status cancel(const status result, const bool notify) noexcept
  {
#if MICROHAL_NORDIC_HAS_SAADC
    if (Config::instance_index != 0U)
      return status::invalid_argument;
    if (!detail::saadc_active.exchange(false))
      return status::ok;
    const auto block = notify ? detail::saadc_block_completion : nullptr;
    if (!notify)
      detail::saadc_completion = nullptr;
    detail::saadc_release_trigger();
    nrfx_saadc_uninit();
    detail::saadc_reinitialize = true;
    detail::saadc_finish(result);
    if (block != nullptr)
      block(result, {});
    return status::ok;
#else
    (void)result;
    (void)notify;
    return status::unsupported;
#endif
  }
};

#include "microhal/backends/nordic/nrfx/qspi_engine.inc"

struct i2s_api final
{
  template <typename Config> static status initialize() noexcept
  {
#if MICROHAL_NORDIC_HAS_I2S
    if (detail::i2s_in_callback)
      return status::busy;
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
      if constexpr (requires { Config::binding::i2s_clock_source; })
        config.clksrc = Config::binding::i2s_clock_source;
#endif
      constexpr auto source_clock = []
      {
        if constexpr (requires { Config::binding::i2s_source_clock_hz; })
          return Config::binding::i2s_source_clock_hz;
        return 32'000'000U;
      }();
      if (source_clock == 0U)
        return status::invalid_argument;
#if NRF_I2S_HAS_CLKCONFIG
      if (config.clksrc == NRF_I2S_CLKSRC_PCLK32M && source_clock != 32'000'000U)
        return status::invalid_argument;
      if (config.clksrc != NRF_I2S_CLKSRC_PCLK32M)
        if constexpr (!requires { Config::binding::i2s_source_clock_hz; })
          return status::unsupported;
#else
      if (source_clock != 32'000'000U)
        return status::unsupported;
#endif
      nrfx_i2s_clk_params_t clocks{source_clock, Config::sample_rate, NRF_I2S_SWIDTH_16BIT, false};
      const auto calculated = detail::driver_result(nrfx_i2s_prescalers_calc(&clocks, &config.prescalers));
      if (calculated != status::ok)
        return calculated;
      const auto initialized =
          detail::driver_result(nrfx_i2s_init(&detail::i2s_instance, &config, detail::i2s_handler));
      if (initialized == status::ok)
      {
        detail::i2s_transmit_mode = Config::mode == i2s_mode::master_tx || Config::mode == i2s_mode::slave_tx;
        detail::i2s_reinitialize = false;
        if constexpr (configured_transfer_mode<Config>() == transfer_mode::polled)
          NRFX_IRQ_DISABLE(nrfx_get_irq_number(detail::i2s_instance.p_reg));
      }
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
  template <typename Config, auto Handler> static status start_dma_rx(const std::span<std::uint16_t> samples) noexcept
  {
    return start<Config, Handler>(nullptr, samples.data(), samples.size());
  }
  template <typename Config, auto Handler>
  static status start_dma_transfer(const std::span<const std::uint16_t> tx, const std::span<std::uint16_t> rx) noexcept
  {
    if (tx.size() != rx.size())
      return status::invalid_argument;
    return start<Config, Handler>(tx.data(), rx.data(), tx.size());
  }
  template <typename Config, auto Handler>
  static status start_dma_stream(const std::span<const std::uint16_t> tx, const std::span<std::uint16_t> rx,
                                 const bool circular = true) noexcept
  {
#if MICROHAL_NORDIC_HAS_I2S
    const auto count = tx.empty() ? rx.size() : tx.size();
    if ((!tx.empty() && !rx.empty() && tx.size() != rx.size()) || count % 4U != 0U)
      return status::invalid_argument;
    return start<Config, Handler>(tx.empty() ? nullptr : tx.data(), rx.empty() ? nullptr : rx.data(), count / 2U, true,
                                  circular);
#else
    (void)tx;
    (void)rx;
    (void)circular;
    return status::unsupported;
#endif
  }
  template <typename Config> static status stop() noexcept
  {
    return cancel<Config>(status::ok, false);
  }
  template <typename Config> static status abort() noexcept
  {
    return cancel<Config>(status::error, true);
  }
  template <typename Config> static result<dma_transfer_state> transfer_status() noexcept
  {
#if MICROHAL_NORDIC_HAS_I2S
    if (detail::i2s_transfer.active.load(std::memory_order_acquire))
      return dma_transfer_state::active;
    if (!detail::i2s_transfer.complete.load(std::memory_order_acquire))
      return dma_transfer_state::idle;
    return detail::i2s_transfer.result == status::ok ? dma_transfer_state::complete : dma_transfer_state::error;
#else
    return status::unsupported;
#endif
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
  template <typename Config>
  static status transfer_blocking(const std::span<const std::uint16_t> tx, const std::span<std::uint16_t> rx) noexcept
  {
#if MICROHAL_NORDIC_SYSTEM_CORE_CLOCK
    if (SystemCoreClock == 0U)
      return status::error;
    const auto result = start_dma_transfer<Config, nullptr>(tx, rx);
    return result == status::ok ? wait<Config>() : result;
#else
    (void)tx;
    (void)rx;
    return status::unsupported;
#endif
  }
  static void handle_interrupt() noexcept
  {
#if MICROHAL_NORDIC_HAS_I2S
    if (nrfx_i2s_init_check(&detail::i2s_instance))
      nrfx_i2s_irq_handler(&detail::i2s_instance);
#endif
  }

private:
  template <typename Config, auto Handler>
  static status start(const std::uint16_t* tx, std::uint16_t* rx, const std::size_t count) noexcept
  {
    return start<Config, Handler>(tx, rx, count, false, false);
  }
  template <typename Config, auto Handler>
  static status start(const std::uint16_t* tx, std::uint16_t* rx, const std::size_t count, const bool streaming,
                      const bool circular) noexcept
  {
#if MICROHAL_NORDIC_HAS_I2S
    if constexpr (requires { Config::instance_index; })
      if (Config::instance_index != 0U)
        return status::invalid_argument;
    constexpr std::size_t max_count = I2S_RXTXD_MAXCNT_MAXCNT_Msk >> I2S_RXTXD_MAXCNT_MAXCNT_Pos;
#if defined(DMA_BUFFER_UNIFIED_BYTE_ACCESS)
    constexpr auto max_words = max_count / sizeof(std::uint32_t);
#else
    constexpr auto max_words = max_count;
#endif
    if ((tx == nullptr && rx == nullptr) || !detail::i2s_count_valid(count, max_words) ||
        reinterpret_cast<std::uintptr_t>(tx) % 4U != 0U || reinterpret_cast<std::uintptr_t>(rx) % 4U != 0U)
      return status::invalid_argument;
    if (detail::i2s_in_callback)
      return status::busy;
    if (detail::i2s_reinitialize)
    {
      const auto initialized = initialize<Config>();
      if (initialized != status::ok)
        return initialized;
    }
    if (!nrfx_i2s_init_check(&detail::i2s_instance))
      return status::unsupported;
    if ((tx == nullptr || rx == nullptr) && ((tx != nullptr) != detail::i2s_transmit_mode))
      return status::unsupported;
    if (!detail::i2s_transfer.begin(detail::notify<Handler>))
      return status::busy;
    detail::i2s_tx_buffer = reinterpret_cast<const std::uint32_t*>(tx);
    detail::i2s_rx_buffer = reinterpret_cast<std::uint32_t*>(rx);
    detail::i2s_stopping = false;
    detail::i2s_result = status::ok;
    detail::i2s_streaming = streaming;
    detail::i2s_circular = circular;
    detail::i2s_half_samples = count;
    detail::i2s_blocks = 0U;
    detail::i2s_next = 1U;
    detail::i2s_block_completion = nullptr;
    if (streaming)
    {
      detail::i2s_transfer.completion = nullptr;
      detail::i2s_block_completion = +[](const status value, const std::span<const std::uint16_t> tx_block,
                                         const std::span<std::uint16_t> rx_block) noexcept
      {
        if constexpr (requires { Handler(value, tx_block, rx_block); })
          Handler(value, tx_block, rx_block);
        else
          detail::notify<Handler>(value);
      };
    }
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
    (void)streaming;
    (void)circular;
    return status::unsupported;
#endif
  }
  template <typename Config> static status wait() noexcept
  {
#if MICROHAL_NORDIC_HAS_I2S
    if (!detail::wait_until<Config>(
            []
            {
              if constexpr (configured_transfer_mode<Config>() == transfer_mode::polled)
                handle_interrupt();
              return detail::i2s_transfer.complete.load(std::memory_order_acquire);
            }))
    {
      (void)cancel<Config>(status::timeout, false);
      return status::timeout;
    }
    return detail::i2s_transfer.result;
#else
    return status::unsupported;
#endif
  }
  template <typename Config> static status cancel(const status result, const bool notify) noexcept
  {
#if MICROHAL_NORDIC_HAS_I2S
    if (!detail::i2s_transfer.active.exchange(false))
      return status::ok;
    const auto callback = notify ? detail::i2s_block_completion : nullptr;
    detail::i2s_block_completion = nullptr;
    if (!notify)
      detail::i2s_transfer.completion = nullptr;
    nrfx_i2s_uninit(&detail::i2s_instance);
    detail::i2s_reinitialize = true;
    detail::i2s_transfer.finish(result);
    if (callback != nullptr)
      callback(result, {}, {});
    return status::ok;
#else
    (void)result;
    (void)notify;
    return status::unsupported;
#endif
  }
};

#include "microhal/backends/nordic/nrfx/rtc_engine.inc"
} // namespace microhal::backends::nordic::nrfx
