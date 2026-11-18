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

  /**
   * @brief Initializes the configured CRC/RNG peripheral.
   * @return Backend initialization status.
   */
  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return crc_api_t<Backend>::template initialize<Config>(); });
  }

  /**
   * @brief Calculates a checksum using the backend's hardware CRC implementation.
   * @param[in] data Bytes to include; the span is used only for this call.
   * @return The computed checksum, not an operation status.
   * @note Available only when the backend advertises hardware CRC-32 support.
   */
  [[nodiscard]] static inline std::uint32_t calculate(const std::span<const std::uint8_t> data) noexcept
    requires(has_hardware_crc)
  {
    return crc_api_t<Backend>::template calculate<Config>(data);
  }

  /**
   * @brief Computes a non-reflected CRC using the configured 32-bit polynomial.
   * @details This portable fallback uses the caller-supplied initial and final XOR values.
   * @param[in] data       Bytes to include.
   * @param[in] initial    Initial CRC accumulator value.
   * @param[in] final_xor  Value XORed with the accumulator before returning.
   * @return Computed CRC value; this operation does not report an error status.
   */
  [[nodiscard]] static constexpr std::uint32_t calculate_software(const std::span<const std::uint8_t> data,
                                                                  const std::uint32_t initial = 0xFFFF'FFFFU,
                                                                  const std::uint32_t final_xor = 0U) noexcept
  {
    std::uint32_t crc = initial;
    for (const std::uint8_t byte : data)
    {
      crc ^= static_cast<std::uint32_t>(byte) << 24U;
      for (std::uint8_t bit = 0U; bit < 8U; ++bit)
      {
        crc = ((crc & 0x8000'0000U) != 0U) ? ((crc << 1U) ^ Config::polynomial) : (crc << 1U);
      }
    }
    return crc ^ final_xor;
  }

  /**
   * @brief Resets backend CRC state when the selected implementation maintains such state.
   * @details Software CRC implementations may not require a reset.
   */
  static inline void reset() noexcept
  {
    crc_api_t<Backend>::template reset<Config>();
  }

  /**
   * @brief Reads one random value from the backend RNG.
   * @return Random value; this convenience operation does not expose backend read errors.
   * @note Available only when the backend advertises an RNG.
   */
  [[nodiscard]] static inline std::uint32_t get_random() noexcept
    requires(has_rng)
  {
    return crc_api_t<Backend>::template get_random<Config>();
  }

  /**
   * @brief Reads one random value from the backend RNG with error reporting.
   * @return Random value on success, or the backend error; returns @ref microhal::status::unsupported when the
   *         backend has no error-reporting RNG read operation.
   * @note Available only when the backend advertises an RNG.
   */
  [[nodiscard]] static result<std::uint32_t> read_random() noexcept
    requires(has_rng)
  {
    if constexpr (requires { crc_api_t<Backend>::template read_random<Config>(); })
      return detail::as_result<std::uint32_t>([] { return crc_api_t<Backend>::template read_random<Config>(); });
    else
      return status::unsupported;
  }
};

} // namespace microhal::crc
