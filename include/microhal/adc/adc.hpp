// SPDX-License-Identifier: Apache-2.0
/**
 * @file adc.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <span>

#include "microhal/core/concepts.hpp"

namespace microhal::adc
{

/**
 * @brief Compile-time ADC configuration.
 */
template <std::uint8_t InstanceIndex, adc_resolution Resolution = adc_resolution::bits_12,
          adc_sample_time SampleTime = adc_sample_time::cycles_84, bool EnableDma = false,
          bool EnableInterrupts = false, bool DifferentialMode = false>
struct adc_config final
{
  static constexpr std::uint8_t instance_index{InstanceIndex};
  static constexpr adc_resolution resolution{Resolution};
  static constexpr adc_sample_time sample_time{SampleTime};
  static constexpr bool enable_dma{EnableDma};
  static constexpr bool enable_interrupts{EnableInterrupts};
  static constexpr bool differential_mode{DifferentialMode};
};

/**
 * @brief Zero-overhead ADC facade with compile-time validation.
 */
template <typename Backend, typename Config>
  requires AdcConcept<Backend, Config>
struct converter final
{
  using backend_tag = Backend;
  using config_type = Config;

  static constexpr peripheral_kind kind{peripheral_kind::adc};
  static constexpr bool has_differential{backend_traits<Backend>::adc_capability().differential};
  static constexpr bool has_oversampling{backend_traits<Backend>::adc_capability().oversampling};
  static constexpr std::uint32_t channel_count{backend_traits<Backend>::adc_capability().channel_count};

  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return adc_api_t<Backend>::template initialize<Config>(); });
  }

  [[nodiscard]] static inline result<std::uint16_t> read_blocking(const std::uint8_t channel) noexcept
  {
    return detail::as_result<std::uint16_t>([channel]
                                            { return adc_api_t<Backend>::template read_blocking<Config>(channel); });
  }

  [[nodiscard]] static inline status start_continuous(const std::span<const std::uint8_t> channels) noexcept
  {
    return detail::as_status([channels] { return adc_api_t<Backend>::template start_continuous<Config>(channels); });
  }

  template <auto CompletionHandler>
  [[nodiscard]] static inline status start_async(const std::uint8_t channel) noexcept
    requires IsrCapableConcept<Backend, peripheral_kind::adc>
  {
    return detail::as_status([channel]
                             { return adc_api_t<Backend>::template start_async<Config, CompletionHandler>(channel); });
  }

  template <auto CompletionHandler>
  [[nodiscard]] static inline status start_dma_scan(const std::span<const std::uint8_t> channels,
                                                    const std::span<std::uint16_t> results) noexcept
    requires DmaCapableConcept<Backend, peripheral_kind::adc>
  {
    return detail::as_status(
        [channels, results]
        { return adc_api_t<Backend>::template start_dma_scan<Config, CompletionHandler>(channels, results); });
  }
};

} // namespace microhal::adc
