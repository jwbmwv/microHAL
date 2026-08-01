// SPDX-License-Identifier: Apache-2.0
/**
 * @file traits.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "microhal/core/status.hpp"

namespace microhal
{

template <typename...> inline constexpr bool dependent_false_v = false;

enum class vendor_kind : std::uint8_t
{
  stm32,
  kinetis,
  zephyr,
  nuttx,
  renesas_ra,
  nordic_nrf,
  ti_simplelink,
  mock,
  generic
};

enum class backend_kind : std::uint8_t
{
  stm32_cube_hal,
  stm32_cube_ll,
  kinetis_espresso,
  zephyr_driver_model,
  nuttx_driver_model,
  // The three entries below name vendor SDKs that microHAL does not implement yet. They exist so
  // that downstream selection code can already spell the target; there is no `backend_traits`
  // specialisation for them, so using one is a compile error rather than a silent no-op.
  renesas_ra_fsp,
  nordic_nrfx,
  ti_simplelink_driverlib,
  mock_recording,
  custom
};

enum class peripheral_kind : std::uint8_t
{
  gpio,
  i2c,
  spi,
  qspi,
  uart,
  rtc,
  flash,
  timer,
  adc,
  dac,
  can,
  usb,
  ethernet,
  sd,
  dma,
  pwm,
  watchdog,
  i2s,
  rcc,
  crc
};

enum class gpio_mode : std::uint8_t
{
  input,
  output,
  alternate,
  analog
};

enum class gpio_pull : std::uint8_t
{
  none,
  up,
  down
};

enum class gpio_speed : std::uint8_t
{
  low,
  medium,
  high,
  very_high
};

enum class gpio_interrupt_trigger : std::uint8_t
{
  none,
  rising,
  falling,
  both
};

enum class i2c_address_mode : std::uint8_t
{
  seven_bit,
  ten_bit
};

enum class spi_duplex_mode : std::uint8_t
{
  full,
  half,
  simplex_rx,
  simplex_tx
};

enum class spi_clock_polarity : std::uint8_t
{
  idle_low,
  idle_high
};

enum class spi_clock_phase : std::uint8_t
{
  first_edge,
  second_edge
};

enum class spi_bit_order : std::uint8_t
{
  msb_first,
  lsb_first
};

enum class uart_parity : std::uint8_t
{
  none,
  even,
  odd
};

enum class uart_stop_bits : std::uint8_t
{
  one,
  one_and_half,
  two
};

enum class timer_counter_mode : std::uint8_t
{
  up,
  down,
  center_aligned
};

enum class timer_clock_source : std::uint8_t
{
  internal,
  external,
  core_cycle_counter
};

enum class flash_operation : std::uint8_t
{
  erase,
  program,
  read
};

enum class adc_resolution : std::uint8_t
{
  bits_6 = 6,
  bits_8 = 8,
  bits_10 = 10,
  bits_12 = 12,
  bits_14 = 14,
  bits_16 = 16
};

enum class adc_sample_time : std::uint8_t
{
  cycles_3,
  cycles_15,
  cycles_28,
  cycles_56,
  cycles_84,
  cycles_112,
  cycles_144,
  cycles_480
};

enum class dac_resolution : std::uint8_t
{
  bits_8 = 8,
  bits_12 = 12
};

enum class can_mode : std::uint8_t
{
  normal,
  loopback,
  silent,
  silent_loopback
};

namespace can
{

struct message final
{
  std::uint32_t id{0U};
  std::uint8_t data[8]{};
  std::uint8_t dlc{0U};
  bool extended_id{false};
  bool remote_frame{false};
};

} // namespace can

enum class usb_speed : std::uint8_t
{
  low,
  full,
  high
};

enum class ethernet_link_speed : std::uint8_t
{
  mbps10,
  mbps100,
  mbps1000
};

enum class ethernet_duplex : std::uint8_t
{
  half,
  full
};

enum class ethernet_media_interface : std::uint8_t
{
  mii,
  rmii
};

enum class sd_bus_width : std::uint8_t
{
  one = 1,
  four = 4,
  eight = 8
};

enum class sd_timing_mode : std::uint8_t
{
  default_speed,
  high_speed,
  uhs
};

enum class i2s_mode : std::uint8_t
{
  master_tx,
  master_rx,
  slave_tx,
  slave_rx
};

enum class i2s_standard : std::uint8_t
{
  philips,
  msb_justified,
  lsb_justified,
  pcm_short,
  pcm_long
};

enum class pwm_polarity : std::uint8_t
{
  active_high,
  active_low
};

inline constexpr std::uint8_t gpio_no_alternate = 0xFFU;

struct stm32_hal_tag final
{
};
struct stm32_ll_tag final
{
};
struct kinetis_espresso_tag final
{
};
struct zephyr_tag final
{
};
struct nuttx_tag final
{
};
struct renesas_ra_tag final
{
};
struct nordic_nrf_tag final
{
};
struct ti_simplelink_tag final
{
};
/// @brief Host-side recording backend used for unit-testing drivers without hardware.
struct mock_tag final
{
};

/**
 * @brief Base capability with common peripheral features.
 */
