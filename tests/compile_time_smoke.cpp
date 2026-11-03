// SPDX-License-Identifier: Apache-2.0
/**
 * @file compile_time_smoke.cpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#include <array>

#include "microhal/dac/dac.hpp"
#include "microhal/dma/dma.hpp"
#include "microhal/ethernet/ethernet.hpp"
#include "microhal/flash/flash.hpp"
#include "microhal/gpio/gpio.hpp"
#include "microhal/i2c/i2c.hpp"
#include "microhal/pwm/pwm.hpp"
#include "microhal/qspi/qspi.hpp"
#include "microhal/rtc/rtc.hpp"
#include "microhal/sd/sd.hpp"
#include "microhal/spi/spi.hpp"
#include "microhal/timers/timer.hpp"
#include "microhal/uart/uart.hpp"
#include "microhal/uart/uart_autobaud.hpp"

using test_backend = microhal::stm32_hal_tag;
using test_backend_ll = microhal::stm32_ll_tag;

using led_pin_cfg = microhal::gpio::pin_config<0U, 5U, microhal::gpio_mode::output>;
using i2c_cfg = microhal::i2c::master_config<1U, 400000U, microhal::i2c_address_mode::seven_bit, true, true>;
using spi_cfg = microhal::spi::master_config<1U, 1000000U, microhal::spi_duplex_mode::full>;
using qspi_cfg = microhal::qspi::controller_config<0U, 80000000U, true, true>;
using dac_cfg = microhal::dac::dac_config<1U, 1U, microhal::dac_resolution::bits_12>;
using uart_cfg = microhal::uart::port_config<2U, 115200U, microhal::uart_parity::none, microhal::uart_stop_bits::one,
                                             true, true, true, false>;
using rtc_cfg = microhal::rtc::rtc_config<true, true>;
using flash_cfg = microhal::flash::flash_config<8U, 2048U, true, true>;
using timer_cfg = microhal::timers::timer_config<3U, 1000000U, microhal::timer_counter_mode::up,
                                                 microhal::timer_clock_source::internal, true, true>;
using timer_channel_cfg = microhal::timers::channel_config<1U, true, false>;
using dma_cfg = microhal::dma::dma_config<0U, 0U, 0U>;
using autobaud_cfg = microhal::uart::autobaud_config<led_pin_cfg, timer_cfg, 9600U, 1000000U, true, false>;

struct status_led_binding final
{
  static constexpr std::uint8_t port_index{2U};
  static constexpr std::uint8_t pin_index{13U};
  static constexpr std::uint8_t alternate_function{microhal::gpio_no_alternate};
  static constexpr std::uint32_t mux{1U};
};

struct status_led_cfg final
{
  using binding = status_led_binding;

  static constexpr microhal::gpio_mode mode{microhal::gpio_mode::output};
  static constexpr microhal::gpio_pull pull{microhal::gpio_pull::none};
  static constexpr microhal::gpio_speed speed{microhal::gpio_speed::low};
  static constexpr microhal::gpio_interrupt_trigger interrupt_trigger{microhal::gpio_interrupt_trigger::none};
};

void board_exti_handler() noexcept {}

struct interrupt_pin_binding final
{
  static constexpr std::uint8_t port_index{0U};
  static constexpr std::uint8_t pin_index{1U};

  template <auto Handler> static void enable_interrupt() noexcept
  {
    (void)Handler;
  }
};

struct interrupt_pin_cfg final
{
  using binding = interrupt_pin_binding;

  static constexpr microhal::gpio_mode mode{microhal::gpio_mode::input};
  static constexpr microhal::gpio_pull pull{microhal::gpio_pull::up};
  static constexpr microhal::gpio_speed speed{microhal::gpio_speed::low};
  static constexpr microhal::gpio_interrupt_trigger interrupt_trigger{microhal::gpio_interrupt_trigger::both};
};

struct console_uart_binding final
{
  static constexpr std::uintptr_t base_address{0x40011000U};
};

struct sensor_i2c_binding final
{
  static constexpr std::uintptr_t base_address{0x40005400U};
};

struct sensor_i2c_cfg final
{
  using binding = sensor_i2c_binding;

  static constexpr std::uint32_t clock_hz{400000U};
  static constexpr microhal::i2c_address_mode address_mode{microhal::i2c_address_mode::seven_bit};
  static constexpr bool enable_dma{true};
  static constexpr bool enable_interrupts{true};
};

struct sensor_spi_binding final
{
  static constexpr std::uintptr_t base_address{0x40013000U};
};

struct sensor_spi_cfg final
{
  using binding = sensor_spi_binding;

  static constexpr std::uint32_t clock_hz{1000000U};
  static constexpr microhal::spi_duplex_mode duplex_mode{microhal::spi_duplex_mode::full};
  static constexpr microhal::spi_clock_polarity clock_polarity{microhal::spi_clock_polarity::idle_low};
  static constexpr microhal::spi_clock_phase clock_phase{microhal::spi_clock_phase::first_edge};
  static constexpr microhal::spi_bit_order bit_order{microhal::spi_bit_order::msb_first};
  static constexpr bool enable_dma{false};
  static constexpr bool enable_interrupts{false};
};

struct console_uart_cfg final
{
  using binding = console_uart_binding;

  static constexpr std::uint32_t baud_rate{115200U};
  static constexpr microhal::uart_parity parity{microhal::uart_parity::none};
  static constexpr microhal::uart_stop_bits stop_bits{microhal::uart_stop_bits::one};
  static constexpr bool enable_dma{true};
  static constexpr bool enable_interrupts{true};
  static constexpr bool require_fifo{true};
  static constexpr bool require_lpuart{false};
};

struct console_uart_secondary_binding final
{
  static constexpr std::uintptr_t base_address{0x40004400U};
};

struct console_uart_secondary_cfg final
{
  using binding = console_uart_secondary_binding;

  static constexpr std::uintptr_t base_address{0x40004400U};
  static constexpr std::uint32_t baud_rate{115200U};
  static constexpr microhal::uart_parity parity{microhal::uart_parity::none};
  static constexpr microhal::uart_stop_bits stop_bits{microhal::uart_stop_bits::one};
  static constexpr bool enable_dma{false};
  static constexpr bool enable_interrupts{true};
  static constexpr bool require_fifo{false};
  static constexpr bool require_lpuart{false};
};

struct sensor_i2c_secondary_binding final
{
  static constexpr std::uintptr_t base_address{0x40005800U};
  static constexpr std::uint32_t timing{0x00000E14U};
};

struct sensor_i2c_secondary_cfg final
{
  using binding = sensor_i2c_secondary_binding;

  static constexpr std::uintptr_t base_address{0x40005800U};
  static constexpr std::uint32_t timing{0x00000E14U};
  static constexpr std::uint32_t clock_hz{400000U};
  static constexpr microhal::i2c_address_mode address_mode{microhal::i2c_address_mode::seven_bit};
  static constexpr bool enable_dma{false};
  static constexpr bool enable_interrupts{true};
};

struct sensor_spi_secondary_binding final
{
  static constexpr std::uintptr_t base_address{0x40003C00U};
};

struct sensor_spi_secondary_cfg final
{
  using binding = sensor_spi_secondary_binding;

  static constexpr std::uintptr_t base_address{0x40003C00U};
  static constexpr std::uint32_t clock_hz{1000000U};
  static constexpr microhal::spi_duplex_mode duplex_mode{microhal::spi_duplex_mode::full};
  static constexpr microhal::spi_clock_polarity clock_polarity{microhal::spi_clock_polarity::idle_low};
  static constexpr microhal::spi_clock_phase clock_phase{microhal::spi_clock_phase::first_edge};
  static constexpr microhal::spi_bit_order bit_order{microhal::spi_bit_order::msb_first};
  static constexpr bool enable_dma{false};
  static constexpr bool enable_interrupts{true};
};

struct aux_timer_binding final
{
  static constexpr std::uintptr_t base_address{0x40000000U};
};

struct aux_timer_cfg final
{
  using binding = aux_timer_binding;

  static constexpr std::uintptr_t base_address{0x40000000U};
  static constexpr std::uint32_t clock_hz{1000000U};
  static constexpr std::uint32_t prescaler{79U};
  static constexpr std::uint32_t period{999U};
  static constexpr microhal::timer_counter_mode counter_mode{microhal::timer_counter_mode::up};
  static constexpr microhal::timer_clock_source clock_source{microhal::timer_clock_source::internal};
  static constexpr bool enable_dma{false};
  static constexpr bool enable_interrupts{true};
};

struct motor_pwm_binding final
{
};

struct motor_pwm_cfg final
{
  using binding = motor_pwm_binding;

  static constexpr std::uint32_t frequency_hz{20000U};
  static constexpr microhal::pwm_polarity polarity{microhal::pwm_polarity::active_high};
  static constexpr bool enable_dead_time{false};
};

struct board_ethernet_binding final
{
  static constexpr std::uintptr_t base_address{0x40028000U};
  static constexpr microhal::ethernet_media_interface media_interface{microhal::ethernet_media_interface::rmii};
  static constexpr std::array<std::uint8_t, 6U> mac_address{{0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x21U}};
  static constexpr std::uint32_t rx_buffer_size{1528U};
  static constexpr std::uint32_t rx_buffer_count{8U};
};

using board_ethernet_cfg =
    microhal::ethernet::bound_mac_config<board_ethernet_binding, microhal::ethernet_link_speed::mbps100,
                                         microhal::ethernet_duplex::full, false, true, true>;

struct board_sd_binding final
{
  static constexpr std::uintptr_t base_address{0x40012C00U};
  static constexpr std::uint32_t clock_divider{0U};
  static constexpr bool hardware_flow_control{false};
};

using board_sd_cfg = microhal::sd::bound_host_config<board_sd_binding, 25'000'000U, microhal::sd_bus_width::four,
                                                     microhal::sd_timing_mode::default_speed, true, true>;

static_assert(microhal::BackendConcept<test_backend>);
static_assert(microhal::GpioPinConcept<test_backend, led_pin_cfg>);
static_assert(microhal::GpioPinConcept<test_backend, status_led_cfg>);
static_assert(microhal::GpioPinConcept<test_backend, interrupt_pin_cfg>);
static_assert(microhal::I2cMasterConcept<test_backend, i2c_cfg>);
static_assert(microhal::I2cMasterConcept<test_backend, sensor_i2c_cfg>);
static_assert(microhal::SpiMasterConcept<test_backend, spi_cfg>);
static_assert(microhal::SpiMasterConcept<test_backend, sensor_spi_cfg>);
static_assert(microhal::QspiConcept<test_backend, qspi_cfg>);
static_assert(
    []() consteval
    {
      if constexpr (microhal::backend_traits<test_backend>::dac_capability().supported)
      {
        return microhal::DacConcept<test_backend, dac_cfg>;
      }

      return true;
    }());
static_assert(microhal::UartConcept<test_backend, uart_cfg>);
static_assert(microhal::UartConcept<test_backend, console_uart_cfg>);
static_assert(microhal::UartConcept<test_backend, console_uart_secondary_cfg>);
static_assert(microhal::RtcConcept<test_backend, rtc_cfg>);
static_assert(microhal::FlashConcept<test_backend, flash_cfg>);
static_assert(microhal::TimerConcept<test_backend, timer_cfg>);
static_assert(microhal::TimerConcept<test_backend, aux_timer_cfg>);
static_assert(microhal::PwmConcept<test_backend, motor_pwm_cfg>);
static_assert(microhal::I2cMasterConcept<test_backend, sensor_i2c_secondary_cfg>);
static_assert(microhal::SpiMasterConcept<test_backend, sensor_spi_secondary_cfg>);
static_assert(microhal::BackendConcept<test_backend_ll>);
static_assert(microhal::UartConcept<test_backend_ll, console_uart_secondary_cfg>);
static_assert(microhal::I2cMasterConcept<test_backend_ll, sensor_i2c_secondary_cfg>);
static_assert(microhal::SpiMasterConcept<test_backend_ll, sensor_spi_secondary_cfg>);
static_assert(microhal::TimerConcept<test_backend_ll, aux_timer_cfg>);
static_assert(
    []() consteval
    {
      if constexpr (microhal::backend_traits<test_backend>::ethernet_capability().supported)
      {
        return microhal::EthernetConcept<test_backend, board_ethernet_cfg>;
      }

      return true;
    }());
static_assert(
    []() consteval
    {
      if constexpr (microhal::backend_traits<test_backend>::sd_capability().supported)
      {
        return microhal::SdConcept<test_backend, board_sd_cfg>;
      }

      return true;
    }());
static_assert(microhal::AutobaudCapableConcept<test_backend, uart_cfg, autobaud_cfg>);

#if defined(GPIO_MODE_IT_RISING) && defined(GPIO_MODE_IT_FALLING) && defined(GPIO_MODE_IT_RISING_FALLING)
static_assert(microhal::backends::stm32::hal::hal_gpio_interrupt_mode(microhal::gpio_interrupt_trigger::rising) ==
              GPIO_MODE_IT_RISING);
static_assert(microhal::backends::stm32::hal::hal_gpio_interrupt_mode(microhal::gpio_interrupt_trigger::falling) ==
              GPIO_MODE_IT_FALLING);
static_assert(microhal::backends::stm32::hal::hal_gpio_interrupt_mode(microhal::gpio_interrupt_trigger::both) ==
              GPIO_MODE_IT_RISING_FALLING);
#endif

#if defined(USART2)
static_assert(microhal::backend_traits<test_backend>::uart_capability().instance_count >= 2U);
static_assert(microhal::backend_traits<test_backend_ll>::uart_capability().instance_count >= 2U);
#endif

#if defined(I2C2)
static_assert(microhal::backend_traits<test_backend>::i2c_capability().instance_count >= 2U);
static_assert(microhal::backend_traits<test_backend_ll>::i2c_capability().instance_count >= 2U);
#endif

#if defined(SPI3)
static_assert(microhal::backend_traits<test_backend>::spi_capability().instance_count >= 3U);
static_assert(microhal::backend_traits<test_backend_ll>::spi_capability().instance_count >= 3U);
#endif

#if defined(TIM2)
static_assert(microhal::backend_traits<test_backend>::timer_capability().instance_count >= 2U);
static_assert(microhal::backend_traits<test_backend_ll>::timer_capability().instance_count >= 2U);
#endif

static_assert(microhal::backend_registry.size() == MICROHAL_EXPECTED_BACKEND_COUNT);
static_assert(microhal::select_backend<test_backend>().kind == microhal::backend_kind::stm32_cube_hal);

/// @brief Folds a status into an accumulator so `[[nodiscard]]` results are genuinely observed.
namespace
{
microhal::status worst_status = microhal::status::ok;

void observe(const microhal::status value) noexcept
{
  if (!microhal::is_ok(value) && microhal::is_ok(worst_status))
  {
    worst_status = value;
  }
}
} // namespace

template <typename Backend> void exercise_dac() noexcept
{
  if constexpr (microhal::backend_traits<Backend>::dac_capability().supported)
  {
    observe(microhal::dac::converter<Backend, dac_cfg>::initialize());
    observe(microhal::dac::converter<Backend, dac_cfg>::write_blocking(0x0800U));
  }
}

int main()
{
  std::array<std::uint8_t, 4U> rx_payload{};
  constexpr std::array<std::uint8_t, 4U> tx_payload{{0x55U, 0xAAU, 0x11U, 0x22U}};

  observe(microhal::uart::port<test_backend, console_uart_secondary_cfg>::initialize());
  observe(microhal::uart::port<test_backend, console_uart_secondary_cfg>::write_blocking(tx_payload));
  observe(microhal::uart::port<test_backend, console_uart_secondary_cfg>::read_blocking(rx_payload));
  observe(microhal::i2c::master<test_backend, sensor_i2c_secondary_cfg>::initialize());
  observe(microhal::i2c::master<test_backend, sensor_i2c_secondary_cfg>::write_blocking(0x1DU, tx_payload));
  observe(microhal::i2c::master<test_backend, sensor_i2c_secondary_cfg>::read_blocking(0x1DU, rx_payload));
  observe(microhal::i2c::master<test_backend, sensor_i2c_secondary_cfg>::write_read_blocking(0x1DU, tx_payload,
                                                                                             rx_payload));
  observe(microhal::spi::master<test_backend, sensor_spi_secondary_cfg>::initialize());
  observe(microhal::spi::master<test_backend, sensor_spi_secondary_cfg>::transfer(tx_payload, rx_payload));
  observe(microhal::timers::timer<test_backend, aux_timer_cfg>::initialize());
  observe(microhal::timers::timer<test_backend, aux_timer_cfg>::start());
  observe(microhal::timers::timer<test_backend, aux_timer_cfg>::set_period(1000U));
  (void)microhal::timers::timer<test_backend, aux_timer_cfg>::capture_value<timer_channel_cfg>();
  (void)microhal::timers::timer<test_backend, aux_timer_cfg>::counter();
  []<typename Backend>()
  {
    if constexpr (microhal::peripheral_available<Backend, microhal::peripheral_kind::dma>())
      (void)microhal::dma::controller<Backend, dma_cfg>::transfer_status();
  }.template operator()<test_backend>();

  const microhal::qspi::command qspi_read{.instruction = 0x0BU,
                                          .address = 0x123456U,
                                          .address_size_bytes = 3U,
                                          .address_mode = microhal::qspi::io_mode::single,
                                          .dummy_cycles = 8U,
                                          .data_mode = microhal::qspi::io_mode::single};
  observe(microhal::qspi::controller<test_backend, qspi_cfg>::initialize());
  observe(microhal::qspi::controller<test_backend, qspi_cfg>::command_read(qspi_read, rx_payload));
  observe(microhal::qspi::controller<test_backend, qspi_cfg>::command_write(qspi_read, tx_payload));
  observe(microhal::qspi::controller<test_backend, qspi_cfg>::enter_memory_mapped(qspi_read));
  observe(microhal::qspi::controller<test_backend, qspi_cfg>::exit_memory_mapped());

  observe(microhal::uart::port<test_backend_ll, console_uart_secondary_cfg>::initialize());
  observe(microhal::uart::port<test_backend_ll, console_uart_secondary_cfg>::write_blocking(tx_payload));
  observe(microhal::uart::port<test_backend_ll, console_uart_secondary_cfg>::read_blocking(rx_payload));
  observe(microhal::i2c::master<test_backend_ll, sensor_i2c_secondary_cfg>::initialize());
  observe(microhal::i2c::master<test_backend_ll, sensor_i2c_secondary_cfg>::write_blocking(0x1DU, tx_payload));
  observe(microhal::i2c::master<test_backend_ll, sensor_i2c_secondary_cfg>::read_blocking(0x1DU, rx_payload));
  observe(microhal::i2c::master<test_backend_ll, sensor_i2c_secondary_cfg>::write_read_blocking(0x1DU, tx_payload,
                                                                                                rx_payload));
  observe(microhal::spi::master<test_backend_ll, sensor_spi_secondary_cfg>::initialize());
  observe(microhal::spi::master<test_backend_ll, sensor_spi_secondary_cfg>::transfer(tx_payload, rx_payload));
  observe(microhal::timers::timer<test_backend_ll, aux_timer_cfg>::initialize());
  observe(microhal::timers::timer<test_backend_ll, aux_timer_cfg>::start());
  observe(microhal::timers::timer<test_backend_ll, aux_timer_cfg>::set_period(1000U));
  (void)microhal::timers::timer<test_backend_ll, aux_timer_cfg>::capture_value<timer_channel_cfg>();
  (void)microhal::timers::timer<test_backend_ll, aux_timer_cfg>::counter();

  observe(microhal::gpio::pin<test_backend, interrupt_pin_cfg>::template enable_interrupt<board_exti_handler>());

  exercise_dac<test_backend>();

  // This translation unit exists to prove the API compiles and links; there is no peripheral behind
  // these calls on the host, so the accumulated status is reported but not asserted on.
  (void)worst_status;
  return 0;
}
