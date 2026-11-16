// SPDX-License-Identifier: Apache-2.0
/**
 * @file can.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <span>

#include "microhal/core/concepts.hpp"

namespace microhal::can
{

/**
 * @brief Compile-time CAN configuration.
 */
template <std::uint8_t InstanceIndex, std::uint32_t Baudrate = 500000U, can_mode Mode = can_mode::normal,
          bool EnableCanFd = false, bool EnableInterrupts = true>
struct can_config final
{
  static constexpr std::uint8_t instance_index{InstanceIndex};
  static constexpr std::uint32_t baudrate{Baudrate};
  static constexpr can_mode mode{Mode};
  static constexpr bool enable_can_fd{EnableCanFd};
  static constexpr bool enable_interrupts{EnableInterrupts};
};

/**
 * @brief Zero-overhead CAN bus facade with compile-time validation.
 */
template <typename Backend, typename Config>
  requires CanConcept<Backend, Config>
struct bus final
{
  using backend_tag = Backend;
  using config_type = Config;

  static constexpr peripheral_kind kind{peripheral_kind::can};
  static constexpr bool has_can_fd{backend_traits<Backend>::can_capability().can_fd};
  static constexpr std::uint32_t mailbox_count{backend_traits<Backend>::can_capability().mailbox_count};
  static constexpr bool has_timing_configuration{backend_traits<Backend>::can_capability().timing_configuration};
  static constexpr bool has_error_state{backend_traits<Backend>::can_capability().state_query};
  static constexpr bool has_bus_recovery{backend_traits<Backend>::can_capability().bus_recovery};

  /**
   * @brief Checks whether a CAN message identifier and data length fit the selected frame format.
   * @param[in] msg Message to validate.
   * @return True when the identifier and DLC fit the standard/extended identifier and data-array limits.
   */
  [[nodiscard]] static constexpr bool is_valid(const message& msg) noexcept
  {
    const std::uint32_t max_id = msg.extended_id ? 0x1FFF'FFFFU : 0x7FFU;
    return (msg.id <= max_id) && (msg.dlc <= sizeof(msg.data));
  }

