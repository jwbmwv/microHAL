// SPDX-License-Identifier: Apache-2.0
/**
 * @file dac.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <cstdint>
#include <span>

#include "microhal/core/concepts.hpp"

namespace microhal::dac
{

/**
 * @brief Compile-time DAC configuration.
 */
template <std::uint8_t InstanceIndex, std::uint8_t Channel = 1, dac_resolution Resolution = dac_resolution::bits_12,
          bool EnableDma = false, bool EnableWaveform = false>
struct dac_config final
{
  static constexpr std::uint8_t instance_index{InstanceIndex};
  static constexpr std::uint8_t channel{Channel};
  static constexpr dac_resolution resolution{Resolution};
  static constexpr bool enable_dma{EnableDma};
  static constexpr bool enable_waveform{EnableWaveform};
};

/**
 * @brief Zero-overhead DAC facade with compile-time validation.
 */
template <typename Backend, typename Config>
  requires DacConcept<Backend, Config>
struct converter final
{
  using backend_tag = Backend;
  using config_type = Config;

  static constexpr peripheral_kind kind{peripheral_kind::dac};
  static constexpr bool has_dual_mode{backend_traits<Backend>::dac_capability().dual_mode};
  static constexpr bool has_waveform{backend_traits<Backend>::dac_capability().waveform_generation};
  static constexpr bool has_output_buffer{backend_traits<Backend>::dac_capability().output_buffer};
  static constexpr bool has_voltage_conversion{backend_traits<Backend>::dac_capability().voltage_conversion};
  static constexpr bool has_sequence_write{backend_traits<Backend>::dac_capability().sequence_write};

  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return dac_api_t<Backend>::template initialize<Config>(); });
  }

  [[nodiscard]] static inline status write_blocking(const std::uint16_t value) noexcept
  {
    return detail::as_status([value] { return dac_api_t<Backend>::template write_blocking<Config>(value); });
  }

  /** @brief Writes each sample in order, stopping at the first backend error. */
  [[nodiscard]] static inline status write_sequence_blocking(const std::span<const std::uint16_t> samples) noexcept
  {
    for (const std::uint16_t sample : samples)
    {
      const status write_status = write_blocking(sample);
      if (write_status != status::ok)
      {
        return write_status;
      }
    }
    return status::ok;
  }

  /** @brief Converts millivolts to a DAC code using the configured resolution. */
  [[nodiscard]] static inline status write_millivolts(const std::uint32_t millivolts,
                                                      const std::uint32_t reference_mv) noexcept
  {
    if ((reference_mv == 0U) || (millivolts > reference_mv))
    {
      return status::invalid_argument;
    }

    constexpr std::uint32_t bits = static_cast<std::uint32_t>(Config::resolution);
    static_assert((bits > 0U) && (bits <= 16U), "microHAL: unsupported DAC resolution.");
    constexpr std::uint32_t max_code = (1U << bits) - 1U;
    const std::uint64_t numerator =
        (static_cast<std::uint64_t>(millivolts) * max_code) + (reference_mv / 2U);
    const auto code = static_cast<std::uint16_t>(numerator / reference_mv);
    return write_blocking(code);
  }

  [[nodiscard]] static inline status write_dual(const std::uint16_t value1, const std::uint16_t value2) noexcept
    requires(has_dual_mode)
  {
    return detail::as_status([value1, value2]
                             { return dac_api_t<Backend>::template write_dual<Config>(value1, value2); });
  }

  template <auto CompletionHandler>
  [[nodiscard]] static inline status start_dma_waveform(const std::span<const std::uint16_t> samples) noexcept
    requires DmaCapableConcept<Backend, peripheral_kind::dac>
  {
    return detail::as_status(
        [samples] { return dac_api_t<Backend>::template start_dma_waveform<Config, CompletionHandler>(samples); });
  }
};

} // namespace microhal::dac
