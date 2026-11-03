// SPDX-License-Identifier: Apache-2.0
/**
 * @file rcc.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include "microhal/core/concepts.hpp"

namespace microhal::rcc
{

enum class clock_source : std::uint8_t
{
  hsi, // High-speed internal
  hse, // High-speed external
  lsi, // Low-speed internal
  lse, // Low-speed external
  pll  // PLL
};

/**
 * @brief Compile-time clock configuration.
 */
template <clock_source Source = clock_source::hsi, std::uint32_t SystemClockHz = 16000000U, bool EnablePll = false,
          std::uint8_t PllM = 8, std::uint16_t PllN = 336, std::uint8_t PllP = 2>
struct clock_config final
{
  static constexpr clock_source source{Source};
  static constexpr std::uint32_t system_clock_hz{SystemClockHz};
  static constexpr bool enable_pll{EnablePll};
  static constexpr std::uint8_t pll_m{PllM};
  static constexpr std::uint16_t pll_n{PllN};
  static constexpr std::uint8_t pll_p{PllP};
};

/**
 * @brief Zero-overhead RCC (Reset and Clock Control) facade with compile-time validation.
 */
template <typename Backend, typename Config>
  requires RccConcept<Backend, Config>
struct clock_controller final
{
  using backend_tag = Backend;
  using config_type = Config;

  static constexpr peripheral_kind kind{peripheral_kind::rcc};
  static constexpr std::uint32_t max_system_clock{backend_traits<Backend>::rcc_capability().max_system_clock_hz};

  [[nodiscard]] static inline status configure() noexcept
  {
    return detail::as_status([] { return rcc_api_t<Backend>::template configure<Config>(); });
  }

  [[nodiscard]] static inline status configure_and_verify() noexcept
  {
    const status configured = configure();
    if (configured != status::ok)
    {
      return configured;
    }
    return (get_system_clock_hz() == Config::system_clock_hz) ? status::ok : status::io_error;
  }

  /// @return The measured system clock in hertz, not an operation status.
  [[nodiscard]] static inline std::uint32_t get_system_clock_hz() noexcept
  {
    return rcc_api_t<Backend>::template get_system_clock_hz<Config>();
  }

  [[nodiscard]] static inline status enable_peripheral_clock(const peripheral_kind peripheral,
                                                             const std::uint8_t instance) noexcept
  {
    return detail::as_status(
        [peripheral, instance]
        { return rcc_api_t<Backend>::template enable_peripheral_clock<Config>(peripheral, instance); });
  }

  [[nodiscard]] static inline status disable_peripheral_clock(const peripheral_kind peripheral,
                                                              const std::uint8_t instance) noexcept
  {
    return detail::as_status(
        [peripheral, instance]
        { return rcc_api_t<Backend>::template disable_peripheral_clock<Config>(peripheral, instance); });
  }
};

} // namespace microhal::rcc
