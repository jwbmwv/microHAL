// SPDX-License-Identifier: Apache-2.0
#define MICROHAL_NORDIC_NRFX_PRESENT 1
#define MICROHAL_NORDIC_SYSTEM_CORE_CLOCK 1
#define NRF52840_XXAA
#define NRFX_TIMER_ENABLED 1
#define NRFX_PWM_ENABLED 1
#define NRFX_SAADC_ENABLED 1
#define NRFX_I2S_ENABLED 1
#define MICROHAL_BACKENDS_EXPLICIT 1
#define MICROHAL_ENABLE_NORDIC_NRFX 1

#include "microhal/backends/nordic/nrfx/acquisition_modes.hpp"
#include "microhal/backends/nordic/nrfx/peripherals.hpp"
#include "microhal/backends/nordic/nrfx/traits.hpp"
#include "microhal/i2s/i2s.hpp"
#include "microhal/pwm/pwm.hpp"
#include "microhal/timers/timer.hpp"
#include "nordic_acquisition_fakes/drivers.hpp"
#include <cassert>

namespace
{
using namespace microhal;
using namespace microhal::backends::nordic::nrfx;
namespace state = microhal::backends::nordic::nrfx::detail;

std::uint32_t notifications{}, blocks{}, audio_blocks{}, trigger_starts{}, trigger_stops{}, disconnects{};
status last_status{};
std::uint16_t last_sample{};
std::span<std::uint16_t> last_block{};
void (*edge_callback)() noexcept {};
std::uintptr_t capture_task{};
std::uint32_t trigger_source{};
adc_trigger_edge trigger_edge{};

struct binding
{
  static constexpr std::array<nrfx_saadc_channel_t, 2U> channels{
      {{{NRF_SAADC_MODE_SINGLE_ENDED}, 0U}, {{NRF_SAADC_MODE_SINGLE_ENDED}, 2U}}};
  static constexpr std::uint32_t output_pin = 10U;
  static constexpr std::uint32_t sck_pin = 11U, lrck_pin = 12U, sdout_pin = 13U, sdin_pin = 14U;
  static status saadc_trigger_connect(std::uintptr_t task, std::uint32_t source, adc_trigger_edge edge) noexcept
  {
    assert(task == 0x40007004U);
    trigger_source = source;
    trigger_edge = edge;
    return status::ok;
  }
  static void saadc_trigger_start() noexcept
  {
    ++trigger_starts;
  }
  static void saadc_trigger_stop() noexcept
  {
    ++trigger_stops;
  }
  static void saadc_trigger_disconnect() noexcept
  {
    ++disconnects;
  }
  static status capture_connect(std::uint8_t channel, std::uintptr_t task, void (*callback)() noexcept) noexcept
  {
    assert(channel == 1U);
    capture_task = task;
    edge_callback = callback;
    return status::ok;
  }
  static void capture_disconnect(std::uint8_t) noexcept
  {
    edge_callback = nullptr;
  }
};
struct config
{
  using binding = ::binding;
  static constexpr std::uint8_t instance_index = 0U, timer_index = 0U, channel = 1U;
  static constexpr adc_resolution resolution = adc_resolution::bits_12;
  static constexpr bool differential_mode = false, enable_dead_time = false, enable_master_clock = false;
  static constexpr bool enable_dma = false;
  static constexpr pwm_polarity polarity = pwm_polarity::active_high;
  static constexpr std::uint32_t frequency_hz = 1000U, sample_rate = 48000U;
  static constexpr std::uint32_t timeout_ms = 0U, period = 1000U, prescaler = 0U, clock_hz = 1'000'000U;
  static constexpr timer_counter_mode counter_mode = timer_counter_mode::up;
  static constexpr timer_clock_source clock_source = timer_clock_source::internal;
  static constexpr i2s_mode mode = i2s_mode::master_tx;
  static constexpr i2s_standard standard = i2s_standard::philips;
  static constexpr transfer_mode transfer_mode_value = transfer_mode::dma;
};
struct binding2 : binding
{
  static constexpr std::uint32_t output_pin = 17U;
};
struct config2 : config
{
  using binding = binding2;
  static constexpr std::uint8_t channel = 2U;
};
struct incompatible : config2
{
  static constexpr std::uint32_t frequency_hz = 2000U;
};
struct differential_binding : binding
{
  static constexpr std::array<nrfx_saadc_channel_t, 1U> channels{{{{NRF_SAADC_MODE_DIFFERENTIAL}, 0U}}};
};
struct differential_config : config
{
  using binding = differential_binding;
  static constexpr bool differential_mode = true;
};
struct internal_binding : binding
{
  static constexpr std::uint16_t saadc_internal_timer_cc = 1000U;
};
struct internal_config : config
{
  using binding = internal_binding;
};
struct polled_config : config
{
  static constexpr transfer_mode transfer_mode_value = transfer_mode::polled;
};
struct interrupt_config : config
{
  static constexpr transfer_mode transfer_mode_value = transfer_mode::interrupt;
};
struct external_config : config
{
  static constexpr std::uint32_t external_trigger = 7U;
  static constexpr adc_trigger_edge trigger_edge = adc_trigger_edge::rising;
};
struct rx_config : config
{
  static constexpr i2s_mode mode = i2s_mode::master_rx;
};
struct capture
{
  static constexpr std::uint8_t channel_index = 1U;
  static constexpr bool enable_capture = true, enable_compare = false;
};
struct compare
{
  static constexpr std::uint8_t channel_index = 2U;
  static constexpr bool enable_capture = false, enable_compare = true;
  static constexpr std::uint32_t compare_ticks = 200U;
};
constexpr auto completion = [](status value) noexcept
{
  ++notifications;
  last_status = value;
};
constexpr auto sample_completion = [](status value, std::uint16_t sample) noexcept
{
  ++notifications;
  last_status = value;
  last_sample = sample;
};
constexpr auto block_completion = [](status value, std::span<std::uint16_t> block) noexcept
{
  ++blocks;
  last_status = value;
  last_block = block;
};
constexpr auto audio_completion =
    [](status value, std::span<const std::uint16_t> tx, std::span<std::uint16_t> rx) noexcept
{
  ++audio_blocks;
  last_status = value;
  assert(value != status::ok || (tx.size() == 8U && rx.size() == 8U));
};

void adc_event(nrfx_saadc_evt_type_t type, std::uint16_t* buffer = nullptr, std::uint16_t size = 0U)
{
  nrfx_saadc_evt_t event{};
  event.type = type;
  event.data.done.p_buffer = reinterpret_cast<nrf_saadc_value_t*>(buffer);
  event.data.done.size = size;
  fake_saadc_callback(&event);
}
void adc_test()
{
  assert(adc_api::initialize<config>() == status::ok);
  assert((adc_api::start_async<config, completion>(1U) == status::invalid_argument));
  assert((adc_api::start_async<config, sample_completion>(0U) == status::ok));
  state::saadc_sample = -2;
  adc_event(NRFX_SAADC_EVT_DONE, reinterpret_cast<std::uint16_t*>(&state::saadc_sample), 1U);
  assert(notifications == 1U && last_sample == 0U && last_status == status::ok);
  adc_event(NRFX_SAADC_EVT_DONE, reinterpret_cast<std::uint16_t*>(&state::saadc_sample), 1U);
  assert(notifications == 1U);
  assert((adc_api::start_async<config, sample_completion>(0U) == status::ok));
  assert(adc_api::abort<config>() == status::ok);
  assert(notifications == 2U && last_status == status::error && fake_saadc_uninits == 1U);
  adc_event(NRFX_SAADC_EVT_DONE, reinterpret_cast<std::uint16_t*>(&state::saadc_sample), 1U);
  assert(notifications == 2U);
  assert(adc_api::read_blocking<config>(0U).error() == status::timeout);
  assert(adc_api::transfer_status<config>().value() == dma_transfer_state::error);

  constexpr std::array<std::uint8_t, 2U> channels{0U, 2U};
  constexpr std::array<std::uint8_t, 2U> descending{2U, 0U};
  std::array<std::uint16_t, 8U> samples{};
  assert((adc_api::start_dma_stream<config, block_completion>(descending, samples) == status::invalid_argument));
  assert((adc_api::start_dma_stream<config, block_completion>(channels, samples, false) == status::ok));
  assert(adc_api::transfer_status<config>().value() == dma_transfer_state::active);
  adc_event(NRFX_SAADC_EVT_READY);
  assert(trigger_starts == 1U);
  samples[0] = 0xFFFFU;
  adc_event(NRFX_SAADC_EVT_DONE, samples.data(), 4U);
  assert(blocks == 1U && last_block.data() == samples.data() && samples[0] == 0U);
  adc_event(NRFX_SAADC_EVT_BUF_REQ);
  adc_event(NRFX_SAADC_EVT_DONE, samples.data() + 4U, 4U);
  adc_event(NRFX_SAADC_EVT_FINISHED);
  assert(blocks == 2U && trigger_stops == 1U && disconnects == 1U);
  assert(adc_api::transfer_status<config>().value() == dma_transfer_state::complete);
  adc_event(NRFX_SAADC_EVT_DONE, samples.data(), 4U);
  assert(blocks == 2U);
  assert((adc_api::start_dma_stream<config, block_completion>(channels, samples) == status::ok));
  adc_event(NRFX_SAADC_EVT_DONE, samples.data(), 4U);
  adc_event(NRFX_SAADC_EVT_BUF_REQ);
  assert(fake_saadc_buffer == reinterpret_cast<nrf_saadc_value_t*>(samples.data()));
  adc_event(NRFX_SAADC_EVT_DONE, samples.data() + 4U, 4U);
  adc_event(NRFX_SAADC_EVT_BUF_REQ);
  assert(fake_saadc_buffer == reinterpret_cast<nrf_saadc_value_t*>(samples.data() + 4U));
  fake_saadc_error = -EIO;
  adc_event(NRFX_SAADC_EVT_BUF_REQ);
  assert(last_status == status::io_error && last_block.empty());
  fake_saadc_error = 0;
  assert((adc_api::start_async<config, completion>(2U) == status::ok));
  assert(adc_api::stop<config>() == status::ok);
  assert(adc_api::start_continuous<config>(channels) == status::ok);
  state::saadc_continuous_storage[0] = 123U;
  state::saadc_continuous_storage[1] = 456U;
  adc_event(NRFX_SAADC_EVT_DONE, state::saadc_continuous_storage.data(), 2U);
  assert(adc_api::read_blocking<config>(0U).value() == 123U);
  assert(adc_api::read_blocking<config>(2U).value() == 456U);
  assert(adc_api::read_blocking<config>(1U).error() == status::invalid_argument);
  assert(adc_api::stop<config>() == status::ok);
  constexpr auto cancel_from_block = [](status, std::span<std::uint16_t>) noexcept
  {
    assert(adc_api::abort<config>() == status::ok);
    assert((adc_api::start_async<config, completion>(0U) == status::busy));
  };
  assert((adc_api::start_dma_stream<config, cancel_from_block>(channels, samples) == status::ok));
  adc_event(NRFX_SAADC_EVT_DONE, samples.data(), 4U);
  // The actual advanced-mode driver updates its state after returning from DONE.
  fake_saadc_initialized = true;
  assert((adc_api::start_async<config, completion>(0U) == status::ok));
  assert(adc_api::stop<config>() == status::ok);
  assert(adc_api::initialize<differential_config>() == status::ok);
  assert((adc_api::start_async<differential_config, sample_completion>(0U) == status::ok));
  state::saadc_sample = -2048;
  adc_event(NRFX_SAADC_EVT_DONE, reinterpret_cast<std::uint16_t*>(&state::saadc_sample), 1U);
  assert(last_sample == 0U);
  assert((adc_api::start_async<differential_config, sample_completion>(0U) == status::ok));
  state::saadc_sample = 0;
  adc_event(NRFX_SAADC_EVT_DONE, reinterpret_cast<std::uint16_t*>(&state::saadc_sample), 1U);
  assert(last_sample == 2048U);
  assert((adc_api::start_async<differential_config, sample_completion>(0U) == status::ok));
  state::saadc_sample = 2047;
  adc_event(NRFX_SAADC_EVT_DONE, reinterpret_cast<std::uint16_t*>(&state::saadc_sample), 1U);
  assert(last_sample == 4095U);
  fake_saadc_complete_on_trigger = true;
  for (const std::int16_t sample : std::array<std::int16_t, 3U>{0, -2048, 2047})
  {
    fake_saadc_trigger_sample = sample;
    const auto signed_sample = adc_api::read_differential_blocking<differential_config>(0U);
    assert(signed_sample && signed_sample.value() == sample);
  }
  assert(adc_api::read_differential_blocking<differential_config>(1U).error() == status::invalid_argument);
  assert(adc_api::read_differential_blocking<config>(0U).error() == status::unsupported);
  fake_saadc_error = -EIO;
  assert(adc_api::read_differential_blocking<differential_config>(0U).error() == status::io_error);
  fake_saadc_error = 0;
  fake_saadc_complete_on_trigger = false;
  assert(adc_api::read_differential_blocking<differential_config>(0U).error() == status::timeout);
  assert((adc_api::start_async<differential_config, completion>(0U) == status::ok));
  assert(adc_api::read_differential_blocking<differential_config>(0U).error() == status::busy);
  assert(adc_api::stop<differential_config>() == status::ok);
  assert(adc_api::initialize<internal_config>() == status::ok);
  assert((adc_api::start_dma_stream<internal_config, block_completion>(channels, samples) == status::invalid_argument));
  constexpr std::array<std::uint8_t, 1U> single_channel{0U};
  assert((adc_api::start_dma_stream<internal_config, block_completion>(single_channel, samples) == status::ok));
  assert(adc_api::stop<internal_config>() == status::ok);
  assert((adc_api::start_dma_stream<external_config, block_completion>(channels, samples) == status::ok));
  assert(trigger_source == 7U && trigger_edge == adc_trigger_edge::rising);
  assert(adc_api::stop<external_config>() == status::ok);
  assert((adc_api::start_async<external_config, completion>(0U) == status::unsupported));
}

void timer_test()
{
  assert(timer_api::initialize<config>() == status::ok);
  assert((timer_api::enable_capture_compare<config, capture, completion>() == status::ok));
  assert(capture_task == 0x40000004U && edge_callback != nullptr);
  fake_timers[0].cc[1] = 321U;
  edge_callback();
  assert((timer_api::capture_value<config, capture>().value() == 321U));
  const auto callbacks_before = notifications;
  state::timer_handler(NRF_TIMER_EVENT_COMPARE1, &state::timer_states[0]);
  assert(notifications == callbacks_before);
  assert((timer_api::enable_capture_compare<config, compare, completion>() == status::ok));
  assert(fake_timers[0].cc[2] == 200U);
  assert(timer_api::set_period<config>(100U) == status::invalid_argument);
  assert(timer_api::set_period<config>(500U) == status::ok && fake_timers[0].cc[2] == 200U);
  assert(timer_api::counter<config>() == 73U);
  assert((timer_api::disable_capture_compare<config, capture>() == status::ok && edge_callback == nullptr));
}

void pwm_test()
{
  assert(pwm_api::initialize<config>() == status::ok);
  assert(pwm_api::initialize<incompatible>() == status::invalid_argument);
  assert(pwm_api::initialize<config2>() == status::ok);
  assert(pwm_api::set_duty_cycle<config>(250U) == status::ok);
  assert(pwm_api::set_duty_cycle<config2>(750U) == status::ok);
  assert(pwm_api::start<config>() == status::ok);
  assert(state::pwm_states[0].values.channel_1 == 0x8000U);
  assert(pwm_api::start<config2>() == status::ok);
  assert(fake_pwm_starts == 1U && fake_pwm_stops == 0U);
  assert(pwm_api::set_duty_cycle<config>(500U) == status::ok);
  assert(fake_pwm_starts == 1U && fake_pwm_stops == 0U);
  assert((state::pwm_states[0].values.channel_1 & 0x7FFFU) == 12'000U);
  assert(pwm_api::set_frequency<config>(2000U) == status::busy);
  assert(pwm_api::stop<config>() == status::ok && fake_pwm_stops == 0U);
  assert(state::pwm_states[0].values.channel_0 == 0x8000U);
  fake_pwm_can_stop = false;
  assert(pwm_api::stop<config2>() == status::timeout);
  assert(state::pwm_states[0].running && state::pwm_states[0].enabled == 2U);
  fake_pwm_can_stop = true;
  assert(pwm_api::stop<config2>() == status::ok);
  assert(pwm_api::set_frequency<config>(2000U) == status::ok);
  assert(pwm_api::start<config2>() == status::ok);
  assert((state::pwm_states[0].values.channel_1 & 0x7FFFU) == 6000U);
  assert(pwm_api::stop<config2>() == status::ok);
}

void i2s_test()
{
  using modes = serial_mode_api<nordic_nrf_tag, peripheral_kind::i2s>;
  alignas(4) std::array<std::uint16_t, 16U> tx{}, rx{};
  assert(i2s_api::initialize<config>() == status::ok);
  assert((i2s_api::start_dma_transfer<config, completion>(tx, rx) == status::ok));
  nrfx_i2s_buffers_t empty{};
  fake_i2s_callback(&empty, NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
  assert(fake_i2s_next.p_tx_buffer != fake_i2s_first.p_tx_buffer);
  const auto before = notifications;
  fake_i2s_callback(&fake_i2s_first, NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
  fake_i2s_callback(&fake_i2s_first, 0U);
  assert(notifications == before);
  fake_i2s_callback(&empty, NRFX_I2S_STATUS_TRANSFER_STOPPED | NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
  assert(notifications == before + 1U && last_status == status::ok);
  fake_i2s_callback(nullptr, NRFX_I2S_STATUS_TRANSFER_STOPPED);
  assert(notifications == before + 1U);
  assert((i2s_api::start_dma_stream<config, audio_completion>(tx, rx, false) == status::ok));
  const auto first = fake_i2s_first;
  fake_i2s_callback(&empty, NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
  const auto second = fake_i2s_next;
  fake_i2s_callback(&first, NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
  assert(audio_blocks == 1U);
  fake_i2s_callback(&second, NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
  assert(audio_blocks == 2U);
  fake_i2s_callback(&empty, NRFX_I2S_STATUS_TRANSFER_STOPPED);
  assert(i2s_api::transfer_status<config>().value() == dma_transfer_state::complete);
  assert((i2s_api::start_dma_stream<config, audio_completion>(tx, rx) == status::ok));
  fake_i2s_callback(nullptr, NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
  fake_i2s_callback(&empty, NRFX_I2S_STATUS_TRANSFER_STOPPED);
  assert(last_status == status::io_error && audio_blocks == 3U);
  assert((i2s_api::start_dma_transfer<config, completion>(tx, rx) == status::ok));
  assert(i2s_api::abort<config>() == status::ok && last_status == status::error);
  const auto after_abort = notifications;
  fake_i2s_callback(&first, NRFX_I2S_STATUS_TRANSFER_STOPPED);
  assert(notifications == after_abort);
  assert((i2s_api::start_dma_transfer<config, completion>(tx, rx) == status::ok));
  fake_i2s_callback(&first, NRFX_I2S_STATUS_TRANSFER_STOPPED);
  assert(i2s_api::transfer_status<config>().value() == dma_transfer_state::active);
  assert(i2s_api::stop<config>() == status::ok);
  assert(i2s_api::transmit_blocking<config>(tx) == status::timeout);
  assert((i2s_api::start_dma_stream<config, audio_completion>(tx, rx) == status::ok));
  assert(modes::transfer_status<config>() == status::busy);
  fake_i2s_callback(&empty, NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
  assert(modes::abort<config>() == status::ok);
  assert(modes::transfer_status<config>() == status::error);
  assert((modes::transmit<config, completion>(tx) == status::ok));
  assert(i2s_api::stop<config>() == status::ok);
  assert(modes::initialize<interrupt_config>() == status::unsupported);
  assert((modes::transmit<interrupt_config, completion>(tx) == status::unsupported));
  assert((modes::transmit<polled_config, completion>(tx) == status::unsupported));
  assert(modes::initialize<polled_config>() == status::ok);
  assert((modes::transfer<polled_config, nullptr>(tx, rx) == status::timeout));
  assert(i2s_api::initialize<rx_config>() == status::ok);
  assert((i2s_api::start_dma_rx<rx_config, completion>(rx) == status::ok));
  assert(fake_i2s_first.p_tx_buffer == nullptr && fake_i2s_first.p_rx_buffer != nullptr);
  fake_i2s_callback(&empty, NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
  fake_i2s_callback(&fake_i2s_first, NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
  fake_i2s_callback(&empty, NRFX_I2S_STATUS_TRANSFER_STOPPED);
  assert(last_status == status::ok);
  assert((i2s_api::start_dma_stream<rx_config, audio_completion>(tx, rx) == status::ok));
  fake_i2s_callback(&empty, NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
  const auto circular_first = fake_i2s_first;
  const auto circular_second = fake_i2s_next;
  const auto audio_before = audio_blocks;
  fake_i2s_callback(&circular_first, NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
  fake_i2s_callback(&circular_first, NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
  fake_i2s_callback(&circular_second, NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
  assert(audio_blocks == audio_before + 2U);
  assert(i2s_api::stop<rx_config>() == status::ok);
  alignas(4) std::array<std::uint16_t, 16U> new_tx{}, new_rx{};
  assert((i2s_api::start_dma_stream<config, audio_completion>(new_tx, new_rx) == status::ok));
  fake_i2s_callback(&circular_first, NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
  fake_i2s_callback(&empty, NRFX_I2S_STATUS_TRANSFER_STOPPED);
  assert(audio_blocks == audio_before + 2U);
  assert(i2s_api::transfer_status<config>().value() == dma_transfer_state::active);
  assert(i2s_api::stop<config>() == status::ok);
}

struct public_auto_config : config
{
  static constexpr bool enable_dma = true;
  static constexpr transfer_mode transfer_mode_value = transfer_mode::automatic;
};
struct public_polled_config : polled_config
{
  static constexpr std::uint32_t timeout_ms = 1U;
};
struct public_auto_rx_config : public_auto_config
{
  static constexpr i2s_mode mode = i2s_mode::master_rx;
};

void public_i2s_test()
{
  using automatic = i2s::interface<nordic_nrf_tag, public_auto_config>;
  using dma = i2s::interface<nordic_nrf_tag, i2s::bound_i2s_config<binding, i2s_mode::master_tx, i2s_standard::philips,
                                                                   48000U, false, true, transfer_mode::dma>>;
  using polled = i2s::interface<nordic_nrf_tag, public_polled_config>;
  using automatic_rx = i2s::interface<nordic_nrf_tag, public_auto_rx_config>;
  using dma_rx =
      i2s::interface<nordic_nrf_tag, i2s::bound_i2s_config<binding, i2s_mode::master_rx, i2s_standard::philips, 48000U,
                                                           false, true, transfer_mode::dma>>;
  alignas(4) std::array<std::uint16_t, 16U> tx{}, rx{};
  const nrfx_i2s_buffers_t empty{};

  assert(automatic::initialize() == status::ok);
  assert((automatic::start_dma_transfer<completion>(tx, std::span{rx}.first(8U)) == status::invalid_argument));
  assert((automatic::start_dma_transfer<completion>(tx, rx) == status::ok));
  assert(fake_i2s_first.p_tx_buffer == reinterpret_cast<const std::uint32_t*>(tx.data()));
  assert(fake_i2s_first.p_rx_buffer == reinterpret_cast<std::uint32_t*>(rx.data()));
  assert(automatic::transfer_status() == status::busy);
  assert(automatic::handle_interrupt() == status::busy);
  assert(automatic::handle_dma_interrupt() == status::busy);
  const auto callbacks_before = notifications;
  fake_i2s_callback(&empty, NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
  fake_i2s_callback(&fake_i2s_first, NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
  fake_i2s_callback(&empty, NRFX_I2S_STATUS_TRANSFER_STOPPED);
  assert(notifications == callbacks_before + 1U && last_status == status::ok);
  assert(automatic::transfer_status() == status::ok);

  assert((automatic::start_dma_stream<audio_completion>(tx, rx, false) == status::ok));
  const auto first = fake_i2s_first;
  fake_i2s_callback(&empty, NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
  const auto second = fake_i2s_next;
  const auto blocks_before = audio_blocks;
  fake_i2s_callback(&first, NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
  fake_i2s_callback(&second, NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
  fake_i2s_callback(&empty, NRFX_I2S_STATUS_TRANSFER_STOPPED);
  assert(audio_blocks == blocks_before + 2U && automatic::transfer_status() == status::ok);
  assert((automatic::start_dma_stream<audio_completion>(tx, rx) == status::ok));
  assert(automatic::stop() == status::ok);
  assert(audio_blocks == blocks_before + 2U && automatic::transfer_status() == status::ok);
  assert((automatic::start_dma_transfer<completion>(tx, rx) == status::ok));
  assert(automatic::abort() == status::ok && last_status == status::error);
  assert(automatic::transfer_status() == status::error);
  const auto after_abort = notifications;
  fake_i2s_callback(&first, NRFX_I2S_STATUS_TRANSFER_STOPPED);
  assert(notifications == after_abort);
  assert(automatic::transfer_blocking(tx, rx) == status::timeout);

  assert(dma::initialize() == status::ok);
  assert((dma::start_dma_transfer<completion>(tx, rx) == status::ok));
  assert(dma::transfer_status() == status::busy);
  assert(dma::stop() == status::ok);
  assert((dma::start_dma_stream<audio_completion>(tx, rx) == status::ok));
  assert(dma::abort() == status::ok && last_status == status::error);
  assert(dma::transfer_status() == status::error);
  assert((dma::start_dma_tx<completion>(tx) == status::ok));
  assert(fake_i2s_first.p_rx_buffer == nullptr);
  assert(dma::stop() == status::ok);

  assert(polled::initialize() == status::ok);
  assert((polled::start_dma_transfer<completion>(tx, rx) == status::unsupported));
  assert((polled::start_dma_stream<audio_completion>(tx, rx) == status::unsupported));
  assert((polled::transmit_async<completion>(tx) == status::unsupported));
  fake_i2s_complete_on_irq = true;
  assert(polled::transfer_blocking(tx, rx) == status::ok);
  assert(fake_i2s_irq_phase == 3U && polled::transfer_status() == status::ok);
  assert(polled::transmit_blocking(tx) == status::ok);
  fake_i2s_complete_on_irq = false;
  assert(polled::transfer_blocking(tx, rx) == status::timeout);
  assert(automatic_rx::initialize() == status::ok);
  assert((automatic_rx::start_dma_rx<completion>(rx) == status::ok));
  assert(fake_i2s_first.p_tx_buffer == nullptr &&
         fake_i2s_first.p_rx_buffer == reinterpret_cast<std::uint32_t*>(rx.data()));
  assert(automatic_rx::stop() == status::ok);
  assert(dma_rx::initialize() == status::ok);
  assert((dma_rx::start_dma_rx<completion>(rx) == status::ok));
  assert(dma_rx::abort() == status::ok && dma_rx::transfer_status() == status::error);
}

struct high_channel_binding : binding
{
  static status capture_connect(std::uint8_t channel, std::uintptr_t task, void (*callback)() noexcept) noexcept
  {
    assert(channel == 4U && task == 0x40000010U);
    capture_task = task;
    edge_callback = callback;
    return status::ok;
  }
};
struct public_timer_config : config
{
  using binding = high_channel_binding;
  static constexpr std::uint8_t instance_index = 3U;
  static constexpr bool enable_interrupts = true;
  static constexpr transfer_mode transfer_mode_value = transfer_mode::automatic;
};
struct public_pwm_binding : binding
{
  static constexpr std::uint32_t output_pin = 18U;
};
struct public_pwm_config : config
{
  using binding = public_pwm_binding;
  static constexpr std::uint8_t timer_index = 1U;
};

void public_waveform_test()
{
  using timer = timers::timer<nordic_nrf_tag, public_timer_config>;
  using capture_channel = timers::channel_config<4U, true, false>;
  using compare_channel = timers::channel_config<4U, false, true>;
  assert(timer::initialize() == status::ok);
  assert((timer::enable_capture_compare<capture_channel, completion>() == status::ok));
  fake_timers[3].cc[4] = 777U;
  const auto before = notifications;
  edge_callback();
  assert(notifications == before + 1U && timer::capture_value<capture_channel>().value() == 777U);
  assert(timer::disable_capture_compare<capture_channel>() == status::ok && edge_callback == nullptr);
  assert(timer::capture_value<capture_channel>().error() == status::unsupported);
  assert((timer::enable_capture_compare<compare_channel, completion>() == status::ok));
  assert(fake_timers[3].cc[4] == 1000U);
  assert(timer::handle_interrupt() == status::ok);
  assert(timer::disable_capture_compare<compare_channel>() == status::ok);

  using output = pwm::output<nordic_nrf_tag, public_pwm_config>;
  assert(output::initialize() == status::ok);
  assert(output::set_frequency(2000U) == status::ok);
  assert(output::set_duty_cycle(250U) == status::ok);
  assert(output::start() == status::ok);
  assert((state::pwm_states[1].values.channel_0 & 0x7FFFU) == 2000U);
  assert(output::set_frequency(1000U) == status::busy);
  assert(output::stop() == status::ok);
  assert(output::set_frequency(1000U) == status::ok);
  assert(output::start() == status::ok);
  assert((state::pwm_states[1].values.channel_0 & 0x7FFFU) == 4000U);
  assert(output::stop() == status::ok);
}
} // namespace

int main()
{
  adc_test();
  timer_test();
  pwm_test();
  i2s_test();
  public_i2s_test();
  public_waveform_test();
}
