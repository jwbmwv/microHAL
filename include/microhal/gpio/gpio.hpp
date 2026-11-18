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
   * @brief Applies the mode, pull, speed, alternate function, and interrupt trigger in @p PinConfig.
   * @return @ref status::ok on success; otherwise the backend configuration status.
   * @note Configure the pin before using it. In particular, @ref set, @ref clear, and @ref toggle require an output
   *       or suitable alternate-function configuration.
   */
  [[nodiscard]] static inline status configure() noexcept
  {
    return detail::as_status([] { return gpio_api_t<Backend>::template configure<PinConfig>(); });
  }

  /**
   * @brief Drives the configured pin high.
   * @note The pin must be configured for output. This operation has no status return and is not a substitute for
   *       configuring the pin.
   */
  static inline void set() noexcept
  {
    gpio_api_t<Backend>::template write<PinConfig>(true);
  }

  /**
   * @brief Drives the configured pin low.
   * @note The pin must be configured for output. This operation has no status return and is not a substitute for
   *       configuring the pin.
   */
  static inline void clear() noexcept
  {
    gpio_api_t<Backend>::template write<PinConfig>(false);
  }

  /**
   * @brief Reads the current logical level of the configured pin.
   * @return The pin level; this is not an operation status and cannot report a read error.
   */
  [[nodiscard]] static inline bool read() noexcept
  {
    return gpio_api_t<Backend>::template read<PinConfig>();
  }

  /**
   * @brief Toggles the configured pin's output latch using a backend-native operation.
   * @return @ref status::ok on success; @ref status::unsupported when the backend has no native toggle operation;
   *         otherwise the backend status.
   * @note The pin must be configured for output. No atomicity guarantee is made beyond the backend operation.
   */
  [[nodiscard]] static inline status toggle() noexcept
  {
    return detail::as_status([] { return gpio_api_t<Backend>::template toggle<PinConfig>(); });
  }

  /**
   * @brief Enables an interrupt for the edge/level configured in @p PinConfig.
   * @tparam Handler Backend-compatible callback invoked when the configured trigger occurs.
   * @return @ref status::ok on success; otherwise the backend status.
   * @note Requires an ISR-capable GPIO backend. The callback executes in interrupt context and must be noexcept,
   *       bounded, and non-blocking. Board-owned interrupt routing/dispatch may also be required.
   */
  template <auto Handler>
  [[nodiscard]] static inline status enable_interrupt() noexcept
    requires IsrCapableConcept<Backend, peripheral_kind::gpio>
  {
    return detail::as_status([] { return gpio_api_t<Backend>::template enable_interrupt<PinConfig, Handler>(); });
  }
};

} // namespace microhal::gpio
