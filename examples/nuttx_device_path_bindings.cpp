// SPDX-License-Identifier: Apache-2.0
// Example: binding NuttX device-node paths to microHAL configs.

#include "microhal/core/capabilities.hpp"

#include <array>
#include <cstdint>

namespace
{

using backend = microhal::nuttx_tag;

struct i2c0_binding final
{
  static constexpr const char* device_path = "/dev/i2c0";
};

struct spi0_binding final
{
  static constexpr const char* device_path = "/dev/spi0";
};

struct sd0_binding final
{
  static constexpr const char* device_path = "/dev/mmcsd0";
};

struct i2c_cfg final
{
  using binding = i2c0_binding;
  static constexpr std::uint8_t instance_index{0U};
  static constexpr std::uint32_t clock_hz{400'000U};
  static constexpr microhal::i2c_address_mode address_mode{microhal::i2c_address_mode::seven_bit};
  static constexpr bool enable_dma{false};
  static constexpr bool enable_interrupts{false};
};

struct spi_cfg final
{
  using binding = spi0_binding;
  static constexpr std::uint8_t instance_index{0U};
  static constexpr std::uint32_t clock_hz{8'000'000U};
  static constexpr microhal::spi_duplex_mode duplex_mode{microhal::spi_duplex_mode::full};
  static constexpr microhal::spi_clock_polarity clock_polarity{microhal::spi_clock_polarity::idle_low};
  static constexpr microhal::spi_clock_phase clock_phase{microhal::spi_clock_phase::first_edge};
  static constexpr microhal::spi_bit_order bit_order{microhal::spi_bit_order::msb_first};
  static constexpr bool enable_dma{false};
  static constexpr bool enable_interrupts{false};
};

struct sd_cfg final
{
  using binding = sd0_binding;
  static constexpr std::uint8_t instance_index{0U};
  static constexpr std::uint32_t clock_hz{25'000'000U};
  static constexpr microhal::sd_bus_width bus_width{microhal::sd_bus_width::four};
  static constexpr microhal::sd_timing_mode timing_mode{microhal::sd_timing_mode::default_speed};
  static constexpr bool enable_dma{true};
  static constexpr bool enable_interrupts{true};
};

} // namespace

int main()
{
  std::array<std::uint8_t, 8U> rx{};
  const std::array<std::uint8_t, 8U> tx{0x9FU};

  using traits = microhal::backend_traits<backend>;
  (void)traits::i2c_api::template initialize<i2c_cfg>();
  (void)traits::i2c_api::template read_blocking<i2c_cfg>(rx, 0x48U);
  (void)traits::spi_api::template initialize<spi_cfg>();
  (void)traits::spi_api::template transfer<spi_cfg>(tx, rx);
  (void)traits::sd_api::template initialize<sd_cfg>();
  (void)traits::sd_api::template capacity_blocks<sd_cfg>();

  return 0;
}
