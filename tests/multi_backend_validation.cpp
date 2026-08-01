// SPDX-License-Identifier: Apache-2.0
/**
 * @file multi_backend_validation.cpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

// Multi-Backend Validation Test
// Tests: All 5 backends with identical configurations to ensure portability

#include <array>

#include "microhal/adc/adc.hpp"
#include "microhal/ethernet/ethernet.hpp"
#include "microhal/flash/flash.hpp"
#include "microhal/gpio/gpio.hpp"
#include "microhal/i2c/i2c.hpp"
#include "microhal/pwm/pwm.hpp"
#include "microhal/rtc/rtc.hpp"
#include "microhal/sd/sd.hpp"
#include "microhal/spi/spi.hpp"
#include "microhal/timers/timer.hpp"
#include "microhal/uart/uart.hpp"

// Define all 5 backend tags
using backend_stm32_hal = microhal::stm32_hal_tag;
using backend_stm32_ll = microhal::stm32_ll_tag;
using backend_kinetis = microhal::kinetis_espresso_tag;
using backend_zephyr = microhal::zephyr_tag;
using backend_nuttx = microhal::nuttx_tag;

// Common configurations (portable across backends)
using gpio_led = microhal::gpio::pin_config<0U, 5U, microhal::gpio_mode::output>;
using gpio_button = microhal::gpio::pin_config<0U, 13U, microhal::gpio_mode::input, microhal::gpio_pull::up>;

using i2c_common = microhal::i2c::master_config<1U, 100000U, microhal::i2c_address_mode::seven_bit>;
using spi_common = microhal::spi::master_config<1U, 1000000U, microhal::spi_duplex_mode::full>;
using uart_common =
    microhal::uart::port_config<1U, 115200U, microhal::uart_parity::none, microhal::uart_stop_bits::one>;
using rtc_common = microhal::rtc::rtc_config<>;
using timer_common = microhal::timers::timer_config<1U, 1000000U, microhal::timer_counter_mode::up,
                                                    microhal::timer_clock_source::internal>;
using adc_common =
    microhal::adc::adc_config<1U, microhal::adc_resolution::bits_12, microhal::adc_sample_time::cycles_84>;
using pwm_common = microhal::pwm::pwm_config<1U, 1U, 1000U>;

struct stm32_ethernet_binding final
{
  static constexpr std::uintptr_t base_address{0x40028000U};
  static constexpr microhal::ethernet_media_interface media_interface{microhal::ethernet_media_interface::rmii};
  static constexpr std::array<std::uint8_t, 6U> mac_address{{0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x31U}};
  static constexpr std::uint32_t rx_buffer_size{1528U};
  static constexpr std::uint32_t rx_buffer_count{8U};
};

using stm32_ethernet_common =
    microhal::ethernet::bound_mac_config<stm32_ethernet_binding, microhal::ethernet_link_speed::mbps100,
                                         microhal::ethernet_duplex::full, false, true, true>;

struct stm32_sd_binding final
{
  static constexpr std::uintptr_t base_address{0x40012C00U};
  static constexpr std::uint32_t clock_divider{0U};
  static constexpr bool hardware_flow_control{false};
};

using stm32_sd_common = microhal::sd::bound_host_config<stm32_sd_binding, 25'000'000U, microhal::sd_bus_width::four,
                                                        microhal::sd_timing_mode::default_speed, true, true>;

// Template to validate a backend with all common configurations
template <typename Backend> struct backend_validator
{
  // GPIO validation
  static_assert(microhal::GpioPinConcept<Backend, gpio_led>);
  static_assert(microhal::GpioPinConcept<Backend, gpio_button>);

  // Communication peripherals
  static_assert(microhal::I2cMasterConcept<Backend, i2c_common>);
  static_assert(microhal::SpiMasterConcept<Backend, spi_common>);
  static_assert(microhal::UartConcept<Backend, uart_common>);

  // Storage and timing
  static_assert(microhal::RtcConcept<Backend, rtc_common>);
  static_assert(microhal::TimerConcept<Backend, timer_common>);

  // Analog and control
  static_assert(microhal::AdcConcept<Backend, adc_common>);
  static_assert(microhal::PwmConcept<Backend, pwm_common>);

  // Capability queries
  static_assert(microhal::backend_traits<Backend>::gpio_capability().supported);
  static_assert(microhal::backend_traits<Backend>::i2c_capability().supported);
  static_assert(microhal::backend_traits<Backend>::spi_capability().supported);
  static_assert(microhal::backend_traits<Backend>::uart_capability().supported);
  static_assert(microhal::backend_traits<Backend>::rtc_capability().supported);
  static_assert(microhal::backend_traits<Backend>::flash_capability().supported);
  static_assert(microhal::backend_traits<Backend>::timer_capability().supported);
  static_assert(microhal::backend_traits<Backend>::adc_capability().supported);
  static_assert(microhal::backend_traits<Backend>::pwm_capability().supported);

  // Backend metadata
  static_assert(microhal::backend_traits<Backend>::name.size() > 0U);
  static_assert(microhal::backend_traits<Backend>::gpio_port_count > 0U);
  static_assert(microhal::backend_traits<Backend>::gpio_pins_per_port > 0U);
};

// Validate all 5 backends
using validator_stm32_hal = backend_validator<backend_stm32_hal>;
using validator_stm32_ll = backend_validator<backend_stm32_ll>;
using validator_kinetis = backend_validator<backend_kinetis>;
using validator_zephyr = backend_validator<backend_zephyr>;
using validator_nuttx = backend_validator<backend_nuttx>;

// Cross-backend capability comparison
static_assert(microhal::backend_traits<backend_stm32_hal>::gpio_capability().alternate_function);
static_assert(microhal::backend_traits<backend_stm32_ll>::gpio_capability().alternate_function);
static_assert(microhal::backend_traits<backend_kinetis>::gpio_capability().alternate_function);
static_assert(microhal::backend_traits<backend_zephyr>::gpio_capability().alternate_function);
static_assert(microhal::backend_traits<backend_nuttx>::gpio_capability().alternate_function);

// Verify ISR support across backends
static_assert(microhal::backend_traits<backend_stm32_hal>::gpio_capability().isr);
static_assert(microhal::backend_traits<backend_stm32_ll>::gpio_capability().isr);
static_assert(microhal::backend_traits<backend_kinetis>::gpio_capability().isr);
static_assert(microhal::backend_traits<backend_zephyr>::gpio_capability().isr);
static_assert(microhal::backend_traits<backend_nuttx>::gpio_capability().isr);

// Verify DMA support across backends
static_assert(microhal::backend_traits<backend_stm32_hal>::uart_capability().dma);
static_assert(microhal::backend_traits<backend_stm32_ll>::uart_capability().dma);
static_assert(!microhal::backend_traits<backend_kinetis>::uart_capability().dma);
static_assert(microhal::backend_traits<backend_zephyr>::uart_capability().dma);
static_assert(microhal::backend_traits<backend_nuttx>::uart_capability().dma);

// Verify blocking support (should be universal)
static_assert(microhal::backend_traits<backend_stm32_hal>::i2c_capability().blocking);
static_assert(microhal::backend_traits<backend_stm32_ll>::i2c_capability().blocking);
static_assert(microhal::backend_traits<backend_kinetis>::i2c_capability().blocking);
static_assert(microhal::backend_traits<backend_zephyr>::i2c_capability().blocking);
static_assert(!microhal::backend_traits<backend_nuttx>::i2c_capability().supported);
static_assert(!microhal::backend_traits<backend_nuttx>::spi_capability().supported);

// These backend APIs are intentionally unavailable until their runtime paths
// use real vendor or RTOS operations.
#if MICROHAL_STM32_LL_HAS_QSPI
static_assert(microhal::backend_traits<backend_stm32_ll>::qspi_capability().supported);
#else
static_assert(!microhal::backend_traits<backend_stm32_ll>::qspi_capability().supported);
#endif
static_assert(!microhal::backend_traits<backend_stm32_ll>::can_capability().supported);
static_assert(!microhal::backend_traits<backend_stm32_ll>::usb_capability().supported);
static_assert(!microhal::backend_traits<backend_kinetis>::usb_capability().supported);
static_assert(!microhal::backend_traits<backend_zephyr>::qspi_capability().supported);

// Backend kind verification
static_assert(microhal::backend_traits<backend_stm32_hal>::kind == microhal::backend_kind::stm32_cube_hal);
static_assert(microhal::backend_traits<backend_stm32_ll>::kind == microhal::backend_kind::stm32_cube_ll);
static_assert(microhal::backend_traits<backend_kinetis>::kind == microhal::backend_kind::kinetis_espresso);
static_assert(microhal::backend_traits<backend_zephyr>::kind == microhal::backend_kind::zephyr_driver_model);
static_assert(microhal::backend_traits<backend_nuttx>::kind == microhal::backend_kind::nuttx_driver_model);

// Vendor verification
static_assert(microhal::backend_traits<backend_stm32_hal>::vendor == microhal::vendor_kind::stm32);
static_assert(microhal::backend_traits<backend_stm32_ll>::vendor == microhal::vendor_kind::stm32);
static_assert(microhal::backend_traits<backend_kinetis>::vendor == microhal::vendor_kind::kinetis);
static_assert(microhal::backend_traits<backend_zephyr>::vendor == microhal::vendor_kind::zephyr);
static_assert(microhal::backend_traits<backend_nuttx>::vendor == microhal::vendor_kind::nuttx);

// Backend registry validation
static_assert(microhal::backend_registry.size() == MICROHAL_EXPECTED_BACKEND_COUNT);
static_assert(microhal::select_backend<backend_stm32_hal>().vendor == microhal::vendor_kind::stm32);
static_assert(microhal::select_backend<backend_kinetis>().vendor == microhal::vendor_kind::kinetis);
static_assert(
    []() consteval
    {
      if constexpr (microhal::backend_traits<backend_stm32_hal>::ethernet_capability().supported)
      {
        return microhal::EthernetConcept<backend_stm32_hal, stm32_ethernet_common>;
      }

      return true;
    }());
static_assert(
    []() consteval
    {
      if constexpr (microhal::backend_traits<backend_stm32_hal>::sd_capability().supported)
      {
        return microhal::SdConcept<backend_stm32_hal, stm32_sd_common>;
      }

      return true;
    }());
static_assert(!microhal::backend_traits<backend_stm32_ll>::ethernet_capability().supported);
#if MICROHAL_STM32_LL_HAS_SD
static_assert(microhal::backend_traits<backend_stm32_ll>::sd_capability().supported);
#else
static_assert(!microhal::backend_traits<backend_stm32_ll>::sd_capability().supported);
#endif
static_assert(!microhal::backend_traits<backend_kinetis>::ethernet_capability().supported);
static_assert(microhal::backend_traits<backend_kinetis>::sd_capability().supported);
static_assert(microhal::backend_traits<backend_zephyr>::ethernet_capability().supported);
static_assert(microhal::backend_traits<backend_zephyr>::sd_capability().supported);
static_assert(!microhal::backend_traits<backend_nuttx>::ethernet_capability().supported);
static_assert(!microhal::backend_traits<backend_nuttx>::sd_capability().supported);

int main()
{
  // All validation happens at compile time
  return 0;
}
