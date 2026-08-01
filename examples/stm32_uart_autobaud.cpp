// SPDX-License-Identifier: Apache-2.0
/**
 * @file stm32_uart_autobaud.cpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#include "microhal/gpio/gpio.hpp"
#include "microhal/timers/timer.hpp"
#include "microhal/uart/uart.hpp"
#include "microhal/uart/uart_autobaud.hpp"

namespace
{

using backend = microhal::stm32_hal_tag;
using rx_sense_pin = microhal::gpio::pin_config<0U, 10U, microhal::gpio_mode::input, microhal::gpio_pull::up>;
using timer_cfg = microhal::timers::timer_config<2U, 1000000U, microhal::timer_counter_mode::up,
                                                 microhal::timer_clock_source::internal, false, true>;
using uart_cfg = microhal::uart::port_config<1U, 115200U, microhal::uart_parity::none, microhal::uart_stop_bits::one,
                                             true, true, true, false>;
using autobaud_cfg = microhal::uart::autobaud_config<rx_sense_pin, timer_cfg, 9600U, 1000000U, false, true>;

void on_autobaud_ready(const microhal::uart::autobaud_measurement measurement) noexcept
{
  if (measurement.stable)
  {
    microhal::uart::autobaud_helper<backend, uart_cfg, autobaud_cfg>::apply(measurement);
  }
}

} // namespace

int main()
{
  static_assert(microhal::AutobaudCapableConcept<backend, uart_cfg, autobaud_cfg>);
  return 0;
}