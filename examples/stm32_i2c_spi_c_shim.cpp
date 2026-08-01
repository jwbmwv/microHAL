// SPDX-License-Identifier: Apache-2.0
/**
 * @file stm32_i2c_spi_c_shim.cpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#include "stm32_i2c_spi_c_shim.h"

#include <cstddef>
#include <cstdint>
#include <span>

#include "microhal/i2c/i2c.hpp"
#include "microhal/spi/spi.hpp"

namespace microhal::examples::stm32_i2c_spi_c_shim
{

struct sensor_i2c_binding final
{
  static constexpr std::uintptr_t base_address{0x40005800U};
  static constexpr std::uint32_t timing{0x00000E14U};
};

struct sensor_i2c_cfg final
{
  using binding = sensor_i2c_binding;

  static constexpr std::uintptr_t base_address{sensor_i2c_binding::base_address};
  static constexpr std::uint32_t timing{sensor_i2c_binding::timing};
  static constexpr std::uint32_t clock_hz{400000U};
  static constexpr microhal::i2c_address_mode address_mode{microhal::i2c_address_mode::seven_bit};
  static constexpr bool enable_dma{false};
  static constexpr bool enable_interrupts{false};
};

struct sensor_spi_binding final
{
  static constexpr std::uintptr_t base_address{0x40003C00U};
};

struct sensor_spi_cfg final
{
  using binding = sensor_spi_binding;

  static constexpr std::uintptr_t base_address{sensor_spi_binding::base_address};
  static constexpr std::uint32_t clock_hz{1000000U};
  static constexpr microhal::spi_duplex_mode duplex_mode{microhal::spi_duplex_mode::full};
  static constexpr microhal::spi_clock_polarity clock_polarity{microhal::spi_clock_polarity::idle_low};
  static constexpr microhal::spi_clock_phase clock_phase{microhal::spi_clock_phase::first_edge};
  static constexpr microhal::spi_bit_order bit_order{microhal::spi_bit_order::msb_first};
  static constexpr bool enable_dma{false};
  static constexpr bool enable_interrupts{false};
};

using sensor_i2c = microhal::i2c::master<microhal::stm32_hal_tag, sensor_i2c_cfg>;
using sensor_spi = microhal::spi::master<microhal::stm32_hal_tag, sensor_spi_cfg>;

// Keep the C enum numerically identical to the C++ one so failures cannot be misread.
static_assert(static_cast<int>(microhal::status::ok) == MICROHAL_STATUS_OK);
static_assert(static_cast<int>(microhal::status::invalid_argument) == MICROHAL_STATUS_INVALID_ARGUMENT);
static_assert(static_cast<int>(microhal::status::io_error) == MICROHAL_STATUS_IO_ERROR);

constexpr microhal_status_t to_c_status(const microhal::status value) noexcept
{
  return static_cast<microhal_status_t>(static_cast<int>(value));
}

} // namespace microhal::examples::stm32_i2c_spi_c_shim

extern "C" microhal_status_t microhal_example_sensor_i2c_initialize(void)
{
  namespace shim = microhal::examples::stm32_i2c_spi_c_shim;
  return shim::to_c_status(shim::sensor_i2c::initialize());
}

extern "C" microhal_status_t microhal_example_sensor_i2c_write(const std::uint16_t address, const std::uint8_t* data,
                                                               const std::size_t size)
{
  namespace shim = microhal::examples::stm32_i2c_spi_c_shim;

  if ((data == nullptr) || (size == 0U))
  {
    return MICROHAL_STATUS_INVALID_ARGUMENT;
  }

  return shim::to_c_status(shim::sensor_i2c::write_blocking(address, std::span<const std::uint8_t>{data, size}));
}

extern "C" microhal_status_t microhal_example_sensor_i2c_read(const std::uint16_t address, std::uint8_t* data,
                                                              const std::size_t size)
{
  namespace shim = microhal::examples::stm32_i2c_spi_c_shim;

  if ((data == nullptr) || (size == 0U))
  {
    return MICROHAL_STATUS_INVALID_ARGUMENT;
  }

  return shim::to_c_status(shim::sensor_i2c::read_blocking(address, std::span<std::uint8_t>{data, size}));
}

extern "C" microhal_status_t microhal_example_sensor_spi_initialize(void)
{
  namespace shim = microhal::examples::stm32_i2c_spi_c_shim;
  return shim::to_c_status(shim::sensor_spi::initialize());
}

extern "C" microhal_status_t microhal_example_sensor_spi_transfer(const std::uint8_t* tx_data, std::uint8_t* rx_data,
                                                                  const std::size_t size)
{
  namespace shim = microhal::examples::stm32_i2c_spi_c_shim;

  if ((tx_data == nullptr) || (rx_data == nullptr) || (size == 0U))
  {
    return MICROHAL_STATUS_INVALID_ARGUMENT;
  }

  return shim::to_c_status(
      shim::sensor_spi::transfer(std::span<const std::uint8_t>{tx_data, size}, std::span<std::uint8_t>{rx_data, size}));
}
