// SPDX-License-Identifier: Apache-2.0
/**
 * @file stm32_can_communication.cpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

// STM32 CAN Bus Communication Example
// Demonstrates: CAN initialization, filter configuration, RX interrupts, periodic TX

#include "microhal/can/can.hpp"
#include "microhal/gpio/gpio.hpp"
#include <array>
#include <cstdint>

namespace
{

using backend = microhal::stm32_hal_tag;

// CAN Configuration: CAN1, 500 kbps, loopback disabled
using can_cfg = microhal::can::can_config<1U,                         // CAN1
                                          500000U,                    // 500 kbps
                                          microhal::can_mode::normal, // Normal mode
                                          false                       // Loopback disabled
                                          >;

// GPIO Configuration: CAN1_TX (PA12), CAN1_RX (PA11)
using can_tx_pin = microhal::gpio::pin_config<0U, 12U, microhal::gpio_mode::alternate, microhal::gpio_pull::none,
                                              microhal::gpio_speed::very_high, 9U>; // AF9

using can_rx_pin = microhal::gpio::pin_config<0U, 11U, microhal::gpio_mode::alternate, microhal::gpio_pull::none,
                                              microhal::gpio_speed::very_high, 9U>; // AF9

constexpr std::size_t rx_queue_size = 16U;
std::array<microhal::can::message, rx_queue_size> rx_queue{};
volatile std::size_t rx_head = 0U;
volatile std::size_t rx_tail = 0U;

// CAN RX interrupt handler
void can_rx_handler() noexcept
{
  // Read received message
  microhal::can::message msg{};

  if (microhal::is_ok(microhal::can::bus<backend, can_cfg>::receive(msg)))
  {
    // Add to queue
    const std::size_t next_head = (rx_head + 1U) % rx_queue_size;
    if (next_head != rx_tail) // Check for queue overflow
    {
      rx_queue[rx_head] = msg;
      rx_head = next_head;
    }
  }
}

// Check if RX queue has messages
bool has_rx_message() noexcept
{
  return rx_head != rx_tail;
}

// Dequeue received message
microhal::can::message get_rx_message() noexcept
{
  if (!has_rx_message())
  {
    return {};
  }

  const microhal::can::message msg = rx_queue[rx_tail];
  rx_tail = (rx_tail + 1U) % rx_queue_size;
  return msg;
}

// Send CAN message
bool send_message(const std::uint32_t id, const std::uint8_t* data, const std::uint8_t dlc) noexcept
{
  microhal::can::message msg{};
  msg.id = id;
  msg.extended_id = (id > 0x7FFU);
  msg.dlc = (dlc > 8U) ? 8U : dlc;

  for (std::size_t index = 0U; index < msg.dlc; ++index)
  {
    msg.data[index] = data[index];
  }

  return microhal::is_ok(microhal::can::bus<backend, can_cfg>::transmit(msg));
}

// Process received messages
void process_can_messages() noexcept
{
  while (has_rx_message())
  {
    const microhal::can::message msg = get_rx_message();

    // Handle different message IDs
    switch (msg.id)
    {
    case 0x100: // Motor control command
      if (msg.dlc >= 2U)
      {
        const std::uint16_t motor_speed = (msg.data[0] << 8U) | msg.data[1];
        (void)motor_speed;
        // Set motor speed based on received data
      }
      break;

    case 0x200: // Sensor data request
    {
      // Send sensor data response
      std::array<std::uint8_t, 8U> response{};
      response[0] = 0x12; // Temperature MSB
      response[1] = 0x34; // Temperature LSB
      response[2] = 0x56; // Pressure MSB
      response[3] = 0x78; // Pressure LSB
      (void)send_message(0x201, response.data(), 4U);
    }
    break;

    case 0x300: // Status request
    {
      // Send status response
      std::array<std::uint8_t, 8U> status{};
      status[0] = 0x01; // System OK
      status[1] = 0x00; // No errors
      (void)send_message(0x301, status.data(), 2U);
    }
    break;

    default:
      // Unknown message ID
      break;
    }
  }
}

} // namespace

int main()
{
  // Compile-time validation
  static_assert(microhal::CanConcept<backend, can_cfg>);

  // Configure GPIO pins for CAN
  if (!microhal::is_ok(microhal::gpio::pin<backend, can_tx_pin>::configure()) ||
      !microhal::is_ok(microhal::gpio::pin<backend, can_rx_pin>::configure()))
  {
    return 1;
  }

  // Initialize CAN peripheral. Every fallible call now reports a status; bail out on the first failure.
  if (!microhal::is_ok(microhal::can::bus<backend, can_cfg>::initialize()))
  {
    return 1;
  }

  // Configure CAN filters (accept specific message IDs)
  // Filter 0: Accept ID 0x100-0x1FF (motor control) into FIFO0
  // Filter 1: Accept ID 0x200-0x2FF (sensor data) into FIFO1
  // Filter 2: Accept ID 0x300-0x3FF (status) into FIFO0
  const microhal::status filter_status[] = {
      microhal::can::bus<backend, can_cfg>::configure_filter(0x100U, 0xFF00U, 0U),
      microhal::can::bus<backend, can_cfg>::configure_filter(0x200U, 0xFF00U, 1U),
      microhal::can::bus<backend, can_cfg>::configure_filter(0x300U, 0xFF00U, 0U),
  };

  for (const microhal::status entry : filter_status)
  {
    if (!microhal::is_ok(entry))
    {
      return 1;
    }
  }

  // Enable RX interrupt
  if (!microhal::is_ok(microhal::can::bus<backend, can_cfg>::template enable_rx_interrupt<can_rx_handler>()))
  {
    return 1;
  }

  // Send initial heartbeat message
  std::array<std::uint8_t, 8U> heartbeat{0xAA, 0xBB, 0xCC, 0xDD, 0x01, 0x02, 0x03, 0x04};
  (void)send_message(0x100, heartbeat.data(), 8U);

  // Main loop: process received messages and send periodic heartbeats
  std::uint32_t heartbeat_counter = 0U;

  while (true)
  {
    // Process any received CAN messages
    process_can_messages();

    // Send periodic heartbeat every ~1 second
    heartbeat_counter++;
    if (heartbeat_counter >= 100000U)
    {
      heartbeat_counter = 0U;
      heartbeat[4]++; // Increment sequence number
      send_message(0x100, heartbeat.data(), 8U);
    }

    // Could enter low-power mode here
    // __WFI(); // Wait for interrupt
  }

  return 0;
}
