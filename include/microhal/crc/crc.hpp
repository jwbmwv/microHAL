// SPDX-License-Identifier: Apache-2.0
/**
 * @file crc.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <span>

#include "microhal/core/concepts.hpp"

namespace microhal::crc
{

/**
 * @brief Compile-time CRC/RNG configuration.
 */
template <std::uint8_t InstanceIndex = 0, bool EnableRng = false, std::uint32_t Polynomial = 0x04C11DB7U>
struct crc_config final
{
  static constexpr std::uint8_t instance_index{InstanceIndex};
  static constexpr bool enable_rng{EnableRng};
  static constexpr std::uint32_t polynomial{Polynomial};
};

/**
 * @brief Zero-overhead CRC/RNG hardware accelerator facade with compile-time validation.
 */
template <typename Backend, typename Config>
  requires CrcConcept<Backend, Config>
struct accelerator final
{
  using backend_tag = Backend;
  using config_type = Config;

  static constexpr peripheral_kind kind{peripheral_kind::crc};
  static constexpr bool has_hardware_crc{backend_traits<Backend>::crc_capability().hardware_crc32};
  static constexpr bool has_rng{backend_traits<Backend>::crc_capability().random_number_generator};

  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return crc_api_t<Backend>::template initialize<Config>(); });
  }

  /// @return The computed checksum, not an operation status.
  [[nodiscard]] static inline std::uint32_t calculate(const std::span<const std::uint8_t> data) noexcept
    requires(has_hardware_crc)
  {
    return crc_api_t<Backend>::template calculate<Config>(data);
  }

  static inline void reset() noexcept
  {
    crc_api_t<Backend>::template reset<Config>();
  }

  [[nodiscard]] static inline std::uint32_t get_random() noexcept
    requires(has_rng)
  {
    return crc_api_t<Backend>::template get_random<Config>();
  }
};

} // namespace microhal::crc
