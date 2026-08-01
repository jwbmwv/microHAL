// SPDX-License-Identifier: Apache-2.0
/**
 * @file i2s.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <span>

#include "microhal/core/concepts.hpp"

namespace microhal::i2s
{

/**
 * @brief Compile-time I2S configuration.
 */
template <std::uint8_t InstanceIndex, i2s_mode Mode = i2s_mode::master_tx,
          i2s_standard Standard = i2s_standard::philips, std::uint32_t SampleRate = 48000U,
          bool EnableMasterClock = false, bool EnableDma = true>
struct i2s_config final
{
  static constexpr std::uint8_t instance_index{InstanceIndex};
  static constexpr i2s_mode mode{Mode};
  static constexpr i2s_standard standard{Standard};
  static constexpr std::uint32_t sample_rate{SampleRate};
  static constexpr bool enable_master_clock{EnableMasterClock};
  static constexpr bool enable_dma{EnableDma};
};

/**
 * @brief Zero-overhead I2S audio interface facade with compile-time validation.
 */
template <typename Backend, typename Config>
  requires I2sConcept<Backend, Config>
struct interface final
{
  using backend_tag = Backend;
  using config_type = Config;

  static constexpr peripheral_kind kind{peripheral_kind::i2s};
  static constexpr bool has_full_duplex{backend_traits<Backend>::i2s_capability().full_duplex};
  static constexpr std::uint32_t max_sample_rate{backend_traits<Backend>::i2s_capability().max_sample_rate};

  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return i2s_api_t<Backend>::template initialize<Config>(); });
  }

  [[nodiscard]] static inline status transmit_blocking(const std::span<const std::uint16_t> samples) noexcept
  {
    return detail::as_status([samples] { return i2s_api_t<Backend>::template transmit_blocking<Config>(samples); });
  }

  [[nodiscard]] static inline status receive_blocking(const std::span<std::uint16_t> samples) noexcept
  {
    return detail::as_status([samples] { return i2s_api_t<Backend>::template receive_blocking<Config>(samples); });
  }

  template <auto CompletionHandler>
  [[nodiscard]] static inline status start_dma_tx(const std::span<const std::uint16_t> samples) noexcept
    requires DmaCapableConcept<Backend, peripheral_kind::i2s>
  {
    return detail::as_status([samples]
                             { return i2s_api_t<Backend>::template start_dma_tx<Config, CompletionHandler>(samples); });
  }
};

} // namespace microhal::i2s
