// SPDX-License-Identifier: Apache-2.0
/**
 * @file dac.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

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

  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return dac_api_t<Backend>::template initialize<Config>(); });
  }

  [[nodiscard]] static inline status write_blocking(const std::uint16_t value) noexcept
  {
    return detail::as_status([value] { return dac_api_t<Backend>::template write_blocking<Config>(value); });
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
