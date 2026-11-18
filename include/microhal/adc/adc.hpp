// SPDX-License-Identifier: Apache-2.0
/**
 * @file adc.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <cstdint>
#include <limits>
#include <span>

#include "microhal/core/adc_types.hpp"
#include "microhal/core/concepts.hpp"

namespace microhal::adc
{

/**
 * @brief Compile-time ADC configuration.
 */
template <std::uint8_t InstanceIndex, adc_resolution Resolution = adc_resolution::bits_12,
          adc_sample_time SampleTime = adc_sample_time::cycles_84, bool EnableDma = false,
          bool EnableInterrupts = false, bool DifferentialMode = false, std::uint32_t ExternalTrigger = 0U,
          adc_trigger_edge TriggerEdge = adc_trigger_edge::software>
struct adc_config final
{
  static constexpr std::uint8_t instance_index{InstanceIndex};
  static constexpr adc_resolution resolution{Resolution};
  static constexpr adc_sample_time sample_time{SampleTime};
  static constexpr bool enable_dma{EnableDma};
  static constexpr bool enable_interrupts{EnableInterrupts};
  static constexpr bool differential_mode{DifferentialMode};
  static constexpr std::uint32_t external_trigger{ExternalTrigger};
  static constexpr adc_trigger_edge trigger_edge{TriggerEdge};
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
  static constexpr bool has_sequential_scan{backend_traits<Backend>::adc_capability().sequential_scan};
  static constexpr bool has_voltage_conversion{backend_traits<Backend>::adc_capability().voltage_conversion};
  static constexpr std::uint32_t channel_count{backend_traits<Backend>::adc_capability().channel_count};

