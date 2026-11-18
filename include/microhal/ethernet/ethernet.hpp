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

/**
 * @brief Compile-time Ethernet MAC configuration using a numeric instance.
 * @tparam InstanceIndex Backend-specific MAC instance index.
 * @tparam LinkSpeed Requested link speed.
 * @tparam Duplex Requested duplex mode.
 * @tparam EnableChecksumOffload Request hardware checksum offload.
 * @tparam EnableDma Request DMA transfers.
 * @tparam EnableInterrupts Request interrupt support.
 */
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

/**
 * @brief Compile-time Ethernet MAC configuration using board-owned resources.
 * @tparam Binding Backend-specific board resource binding.
 * @tparam LinkSpeed Requested link speed.
 * @tparam Duplex Requested duplex mode.
 * @tparam EnableChecksumOffload Request hardware checksum offload.
 * @tparam EnableDma Request DMA transfers.
 * @tparam EnableInterrupts Request interrupt support.
 */
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

/**
 * @brief Compile-time Ethernet MAC facade.
 * @tparam Backend Registered backend tag.
 * @tparam Config MAC configuration accepted by @c EthernetConcept.
 * @note The application must serialize access to the configured MAC.
 */
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

  /**
   * @brief Initializes the configured Ethernet MAC.
   * @return Backend initialization status.
   */
  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return ethernet_api_t<Backend>::template initialize<Config>(); });
  }

  /**
   * @brief Starts the configured Ethernet MAC.
   * @return Backend start status.
   */
  [[nodiscard]] static inline status start() noexcept
  {
    return detail::as_status([] { return ethernet_api_t<Backend>::template start<Config>(); });
  }

  /**
   * @brief Stops the configured Ethernet MAC.
   * @return Backend stop status.
   */
  [[nodiscard]] static inline status stop() noexcept
  {
    return detail::as_status([] { return ethernet_api_t<Backend>::template stop<Config>(); });
  }

  /**
   * @brief Submits an Ethernet frame for transmission.
   * @param[in] frame Frame bytes; caller-owned storage is used for the duration of this call.
   * @return Backend transmit status.
   */
  [[nodiscard]] static inline status transmit_frame(const std::span<const std::uint8_t> frame) noexcept
  {
    return detail::as_status([frame] { return ethernet_api_t<Backend>::template transmit_frame<Config>(frame); });
  }

  /**
   * @brief Receives a frame into caller-provided storage.
   * @param[out] buffer Storage for received frame bytes; it must remain valid for this call.
   * @return Number of bytes received; zero means no frame was available. A backend error is reported in the result.
   */
  [[nodiscard]] static inline result<std::uint32_t> receive_frame(const std::span<std::uint8_t> buffer) noexcept
  {
    return detail::as_result<std::uint32_t>(
        [buffer] { return ethernet_api_t<Backend>::template receive_frame<Config>(buffer); });
  }

  /**
   * @brief Sets the MAC address used by the configured interface.
   * @param[in] address Six-byte MAC address.
   * @return Backend address-configuration status.
   */
  [[nodiscard]] static inline status set_mac_address(const std::array<std::uint8_t, 6>& address) noexcept
  {
    return detail::as_status([&address] { return ethernet_api_t<Backend>::template set_mac_address<Config>(address); });
  }

  /**
   * @brief Enables the Ethernet receive interrupt and registers its compile-time handler.
   * @tparam RxHandler Backend-compatible receive handler invoked from interrupt context.
   * @return Backend interrupt setup status, or @ref microhal::status::unsupported if the backend omits the operation.
   * @note Requires an ISR-capable Ethernet backend. Keep the handler bounded, non-blocking, and noexcept; do not call
   *       blocking facade operations from it.
   */
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
