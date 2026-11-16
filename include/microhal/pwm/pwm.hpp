// SPDX-License-Identifier: Apache-2.0
/**
 * @file pwm.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include "microhal/core/concepts.hpp"

namespace microhal::pwm
{

/**
 * @brief Compile-time PWM configuration (typically timer-based).
 */
template <std::uint8_t TimerIndex, std::uint8_t Channel, std::uint32_t FrequencyHz,
          pwm_polarity Polarity = pwm_polarity::active_high, bool EnableDeadTime = false>
struct pwm_config final
{
  static constexpr std::uint8_t timer_index{TimerIndex};
  static constexpr std::uint8_t channel{Channel};
  static constexpr std::uint32_t frequency_hz{FrequencyHz};
  static constexpr pwm_polarity polarity{Polarity};
  static constexpr bool enable_dead_time{EnableDeadTime};
};

/**
 * @brief Zero-overhead PWM output facade with compile-time validation.
 */
template <typename Backend, typename Config>
  requires PwmConcept<Backend, Config>
struct output final
{
  using backend_tag = Backend;
  using config_type = Config;

  static constexpr peripheral_kind kind{peripheral_kind::pwm};

  /**
   * @brief Initializes the PWM output described by @p Config.
   * @return @ref status::ok on success; otherwise the backend initialization status.
   */
  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return pwm_api_t<Backend>::template initialize<Config>(); });
  }

  /**
   * @brief Sets the output duty cycle in thousandths of the full period.
   * @param[in] duty_permille Duty cycle from 0 through 1000, inclusive.
   * @return @ref status::invalid_argument above 1000; otherwise @ref status::ok or the backend status.
   * @note Runtime update support is backend-dependent.
   */
  [[nodiscard]] static inline status set_duty_cycle(const std::uint16_t duty_permille) noexcept
  {
    if (duty_permille > 1'000U)
    {
      return status::invalid_argument;
    }
    return detail::as_status([duty_permille]
                             { return pwm_api_t<Backend>::template set_duty_cycle<Config>(duty_permille); });
  }

  /**
   * @brief Sets the output duty cycle as a percentage of the full period.
   * @param[in] duty_percent Percentage from 0 through 100, inclusive.
   * @return @ref status::invalid_argument above 100; otherwise the result of @ref set_duty_cycle.
   */
  [[nodiscard]] static inline status set_duty_cycle_percent(const std::uint8_t duty_percent) noexcept
  {
    if (duty_percent > 100U)
    {
      return status::invalid_argument;
    }
    return set_duty_cycle(static_cast<std::uint16_t>(duty_percent) * 10U);
  }

  /**
   * @brief Starts the configured PWM output.
   * @return @ref status::ok on success; otherwise the backend status, including unsupported runtime start.
   */
  [[nodiscard]] static inline status start() noexcept
  {
    return detail::as_status([] { return pwm_api_t<Backend>::template start<Config>(); });
  }

  /**
   * @brief Stops the configured PWM output.
   * @return @ref status::ok on success; otherwise the backend status, including unsupported runtime stop.
   */
  [[nodiscard]] static inline status stop() noexcept
  {
    return detail::as_status([] { return pwm_api_t<Backend>::template stop<Config>(); });
  }

  /**
   * @brief Enables or disables the configured PWM output.
   * @param[in] enabled True to start the output; false to stop it.
   * @return The result of @ref start when enabled, or @ref stop otherwise.
   */
  [[nodiscard]] static inline status set_enabled(const bool enabled) noexcept
  {
    return enabled ? start() : stop();
  }

  /**
   * @brief Changes the PWM frequency at runtime when the backend provides this operation.
   * @param[in] frequency_hz Requested frequency in hertz; must be non-zero.
   * @return @ref status::invalid_argument for zero; @ref status::unsupported if the backend has no runtime
   *         frequency operation; otherwise the backend status.
   * @note Frequency realizability and any requirement to stop related outputs are backend-specific. The output's
   *       compile-time configuration provides its initial frequency.
   */
  [[nodiscard]] static status set_frequency(const std::uint32_t frequency_hz) noexcept
  {
    if (frequency_hz == 0U)
      return status::invalid_argument;
    if constexpr (requires { pwm_api_t<Backend>::template set_frequency<Config>(frequency_hz); })
      return detail::as_status([frequency_hz]
                               { return pwm_api_t<Backend>::template set_frequency<Config>(frequency_hz); });
    else
      return status::unsupported;
  }
};

} // namespace microhal::pwm