struct base_capability
{
  bool supported{false};
  bool blocking{false};
  bool non_blocking{false};
  bool isr{false};
  bool dma{false};
  std::uint32_t instance_count{0U};
  std::uint32_t max_frequency_hz{0U};
};

/**
 * @brief GPIO specific capabilities.
 */
struct gpio_capability
{
  bool supported{false};
  bool blocking{false};
  bool non_blocking{false};
  bool isr{false};
  bool dma{false};
  std::uint32_t instance_count{0U};
  std::uint32_t max_frequency_hz{0U};
  bool alternate_function{false};
  std::uint32_t alternate_function_count{0U};
  std::uint32_t port_count{0U};
  std::uint32_t pins_per_port{0U};
};

/**
 * @brief I2C specific capabilities.
 */
struct i2c_capability
{
  bool supported{false};
  bool blocking{false};
  bool non_blocking{false};
  bool isr{false};
  bool dma{false};
  std::uint32_t instance_count{0U};
  std::uint32_t max_frequency_hz{0U};
};

/**
 * @brief SPI specific capabilities.
 */
struct spi_capability
{
  bool supported{false};
  bool blocking{false};
  bool non_blocking{false};
  bool isr{false};
  bool dma{false};
  std::uint32_t instance_count{0U};
  std::uint32_t max_frequency_hz{0U};
};

/**
 * @brief QSPI/OctoSPI specific capabilities.
 */
struct qspi_capability
{
  bool supported{false};
  bool blocking{false};
  bool non_blocking{false};
  bool isr{false};
  bool dma{false};
  std::uint32_t instance_count{0U};
  std::uint32_t max_frequency_hz{0U};
  bool quad_mode{false};
  bool octo_mode{false};
  bool ddr_mode{false};
};

/**
 * @brief UART/USART/LPUART specific capabilities.
 */
struct uart_capability
{
  bool supported{false};
  bool blocking{false};
  bool non_blocking{false};
  bool isr{false};
  bool dma{false};
  std::uint32_t instance_count{0U};
  std::uint32_t max_frequency_hz{0U};
  bool fifo{false};
  bool lpuart{false};
  bool autobaud{false};
  std::uint32_t fifo_depth{0U};
};

/**
 * @brief RTC specific capabilities.
 */
struct rtc_capability
{
  bool supported{false};
  bool blocking{false};
  bool non_blocking{false};
  bool isr{false};
  bool dma{false};
  std::uint32_t instance_count{0U};
  std::uint32_t max_frequency_hz{0U};
  bool periodic{false};
  bool alarm{false};
};

/**
 * @brief Flash specific capabilities.
 */
struct flash_capability
{
  bool supported{false};
  bool blocking{false};
  bool non_blocking{false};
  bool isr{false};
  bool dma{false};
  std::uint32_t instance_count{0U};
  std::uint32_t max_frequency_hz{0U};
  std::uint32_t alignment_bytes{0U};
  std::uint32_t erase_bytes{0U};
  std::uint32_t total_size_kb{0U};
};

/**
 * @brief Timer specific capabilities.
 */
struct timer_capability
{
  bool supported{false};
  bool blocking{false};
  bool non_blocking{false};
  bool isr{false};
  bool dma{false};
  std::uint32_t instance_count{0U};
  std::uint32_t max_frequency_hz{0U};
  bool capture_compare{false};
  bool periodic{false};
  bool pwm{false};
  std::uint32_t channel_count{0U};
};

/**
 * @brief ADC specific capabilities.
 */
struct adc_capability
{
  bool supported{false};
  bool blocking{false};
  bool non_blocking{false};
  bool isr{false};
  bool dma{false};
  std::uint32_t instance_count{0U};
  std::uint32_t max_frequency_hz{0U};
  std::uint32_t resolution_bits{12U};
  std::uint32_t channel_count{0U};
  bool differential{false};
  bool oversampling{false};
  bool injected{false};
};

/**
 * @brief DAC specific capabilities.
 */
struct dac_capability
{
  bool supported{false};
  bool blocking{false};
  bool non_blocking{false};
  bool isr{false};
  bool dma{false};
  std::uint32_t instance_count{0U};
  std::uint32_t max_frequency_hz{0U};
  std::uint32_t resolution_bits{12U};
  std::uint32_t channel_count{0U};
  bool dual_mode{false};
  bool waveform_generation{false};
};

