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

  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return can_api_t<Backend>::template initialize<Config>(); });
  }

  [[nodiscard]] static inline status transmit(const message& msg) noexcept
  {
    if constexpr (requires { can_api_t<Backend>::template transmit<Config>(msg); })
    {
      return detail::as_status([&msg] { return can_api_t<Backend>::template transmit<Config>(msg); });
    }
    else
    {
      return detail::as_status([&msg] { return can_api_t<Backend>::template transmit<Config>(msg.data, msg.dlc); });
    }
  }

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

  [[nodiscard]] static inline status configure_filter(const std::uint32_t id, const std::uint32_t mask,
                                                      const std::uint8_t fifo) noexcept
  {
    return detail::as_status([id, mask, fifo]
                             { return can_api_t<Backend>::template configure_filter<Config>(id, mask, fifo); });
  }

  template <auto RxHandler>
  [[nodiscard]] static inline status enable_rx_interrupt() noexcept
    requires IsrCapableConcept<Backend, peripheral_kind::can>
  {
    return detail::as_status([] { return can_api_t<Backend>::template enable_rx_interrupt<Config, RxHandler>(); });
  }
};

} // namespace microhal::can