  /**
   * @brief Initializes the ADC for the compile-time @p Config.
   * @return @ref status::ok on success; otherwise the backend initialization status.
   */
  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return adc_api_t<Backend>::template initialize<Config>(); });
  }

  /**
   * @brief Performs a blocking conversion and returns the raw sample.
   * @param[in] channel Numeric ADC channel index.
   * @return The sample on success, or the backend error. This call has no facade timeout parameter and must not be
   *         made from interrupt context.
   */
  [[nodiscard]] static inline result<std::uint16_t> read_blocking(const std::uint8_t channel) noexcept
  {
    return detail::as_result<std::uint16_t>([channel]
                                            { return adc_api_t<Backend>::template read_blocking<Config>(channel); });
  }

  /**
   * @brief Performs a blocking differential conversion and returns signed raw counts.
   * @param[in] channel Numeric ADC channel index configured for differential operation.
   * @return Signed sample on success; @ref status::unsupported if differential mode is disabled; otherwise the
   *         backend error. This call has no facade timeout parameter and must not be made from interrupt context.
   */
  [[nodiscard]] static result<std::int16_t> read_differential_blocking(std::uint8_t channel) noexcept
  {
    if constexpr (Config::differential_mode)
      return adc_api_t<Backend>::template read_differential_blocking<Config>(channel);
    else
      return status::unsupported;
  }

  /**
   * @brief Converts signed differential counts to millivolts using the positive full-scale reference.
   * @param[in] channel      Numeric ADC channel index configured for differential operation.
   * @param[in] reference_mv Positive full-scale input voltage in millivolts; must be non-zero and fit in int32_t.
   * @return Converted signed voltage, or an error from argument validation or the differential conversion. Returns
   *         @ref status::unsupported when the configured ADC is not differential.
   */
  [[nodiscard]] static result<std::int32_t> read_differential_millivolts(std::uint8_t channel,
                                                                         std::uint32_t reference_mv) noexcept
  {
    if (reference_mv == 0U || reference_mv > static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()))
      return status::invalid_argument;
    constexpr auto bits = static_cast<std::uint32_t>(Config::resolution);
    static_assert(bits > 0U && bits <= 16U, "microHAL: unsupported ADC resolution.");
    constexpr std::int64_t scale = std::int64_t{1} << (bits - 1U);
    const auto sample = read_differential_blocking(channel);
    if (!sample)
      return sample.error();
    const auto code = static_cast<std::int64_t>(sample.value());
    if (code < -scale || code >= scale)
      return status::error;
    const auto numerator = code * reference_mv;
    const auto rounded = numerator + (numerator < 0 ? -(scale / 2) : scale / 2);
    return static_cast<std::int32_t>(rounded / scale);
  }

  /**
   * @brief Reads channels in order into an equally sized output span.
   * @param[in]  channels Numeric channel indexes, in conversion order; must not be empty.
   * @param[out] samples  Caller-owned storage with exactly one element per channel.
   * @return @ref status::ok when all conversions succeed; @ref status::invalid_argument for empty or mismatched
   *         spans; otherwise the first conversion error. Earlier samples may already have been written on failure.
   *         Conversions block, have no facade timeout parameter, and must not be called from interrupt context.
   */
  [[nodiscard]] static inline status read_sequence_blocking(const std::span<const std::uint8_t> channels,
                                                            const std::span<std::uint16_t> samples) noexcept
  {
    if (channels.empty() || (channels.size() != samples.size()))
    {
      return status::invalid_argument;
    }

    for (std::size_t index = 0U; index < channels.size(); ++index)
    {
      const auto sample = read_blocking(channels[index]);
      if (!sample)
      {
        return sample.error();
      }
      samples[index] = sample.value();
    }
    return status::ok;
  }

  /**
   * @brief Reads one single-ended sample and converts it to millivolts using the configured resolution.
   * @param[in] channel      Numeric ADC channel index.
   * @param[in] reference_mv Positive full-scale input voltage in millivolts; must be non-zero.
   * @return Converted voltage, or an argument, conversion, or backend error. Returns @ref status::unsupported for
   *         differential configurations. The conversion blocks, has no facade timeout parameter, and must not be
   *         called from interrupt context.
   */
  [[nodiscard]] static inline result<std::uint32_t> read_millivolts(const std::uint8_t channel,
                                                                    const std::uint32_t reference_mv) noexcept
  {
    if (reference_mv == 0U)
    {
      return status::invalid_argument;
    }
    if constexpr (Config::differential_mode)
      return status::unsupported;

    constexpr std::uint32_t bits = static_cast<std::uint32_t>(Config::resolution);
    static_assert((bits > 0U) && (bits <= 16U), "microHAL: unsupported ADC resolution.");
    constexpr std::uint32_t max_code = (1U << bits) - 1U;
    const auto sample = read_blocking(channel);
    if (!sample)
    {
      return sample.error();
    }
    if (sample.value() > max_code)
    {
      return status::error;
    }

    const std::uint64_t numerator = (static_cast<std::uint64_t>(sample.value()) * reference_mv) + (max_code / 2U);
    return static_cast<std::uint32_t>(numerator / max_code);
  }

  /**
   * @brief Starts continuous conversions over the supplied channel sequence.
   * @param[in] channels Numeric channel indexes in conversion order; must remain valid through this call.
   * @return @ref status::ok when started; otherwise the backend status, including unsupported operations.
   * @note This operation is backend-dependent and requires any configured trigger/IRQ resources to be routed by the
   *       application. It does not provide a facade output buffer or callback.
   */
  [[nodiscard]] static inline status start_continuous(const std::span<const std::uint8_t> channels) noexcept
  {
    return detail::as_status([channels] { return adc_api_t<Backend>::template start_continuous<Config>(channels); });
  }

  /**
   * @brief Starts one interrupt-driven conversion.
   * @tparam CompletionHandler Backend completion callback; supported forms include
   *                           `(status, uint16_t)`, `(status)`, `(uint16_t)`, and `()`.
   * @param[in] channel Numeric ADC channel index.
   * @return @ref status::ok when started; otherwise the backend status.
   * @note Requires an ISR-capable ADC backend. The callback runs in the backend's completion/interrupt context;
   *       keep it noexcept, bounded, and non-blocking. The facade supplies no timeout.
   */
  template <auto CompletionHandler>
  [[nodiscard]] static inline status start_async(const std::uint8_t channel) noexcept
    requires IsrCapableConcept<Backend, peripheral_kind::adc>
  {
    return detail::as_status([channel]
                             { return adc_api_t<Backend>::template start_async<Config, CompletionHandler>(channel); });
  }

  /**
   * @brief Starts a finite DMA scan into caller-owned sample storage.
   * @tparam CompletionHandler Backend completion callback; its accepted signature and notification behavior are
   *                           backend-specific.
   * @param[in]  channels Numeric channel indexes in conversion order; must remain valid through this call.
   * @param[out] results  DMA destination storage; keep it DMA-accessible and alive until completion.
   * @return @ref status::ok when started; otherwise the backend status.
   * @note Requires a DMA-capable ADC backend. Any callback runs in the backend's completion/interrupt context and
   *       must be noexcept, bounded, and non-blocking.
   */
  template <auto CompletionHandler>
  [[nodiscard]] static inline status start_dma_scan(const std::span<const std::uint8_t> channels,
                                                    const std::span<std::uint16_t> results) noexcept
    requires DmaCapableConcept<Backend, peripheral_kind::adc>
  {
    return detail::as_status(
        [channels, results]
        { return adc_api_t<Backend>::template start_dma_scan<Config, CompletionHandler>(channels, results); });
  }

  /**
   * @brief Starts half-buffer DMA acquisition over the supplied channel sequence.
   * @tparam Handler Callback accepting `(status, span<uint16_t>)` for a completed half, or a backend-supported
   *                 status-only/no-argument form.
   * @param[in]  channels Numeric channel indexes in conversion order; must remain valid through this call.
   * @param[out] results  DMA-accessible storage split into two halves, each containing whole scans. Keep it alive
   *                      until completion or a successful stop/abort.
   * @param[in]  circular Keep acquiring and notify for each completed half; `false` produces two half-block
   *                      notifications and then stops.
   * @return @ref status::ok when started; otherwise the backend status.
   * @note Requires a DMA-capable ADC backend. The completed half is valid only until hardware reuses it (normally
   *       the next callback). The callback runs in the backend's completion/interrupt context and must be noexcept,
   *       bounded, and non-blocking. Both halves must contain whole scans.
   */
  template <auto Handler>
  [[nodiscard]] static status start_dma_stream(std::span<const std::uint8_t> channels, std::span<std::uint16_t> results,
                                               bool circular = true) noexcept
    requires DmaCapableConcept<Backend, peripheral_kind::adc>
  {
    return detail::as_status(
        [=] { return adc_api_t<Backend>::template start_dma_stream<Config, Handler>(channels, results, circular); });
  }

  /**
   * @brief Stops active ADC acquisition and releases its resources.
   * @return @ref status::ok on success; otherwise the backend status. Reuse DMA storage only after a successful stop.
   */
  [[nodiscard]] static status stop() noexcept
  {
    return detail::as_status([] { return adc_api_t<Backend>::template stop<Config>(); });
  }
  /**
   * @brief Cancels active ADC acquisition.
   * @return @ref status::ok on success; otherwise the backend status. Reuse DMA storage only after a successful
   *         abort or confirmed completion.
   */
  [[nodiscard]] static status abort() noexcept
  {
    return detail::as_status([] { return adc_api_t<Backend>::template abort<Config>(); });
  }
  /**
   * @brief Queries the current ADC/DMA acquisition state.
   * @return Transfer state on success, or the backend error (including unsupported status queries).
   */
  [[nodiscard]] static result<dma_transfer_state> transfer_status() noexcept
  {
    return adc_api_t<Backend>::template transfer_status<Config>();
  }
  /**
   * @brief Dispatches the ADC peripheral interrupt to the backend acquisition handler.
   * @note Call from the configured ADC interrupt vector; callback work remains subject to ISR restrictions.
   */
  static void handle_interrupt() noexcept
  {
    adc_api_t<Backend>::template handle_interrupt<Config>();
  }
  /**
   * @brief Dispatches the ADC DMA interrupt to the backend acquisition handler.
   * @note Call from the configured DMA interrupt vector; callback work remains subject to ISR restrictions.
   */
  static void handle_dma_interrupt() noexcept
  {
    adc_api_t<Backend>::template handle_dma_interrupt<Config>();
  }
};

} // namespace microhal::adc
