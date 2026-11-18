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

/**
 * @brief Compile-time USB configuration using board-owned resources.
 * @tparam Binding Backend-specific board resource binding.
 * @tparam Speed Requested USB speed.
 * @tparam DeviceMode Request device-mode support.
 * @tparam HostMode Request host-mode support.
 * @tparam OtgMode Request OTG support.
 */
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

private:
  /// @brief Checks whether the backend exposes a USB control-transfer operation.
  static consteval bool backend_has_control_transfer()
  {
    return requires(const usb_setup_packet& setup, const std::span<const std::uint8_t> tx,
                    const std::span<std::uint8_t> rx) {
      usb_api_t<Backend>::template control_transfer<Config>(setup, tx, rx);
    };
  }

  /// @brief Checks whether the backend exposes a USB descriptor-read operation.
  static consteval bool backend_has_descriptor_access()
  {
    return requires(const std::uint8_t type, const std::uint8_t index, const std::uint16_t language,
                    const std::span<std::uint8_t> buffer) {
      usb_api_t<Backend>::template get_descriptor<Config>(type, index, language, buffer);
    };
  }

public:
  static constexpr peripheral_kind kind{peripheral_kind::usb};
  static constexpr bool has_device{backend_traits<Backend>::usb_capability().device};
  static constexpr bool has_host{backend_traits<Backend>::usb_capability().host};
  static constexpr bool has_otg{backend_traits<Backend>::usb_capability().otg};
  static constexpr std::uint32_t endpoint_count{backend_traits<Backend>::usb_capability().endpoint_count};
  static constexpr std::uint16_t max_packet_size_bytes{backend_traits<Backend>::usb_capability().max_packet_size_bytes};
  static constexpr bool has_control_transfer{backend_traits<Backend>::usb_capability().control_transfer ||
                                             backend_has_control_transfer()};
  static constexpr bool has_descriptor_access{backend_traits<Backend>::usb_capability().descriptor_access ||
                                              backend_has_descriptor_access()};

  /**
   * @brief Initializes the configured USB controller.
   * @return Backend initialization status.
   */
  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return usb_api_t<Backend>::template initialize<Config>(); });
  }

  /**
   * @brief Connects the USB interface.
   * @return Backend connection status.
   */
  [[nodiscard]] static inline status connect() noexcept
  {
    return detail::as_status([] { return usb_api_t<Backend>::template connect<Config>(); });
  }

  /**
   * @brief Disconnects the USB interface.
   * @return Backend disconnection status.
   */
  [[nodiscard]] static inline status disconnect() noexcept
  {
    return detail::as_status([] { return usb_api_t<Backend>::template disconnect<Config>(); });
  }

  /**
   * @brief Performs a USB control transfer when implemented by the backend.
   * @param[in] setup    Setup packet; its length must match the direction's data span.
   * @param[in] tx_data  OUT data; must be empty for an IN or no-data request.
   * @param[out] rx_data IN destination; must be empty for an OUT or no-data request.
   * @return @ref microhal::status::invalid_argument for inconsistent direction/length, or the backend status;
   *         @ref microhal::status::unsupported when the backend has no control-transfer implementation.
   */
  [[nodiscard]] static status control_transfer(const usb_setup_packet& setup,
                                               const std::span<const std::uint8_t> tx_data = {},
                                               const std::span<std::uint8_t> rx_data = {}) noexcept
  {
    const bool device_to_host = (setup.request_type & 0x80U) != 0U;
    const bool spans_match = (setup.length == 0U)
                                 ? (tx_data.empty() && rx_data.empty())
                                 : (device_to_host ? (tx_data.empty() && (rx_data.size() == setup.length))
                                                   : (rx_data.empty() && (tx_data.size() == setup.length)));
    if (!spans_match)
    {
      return status::invalid_argument;
    }

    if constexpr (requires { usb_api_t<Backend>::template control_transfer<Config>(setup, tx_data, rx_data); })
    {
      using transfer_result = decltype(usb_api_t<Backend>::template control_transfer<Config>(setup, tx_data, rx_data));
      static_assert(std::same_as<std::remove_cvref_t<transfer_result>, status> ||
                        std::same_as<std::remove_cvref_t<transfer_result>, bool> ||
                        std::is_void_v<std::remove_cvref_t<transfer_result>>,
                    "microHAL: USB control_transfer backend operation must return status, bool, or void.");
      return detail::as_status(
          [&setup, tx_data, rx_data]
          { return usb_api_t<Backend>::template control_transfer<Config>(setup, tx_data, rx_data); });
    }
    return status::unsupported;
  }

  /**
   * @brief Retrieves a USB descriptor through the backend when descriptor access is supported.
   * @param[in]  descriptor_type USB descriptor type; zero is reserved and invalid.
   * @param[in]  descriptor_index Descriptor index within that type.
   * @param[in]  language_id      Language identifier for string descriptors, or zero when not applicable.
   * @param[out] buffer           Caller-owned destination for descriptor bytes.
   * @return Number of bytes written, or an error status including @ref microhal::status::unsupported when
   *         descriptor retrieval is unavailable.
   */
  [[nodiscard]] static result<std::uint32_t> get_descriptor(const std::uint8_t descriptor_type,
                                                            const std::uint8_t descriptor_index,
                                                            const std::uint16_t language_id,
                                                            const std::span<std::uint8_t> buffer) noexcept
  {
    if ((descriptor_type == 0U) || buffer.empty() || (buffer.size() > std::numeric_limits<std::uint16_t>::max()))
    {
      return status::invalid_argument;
    }

    if constexpr (requires {
                    usb_api_t<Backend>::template get_descriptor<Config>(descriptor_type, descriptor_index, language_id,
                                                                        buffer);
                  })
    {
      using descriptor_result = decltype(usb_api_t<Backend>::template get_descriptor<Config>(
          descriptor_type, descriptor_index, language_id, buffer));
      static_assert(std::same_as<std::remove_cvref_t<descriptor_result>, result<std::uint32_t>>,
                    "microHAL: USB get_descriptor backend operation must return result<uint32_t>.");
      return usb_api_t<Backend>::template get_descriptor<Config>(descriptor_type, descriptor_index, language_id,
                                                                 buffer);
    }
    return status::unsupported;
  }

  /**
   * @brief Transmits data through a USB endpoint.
   * @param[in] endpoint Endpoint number and direction bits accepted by is_valid_endpoint().
   * @param[in] data     Bytes to transmit; the caller's storage is used for this call.
   * @return @ref microhal::status::invalid_argument for an invalid endpoint or oversized transfer, or the backend
   *         transmit status.
   */
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

  /**
   * @brief Receives endpoint FIFO data into caller-provided storage.
   * @param[in]  endpoint Endpoint number and direction bits accepted by is_valid_endpoint().
   * @param[out] buffer   Destination storage; it must remain valid for this call.
   * @return Number of bytes read from the endpoint FIFO, or an error status for invalid arguments/backend failure.
   */
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

  /**
   * @brief Queries the backend status for a USB endpoint.
   * @param[in] endpoint Endpoint number and direction bits accepted by is_valid_endpoint().
   * @return Backend endpoint status, @ref microhal::status::invalid_argument for an invalid endpoint, or
   *         @ref microhal::status::unsupported when the backend omits the query.
   */
  [[nodiscard]] static status endpoint_status(const std::uint8_t endpoint) noexcept
  {
    if (!is_valid_endpoint(endpoint))
      return status::invalid_argument;
    if constexpr (requires { usb_api_t<Backend>::template endpoint_status<Config>(endpoint); })
      return detail::as_status([endpoint] { return usb_api_t<Backend>::template endpoint_status<Config>(endpoint); });
    else
      return status::unsupported;
  }

  /**
   * @brief Aborts an operation on a USB endpoint when supported by the backend.
   * @param[in] endpoint Endpoint number and direction bits accepted by is_valid_endpoint().
   * @return Backend abort status, @ref microhal::status::invalid_argument for an invalid endpoint, or
   *         @ref microhal::status::unsupported when the backend omits abort.
   */
  [[nodiscard]] static status endpoint_abort(const std::uint8_t endpoint) noexcept
  {
    if (!is_valid_endpoint(endpoint))
      return status::invalid_argument;
    if constexpr (requires { usb_api_t<Backend>::template endpoint_abort<Config>(endpoint); })
      return detail::as_status([endpoint] { return usb_api_t<Backend>::template endpoint_abort<Config>(endpoint); });
    else
      return status::unsupported;
  }

  /**
   * @brief Enables or clears the stall condition on a USB endpoint when supported.
   * @param[in] endpoint Endpoint number and direction bits accepted by is_valid_endpoint().
   * @param[in] enabled  True to stall the endpoint; false to clear its stall condition.
   * @return Backend stall-control status, @ref microhal::status::invalid_argument for an invalid endpoint, or
   *         @ref microhal::status::unsupported when the backend omits stall control.
   */
  [[nodiscard]] static status endpoint_stall(const std::uint8_t endpoint, const bool enabled = true) noexcept
  {
    if (!is_valid_endpoint(endpoint))
      return status::invalid_argument;
    if constexpr (requires { usb_api_t<Backend>::template endpoint_stall<Config>(endpoint, enabled); })
      return detail::as_status([endpoint, enabled]
                               { return usb_api_t<Backend>::template endpoint_stall<Config>(endpoint, enabled); });
    else
      return status::unsupported;
  }

  /**
   * @brief Dispatches pending USB interrupt handling to the backend.
   * @return Backend interrupt-handling status, or @ref microhal::status::unsupported when no handler is provided.
   * @note Call from the USB interrupt path when required by the backend integration. Do not call blocking facade
   *       operations from interrupt context.
   */
  [[nodiscard]] static status handle_interrupt() noexcept
  {
    if constexpr (requires { usb_api_t<Backend>::template handle_interrupt<Config>(); })
      return detail::as_status([] { return usb_api_t<Backend>::template handle_interrupt<Config>(); });
    else if constexpr (requires { usb_api_t<Backend>::handle_interrupt(); })
      return detail::as_status([] { return usb_api_t<Backend>::handle_interrupt(); });
    else
      return status::unsupported;
  }

  /**
   * @brief Checks whether an endpoint identifier fits the backend's endpoint count and reserved-bit layout.
   * @param[in] endpoint Endpoint identifier; reserved bits 6:4 must be clear and the low endpoint number must be
   *                     less than endpoint_count.
   * @return True when the endpoint identifier is accepted by the facade.
   */
  [[nodiscard]] static constexpr bool is_valid_endpoint(const std::uint8_t endpoint) noexcept
  {
    constexpr std::uint8_t endpoint_number_mask{0x0FU};
    constexpr std::uint8_t reserved_bits_mask{0x70U};
    return ((endpoint & reserved_bits_mask) == 0U) && ((endpoint & endpoint_number_mask) < endpoint_count);
  }
};

} // namespace microhal::usb
