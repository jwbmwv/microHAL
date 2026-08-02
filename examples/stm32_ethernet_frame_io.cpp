// SPDX-License-Identifier: Apache-2.0
/**
 * @file stm32_ethernet_frame_io.cpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

// STM32 Ethernet MAC Example
// Demonstrates: MAC initialization, address assignment, and basic frame I/O plumbing.

#include "microhal/ethernet/ethernet.hpp"

#include <array>
#include <cstdint>

namespace
{

using backend = microhal::stm32_hal_tag;

struct ethernet_binding final
{
  [[maybe_unused]] static constexpr std::uintptr_t base_address{0x40028000U};
  [[maybe_unused]] static constexpr microhal::ethernet_media_interface media_interface{
      microhal::ethernet_media_interface::rmii};
  [[maybe_unused]] static constexpr std::array<std::uint8_t, 6U> mac_address{
      {0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U}};
  [[maybe_unused]] static constexpr std::uint32_t rx_buffer_size{1528U};
  [[maybe_unused]] static constexpr std::uint32_t rx_buffer_count{8U};
};

using ethernet_cfg = microhal::ethernet::bound_mac_config<ethernet_binding, microhal::ethernet_link_speed::mbps100,
                                                          microhal::ethernet_duplex::full, false, true, true>;
using ethernet_mac = microhal::ethernet::mac<backend, ethernet_cfg>;

constexpr std::array<std::uint8_t, 6U> mac_address{{0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U}};
std::array<std::uint8_t, 128U> tx_frame{{0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU}};
std::array<std::uint8_t, 1536U> rx_frame{};

} // namespace

int main()
{
  static_assert(microhal::EthernetConcept<backend, ethernet_cfg>);

  if (!microhal::is_ok(ethernet_mac::initialize()) || !microhal::is_ok(ethernet_mac::set_mac_address(mac_address)) ||
      !microhal::is_ok(ethernet_mac::start()))
  {
    return 1;
  }

  if (!microhal::is_ok(ethernet_mac::transmit_frame(tx_frame)))
  {
    return 1;
  }

  // A receive failure is not fatal here: an empty ring simply means no frame has arrived yet.
  (void)ethernet_mac::receive_frame(rx_frame);

  return microhal::is_ok(ethernet_mac::stop()) ? 0 : 1;
}
