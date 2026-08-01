// SPDX-License-Identifier: Apache-2.0
/**
 * @file stm32_uart_c_shim.cpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#include "stm32_uart_c_shim.h"

#include <cstddef>
#include <cstdint>
#include <span>

#include "microhal/uart/uart.hpp"

namespace microhal::examples::stm32_uart_c_shim
{

struct console_uart_binding final
{
  static constexpr std::uintptr_t base_address{0x40004400U};
};

struct console_uart_cfg final
{
  using binding = console_uart_binding;

  static constexpr std::uintptr_t base_address{0x40004400U};
  static constexpr std::uint32_t baud_rate{115200U};
  static constexpr microhal::uart_parity parity{microhal::uart_parity::none};
  static constexpr microhal::uart_stop_bits stop_bits{microhal::uart_stop_bits::one};
  static constexpr bool enable_dma{false};
  static constexpr bool enable_interrupts{false};
  static constexpr bool require_fifo{false};
  static constexpr bool require_lpuart{false};
};

using console_uart = microhal::uart::port<microhal::stm32_hal_tag, console_uart_cfg>;

// The C enum must stay numerically identical to the C++ one, otherwise callers would silently
// misinterpret failures.
static_assert(static_cast<int>(microhal::status::ok) == MICROHAL_STATUS_OK);
static_assert(static_cast<int>(microhal::status::error) == MICROHAL_STATUS_ERROR);
static_assert(static_cast<int>(microhal::status::timeout) == MICROHAL_STATUS_TIMEOUT);
static_assert(static_cast<int>(microhal::status::busy) == MICROHAL_STATUS_BUSY);
static_assert(static_cast<int>(microhal::status::unsupported) == MICROHAL_STATUS_UNSUPPORTED);
static_assert(static_cast<int>(microhal::status::invalid_argument) == MICROHAL_STATUS_INVALID_ARGUMENT);
static_assert(static_cast<int>(microhal::status::io_error) == MICROHAL_STATUS_IO_ERROR);

constexpr microhal_status_t to_c_status(const microhal::status value) noexcept
{
  return static_cast<microhal_status_t>(static_cast<int>(value));
}

} // namespace microhal::examples::stm32_uart_c_shim

extern "C" microhal_status_t microhal_example_console_uart_initialize(void)
{
  namespace shim = microhal::examples::stm32_uart_c_shim;
  return shim::to_c_status(shim::console_uart::initialize());
}

extern "C" microhal_status_t microhal_example_console_uart_write(const std::uint8_t* data, const std::size_t size)
{
  namespace shim = microhal::examples::stm32_uart_c_shim;

  if ((data == nullptr) || (size == 0U))
  {
    return MICROHAL_STATUS_INVALID_ARGUMENT;
  }

  return shim::to_c_status(shim::console_uart::write_blocking(std::span<const std::uint8_t>{data, size}));
}

extern "C" microhal_status_t microhal_example_console_uart_read(std::uint8_t* data, const std::size_t size)
{
  namespace shim = microhal::examples::stm32_uart_c_shim;

  if ((data == nullptr) || (size == 0U))
  {
    return MICROHAL_STATUS_INVALID_ARGUMENT;
  }

  return shim::to_c_status(shim::console_uart::read_blocking(std::span<std::uint8_t>{data, size}));
}
