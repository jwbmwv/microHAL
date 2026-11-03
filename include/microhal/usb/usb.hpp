// SPDX-License-Identifier: Apache-2.0
/**
 * @file usb.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <limits>
#include <span>

#include "microhal/core/concepts.hpp"

namespace microhal::usb
{

/**
 * @brief Compile-time USB configuration.
 */
template <std::uint8_t InstanceIndex, usb_speed Speed = usb_speed::full, bool DeviceMode = true, bool HostMode = false,
          bool OtgMode = false>
struct usb_config final
{
  static constexpr std::uint8_t instance_index{InstanceIndex};
  static constexpr usb_speed speed{Speed};
  static constexpr bool device_mode{DeviceMode};
  static constexpr bool host_mode{HostMode};
  static constexpr bool otg_mode{OtgMode};
};

template <typename Binding, usb_speed Speed = usb_speed::full, bool DeviceMode = true, bool HostMode = false,
          bool OtgMode = false>
struct bound_usb_config final
{
  using binding = Binding;

  static constexpr usb_speed speed{Speed};
  static constexpr bool device_mode{DeviceMode};
  static constexpr bool host_mode{HostMode};
  static constexpr bool otg_mode{OtgMode};
};

/**
 * @brief Zero-overhead USB device facade with compile-time validation.
 */
template <typename Backend, typename Config>
  requires UsbConcept<Backend, Config>
struct device final
{
  using backend_tag = Backend;
  using config_type = Config;

  static constexpr peripheral_kind kind{peripheral_kind::usb};
  static constexpr bool has_device{backend_traits<Backend>::usb_capability().device};
  static constexpr bool has_host{backend_traits<Backend>::usb_capability().host};
  static constexpr bool has_otg{backend_traits<Backend>::usb_capability().otg};
  static constexpr std::uint32_t endpoint_count{backend_traits<Backend>::usb_capability().endpoint_count};
  static constexpr std::uint16_t max_packet_size_bytes{backend_traits<Backend>::usb_capability().max_packet_size_bytes};

  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return usb_api_t<Backend>::template initialize<Config>(); });
  }

  [[nodiscard]] static inline status connect() noexcept
  {
    return detail::as_status([] { return usb_api_t<Backend>::template connect<Config>(); });
  }

  [[nodiscard]] static inline status disconnect() noexcept
  {
    return detail::as_status([] { return usb_api_t<Backend>::template disconnect<Config>(); });
  }

  [[nodiscard]] static inline status endpoint_transmit(const std::uint8_t endpoint,
                                                       const std::span<const std::uint8_t> data) noexcept
  {
    if (!is_valid_endpoint(endpoint) || (data.size() > std::numeric_limits<std::uint16_t>::max()))
    {
      return status::invalid_argument;
    }

    return detail::as_status([endpoint, data]
                             { return usb_api_t<Backend>::template endpoint_transmit<Config>(endpoint, data); });
  }

  /// @return Number of bytes read from the endpoint FIFO.
  [[nodiscard]] static inline result<std::uint32_t> endpoint_receive(const std::uint8_t endpoint,
                                                                     const std::span<std::uint8_t> buffer) noexcept
  {
    if (!is_valid_endpoint(endpoint) || (buffer.size() > std::numeric_limits<std::uint16_t>::max()))
    {
      return status::invalid_argument;
    }

    return detail::as_result<std::uint32_t>(
        [endpoint, buffer] { return usb_api_t<Backend>::template endpoint_receive<Config>(endpoint, buffer); });
  }

  [[nodiscard]] static constexpr bool is_valid_endpoint(const std::uint8_t endpoint) noexcept
  {
    constexpr std::uint8_t endpoint_number_mask{0x0FU};
    constexpr std::uint8_t reserved_bits_mask{0x70U};
    return ((endpoint & reserved_bits_mask) == 0U) && ((endpoint & endpoint_number_mask) < endpoint_count);
  }
};

} // namespace microhal::usb
