// SPDX-License-Identifier: Apache-2.0
/**
 * @file ethernet.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <array>
#include <span>

#include "microhal/core/concepts.hpp"

namespace microhal::ethernet
{

template <std::uint8_t InstanceIndex, ethernet_link_speed LinkSpeed = ethernet_link_speed::mbps100,
          ethernet_duplex Duplex = ethernet_duplex::full, bool EnableChecksumOffload = true, bool EnableDma = true,
          bool EnableInterrupts = true>
struct mac_config final
{
  static constexpr std::uint8_t instance_index{InstanceIndex};
  static constexpr ethernet_link_speed link_speed{LinkSpeed};
  static constexpr ethernet_duplex duplex{Duplex};
  static constexpr bool enable_checksum_offload{EnableChecksumOffload};
  static constexpr bool enable_dma{EnableDma};
  static constexpr bool enable_interrupts{EnableInterrupts};
};

template <typename Binding, ethernet_link_speed LinkSpeed = ethernet_link_speed::mbps100,
          ethernet_duplex Duplex = ethernet_duplex::full, bool EnableChecksumOffload = true, bool EnableDma = true,
          bool EnableInterrupts = true>
struct bound_mac_config final
{
  using binding = Binding;

  static constexpr ethernet_link_speed link_speed{LinkSpeed};
  static constexpr ethernet_duplex duplex{Duplex};
  static constexpr bool enable_checksum_offload{EnableChecksumOffload};
  static constexpr bool enable_dma{EnableDma};
  static constexpr bool enable_interrupts{EnableInterrupts};
};

template <typename Backend, typename Config>
  requires EthernetConcept<Backend, Config>
struct mac final
{
  using backend_tag = Backend;
  using config_type = Config;

  static constexpr peripheral_kind kind{peripheral_kind::ethernet};
  static constexpr bool has_checksum_offload{backend_traits<Backend>::ethernet_capability().checksum_offload};
  static constexpr bool has_ptp_timestamp{backend_traits<Backend>::ethernet_capability().ptp_timestamp};
  static constexpr bool has_vlan_filter{backend_traits<Backend>::ethernet_capability().vlan_filter};
  static constexpr std::uint32_t max_mtu_bytes{backend_traits<Backend>::ethernet_capability().mtu_bytes};

  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return ethernet_api_t<Backend>::template initialize<Config>(); });
  }

  [[nodiscard]] static inline status start() noexcept
  {
    return detail::as_status([] { return ethernet_api_t<Backend>::template start<Config>(); });
  }

  [[nodiscard]] static inline status stop() noexcept
  {
    return detail::as_status([] { return ethernet_api_t<Backend>::template stop<Config>(); });
  }

  [[nodiscard]] static inline status transmit_frame(const std::span<const std::uint8_t> frame) noexcept
  {
    return detail::as_status([frame] { return ethernet_api_t<Backend>::template transmit_frame<Config>(frame); });
  }

  /// @return Number of bytes received; zero means no frame was available.
  [[nodiscard]] static inline result<std::uint32_t> receive_frame(const std::span<std::uint8_t> buffer) noexcept
  {
    return detail::as_result<std::uint32_t>(
        [buffer] { return ethernet_api_t<Backend>::template receive_frame<Config>(buffer); });
  }

  [[nodiscard]] static inline status set_mac_address(const std::array<std::uint8_t, 6>& address) noexcept
  {
    return detail::as_status([&address] { return ethernet_api_t<Backend>::template set_mac_address<Config>(address); });
  }

  template <auto RxHandler>
  [[nodiscard]] static inline status enable_rx_interrupt() noexcept
    requires IsrCapableConcept<Backend, peripheral_kind::ethernet>
  {
    if constexpr (requires { ethernet_api_t<Backend>::template enable_rx_interrupt<Config, RxHandler>(); })
    {
      return detail::as_status([]
                               { return ethernet_api_t<Backend>::template enable_rx_interrupt<Config, RxHandler>(); });
    }
    else
    {
      return status::unsupported;
    }
  }
};

} // namespace microhal::ethernet
