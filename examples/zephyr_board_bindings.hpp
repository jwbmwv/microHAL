// SPDX-License-Identifier: Apache-2.0
/**
 * @file zephyr_board_bindings.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#ifdef CONFIG_ZEPHYR_KERNEL

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include "microhal/gpio/gpio.hpp"
#include "microhal/uart/uart.hpp"

#if !DT_NODE_EXISTS(DT_ALIAS(led0))
#error "microHAL Zephyr board bindings require DT_ALIAS(led0)."
#endif

#if !DT_HAS_CHOSEN(zephyr_console)
#error "microHAL Zephyr board bindings require DT_CHOSEN(zephyr_console)."
#endif

namespace microhal::examples::zephyr_board
{

struct status_led_binding final
{
  static constexpr gpio_dt_spec spec = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
};

struct status_led_cfg final
{
  using binding = status_led_binding;

  static constexpr microhal::gpio_mode mode{microhal::gpio_mode::output};
  static constexpr microhal::gpio_pull pull{microhal::gpio_pull::none};
  static constexpr microhal::gpio_speed speed{microhal::gpio_speed::low};
  static constexpr microhal::gpio_interrupt_trigger interrupt_trigger{microhal::gpio_interrupt_trigger::none};
};

using status_led = microhal::gpio::pin<microhal::zephyr_tag, status_led_cfg>;

#if DT_NODE_EXISTS(DT_ALIAS(sw0))
struct user_button_binding final
{
  static constexpr gpio_dt_spec spec = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
};

struct user_button_cfg final
{
  using binding = user_button_binding;

  static constexpr microhal::gpio_mode mode{microhal::gpio_mode::input};
  static constexpr microhal::gpio_pull pull{microhal::gpio_pull::up};
  static constexpr microhal::gpio_speed speed{microhal::gpio_speed::low};
  static constexpr microhal::gpio_interrupt_trigger interrupt_trigger{microhal::gpio_interrupt_trigger::none};
};

using user_button = microhal::gpio::pin<microhal::zephyr_tag, user_button_cfg>;
#endif

struct console_uart_binding final
{
  static constexpr auto node_id = DT_CHOSEN(zephyr_console);
};

struct console_uart_cfg final
{
  using binding = console_uart_binding;

  static constexpr std::uint32_t baud_rate{115200U};
  static constexpr microhal::uart_parity parity{microhal::uart_parity::none};
  static constexpr microhal::uart_stop_bits stop_bits{microhal::uart_stop_bits::one};
  static constexpr bool enable_dma{false};
  static constexpr bool enable_interrupts{true};
  static constexpr bool require_fifo{false};
  static constexpr bool require_lpuart{false};
};

using console_uart = microhal::uart::port<microhal::zephyr_tag, console_uart_cfg>;

} // namespace microhal::examples::zephyr_board

#endif
