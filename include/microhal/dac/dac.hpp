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

  /**
   * @brief Initializes the DAC channel selected by @p Config.
   * @return @ref status::ok on success; otherwise the backend initialization status.
   */
  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return dac_api_t<Backend>::template initialize<Config>(); });
  }

  /**
   * @brief Writes one raw DAC code to the configured channel.
   * @param[in] value Output code in the range represented by the configured resolution.
   * @return @ref status::ok on success; otherwise the backend status. This call has no facade timeout parameter
   *         and must not be made from interrupt context.
   */
  [[nodiscard]] static inline status write_blocking(const std::uint16_t value) noexcept
  {
    return detail::as_status([value] { return dac_api_t<Backend>::template write_blocking<Config>(value); });
  }

  /**
   * @brief Writes raw DAC codes in order, stopping at the first backend error.
   * @param[in] samples Caller-owned sequence of codes in the range represented by the configured resolution; the
   *                    span is used only during this call.
   * @return @ref status::ok when all writes succeed (including an empty sequence); otherwise the first backend
   *         error. Earlier samples may already have been written on failure. Writes block, have no facade timeout
   *         parameter, and must not be called from interrupt context.
   */
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

  /**
   * @brief Converts a voltage to a DAC code using the configured resolution and writes it.
   * @param[in] millivolts Voltage to output; must not exceed @p reference_mv.
   * @param[in] reference_mv Positive full-scale output voltage in millivolts; must be non-zero.
   * @return @ref status::invalid_argument for an invalid reference or voltage; otherwise the status from writing
   *         the rounded DAC code. The write blocks, has no facade timeout parameter, and must not be called from
   *         interrupt context.
   */
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
    const std::uint64_t numerator = (static_cast<std::uint64_t>(millivolts) * max_code) + (reference_mv / 2U);
    const auto code = static_cast<std::uint16_t>(numerator / reference_mv);
    return write_blocking(code);
  }

  /**
   * @brief Writes two raw DAC codes through the backend's dual-channel operation.
   * @param[in] value1 First code in the range represented by the configured resolution.
   * @param[in] value2 Second code in the range represented by the configured resolution.
   * @return @ref status::ok on success; otherwise the backend status. Available only when
   *         @ref has_dual_mode is true.
   * @note This is a synchronous backend operation with no facade timeout parameter; do not call it from interrupt
   *       context.
   */
  [[nodiscard]] static inline status write_dual(const std::uint16_t value1, const std::uint16_t value2) noexcept
    requires(has_dual_mode)
  {
    return detail::as_status([value1, value2]
                             { return dac_api_t<Backend>::template write_dual<Config>(value1, value2); });
  }

  /**
   * @brief Starts DMA playback of a DAC sample sequence.
   * @tparam CompletionHandler Backend-specific completion callback; its accepted signature and notification
   *                           behavior depend on the selected backend.
   * @param[in] samples DMA-accessible sample storage; keep it alive for the backend's transfer duration.
   * @return @ref status::ok when started; otherwise the backend status.
   * @note Requires a DMA-capable DAC backend. Any callback runs in the backend's completion/interrupt context and
   *       must be noexcept, bounded, and non-blocking.
   */
  template <auto CompletionHandler>
  [[nodiscard]] static inline status start_dma_waveform(const std::span<const std::uint16_t> samples) noexcept
    requires DmaCapableConcept<Backend, peripheral_kind::dac>
  {
    return detail::as_status(
        [samples] { return dac_api_t<Backend>::template start_dma_waveform<Config, CompletionHandler>(samples); });
  }
};

} // namespace microhal::dac
