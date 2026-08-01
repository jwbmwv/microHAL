// SPDX-License-Identifier: Apache-2.0
/**
 * @file gpio.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include "microhal/core/concepts.hpp"

namespace microhal::gpio
{

/**
 * @brief Compile-time GPIO pin description.
 */
template <std::uint8_t PortIndex, std::uint8_t PinIndex, gpio_mode Mode, gpio_pull Pull = gpio_pull::none,
          gpio_speed Speed = gpio_speed::low, std::uint8_t AlternateFunction = gpio_no_alternate,
          gpio_interrupt_trigger Trigger = gpio_interrupt_trigger::none>
struct pin_config final
{
  static constexpr std::uint8_t port_index{PortIndex};
  static constexpr std::uint8_t pin_index{PinIndex};
  static constexpr gpio_mode mode{Mode};
  static constexpr gpio_pull pull{Pull};
  static constexpr gpio_speed speed{Speed};
  static constexpr std::uint8_t alternate_function{AlternateFunction};
  static constexpr gpio_interrupt_trigger interrupt_trigger{Trigger};
};

/**
 * @brief Zero-overhead GPIO facade bound to a backend at compile time.
 */
template <typename Backend, typename PinConfig>
  requires GpioPinConcept<Backend, PinConfig>
struct pin final
{
  using backend_tag = Backend;
  using config_type = PinConfig;

  static constexpr peripheral_kind kind{peripheral_kind::gpio};
  static constexpr bool is_interrupt_capable{capability_for<Backend, peripheral_kind::gpio>().isr};
  static constexpr bool is_dma_trigger_capable{capability_for<Backend, peripheral_kind::gpio>().dma};

  /**
   * @brief Configures the pin mode, pull, speed, and alternate-function state.
   */
  [[nodiscard]] static inline status configure() noexcept
  {
    return detail::as_status([] { return gpio_api_t<Backend>::template configure<PinConfig>(); });
  }

  // Driving a port register cannot fail, so these intentionally stay void.
  /** @brief Drives the pin high. */
  static inline void set() noexcept
  {
    gpio_api_t<Backend>::template write<PinConfig>(true);
  }

  /** @brief Drives the pin low. */
  static inline void clear() noexcept
  {
    gpio_api_t<Backend>::template write<PinConfig>(false);
  }

  /// @return The electrical level of the pin, not an operation status.
  [[nodiscard]] static inline bool read() noexcept
  {
    return gpio_api_t<Backend>::template read<PinConfig>();
  }

  /**
   * @brief Enables the backend interrupt route for this pin.
   * @tparam Handler ISR callback invoked by the backend when the configured edge/level occurs.
   */
  template <auto Handler>
  [[nodiscard]] static inline status enable_interrupt() noexcept
    requires IsrCapableConcept<Backend, peripheral_kind::gpio>
  {
    return detail::as_status([] { return gpio_api_t<Backend>::template enable_interrupt<PinConfig, Handler>(); });
  }
};

} // namespace microhal::gpio
