// SPDX-License-Identifier: Apache-2.0
/**
 * @file timer.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <cstdint>

#include "microhal/core/concepts.hpp"

namespace microhal::timers
{

/**
 * @brief Compile-time timer configuration shared by general-purpose timer use cases.
 */
template <std::uint8_t InstanceIndex, std::uint32_t ClockHz, timer_counter_mode CounterMode = timer_counter_mode::up,
          timer_clock_source ClockSource = timer_clock_source::internal, bool EnableDma = false,
          bool EnableInterrupts = false, std::uint32_t Prescaler = 1U, std::uint32_t Period = 65'536U>
struct timer_config final
{
  static_assert(Prescaler > 0U, "microHAL: timer prescaler must be non-zero.");
  static_assert(Period > 0U, "microHAL: timer period in ticks must be non-zero.");

  static constexpr std::uint8_t instance_index{InstanceIndex};
  static constexpr std::uint32_t clock_hz{ClockHz};
  static constexpr timer_counter_mode counter_mode{CounterMode};
  static constexpr timer_clock_source clock_source{ClockSource};
  static constexpr bool enable_dma{EnableDma};
  static constexpr bool enable_interrupts{EnableInterrupts};
  static constexpr std::uint32_t prescaler{Prescaler};
  /// @brief Timer period in counter ticks, including both endpoints of the count range.
  static constexpr std::uint32_t period{Period};
};

/**
 * @brief Compile-time timer channel description for a one-based capture/compare channel.
 */
template <std::uint8_t ChannelIndex, bool EnableCapture = false, bool EnableCompare = false> struct channel_config final
{
  static constexpr std::uint8_t channel_index{ChannelIndex};
  static constexpr bool enable_capture{EnableCapture};
  static constexpr bool enable_compare{EnableCompare};
};

/**
 * @brief Zero-overhead timer facade suitable for periodic, capture, or autobaud timing roles.
 */
template <typename Backend, typename Config>
  requires TimerConcept<Backend, Config>
struct timer final
{
  using backend_tag = Backend;
  using config_type = Config;

  static constexpr peripheral_kind kind{peripheral_kind::timer};

  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return timer_api_t<Backend>::template initialize<Config>(); });
  }

  [[nodiscard]] static inline status start() noexcept
  {
    return detail::as_status([] { return timer_api_t<Backend>::template start<Config>(); });
  }

  [[nodiscard]] static inline status stop() noexcept
  {
    return detail::as_status([] { return timer_api_t<Backend>::template stop<Config>(); });
  }

  /** @brief Resets the counter to zero without changing its running state. */
  [[nodiscard]] static inline status reset_counter() noexcept
  {
    return detail::as_status([] { return timer_api_t<Backend>::template reset_counter<Config>(); });
  }

  [[nodiscard]] static inline status set_period(const std::uint32_t period) noexcept
  {
    if (period == 0U)
    {
      return status::invalid_argument;
    }
    return detail::as_status([period] { return timer_api_t<Backend>::template set_period<Config>(period); });
  }

  /// @return The raw counter value, not an operation status.
  [[nodiscard]] static inline std::uint32_t counter() noexcept
  {
    return timer_api_t<Backend>::template counter<Config>();
  }

  /** @brief Returns the configured period in timer ticks when the backend supports readback. */
  [[nodiscard]] static inline result<std::uint32_t> period() noexcept
  {
    return timer_api_t<Backend>::template period<Config>();
  }

  template <typename ChannelConfig, auto Handler>
  [[nodiscard]] static inline status enable_capture_compare() noexcept
    requires IsrCapableConcept<Backend, peripheral_kind::timer>
  {
    return detail::as_status(
        [] { return timer_api_t<Backend>::template enable_capture_compare<Config, ChannelConfig, Handler>(); });
  }

  /// @brief Returns the raw captured tick count from a configured one-based input channel.
  template <typename ChannelConfig> [[nodiscard]] static inline result<std::uint32_t> capture_value() noexcept
  {
    if constexpr (!ChannelConfig::enable_capture || (ChannelConfig::channel_index < 1U) ||
                  (ChannelConfig::channel_index > 4U))
    {
      return status::invalid_argument;
    }
    return timer_api_t<Backend>::template capture_value<Config, ChannelConfig>();
  }
};

} // namespace microhal::timers
