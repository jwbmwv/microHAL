// SPDX-License-Identifier: Apache-2.0
/**
 * @file stm32_adc_dma_sampling.cpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

// STM32 ADC DMA Sampling Example
// Demonstrates: Multi-channel ADC DMA scans with a rolling average window

#include "microhal/adc/adc.hpp"
#include "microhal/gpio/gpio.hpp"
#include <array>
#include <cstdint>
#include <span>

namespace
{

using backend = microhal::stm32_hal_tag;

// ADC Configuration: ADC1, 12-bit resolution, continuous conversion
using adc_cfg = microhal::adc::adc_config<1U,                                  // ADC instance 1
                                          microhal::adc_resolution::bits_12,   // 12-bit resolution
                                          microhal::adc_sample_time::cycles_84 // 84 cycles sampling time
                                          >;

// ADC scan channels: temperature sensor, internal VREF, PA0 analog input
constexpr std::size_t num_channels = 3U;
constexpr std::size_t samples_per_channel = 10U;
constexpr std::array<std::uint8_t, num_channels> scan_channels{16U, 17U, 0U};

// DMA writes one scan result set at a time into latest_scan.
std::array<std::uint16_t, num_channels> latest_scan{};

// Rolling sample window used to average 10 DMA scans.
std::array<std::uint16_t, num_channels * samples_per_channel> adc_buffer{};

// Averaged results
struct sensor_readings
{
  std::uint16_t temperature_raw; // Internal temperature sensor (channel 16)
  std::uint16_t vrefint_raw;     // Internal voltage reference (channel 17)
  std::uint16_t analog_in_raw;   // External analog input (PA0 = channel 0)

  float temperature_celsius;
  float vdda_volts;
  float analog_in_volts;
};

sensor_readings latest_readings{};
volatile bool scan_complete = false;
std::size_t collected_scans = 0U;

// DMA transfer complete callback
void dma_transfer_complete_handler() noexcept
{
  scan_complete = true;
}

// Store one completed DMA scan into the rolling averaging window.
void store_latest_scan() noexcept
{
  const std::size_t sample_index = collected_scans % samples_per_channel;
  for (std::size_t channel_index = 0U; channel_index < num_channels; ++channel_index)
  {
    adc_buffer[sample_index * num_channels + channel_index] = latest_scan[channel_index];
  }

  ++collected_scans;
}

// Calculate average for one channel from circular buffer
std::uint16_t calculate_average(const std::size_t channel_index) noexcept
{
  std::uint32_t sum = 0U;
  for (std::size_t i = 0U; i < samples_per_channel; ++i)
  {
    sum += adc_buffer[i * num_channels + channel_index];
  }
  return static_cast<std::uint16_t>(sum / samples_per_channel);
}

// Convert raw ADC values to physical units
void process_readings() noexcept
{
  // Average raw values
  latest_readings.temperature_raw = calculate_average(0U);
  latest_readings.vrefint_raw = calculate_average(1U);
  latest_readings.analog_in_raw = calculate_average(2U);

  // STM32 internal temperature sensor: 25°C = ~760, slope ~2.5 mV/°C
  constexpr float temp_v25 = 0.76f;     // Voltage at 25°C
  constexpr float temp_slope = 0.0025f; // 2.5 mV/°C
  const float temp_voltage = (latest_readings.temperature_raw * 3.3f) / 4096.0f;
  latest_readings.temperature_celsius = ((temp_voltage - temp_v25) / temp_slope) + 25.0f;

  // Calculate VDDA from internal reference (typically 1.21V at 25°C)
  constexpr float vrefint_cal = 1.21f;
  latest_readings.vdda_volts = (vrefint_cal * 4096.0f) / latest_readings.vrefint_raw;

  // Convert external analog input
  latest_readings.analog_in_volts = (latest_readings.analog_in_raw * latest_readings.vdda_volts) / 4096.0f;
}

} // namespace

int main()
{
  // Compile-time validation
  static_assert(microhal::AdcConcept<backend, adc_cfg>);

  // Configure GPIO pin PA0 as analog input
  using analog_pin = microhal::gpio::pin_config<0U, 0U, microhal::gpio_mode::analog>;
  if (!microhal::is_ok(microhal::gpio::pin<backend, analog_pin>::configure()))
  {
    return 1;
  }

  // Initialize ADC
  if (!microhal::is_ok(microhal::adc::converter<backend, adc_cfg>::initialize()))
  {
    return 1;
  }

  // Start the first multi-channel DMA scan.
  if (!microhal::is_ok(
          microhal::adc::converter<backend, adc_cfg>::template start_dma_scan<dma_transfer_complete_handler>(
              std::span{scan_channels}, std::span{latest_scan})))
  {
    return 1;
  }

  // Main processing loop
  while (true)
  {
    if (scan_complete)
    {
      scan_complete = false;
      store_latest_scan();

      if (collected_scans >= samples_per_channel)
      {
        process_readings();
        collected_scans = 0U;
      }

      // Re-arm the DMA scan so the rolling average window keeps filling. A re-arm failure is
      // terminal for this example: without it no further conversions will ever complete.
      if (!microhal::is_ok(
              microhal::adc::converter<backend, adc_cfg>::template start_dma_scan<dma_transfer_complete_handler>(
                  std::span{scan_channels}, std::span{latest_scan})))
      {
        return 1;
      }

      // Use latest_readings.temperature_celsius, .vdda_volts, .analog_in_volts
      // Example: Send via UART, store to Flash, trigger actions, etc.
    }

    // Could enter low-power mode here while waiting for DMA interrupt
    // __WFI(); // Wait for interrupt
  }

  return 0;
}
