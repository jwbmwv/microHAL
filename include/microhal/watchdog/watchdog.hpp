// SPDX-License-Identifier: Apache-2.0
/**
 * @file watchdog.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include "microhal/core/concepts.hpp"

namespace microhal::watchdog
{

/**
 * @brief Compile-time watchdog configuration.
 */
template <std::uint8_t InstanceIndex, std::uint32_t TimeoutMs, bool IndependentMode = true, bool WindowMode = false>
struct watchdog_config final
{
  static constexpr std::uint8_t instance_index{InstanceIndex};
  static constexpr std::uint32_t timeout_ms{TimeoutMs};
  static constexpr bool independent_mode{IndependentMode};
  static constexpr bool window_mode{WindowMode};
};

/**
 * @brief Zero-overhead watchdog timer facade with compile-time validation.
 */
template <typename Backend, typename Config>
  requires WatchdogConcept<Backend, Config>
struct timer final
{
  using backend_tag = Backend;
  using config_type = Config;

  static constexpr peripheral_kind kind{peripheral_kind::watchdog};
  static constexpr bool has_window{backend_traits<Backend>::watchdog_capability().window};

  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return watchdog_api_t<Backend>::template initialize<Config>(); });
  }

  [[nodiscard]] static inline status start() noexcept
  {
    return detail::as_status([] { return watchdog_api_t<Backend>::template start<Config>(); });
  }

  [[nodiscard]] static inline status refresh() noexcept
  {
    return detail::as_status([] { return watchdog_api_t<Backend>::template refresh<Config>(); });
  }
};

} // namespace microhal::watchdog
