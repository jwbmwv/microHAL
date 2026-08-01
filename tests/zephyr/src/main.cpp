// SPDX-License-Identifier: Apache-2.0
/**
 * @file main.cpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#include <array>
#include <span>

#include <zephyr/drivers/gpio.h>
#include <zephyr/ztest.h>

#include "examples/zephyr_board_bindings.hpp"
#include "microhal/core/backend_selector.hpp"

namespace board = microhal::examples::zephyr_board;

using zephyr_backend = microhal::zephyr_tag;

static_assert(microhal::GpioPinConcept<zephyr_backend, board::status_led_cfg>);
static_assert(microhal::UartConcept<zephyr_backend, board::console_uart_cfg>);

ZTEST(microhal_renode, test_status_led_gpio_binding)
{
  zassert_true(device_is_ready(board::status_led_binding::spec.port), "led0 GPIO device must be ready");

  board::status_led::configure();
  board::status_led::clear();
  zassert_equal(gpio_pin_get_dt(&board::status_led_binding::spec), 0, "led0 should read low after clear");

  board::status_led::set();
  zassert_equal(gpio_pin_get_dt(&board::status_led_binding::spec), 1, "led0 should read high after set");

  board::status_led::clear();
}

ZTEST(microhal_renode, test_console_uart_binding)
{
  const bool initialized =
      microhal::backend_dispatch_table<zephyr_backend>::uart::template initialize<board::console_uart_cfg>();
  zassert_true(initialized, "console UART should be ready through the Zephyr binding");

  constexpr std::array<std::uint8_t, 24> payload{{'m', 'i', 'c', 'r', 'o', 'H', 'A', 'L', ' ',  'R',  'e', 'n',
                                                  'o', 'd', 'e', ' ', 't', 'e', 's', 't', '\r', '\n', '>', ' '}};

  const bool transmitted =
      microhal::backend_dispatch_table<zephyr_backend>::uart::template write_blocking<board::console_uart_cfg>(
          std::span<const std::uint8_t>{payload});
  zassert_true(transmitted, "console UART write should succeed");
}

ZTEST_SUITE(microhal_renode, nullptr, nullptr, nullptr, nullptr, nullptr);
