// SPDX-License-Identifier: Apache-2.0
/**
 * @file extended_peripherals_test.cpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

// Extended Peripherals Compile-Time Validation Test
// Tests: ADC, DAC, CAN, USB, DMA, PWM, Watchdog, I2S, RCC, CRC

#include "microhal/adc/adc.hpp"
#include "microhal/can/can.hpp"
#include "microhal/crc/crc.hpp"
#include "microhal/dac/dac.hpp"
#include "microhal/dma/dma.hpp"
#include "microhal/ethernet/ethernet.hpp"
#include "microhal/i2s/i2s.hpp"
#include "microhal/pwm/pwm.hpp"
#include "microhal/rcc/rcc.hpp"
#include "microhal/sd/sd.hpp"
#include "microhal/usb/usb.hpp"
#include "microhal/watchdog/watchdog.hpp"

#include <array>

// Test all 5 backends
using backend_stm32_hal = microhal::stm32_hal_tag;
using backend_stm32_ll = microhal::stm32_ll_tag;
using backend_kinetis = microhal::kinetis_espresso_tag;
using backend_zephyr = microhal::zephyr_tag;
using backend_nuttx = microhal::nuttx_tag;

// ADC Configuration
using adc_cfg = microhal::adc::adc_config<1U, microhal::adc_resolution::bits_12, microhal::adc_sample_time::cycles_84>;

struct secondary_adc_binding final
{
  static constexpr std::uintptr_t base_address{0x40012100U};
};

struct secondary_adc_cfg final
{
  using binding = secondary_adc_binding;

  static constexpr std::uintptr_t base_address{secondary_adc_binding::base_address};
  static constexpr microhal::adc_resolution resolution{microhal::adc_resolution::bits_12};
  static constexpr microhal::adc_sample_time sample_time{microhal::adc_sample_time::cycles_84};
  static constexpr bool enable_dma{false};
  static constexpr bool enable_interrupts{true};
  static constexpr bool differential_mode{false};
};

static_assert(microhal::AdcConcept<backend_stm32_hal, adc_cfg>);
static_assert(microhal::AdcConcept<backend_stm32_ll, adc_cfg>);
static_assert(microhal::AdcConcept<backend_stm32_hal, secondary_adc_cfg>);
static_assert(microhal::AdcConcept<backend_stm32_ll, secondary_adc_cfg>);
static_assert(microhal::AdcConcept<backend_kinetis, adc_cfg>);
static_assert(microhal::AdcConcept<backend_zephyr, adc_cfg>);
static_assert(microhal::AdcConcept<backend_nuttx, adc_cfg>);

#if defined(ADC2)
static_assert(microhal::backend_traits<backend_stm32_hal>::adc_capability().instance_count >= 2U);
static_assert(microhal::backend_traits<backend_stm32_ll>::adc_capability().instance_count >= 2U);
#endif

// DAC Configuration
using dac_cfg = microhal::dac::dac_config<1U, 1U, microhal::dac_resolution::bits_12>;

static_assert(microhal::DacConcept<backend_stm32_hal, dac_cfg>);
static_assert(microhal::DacConcept<backend_stm32_ll, dac_cfg>);
static_assert(microhal::DacConcept<backend_kinetis, dac_cfg>);
static_assert(microhal::DacConcept<backend_zephyr, dac_cfg>);
static_assert(microhal::DacConcept<backend_nuttx, dac_cfg>);

// CAN Configuration
using can_cfg = microhal::can::can_config<1U, 500000U, microhal::can_mode::normal, false, false>;

struct secondary_can_binding final
{
  static constexpr std::uintptr_t base_address{0x40006800U};
};

struct secondary_can_cfg final
{
  using binding = secondary_can_binding;

  static constexpr std::uintptr_t base_address{secondary_can_binding::base_address};
  static constexpr std::uint32_t baudrate{500000U};
  static constexpr microhal::can_mode mode{microhal::can_mode::normal};
  static constexpr bool enable_can_fd{false};
  static constexpr bool enable_interrupts{true};
};

static_assert(
    []() consteval
    {
      if constexpr (microhal::backend_traits<backend_stm32_hal>::can_capability().supported)
      {
        return microhal::CanConcept<backend_stm32_hal, can_cfg>;
      }

      return true;
    }());
static_assert(
    []() consteval
    {
      if constexpr (microhal::backend_traits<backend_stm32_hal>::can_capability().supported)
      {
        return microhal::CanConcept<backend_stm32_hal, secondary_can_cfg>;
      }

      return true;
    }());
// Note: LL backend has placeholder for CAN (not commonly available in LL)
static_assert(microhal::CanConcept<backend_kinetis, can_cfg>);
static_assert(microhal::CanConcept<backend_zephyr, can_cfg>);
static_assert(microhal::CanConcept<backend_nuttx, can_cfg>);

#if defined(CAN2)
static_assert(microhal::backend_traits<backend_stm32_hal>::can_capability().instance_count >= 2U);
#endif

// USB Configuration
using usb_cfg = microhal::usb::usb_config<0U, microhal::usb_speed::full, true, false, false>;

static_assert(
    []() consteval
    {
      if constexpr (microhal::backend_traits<backend_stm32_hal>::usb_capability().supported)
      {
        return microhal::UsbConcept<backend_stm32_hal, usb_cfg>;
      }

      return true;
    }());
// Note: LL and Kinetis have placeholders for USB (complex driver stack)
static_assert(microhal::UsbConcept<backend_zephyr, usb_cfg>);
static_assert(microhal::UsbConcept<backend_nuttx, usb_cfg>);

// Ethernet Configuration
struct stm32_ethernet_binding final
{
  static constexpr std::uintptr_t base_address{0x40028000U};
  static constexpr microhal::ethernet_media_interface media_interface{microhal::ethernet_media_interface::rmii};
  static constexpr std::array<std::uint8_t, 6U> mac_address{{0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x11U}};
  static constexpr std::uint32_t rx_buffer_size{1528U};
  static constexpr std::uint32_t rx_buffer_count{8U};
};

using ethernet_cfg =
    microhal::ethernet::bound_mac_config<stm32_ethernet_binding, microhal::ethernet_link_speed::mbps100,
                                         microhal::ethernet_duplex::full, false, true, true>;

static_assert(
    []() consteval
    {
      if constexpr (microhal::backend_traits<backend_stm32_hal>::ethernet_capability().supported)
      {
        return microhal::EthernetConcept<backend_stm32_hal, ethernet_cfg>;
      }

      return true;
    }());
static_assert(!microhal::backend_traits<backend_stm32_ll>::ethernet_capability().supported);
static_assert(!microhal::backend_traits<backend_kinetis>::ethernet_capability().supported);
static_assert(microhal::backend_traits<backend_zephyr>::ethernet_capability().supported);
static_assert(!microhal::backend_traits<backend_nuttx>::ethernet_capability().supported);

// SD Configuration
struct stm32_sd_binding final
{
  static constexpr std::uintptr_t base_address{0x40012C00U};
  static constexpr std::uint32_t clock_divider{0U};
  static constexpr bool hardware_flow_control{false};
};

using sd_cfg = microhal::sd::bound_host_config<stm32_sd_binding, 25'000'000U, microhal::sd_bus_width::four,
                                               microhal::sd_timing_mode::default_speed, true, true>;

static_assert(
    []() consteval
    {
      if constexpr (microhal::backend_traits<backend_stm32_hal>::sd_capability().supported)
      {
        return microhal::SdConcept<backend_stm32_hal, sd_cfg>;
      }

      return true;
    }());
#if MICROHAL_STM32_LL_HAS_SD
static_assert(microhal::backend_traits<backend_stm32_ll>::sd_capability().supported);
#else
static_assert(!microhal::backend_traits<backend_stm32_ll>::sd_capability().supported);
#endif
static_assert(microhal::backend_traits<backend_kinetis>::sd_capability().supported);
static_assert(microhal::backend_traits<backend_zephyr>::sd_capability().supported);
static_assert(!microhal::backend_traits<backend_nuttx>::sd_capability().supported);

// DMA Configuration
using dma_cfg = microhal::dma::dma_config<1U, 0U, 0U>; // Instance, Stream, Channel

struct secondary_dma_binding final
{
  static constexpr std::uintptr_t base_address{0x40026410U};
  static constexpr std::uint8_t instance_index{2U};
  static constexpr std::uint8_t stream{0U};
  static constexpr std::uint8_t channel{0U};
};

struct secondary_dma_cfg final
{
  using binding = secondary_dma_binding;

  static constexpr std::uintptr_t base_address{secondary_dma_binding::base_address};
  static constexpr std::uint8_t instance_index{secondary_dma_binding::instance_index};
  static constexpr std::uint8_t stream{secondary_dma_binding::stream};
  static constexpr std::uint8_t channel{secondary_dma_binding::channel};
  static constexpr microhal::dma::dma_direction direction{microhal::dma::dma_direction::memory_to_memory};
  static constexpr microhal::dma::dma_priority priority{microhal::dma::dma_priority::medium};
  static constexpr bool circular_mode{false};
  static constexpr bool double_buffer{false};
};

static_assert(
    []() consteval
    {
      if constexpr (microhal::backend_traits<backend_stm32_hal>::dma_capability().supported)
      {
        return microhal::DmaConcept<backend_stm32_hal, dma_cfg>;
      }

      return true;
    }());
static_assert(
    []() consteval
    {
      if constexpr (microhal::backend_traits<backend_stm32_hal>::dma_capability().supported)
      {
        return microhal::DmaConcept<backend_stm32_hal, secondary_dma_cfg>;
      }

      return true;
    }());
static_assert(
    []() consteval
    {
      if constexpr (microhal::backend_traits<backend_stm32_ll>::dma_capability().supported)
      {
        return microhal::DmaConcept<backend_stm32_ll, dma_cfg>;
      }

      return true;
    }());
static_assert(
    []() consteval
    {
      if constexpr (microhal::backend_traits<backend_stm32_ll>::dma_capability().supported)
      {
        return microhal::DmaConcept<backend_stm32_ll, secondary_dma_cfg>;
      }

      return true;
    }());
static_assert(microhal::DmaConcept<backend_kinetis, dma_cfg>);
static_assert(microhal::DmaConcept<backend_zephyr, dma_cfg>);
static_assert(microhal::DmaConcept<backend_nuttx, dma_cfg>);

#if defined(DMA2)
static_assert(microhal::backend_traits<backend_stm32_hal>::dma_capability().instance_count >= 2U);
static_assert(microhal::backend_traits<backend_stm32_ll>::dma_capability().instance_count >= 2U);
#endif

// PWM Configuration
using pwm_cfg = microhal::pwm::pwm_config<1U, 1U, 4200U>;

struct secondary_pwm_binding final
{
  static constexpr std::uintptr_t base_address{0x40000000U};
};

struct secondary_pwm_cfg final
{
  using binding = secondary_pwm_binding;

  static constexpr std::uintptr_t base_address{secondary_pwm_binding::base_address};
  static constexpr std::uint8_t timer_index{2U};
  static constexpr std::uint8_t channel{1U};
  static constexpr std::uint32_t frequency_hz{4200U};
  static constexpr microhal::pwm_polarity polarity{microhal::pwm_polarity::active_high};
  static constexpr bool enable_dead_time{false};
  static constexpr std::uint32_t prescaler{79U};
  static constexpr std::uint32_t period{999U};
};

static_assert(microhal::PwmConcept<backend_stm32_hal, pwm_cfg>);
static_assert(microhal::PwmConcept<backend_stm32_ll, pwm_cfg>);
static_assert(microhal::PwmConcept<backend_stm32_hal, secondary_pwm_cfg>);
static_assert(microhal::PwmConcept<backend_stm32_ll, secondary_pwm_cfg>);
static_assert(microhal::PwmConcept<backend_kinetis, pwm_cfg>);
static_assert(microhal::PwmConcept<backend_zephyr, pwm_cfg>);
static_assert(microhal::PwmConcept<backend_nuttx, pwm_cfg>);

#if defined(TIM2)
static_assert(microhal::backend_traits<backend_stm32_hal>::timer_capability().instance_count >= 2U);
static_assert(microhal::backend_traits<backend_stm32_ll>::timer_capability().instance_count >= 2U);
#endif

// Watchdog Configuration
using wdog_cfg = microhal::watchdog::watchdog_config<0U, 1000U>; // 1 second timeout

static_assert(microhal::WatchdogConcept<backend_stm32_hal, wdog_cfg>);
static_assert(microhal::WatchdogConcept<backend_stm32_ll, wdog_cfg>);
static_assert(microhal::WatchdogConcept<backend_kinetis, wdog_cfg>);
static_assert(microhal::WatchdogConcept<backend_zephyr, wdog_cfg>);
static_assert(microhal::WatchdogConcept<backend_nuttx, wdog_cfg>);

// I2S Configuration
using i2s_cfg = microhal::i2s::i2s_config<1U, microhal::i2s_mode::master_tx, microhal::i2s_standard::philips, 48000U>;

static_assert(microhal::I2sConcept<backend_stm32_hal, i2s_cfg>);
static_assert(microhal::I2sConcept<backend_stm32_ll, i2s_cfg>);
static_assert(microhal::I2sConcept<backend_kinetis, i2s_cfg>);
static_assert(microhal::I2sConcept<backend_zephyr, i2s_cfg>);
static_assert(microhal::I2sConcept<backend_nuttx, i2s_cfg>);

// RCC Configuration
using rcc_cfg = microhal::rcc::clock_config<microhal::rcc::clock_source::hse, 84000000U>;

static_assert(microhal::RccConcept<backend_stm32_hal, rcc_cfg>);
static_assert(microhal::RccConcept<backend_stm32_ll, rcc_cfg>);
static_assert(microhal::RccConcept<backend_kinetis, rcc_cfg>);
static_assert(microhal::RccConcept<backend_zephyr, rcc_cfg>);
static_assert(microhal::RccConcept<backend_nuttx, rcc_cfg>);

// CRC Configuration
using crc_cfg = microhal::crc::crc_config<0U, false, 0x04C11DB7U>; // CRC-32 polynomial

static_assert(microhal::CrcConcept<backend_stm32_hal, crc_cfg>);
static_assert(microhal::CrcConcept<backend_stm32_ll, crc_cfg>);
static_assert(microhal::CrcConcept<backend_kinetis, crc_cfg>);
static_assert(microhal::CrcConcept<backend_zephyr, crc_cfg>);
static_assert(microhal::CrcConcept<backend_nuttx, crc_cfg>);

// Verify capability queries work
static_assert(microhal::backend_traits<backend_stm32_hal>::adc_capability().supported);
static_assert(microhal::backend_traits<backend_stm32_hal>::dac_capability().supported);
static_assert(microhal::backend_traits<backend_stm32_hal>::pwm_capability().supported);
static_assert(microhal::backend_traits<backend_stm32_hal>::watchdog_capability().supported);
static_assert(microhal::backend_traits<backend_stm32_hal>::i2s_capability().supported);
static_assert(microhal::backend_traits<backend_stm32_hal>::rcc_capability().supported);
static_assert(microhal::backend_traits<backend_stm32_hal>::crc_capability().supported);

// Verify backend registry includes all peripherals
static_assert(microhal::backend_registry.size() == MICROHAL_EXPECTED_BACKEND_COUNT);

int main()
{
  return 0;
}
