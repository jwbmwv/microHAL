// SPDX-License-Identifier: Apache-2.0
// Compile-only: instantiate driver bodies, not just requires expressions.
#include "microhal/adc/adc.hpp"
#include "microhal/backends/nordic/nrfx/traits.hpp"
#include "microhal/i2c/i2c.hpp"
#include "microhal/i2s/i2s.hpp"
#include "microhal/pwm/pwm.hpp"
#include "microhal/qspi/qspi.hpp"
#include "microhal/rtc/rtc.hpp"
#include "microhal/spi/spi.hpp"
#include "microhal/timers/timer.hpp"
#include "microhal/uart/uart.hpp"
#include "microhal/watchdog/watchdog.hpp"

#if MICROHAL_NORDIC_NATIVE_TARGET
namespace nordic_validation
{
using namespace microhal;
using namespace microhal::backends::nordic::nrfx;

struct binding
{
  static constexpr std::uint32_t sck_pin{4U}, mosi_pin{5U}, miso_pin{6U}, ss_pin{7U};
  static constexpr std::uint32_t tx_pin{8U}, rx_pin{9U}, output_pin{10U};
  static constexpr std::uint32_t scl_pin{11U}, sda_pin{12U};
  static constexpr std::uint32_t lrck_pin{13U}, sdout_pin{14U}, sdin_pin{15U}, mck_pin{16U};
#if MICROHAL_NORDIC_HAS_QSPI
  static constexpr nrfx_qspi_config_t configuration = []
  {
    nrfx_qspi_config_t value{};
    value.pins = {4U, 5U, 6U, 7U, 8U, 9U};
    value.prot_if.readoc = NRF_QSPI_READOC_READ4O;
    value.prot_if.writeoc = NRF_QSPI_WRITEOC_PP4O;
    value.prot_if.addrmode = NRF_QSPI_ADDRMODE_24BIT;
#if defined(NRF5340_XXAA_APPLICATION)
    value.phy_if.sck_freq = NRF_QSPI_FREQ_DIV3;
#else
    value.phy_if.sck_freq = NRF_QSPI_FREQ_DIV1;
#endif
    return value;
  }();
#endif
#if MICROHAL_NORDIC_HAS_SAADC
  static constexpr std::array<nrfx_saadc_channel_t, 1U> channels{
      {[]
       {
         nrfx_saadc_channel_t value{};
         value.pin_p = NRFX_ANALOG_EXTERNAL_AIN0;
         value.pin_n = NRFX_ANALOG_INPUT_DISABLED;
         value.channel_config.mode = NRF_SAADC_MODE_SINGLE_ENDED;
         value.channel_config.reference = NRF_SAADC_REFERENCE_INTERNAL;
         value.channel_config.acq_time = NRFX_SAADC_DEFAULT_ACQTIME;
#if NRF_SAADC_HAS_CH_GAIN
         value.channel_config.gain = NRF_SAADC_GAIN1;
#endif
#if NRF_SAADC_HAS_CONVTIME
         value.channel_config.conv_time = NRFX_SAADC_DEFAULT_CONV_TIME;
#endif
         return value;
       }()}};
#endif
};
struct config
{
  using binding = nordic_validation::binding;
  static constexpr std::uint8_t instance_index{0U}, timer_index{0U}, channel{1U};
  static constexpr std::uint32_t clock_hz{1'000'000U}, frequency_hz{1000U}, baud_rate{115200U};
  static constexpr std::uint32_t timeout_ms{10U}, prescaler{0U}, period{1000U};
  static constexpr spi_duplex_mode duplex_mode{spi_duplex_mode::full};
  static constexpr spi_clock_polarity clock_polarity{spi_clock_polarity::idle_low};
  static constexpr spi_clock_phase clock_phase{spi_clock_phase::first_edge};
  static constexpr spi_bit_order bit_order{spi_bit_order::msb_first};
  static constexpr uart_parity parity{uart_parity::none};
  static constexpr uart_stop_bits stop_bits{uart_stop_bits::one};
  static constexpr timer_counter_mode counter_mode{timer_counter_mode::up};
  static constexpr timer_clock_source clock_source{timer_clock_source::internal};
  static constexpr adc_resolution resolution{adc_resolution::bits_12};
  static constexpr pwm_polarity polarity{pwm_polarity::active_high};
  static constexpr bool enable_dma{false}, differential_mode{false}, enable_dead_time{false};
  static constexpr bool window_mode{false}, independent_mode{true};
  static constexpr i2s_mode mode{i2s_mode::master_tx};
  static constexpr i2s_standard standard{i2s_standard::philips};
  static constexpr std::uint32_t sample_rate{48000U};
  static constexpr bool enable_master_clock{true};
  static constexpr bool enable_interrupts{true}, require_fifo{false}, require_lpuart{false};
  static constexpr adc_sample_time sample_time{adc_sample_time::cycles_15};
  static constexpr bool enable_periodic_irq{false}, enable_alarm_irq{true};
  static constexpr i2c_address_mode address_mode{i2c_address_mode::seven_bit};
};
constexpr auto callback = [](status) noexcept {};
struct qspi_config
{
  using binding = nordic_validation::binding;
  static constexpr std::uint32_t clock_hz{32'000'000U};
  static constexpr std::uint32_t timeout_ms{10U};
  static constexpr bool enable_dma{false}, enable_interrupts{false};
};
struct pin_config
{
  static constexpr std::uint8_t port_index{0U}, pin_index{0U};
  static constexpr gpio_mode mode{gpio_mode::input};
  static constexpr gpio_pull pull{gpio_pull::up};
  static constexpr gpio_interrupt_trigger interrupt_trigger{gpio_interrupt_trigger::both};
};

static_assert(backend_traits<nordic_nrf_tag>::spi_capability().supported);
static_assert(backend_traits<nordic_nrf_tag>::uart_capability().supported);
static_assert(backend_traits<nordic_nrf_tag>::timer_capability().supported);
static_assert(backend_traits<nordic_nrf_tag>::adc_capability().supported);
static_assert(backend_traits<nordic_nrf_tag>::pwm_capability().supported);
static_assert(backend_traits<nordic_nrf_tag>::watchdog_capability().supported);
static_assert(backend_traits<nordic_nrf_tag>::adc_capability().dma);
#if MICROHAL_NORDIC_HAS_I2S
static_assert(backend_traits<nordic_nrf_tag>::i2s_capability().transmit_dma);
static_assert(backend_traits<nordic_nrf_tag>::i2s_capability().receive_dma);
static_assert(backend_traits<nordic_nrf_tag>::i2s_capability().full_duplex);
#else
static_assert(!backend_traits<nordic_nrf_tag>::i2s_capability().supported);
#endif
static_assert(backend_traits<nordic_nrf_tag>::gpio_capability().isr);
#if defined(NRF54H20_XXAA)
static_assert(backend_traits<nordic_nrf_tag>::i2c_capability().instance_count == 8U);
#elif defined(NRF5340_XXAA_APPLICATION) || defined(NRF54L15_XXAA)
static_assert(backend_traits<nordic_nrf_tag>::i2c_capability().instance_count == 4U);
#else
static_assert(backend_traits<nordic_nrf_tag>::i2c_capability().instance_count == 2U);
#endif
#if defined(NRF54L15_XXAA) || defined(NRF54H20_XXAA)
static_assert(!backend_traits<nordic_nrf_tag>::qspi_capability().supported);
#if defined(NRF54L15_XXAA)
static_assert(backend_traits<nordic_nrf_tag>::gpio_port_count == 3U);
static_assert(backend_traits<nordic_nrf_tag>::spi_capability().instance_count == 5U);
#endif
#else
static_assert(backend_traits<nordic_nrf_tag>::qspi_capability().supported);
#endif
static_assert(backend_traits<nordic_nrf_tag>::rtc_capability().supported);

void instantiate(std::span<std::uint8_t> bytes)
{
  (void)gpio_api::enable_interrupt<pin_config, callback>();
  gpio_api::handle_interrupt();
  (void)i2c_api::initialize<config>();
  (void)i2c_api::start_transfer<config, callback>(bytes, 0x50U);
  (void)i2c_api::probe<config>(0x50U);
  (void)i2c_api::write_read_blocking<config>(bytes, bytes, 0x50U);
  i2c_api::handle_interrupt(0U);
  (void)rtc_api::initialize<config>();
  (void)rtc_api::set_time<config>(12U, 30U, 0U);
  std::uint8_t hour{}, minute{}, second{};
  (void)rtc_api::get_time<config>(hour, minute, second);
  (void)rtc_api::enable_alarm<config, callback>(rtc::date_time{.hour = 12U, .minute = 31U});
  rtc_api::handle_interrupt(0U);
  (void)rcc_api::get_system_clock_hz<config>();
  (void)spi_api::initialize<config>();
  (void)spi_api::transfer<config>(bytes, bytes);
  (void)spi_api::transfer_async<config, callback>(bytes, bytes);
  (void)uart_api::initialize<config>();
  (void)uart_api::write_blocking<config>(bytes);
  (void)uart_api::read_blocking<config>(bytes);
  (void)uart_api::start_async_tx<config, callback>(bytes);
  (void)uart_api::start_async_rx<config, callback>(bytes);
  (void)timer_api::initialize<config>();
  (void)timer_api::start<config>();
  (void)timer_api::stop<config>();
  (void)timer_api::reset_counter<config>();
  (void)timer_api::counter<config>();
  (void)timer_api::period<config>();
  (void)timer_api::set_period<config>(100U);
  (void)timer_api::enable_capture_compare<config, timers::channel_config<1U, false, true>, callback>();
  (void)adc_api::initialize<config>();
  (void)adc_api::read_blocking<config>(0U);
  (void)adc_api::start_async<config, callback>(0U);
  std::array<std::uint8_t, 1U> channels{0U};
  std::array<std::uint16_t, 1U> samples{};
  (void)adc_api::start_dma_scan<config, callback>(channels, samples);
  (void)qspi_api::initialize<qspi_config>();
  (void)qspi_api::command_read<qspi_config>({}, bytes);
  (void)qspi_api::command_write<qspi_config>({}, bytes);
  (void)i2s_api::initialize<config>();
  (void)i2s_api::transmit_blocking<config>(samples);
  (void)i2s_api::receive_blocking<config>(samples);
  (void)i2s_api::start_dma_tx<config, callback>(samples);
  i2s_api::handle_interrupt();
  (void)pwm_api::initialize<config>();
  (void)pwm_api::set_duty_cycle<config>(500U);
  (void)pwm_api::start<config>();
  (void)pwm_api::stop<config>();
  (void)watchdog_api::initialize<config>();
  (void)watchdog_api::start<config>();
  (void)watchdog_api::refresh<config>();
  spi_api::handle_interrupt(0U);
  uart_api::handle_interrupt(0U);
  timer_api::handle_interrupt(0U);
  adc_api::handle_interrupt();

  using backend = nordic_nrf_tag;
  (void)spi::master<backend, config>::initialize();
  (void)spi::master<backend, config>::transfer(bytes, bytes);
  (void)uart::port<backend, config>::initialize();
  (void)uart::port<backend, config>::write_blocking(bytes);
  (void)timers::timer<backend, config>::initialize();
  (void)timers::timer<backend, config>::period();
  (void)timers::timer<backend, config>::handle_interrupt();
  (void)timers::timer<backend, config>::disable_capture_compare<timers::channel_config<1U, false, true>>();
  (void)adc::converter<backend, config>::initialize();
  (void)adc::converter<backend, config>::read_blocking(0U);
  (void)adc::converter<backend, config>::read_differential_blocking(0U);
  (void)adc::converter<backend, config>::read_differential_millivolts(0U, 600U);
  (void)adc::converter<backend, config>::start_dma_scan<callback>(channels, samples);
  (void)pwm::output<backend, config>::initialize();
  (void)pwm::output<backend, config>::set_frequency(2000U);
  (void)watchdog::timer<backend, config>::initialize();
#if MICROHAL_NORDIC_HAS_I2S
  using audio = i2s::interface<backend, config>;
  constexpr auto audio_block = [](status, std::span<const std::uint16_t>, std::span<std::uint16_t>) noexcept {};
  (void)audio::initialize();
  (void)audio::start_dma_tx<callback>(samples);
  (void)audio::start_dma_rx<callback>(samples);
  (void)audio::start_dma_transfer<callback>(samples, samples);
  (void)audio::start_dma_stream<audio_block>(samples, samples);
  (void)audio::transfer_blocking(samples, samples);
  (void)audio::stop();
  (void)audio::abort();
  (void)audio::transfer_status();
  (void)audio::handle_interrupt();
  (void)audio::handle_dma_interrupt();
#endif
#if MICROHAL_NORDIC_HAS_QSPI
  (void)qspi::controller<backend, qspi_config>::initialize();
#endif
#if MICROHAL_NORDIC_HAS_RTC || MICROHAL_NORDIC_HAS_GRTC
  (void)rtc::controller<backend, config>::initialize();
  (void)rtc::controller<backend, config>::read();
#endif
}
} // namespace nordic_validation
#endif
