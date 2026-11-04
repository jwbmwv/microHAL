// SPDX-License-Identifier: Apache-2.0
#include "microhal/backends/nordic/nrfx/acquisition_modes.hpp"
#include "microhal/backends/nordic/nrfx/traits.hpp"

namespace nordic_acquisition_validation
{
using namespace microhal;
using namespace microhal::backends::nordic::nrfx;

struct binding
{
  static constexpr std::uint32_t output_pin = 10U;
  static constexpr std::uint32_t sck_pin = 11U, lrck_pin = 12U, sdout_pin = 13U, sdin_pin = 14U;
  static constexpr std::uint16_t saadc_internal_timer_cc = 1000U;
#if MICROHAL_NORDIC_HAS_SAADC
  static constexpr std::array<nrfx_saadc_channel_t, 1U> channels{
      {[]
       {
         nrfx_saadc_channel_t channel{};
         channel.pin_p = NRFX_ANALOG_EXTERNAL_AIN0;
         channel.pin_n = NRFX_ANALOG_INPUT_DISABLED;
         channel.channel_config.mode = NRF_SAADC_MODE_SINGLE_ENDED;
         channel.channel_config.reference = NRF_SAADC_REFERENCE_INTERNAL;
         channel.channel_config.acq_time = NRFX_SAADC_DEFAULT_ACQTIME;
#if NRF_SAADC_HAS_CH_GAIN
         channel.channel_config.gain = NRF_SAADC_GAIN1;
#endif
#if NRF_SAADC_HAS_CONVTIME
         channel.channel_config.conv_time = NRFX_SAADC_DEFAULT_CONV_TIME;
#endif
         return channel;
       }()}};
#endif
  static status capture_connect(std::uint8_t, std::uintptr_t, void (*)() noexcept) noexcept
  {
    return status::ok;
  }
  static void capture_disconnect(std::uint8_t) noexcept {}
};
struct config
{
  using binding = nordic_acquisition_validation::binding;
  static constexpr std::uint8_t instance_index = 0U, timer_index = 0U, channel = 1U;
  static constexpr adc_resolution resolution = adc_resolution::bits_12;
  static constexpr bool differential_mode = false, enable_dead_time = false, enable_master_clock = false;
  static constexpr bool enable_dma = true;
  static constexpr pwm_polarity polarity = pwm_polarity::active_high;
  static constexpr std::uint32_t frequency_hz = 1000U, sample_rate = 48000U;
  static constexpr i2s_mode mode = i2s_mode::master_tx;
  static constexpr i2s_standard standard = i2s_standard::philips;
  static constexpr transfer_mode transfer_mode_value = transfer_mode::dma;
};
struct capture
{
  static constexpr std::uint8_t channel_index = 1U;
  static constexpr bool enable_capture = true, enable_compare = false;
};
struct differential_config : config
{
  static constexpr bool differential_mode = true;
};
constexpr auto callback = [](status) noexcept {};
constexpr auto sample_callback = [](status, std::uint16_t) noexcept {};
constexpr auto block_callback = [](status, std::span<std::uint16_t>) noexcept {};
constexpr auto audio_callback = [](status, std::span<const std::uint16_t>, std::span<std::uint16_t>) noexcept {};

void instantiate(std::span<std::uint16_t> samples)
{
  constexpr std::array<std::uint8_t, 1U> channels{0U};
  (void)adc_api::initialize<config>();
  (void)adc_api::start_async<config, sample_callback>(0U);
  (void)adc_api::read_differential_blocking<differential_config>(0U);
  (void)adc_api::start_dma_stream<config, block_callback>(channels, samples, true);
  (void)adc_api::start_continuous<config>(channels);
  (void)adc_api::stop<config>();
  (void)adc_api::abort<config>();
  (void)adc_api::transfer_status<config>();
  adc_api::handle_interrupt<config>();
  adc_api::handle_dma_interrupt<config>();
  (void)timer_api::enable_capture_compare<config, capture, callback>();
  (void)timer_api::capture_value<config, capture>();
  (void)timer_api::disable_capture_compare<config, capture>();
  (void)pwm_api::initialize<config>();
  (void)pwm_api::set_frequency<config>(1000U);
  (void)i2s_api::start_dma_rx<config, callback>(samples);
  (void)i2s_api::start_dma_transfer<config, callback>(samples, samples);
  (void)i2s_api::transfer_blocking<config>(samples, samples);
  (void)i2s_api::start_dma_stream<config, audio_callback>(samples, samples, false);
  (void)i2s_api::stop<config>();
  (void)i2s_api::abort<config>();
  (void)i2s_api::transfer_status<config>();
  using modes = serial_mode_api<nordic_nrf_tag, peripheral_kind::i2s>;
  (void)modes::initialize<config>();
  (void)modes::transmit<config, callback>(samples);
  (void)modes::receive<config, callback>(samples);
  (void)modes::transfer<config, callback>(samples, samples);
  (void)modes::transfer_status<config>();
  (void)modes::handle_interrupt<config>();
  (void)modes::abort<config>();
}
} // namespace nordic_acquisition_validation
