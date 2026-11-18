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

  /**
   * @brief Initializes the timer using the clock, counter mode, prescaler, and period in @p Config.
   * @return @ref status::ok on success; otherwise the backend initialization status.
   */
  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return timer_api_t<Backend>::template initialize<Config>(); });
  }

  /**
   * @brief Starts the configured timer.
   * @return @ref status::ok on success; otherwise the backend status, including unsupported operations.
   */
  [[nodiscard]] static inline status start() noexcept
  {
    return detail::as_status([] { return timer_api_t<Backend>::template start<Config>(); });
  }

  /**
   * @brief Stops the configured timer.
   * @return @ref status::ok on success; otherwise the backend status, including unsupported operations.
   */
  [[nodiscard]] static inline status stop() noexcept
  {
    return detail::as_status([] { return timer_api_t<Backend>::template stop<Config>(); });
  }

  /**
   * @brief Resets the timer counter to zero without requesting a start or stop.
   * @return @ref status::ok on success; otherwise the backend status, including unsupported counter reset.
   */
  [[nodiscard]] static inline status reset_counter() noexcept
  {
    return detail::as_status([] { return timer_api_t<Backend>::template reset_counter<Config>(); });
  }

  /**
   * @brief Sets the timer period in counter ticks.
   * @param[in] period Period in ticks; must be non-zero.
   * @return @ref status::invalid_argument for zero; otherwise @ref status::ok or the backend status.
   * @note Supported ranges and side effects on the running counter are backend-specific.
   */
  [[nodiscard]] static inline status set_period(const std::uint32_t period) noexcept
  {
    if (period == 0U)
    {
      return status::invalid_argument;
    }
    return detail::as_status([period] { return timer_api_t<Backend>::template set_period<Config>(period); });
  }

  /**
   * @brief Reads the current raw counter value.
   * @return Counter value in ticks; this is not an operation status and cannot report a read error.
   */
  [[nodiscard]] static inline std::uint32_t counter() noexcept
  {
    return timer_api_t<Backend>::template counter<Config>();
  }

  /**
   * @brief Reads the timer period in counter ticks.
   * @return Period on success, or the backend error; backends without period readback may return
   *         @ref status::unsupported.
   */
  [[nodiscard]] static inline result<std::uint32_t> period() noexcept
  {
    return timer_api_t<Backend>::template period<Config>();
  }

  /**
   * @brief Enables capture and/or compare notification for a configured timer channel.
   * @tparam ChannelConfig One-based channel configuration; its enable_capture/enable_compare options select the
   *                       requested functions, and a backend may use an optional compare_ticks member.
   * @tparam Handler       Backend-compatible callback for capture/compare events.
   * @return @ref status::ok on success; otherwise the backend status, including invalid channel or unsupported
   *         operation.
   * @note Requires an ISR-capable timer backend. Callbacks run in interrupt context and must be noexcept, bounded,
   *       and non-blocking. Input capture may require board-provided hardware event routing.
   */
  template <typename ChannelConfig, auto Handler>
  [[nodiscard]] static inline status enable_capture_compare() noexcept
    requires IsrCapableConcept<Backend, peripheral_kind::timer>
  {
    return detail::as_status(
        [] { return timer_api_t<Backend>::template enable_capture_compare<Config, ChannelConfig, Handler>(); });
  }

  /**
   * @brief Reads the raw captured tick count from a configured one-based input channel.
   * @tparam ChannelConfig Channel configuration with capture enabled and an index supported by the backend.
   * @return Captured ticks on success; @ref status::invalid_argument when capture is disabled or the channel index
   *         is out of range; otherwise the backend error, including unsupported readback.
   * @note Board or backend setup must provide any required input-capture signal routing.
   */
  template <typename ChannelConfig> [[nodiscard]] static inline result<std::uint32_t> capture_value() noexcept
  {
    if constexpr (!ChannelConfig::enable_capture || (ChannelConfig::channel_index < 1U) ||
                  (ChannelConfig::channel_index > backend_traits<Backend>::timer_capability().channel_count))
    {
      return status::invalid_argument;
    }
    return timer_api_t<Backend>::template capture_value<Config, ChannelConfig>();
  }

  /**
   * @brief Disables capture/compare handling for a channel and releases backend-owned channel resources.
   * @tparam ChannelConfig Channel configuration previously used to enable capture/compare.
   * @return @ref status::ok on success; otherwise the backend status.
   */
  template <typename ChannelConfig> [[nodiscard]] static status disable_capture_compare() noexcept
  {
    return timer_api_t<Backend>::template disable_capture_compare<Config, ChannelConfig>();
  }

  /**
   * @brief Dispatches this timer's peripheral interrupt to the backend.
   * @return @ref status::ok when handled; otherwise the backend status, including unsupported dispatch.
   * @note Call from the configured timer interrupt vector. Any event callback executes under interrupt-context
   *       restrictions.
   */
  [[nodiscard]] static status handle_interrupt() noexcept
  {
    return timer_api_t<Backend>::template handle_interrupt<Config>();
  }
};

} // namespace microhal::timers