  /**
   * @brief Initializes the configured CAN peripheral.
   * @return Backend initialization status.
   */
  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return can_api_t<Backend>::template initialize<Config>(); });
  }

  /**
   * @brief Configures nominal CAN bit timing when supported by the backend.
   * @param[in] timing Prescaler, synchronization jump width, and time segments.
   * @return @ref status::invalid_argument for invalid timing, or @ref status::unsupported when not implemented.
   * @note Call while the CAN controller is stopped unless the backend documents otherwise.
   */
  [[nodiscard]] static status configure_timing(const can_bit_timing& timing) noexcept
  {
    if (!timing.is_valid())
    {
      return status::invalid_argument;
    }
    if constexpr (requires { can_api_t<Backend>::template configure_timing<Config>(timing); })
    {
      return detail::as_status([&timing] { return can_api_t<Backend>::template configure_timing<Config>(timing); });
    }
    return status::unsupported;
  }

  /**
   * @brief Queries the controller's CAN error state and transmit/receive error counters.
   * @return The sampled state, or an error status including @ref status::unsupported when unavailable.
   */
  [[nodiscard]] static result<can_error_state> error_state() noexcept
  {
    can_error_state state{};
    if constexpr (requires { can_api_t<Backend>::template query_error_state<Config>(state); })
    {
      const status query_status =
          detail::as_status([&state] { return can_api_t<Backend>::template query_error_state<Config>(state); });
      if (query_status != status::ok)
      {
        return query_status;
      }
      return state;
    }
    return status::unsupported;
  }

  /**
   * @brief Requests recovery of a CAN controller in the bus-off state.
   * @param[in] timeout_ms Maximum time allowed for backend recovery; zero is invalid.
   * @return Recovery status, or @ref status::unsupported when the backend has no recovery operation.
   */
  [[nodiscard]] static status recover_bus(const std::uint32_t timeout_ms = 1000U) noexcept
  {
    if (timeout_ms == 0U)
    {
      return status::invalid_argument;
    }
    if constexpr (requires { can_api_t<Backend>::template recover_bus<Config>(timeout_ms); })
    {
      return detail::as_status([timeout_ms] { return can_api_t<Backend>::template recover_bus<Config>(timeout_ms); });
    }
    return status::unsupported;
  }

  /**
   * @brief Transmits one CAN message.
   * @param[in] msg Message to send; its identifier and DLC are validated before the backend call.
   * @return @ref microhal::status::invalid_argument for an invalid message; otherwise the backend transmit status.
   */
  [[nodiscard]] static inline status transmit(const message& msg) noexcept
  {
    if (!is_valid(msg))
    {
      return status::invalid_argument;
    }
    if constexpr (requires { can_api_t<Backend>::template transmit<Config>(msg); })
    {
      return detail::as_status([&msg] { return can_api_t<Backend>::template transmit<Config>(msg); });
    }
    else
    {
      return detail::as_status([&msg] { return can_api_t<Backend>::template transmit<Config>(msg.data, msg.dlc); });
    }
  }

  /**
   * @brief Transmits messages in order, stopping at the first failure.
   * @details Earlier messages may already have reached the bus when an error is returned.
   * @param[in] messages Messages to transmit, in order; the span is used only for this call.
   * @return @ref microhal::status::ok if all messages are transmitted, or the first per-message failure.
   */
  [[nodiscard]] static inline status transmit_many(const std::span<const message> messages) noexcept
  {
    for (const message& msg : messages)
    {
      const status result = transmit(msg);
      if (result != status::ok)
      {
        return result;
      }
    }
    return status::ok;
  }

  /**
   * @brief Receives one CAN message into caller-provided storage.
   * @param[out] msg Storage for the received message; its value is updated on success.
   * @return Backend receive status. Legacy data-only backends report @ref microhal::status::io_error when no
   *         message is received.
   */
  [[nodiscard]] static inline status receive(message& msg) noexcept
  {
    if constexpr (requires { can_api_t<Backend>::template receive<Config>(msg); })
    {
      return detail::as_status([&msg] { return can_api_t<Backend>::template receive<Config>(msg); });
    }
    else
    {
      std::size_t length = 0U;
      if (can_api_t<Backend>::template receive<Config>(msg.data, length))
      {
        msg.id = 0U;
        msg.dlc = static_cast<std::uint8_t>((length > 8U) ? 8U : length);
        msg.extended_id = false;
        msg.remote_frame = false;
        return status::ok;
      }
      return status::io_error;
    }
  }

  /**
   * @brief Receives into each message slot in order, stopping at the first backend failure.
   * @details Slots after the first failed receive are left untouched.
   * @param[out] messages Caller-owned message slots, filled in order.
   * @return @ref microhal::status::ok if all slots are filled, or the first receive failure.
   */
  [[nodiscard]] static inline status receive_many(const std::span<message> messages) noexcept
  {
    for (message& msg : messages)
    {
      const status result = receive(msg);
      if (result != status::ok)
      {
        return result;
      }
    }
    return status::ok;
  }

  /**
   * @brief Configures a CAN acceptance filter.
   * @param[in] id   Filter identifier, interpreted by the backend.
   * @param[in] mask Filter mask, interpreted by the backend.
   * @param[in] fifo Backend-specific receive FIFO selection.
   * @return Backend filter-configuration status.
   */
  [[nodiscard]] static inline status configure_filter(const std::uint32_t id, const std::uint32_t mask,
                                                      const std::uint8_t fifo) noexcept
  {
    return detail::as_status([id, mask, fifo]
                             { return can_api_t<Backend>::template configure_filter<Config>(id, mask, fifo); });
  }

  /**
   * @brief Enables the CAN receive interrupt and registers its compile-time handler.
   * @tparam RxHandler Backend-compatible receive handler invoked from interrupt context.
   * @return Backend interrupt setup status.
   * @note Requires an ISR-capable CAN backend. Keep the handler bounded, non-blocking, and noexcept; do not call
   *       blocking facade operations from it.
   */
  template <auto RxHandler>
  [[nodiscard]] static inline status enable_rx_interrupt() noexcept
    requires IsrCapableConcept<Backend, peripheral_kind::can>
  {
    return detail::as_status([] { return can_api_t<Backend>::template enable_rx_interrupt<Config, RxHandler>(); });
  }
};

} // namespace microhal::can
