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

  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return pwm_api_t<Backend>::template initialize<Config>(); });
  }

  [[nodiscard]] static inline status set_duty_cycle(const std::uint16_t duty_permille) noexcept
  {
    if (duty_permille > 1'000U)
    {
      return status::invalid_argument;
    }
    return detail::as_status([duty_permille]
                             { return pwm_api_t<Backend>::template set_duty_cycle<Config>(duty_permille); });
  }

  [[nodiscard]] static inline status set_duty_cycle_percent(const std::uint8_t duty_percent) noexcept
  {
    if (duty_percent > 100U)
    {
      return status::invalid_argument;
    }
    return set_duty_cycle(static_cast<std::uint16_t>(duty_percent) * 10U);
  }

  [[nodiscard]] static inline status start() noexcept
  {
    return detail::as_status([] { return pwm_api_t<Backend>::template start<Config>(); });
  }

  [[nodiscard]] static inline status stop() noexcept
  {
    return detail::as_status([] { return pwm_api_t<Backend>::template stop<Config>(); });
  }

  [[nodiscard]] static inline status set_enabled(const bool enabled) noexcept
  {
    return enabled ? start() : stop();
  }
};

} // namespace microhal::pwm
