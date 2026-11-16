// SPDX-License-Identifier: Apache-2.0
/**
 * @file edge_cases_test.cpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

// Edge Cases and Boundary Conditions Test
// Tests: Zero values, maximum values, corner cases, compile-time constraints

#include <array>
#include <limits>

#include "microhal/adc/adc.hpp"
#include "microhal/can/can.hpp"
#include "microhal/gpio/gpio.hpp"
#include "microhal/i2c/i2c.hpp"
#include "microhal/pwm/pwm.hpp"
#include "microhal/spi/spi.hpp"
#include "microhal/uart/uart.hpp"
#include "microhal/usb/usb.hpp"

using test_backend = microhal::stm32_hal_tag;

using can_at_limit = microhal::can::can_config<0U, 1'000'000U, microhal::can_mode::normal, false, false>;
using can_over_limit = microhal::can::can_config<0U, 1'000'001U, microhal::can_mode::normal, false, false>;
using can_out_of_range = microhal::can::can_config<255U, 500'000U, microhal::can_mode::normal, false, false>;
using unsupported_zephyr_can_fd = microhal::can::can_config<1U, 500'000U, microhal::can_mode::normal, true, false>;
static_assert(microhal::CanConcept<test_backend, can_at_limit>);
static_assert(!microhal::CanConcept<test_backend, can_over_limit>);
static_assert(!microhal::CanConcept<microhal::mock_tag, can_out_of_range>);
static_assert(!microhal::CanConcept<microhal::zephyr_tag, unsupported_zephyr_can_fd>);

using mock_can_config = microhal::can::can_config<0U, 500'000U, microhal::can_mode::normal, false, false>;
using mock_can = microhal::can::bus<microhal::mock_tag, mock_can_config>;
using mock_i2c_config = microhal::i2c::master_config<1U, 100'000U>;
using mock_i2c = microhal::i2c::master<microhal::mock_tag, mock_i2c_config>;
using disallowed_stretch_i2c_config =
    microhal::i2c::master_config<1U, 100'000U, microhal::i2c_address_mode::seven_bit, false, false, 1000U,
                                 microhal::transfer_mode::automatic, microhal::i2c_clock_stretch_policy::disallow>;
using timed_stretch_i2c_config =
    microhal::i2c::master_config<1U, 100'000U, microhal::i2c_address_mode::seven_bit, false, false, 1000U,
                                 microhal::transfer_mode::automatic, microhal::i2c_clock_stretch_policy::allow, 1000U>;
using mock_usb_config = microhal::usb::usb_config<0U>;
using mock_usb = microhal::usb::device<microhal::mock_tag, mock_usb_config>;
static_assert(!mock_can::has_timing_configuration);
static_assert(!mock_can::has_error_state);
static_assert(!mock_can::has_bus_recovery);
static_assert(!mock_i2c::has_bus_recovery);
static_assert(microhal::I2cMasterConcept<microhal::mock_tag, mock_i2c_config>);
static_assert(!microhal::I2cMasterConcept<microhal::mock_tag, disallowed_stretch_i2c_config>);
static_assert(!microhal::I2cMasterConcept<microhal::mock_tag, timed_stretch_i2c_config>);
static_assert(!mock_usb::has_control_transfer);
static_assert(!mock_usb::has_descriptor_access);
constexpr std::array<std::uint8_t, 1U> smbus_pec_payload{0x34U};
static_assert(microhal::i2c::smbus::pec(0x12U, false, smbus_pec_payload) == 0x76U);

// GPIO Edge Cases
// Maximum GPIO port (depends on device, typically 0-8 for STM32F4)
using gpio_port_max = microhal::gpio::pin_config<7U, 15U, microhal::gpio_mode::output>;
static_assert(microhal::GpioPinConcept<test_backend, gpio_port_max>);

// GPIO pin 0 (minimum)
using gpio_pin_min = microhal::gpio::pin_config<0U, 0U, microhal::gpio_mode::input>;
static_assert(microhal::GpioPinConcept<test_backend, gpio_pin_min>);

// GPIO all modes
using gpio_input = microhal::gpio::pin_config<0U, 0U, microhal::gpio_mode::input>;
using gpio_output = microhal::gpio::pin_config<0U, 1U, microhal::gpio_mode::output>;
using gpio_alternate = microhal::gpio::pin_config<0U, 2U, microhal::gpio_mode::alternate>;
using gpio_analog = microhal::gpio::pin_config<0U, 3U, microhal::gpio_mode::analog>;
static_assert(microhal::GpioPinConcept<test_backend, gpio_input>);
static_assert(microhal::GpioPinConcept<test_backend, gpio_output>);
static_assert(microhal::GpioPinConcept<test_backend, gpio_alternate>);
static_assert(microhal::GpioPinConcept<test_backend, gpio_analog>);

// I2C Edge Cases
// Minimum I2C frequency (typically 10 kHz)
using i2c_slow = microhal::i2c::master_config<1U, 10000U, microhal::i2c_address_mode::seven_bit>;
static_assert(microhal::I2cMasterConcept<test_backend, i2c_slow>);

// Maximum I2C frequency (Fast Mode Plus: 1 MHz)
using i2c_fast = microhal::i2c::master_config<1U, 1000000U, microhal::i2c_address_mode::seven_bit>;
static_assert(microhal::I2cMasterConcept<test_backend, i2c_fast>);

// I2C 10-bit addressing
using i2c_10bit = microhal::i2c::master_config<1U, 100000U, microhal::i2c_address_mode::ten_bit>;
static_assert(microhal::I2cMasterConcept<test_backend, i2c_10bit>);
using i2c_out_of_range_instance = microhal::i2c::master_config<255U, 100000U>;
static_assert(!microhal::I2cMasterConcept<microhal::mock_tag, i2c_out_of_range_instance>);
using zephyr_i2c_config = microhal::i2c::master_config<1U, 100000U>;
static_assert(microhal::I2cMasterConcept<microhal::zephyr_tag, zephyr_i2c_config>);
static_assert(!microhal::IsrCapableConcept<microhal::zephyr_tag, microhal::peripheral_kind::i2c>);

// SPI Edge Cases
// Minimum SPI frequency
using spi_slow = microhal::spi::master_config<1U, 100000U, microhal::spi_duplex_mode::full>;
static_assert(microhal::SpiMasterConcept<test_backend, spi_slow>);

// Maximum SPI frequency (device dependent, typically 42 MHz for STM32F4)
using spi_fast = microhal::spi::master_config<1U, 42000000U, microhal::spi_duplex_mode::full>;
static_assert(microhal::SpiMasterConcept<test_backend, spi_fast>);

// SPI half-duplex
using spi_half_duplex = microhal::spi::master_config<1U, 1000000U, microhal::spi_duplex_mode::half>;
static_assert(microhal::SpiMasterConcept<test_backend, spi_half_duplex>);

// SPI simplex TX only
using spi_simplex_tx = microhal::spi::master_config<1U, 1000000U, microhal::spi_duplex_mode::simplex_tx>;
static_assert(microhal::SpiMasterConcept<test_backend, spi_simplex_tx>);

// UART Edge Cases
// Minimum baud rate
using uart_slow = microhal::uart::port_config<1U, 300U, microhal::uart_parity::none, microhal::uart_stop_bits::one>;
static_assert(microhal::UartConcept<test_backend, uart_slow>);

// Maximum baud rate (device dependent, typically 12.5 Mbps)
using uart_fast =
    microhal::uart::port_config<1U, 12500000U, microhal::uart_parity::none, microhal::uart_stop_bits::one>;
static_assert(microhal::UartConcept<test_backend, uart_fast>);

// UART with all parity options
using uart_even_parity =
    microhal::uart::port_config<1U, 115200U, microhal::uart_parity::even, microhal::uart_stop_bits::one>;
using uart_odd_parity =
    microhal::uart::port_config<1U, 115200U, microhal::uart_parity::odd, microhal::uart_stop_bits::one>;
static_assert(microhal::UartConcept<test_backend, uart_even_parity>);
static_assert(microhal::UartConcept<test_backend, uart_odd_parity>);
using uart_nine_data_bits =
    microhal::uart::port_config<1U, 115200U, microhal::uart_parity::none, microhal::uart_stop_bits::one, false, false,
                                false, false, 1000U, microhal::transfer_mode::automatic,
                                microhal::uart_data_bits::nine>;
static_assert(microhal::UartConcept<test_backend, uart_nine_data_bits>);
using uart_nine_data_bits_with_parity =
    microhal::uart::port_config<1U, 115200U, microhal::uart_parity::even, microhal::uart_stop_bits::one, false, false,
                                false, false, 1000U, microhal::transfer_mode::automatic,
                                microhal::uart_data_bits::nine>;
static_assert(!microhal::UartConcept<test_backend, uart_nine_data_bits_with_parity>);

// UART with 2 stop bits
using uart_2_stop =
    microhal::uart::port_config<1U, 115200U, microhal::uart_parity::none, microhal::uart_stop_bits::two>;
static_assert(microhal::UartConcept<test_backend, uart_2_stop>);

// ADC Edge Cases
// All ADC resolutions
using adc_8bit = microhal::adc::adc_config<1U, microhal::adc_resolution::bits_8, microhal::adc_sample_time::cycles_84>;
using adc_10bit =
    microhal::adc::adc_config<1U, microhal::adc_resolution::bits_10, microhal::adc_sample_time::cycles_84>;
using adc_12bit =
    microhal::adc::adc_config<1U, microhal::adc_resolution::bits_12, microhal::adc_sample_time::cycles_84>;
using adc_14bit =
    microhal::adc::adc_config<1U, microhal::adc_resolution::bits_14, microhal::adc_sample_time::cycles_84>;
using adc_16bit =
    microhal::adc::adc_config<1U, microhal::adc_resolution::bits_16, microhal::adc_sample_time::cycles_84>;
static_assert(microhal::AdcConcept<test_backend, adc_8bit>);
static_assert(microhal::AdcConcept<test_backend, adc_10bit>);
static_assert(microhal::AdcConcept<test_backend, adc_12bit>);
static_assert(microhal::AdcConcept<test_backend, adc_14bit>);
static_assert(microhal::AdcConcept<test_backend, adc_16bit>);

// All ADC sample times
using adc_cycles3 =
    microhal::adc::adc_config<1U, microhal::adc_resolution::bits_12, microhal::adc_sample_time::cycles_3>;
using adc_cycles56 =
    microhal::adc::adc_config<1U, microhal::adc_resolution::bits_12, microhal::adc_sample_time::cycles_56>;
using adc_cycles112 =
    microhal::adc::adc_config<1U, microhal::adc_resolution::bits_12, microhal::adc_sample_time::cycles_112>;
using adc_cycles480 =
    microhal::adc::adc_config<1U, microhal::adc_resolution::bits_12, microhal::adc_sample_time::cycles_480>;
static_assert(microhal::AdcConcept<test_backend, adc_cycles3>);
static_assert(microhal::AdcConcept<test_backend, adc_cycles56>);
static_assert(microhal::AdcConcept<test_backend, adc_cycles112>);
static_assert(microhal::AdcConcept<test_backend, adc_cycles480>);

// PWM Edge Cases
// PWM with different period values
using pwm_short_period = microhal::pwm::pwm_config<1U, 1U, 100U>;  // High frequency
using pwm_long_period = microhal::pwm::pwm_config<1U, 1U, 65535U>; // Low frequency
static_assert(microhal::PwmConcept<test_backend, pwm_short_period>);
static_assert(microhal::PwmConcept<test_backend, pwm_long_period>);

// PWM different channels
using pwm_ch1 = microhal::pwm::pwm_config<1U, 1U, 1000U>;
using pwm_ch2 = microhal::pwm::pwm_config<1U, 2U, 1000U>;
using pwm_ch3 = microhal::pwm::pwm_config<1U, 3U, 1000U>;
using pwm_ch4 = microhal::pwm::pwm_config<1U, 4U, 1000U>;
static_assert(microhal::PwmConcept<test_backend, pwm_ch1>);
static_assert(microhal::PwmConcept<test_backend, pwm_ch2>);
static_assert(microhal::PwmConcept<test_backend, pwm_ch3>);
static_assert(microhal::PwmConcept<test_backend, pwm_ch4>);

// Verify consteval capability queries at compile time
static_assert(microhal::backend_traits<test_backend>::gpio_capability().instance_count > 0U);
static_assert(microhal::backend_traits<test_backend>::i2c_capability().max_frequency_hz > 0U);
static_assert(microhal::backend_traits<test_backend>::spi_capability().max_frequency_hz > 0U);
static_assert(microhal::backend_traits<test_backend>::uart_capability().max_frequency_hz > 0U);
static_assert(microhal::backend_traits<test_backend>::adc_capability().resolution_bits > 0U);

// Verify cross-cutting capabilities
static_assert(microhal::backend_traits<test_backend>::gpio_capability().isr);
static_assert(microhal::backend_traits<test_backend>::i2c_capability().dma);
static_assert(microhal::backend_traits<test_backend>::spi_capability().dma);
static_assert(microhal::backend_traits<test_backend>::uart_capability().dma);
static_assert(microhal::backend_traits<test_backend>::adc_capability().dma);

int main()
{
  // Runtime verification (should optimize to return 0)
  constexpr auto gpio_cap = microhal::backend_traits<test_backend>::gpio_capability();
  if constexpr (!gpio_cap.supported)
  {
    return 1; // Should never execute
  }

  constexpr auto i2c_cap = microhal::backend_traits<test_backend>::i2c_capability();
  if constexpr (i2c_cap.max_frequency_hz == 0U)
  {
    return 1; // Should never execute
  }

  constexpr std::array<std::uint8_t, 1U> tx{0x01U};
  std::array<std::uint8_t, 1U> rx{};
  using zephyr_i2c = microhal::i2c::master<microhal::zephyr_tag, zephyr_i2c_config>;
  if (zephyr_i2c::write_read_async<nullptr>(0x50U, tx, rx) != microhal::status::unsupported)
  {
    return 1;
  }
  if (zephyr_i2c::start_transfer<nullptr>(0x50U, rx) != microhal::status::unsupported)
  {
    return 1;
  }

  constexpr microhal::can_bit_timing valid_can_timing{.prescaler = 4U,
                                                      .synchronization_jump_width = 1U,
                                                      .propagation_segment = 2U,
                                                      .phase_segment1 = 5U,
                                                      .phase_segment2 = 2U};
  constexpr microhal::can_bit_timing invalid_can_timing{};
  if (!valid_can_timing.is_valid() || invalid_can_timing.is_valid() ||
      (mock_can::configure_timing(invalid_can_timing) != microhal::status::invalid_argument) ||
      (mock_can::configure_timing(valid_can_timing) != microhal::status::unsupported) ||
      (mock_can::error_state().error() != microhal::status::unsupported) ||
      (mock_can::recover_bus() != microhal::status::unsupported))
  {
    return 1;
  }

  constexpr microhal::usb_setup_packet empty_setup{};
  std::array<std::uint8_t, 2U> descriptor_buffer{};
  if ((mock_usb::control_transfer(empty_setup) != microhal::status::unsupported) ||
      (mock_usb::control_transfer(empty_setup, std::span<const std::uint8_t>{descriptor_buffer}) !=
       microhal::status::invalid_argument) ||
      (mock_usb::get_descriptor(0U, 0U, 0U, descriptor_buffer).error() != microhal::status::invalid_argument) ||
      (mock_usb::get_descriptor(1U, 0U, 0U, descriptor_buffer).error() != microhal::status::unsupported))
  {
    return 1;
  }

  static std::array<std::uint8_t, std::numeric_limits<std::uint16_t>::max() + 1U> oversized_i2c_payload{};
  using stm32_i2c = microhal::i2c::master<test_backend, microhal::i2c::master_config<1U, 100'000U>>;
  if ((stm32_i2c::write_blocking(0x50U, oversized_i2c_payload) != microhal::status::invalid_argument) ||
      (mock_i2c::recover_bus() != microhal::status::unsupported))
  {
    return 1;
  }

  return 0;
}