/**
 * @brief CAN/CAN FD specific capabilities.
 */
struct can_capability
{
  bool supported{false};
  bool blocking{false};
  bool non_blocking{false};
  bool isr{false};
  bool dma{false};
  std::uint32_t instance_count{0U};
  std::uint32_t max_frequency_hz{0U};
  bool can_fd{false};
  bool time_triggered{false};
  std::uint32_t mailbox_count{0U};
  std::uint32_t filter_banks{0U};
};

/**
 * @brief USB specific capabilities.
 */
struct usb_capability
{
  bool supported{false};
  bool blocking{false};
  bool non_blocking{false};
  bool isr{false};
  bool dma{false};
  std::uint32_t instance_count{0U};
  std::uint32_t max_frequency_hz{0U};
  bool device{false};
  bool host{false};
  bool otg{false};
  bool high_speed{false};
  std::uint32_t endpoint_count{0U};
};

/**
 * @brief Ethernet MAC specific capabilities.
 */
struct ethernet_capability
{
  bool supported{false};
  bool blocking{false};
  bool non_blocking{false};
  bool isr{false};
  bool dma{false};
  std::uint32_t instance_count{0U};
  std::uint32_t max_frequency_hz{0U};
  bool checksum_offload{false};
  bool ptp_timestamp{false};
  bool vlan_filter{false};
  bool gigabit{false};
  std::uint32_t mtu_bytes{0U};
};

/**
 * @brief SD/MMC host specific capabilities.
 */
struct sd_capability
{
  bool supported{false};
  bool blocking{false};
  bool non_blocking{false};
  bool isr{false};
  bool dma{false};
  std::uint32_t instance_count{0U};
  std::uint32_t max_frequency_hz{0U};
  bool high_speed{false};
  bool uhs{false};
  bool bus_width_4{false};
  bool bus_width_8{false};
  std::uint32_t block_size_bytes{512U};
};

/**
 * @brief DMA specific capabilities.
 */
struct dma_capability
{
  bool supported{false};
  bool blocking{false};
  bool non_blocking{false};
  bool isr{false};
  bool dma{false};
  std::uint32_t instance_count{0U};
  std::uint32_t max_frequency_hz{0U};
  std::uint32_t stream_count{0U};
  std::uint32_t channel_count{0U};
  bool mem_to_mem{false};
  bool fifo{false};
  bool double_buffer{false};
};

/**
 * @brief PWM specific capabilities.
 */
struct pwm_capability
{
  bool supported{false};
  bool blocking{false};
  bool non_blocking{false};
  bool isr{false};
  bool dma{false};
  std::uint32_t instance_count{0U};
  std::uint32_t max_frequency_hz{0U};
  std::uint32_t channel_count{0U};
  bool complementary_output{false};
  bool dead_time{false};
};

/**
 * @brief Watchdog specific capabilities.
 */
struct watchdog_capability
{
  bool supported{false};
  bool blocking{false};
  bool non_blocking{false};
  bool isr{false};
  bool dma{false};
  std::uint32_t instance_count{0U};
  std::uint32_t max_frequency_hz{0U};
  bool independent{false};
  bool window{false};
  std::uint32_t min_timeout_ms{0U};
  std::uint32_t max_timeout_ms{0U};
};

/**
 * @brief I2S specific capabilities.
 */
struct i2s_capability
{
  bool supported{false};
  bool blocking{false};
  bool non_blocking{false};
  bool isr{false};
  bool dma{false};
  std::uint32_t instance_count{0U};
  std::uint32_t max_frequency_hz{0U};
  bool full_duplex{false};
  bool master_clock_output{false};
  std::uint32_t max_sample_rate{0U};
};

/**
 * @brief RCC (clock control) specific capabilities.
 */
struct rcc_capability
{
  bool supported{false};
  bool blocking{false};
  bool non_blocking{false};
  bool isr{false};
  bool dma{false};
  std::uint32_t instance_count{0U};
  std::uint32_t max_frequency_hz{0U};
  bool pll_config{false};
  bool hse{false};
  bool lse{false};
  std::uint32_t max_system_clock_hz{0U};
};

/**
 * @brief CRC/RNG specific capabilities.
 */
struct crc_capability
{
  bool supported{false};
  bool blocking{false};
  bool non_blocking{false};
  bool isr{false};
  bool dma{false};
  std::uint32_t instance_count{0U};
  std::uint32_t max_frequency_hz{0U};
  bool hardware_crc32{false};
  bool programmable_polynomial{false};
  bool random_number_generator{false};
};

/**
 * @brief Lightweight constexpr view of a registered backend.
 */
struct backend_descriptor final
{
  std::string_view name{};
  vendor_kind vendor{};
  backend_kind kind{};
};

template <typename BackendTag> struct backend_traits;

} // namespace microhal
