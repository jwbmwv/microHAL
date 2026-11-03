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
#include "microhal/dma/dma.hpp"
#include "microhal/flash/flash.hpp"
#include "microhal/i2c/i2c.hpp"
#include "microhal/timers/timer.hpp"

namespace board = microhal::examples::zephyr_board;

using zephyr_backend = microhal::zephyr_tag;

struct timer_compile_config final
{
  static constexpr const struct device* device{nullptr};
};

struct dma_compile_config final
{
  static constexpr const struct device* device{nullptr};
  static constexpr std::uint8_t channel{0U};
};

struct i2c_compile_config final
{
  static constexpr const struct device* device{nullptr};
  static constexpr std::uint8_t instance_index{0U};
  static constexpr std::uint32_t clock_hz{100'000U};
  static constexpr microhal::i2c_address_mode address_mode{microhal::i2c_address_mode::seven_bit};
  static constexpr bool enable_dma{false};
  static constexpr bool enable_interrupts{false};
};

using flash_compile_config = microhal::flash::flash_config<4U, 4'096U>;

static_assert(microhal::GpioPinConcept<zephyr_backend, board::status_led_cfg>);
static_assert(microhal::UartConcept<zephyr_backend, board::console_uart_cfg>);
static_assert(microhal::I2cMasterConcept<zephyr_backend, i2c_compile_config>);
static_assert(microhal::FlashConcept<zephyr_backend, flash_compile_config>);

void dma_compile_callback(const struct device*, void*, std::uint32_t, int) noexcept {}

void zephyr_extended_peripheral_compile_smoke() noexcept
{
  using timer_api = microhal::backend_traits<zephyr_backend>::timer_api;
  using dma_api = microhal::backend_traits<zephyr_backend>::dma_api;
  std::array<std::uint8_t, 1U> tx{};
  std::array<std::uint8_t, 1U> rx{};
  (void)timer_api::start<timer_compile_config>();
  (void)timer_api::stop<timer_compile_config>();
  (void)timer_api::set_period<timer_compile_config>(1000U);
  (void)dma_api::template start_transfer<dma_compile_config, dma_compile_callback>(0U, 0U, 1U);
  (void)dma_api::transfer_status<dma_compile_config>();
  (void)microhal::i2c::master<zephyr_backend, i2c_compile_config>::write_read_blocking(0x48U, tx, rx);
  (void)microhal::flash::controller<zephyr_backend, flash_compile_config>::erase_region(0U);
}

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
