// SPDX-License-Identifier: Apache-2.0
/**
 * @file qspi.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include "microhal/core/concepts.hpp"

namespace microhal::qspi
{

/**
 * @brief Compile-time QSPI controller configuration.
 */
template <std::uint8_t InstanceIndex, std::uint32_t ClockHz, bool EnableDma = false, bool EnableInterrupts = false>
struct controller_config final
{
  static constexpr std::uint8_t instance_index{InstanceIndex};
  static constexpr std::uint32_t clock_hz{ClockHz};
  static constexpr bool enable_dma{EnableDma};
  static constexpr bool enable_interrupts{EnableInterrupts};
};

/**
 * @brief Zero-overhead QSPI controller facade.
 */
template <typename Backend, typename Config>
  requires QspiConcept<Backend, Config>
struct controller final
{
  using backend_tag = Backend;
  using config_type = Config;

  static constexpr peripheral_kind kind{peripheral_kind::qspi};

  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return qspi_api_t<Backend>::template initialize<Config>(); });
  }
};

} // namespace microhal::qspi
