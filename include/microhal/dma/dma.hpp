// SPDX-License-Identifier: Apache-2.0
/**
 * @file dma.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <span>

#include "microhal/core/concepts.hpp"

namespace microhal::dma
{

enum class dma_direction : std::uint8_t
{
  periph_to_memory,
  memory_to_periph,
  memory_to_memory
};

enum class dma_priority : std::uint8_t
{
  low,
  medium,
  high,
  very_high
};

using transfer_state = dma_transfer_state;

/**
 * @brief Compile-time DMA configuration.
 */
template <std::uint8_t InstanceIndex, std::uint8_t Stream, std::uint8_t Channel,
          dma_direction Direction = dma_direction::periph_to_memory, dma_priority Priority = dma_priority::medium,
          bool CircularMode = false, bool DoubleBuffer = false>
struct dma_config final
{
  static constexpr std::uint8_t instance_index{InstanceIndex};
  static constexpr std::uint8_t stream{Stream};
  static constexpr std::uint8_t channel{Channel};
  static constexpr dma_direction direction{Direction};
  static constexpr dma_priority priority{Priority};
  static constexpr bool circular_mode{CircularMode};
  static constexpr bool double_buffer{DoubleBuffer};
};

/**
 * @brief Zero-overhead DMA controller facade with compile-time validation.
 */
template <typename Backend, typename Config>
  requires DmaConcept<Backend, Config>
struct controller final
{
  using backend_tag = Backend;
  using config_type = Config;

  static constexpr peripheral_kind kind{peripheral_kind::dma};
  static constexpr bool has_mem_to_mem{backend_traits<Backend>::dma_capability().mem_to_mem};
  static constexpr bool has_fifo{backend_traits<Backend>::dma_capability().fifo};

  /**
   * @brief Checks whether a non-empty 32-bit address range does not wrap.
   * @param[in] address Start address.
   * @param[in] length  Range length in bytes; zero is invalid.
   * @return True when the range fits in the 32-bit address space, including its final byte.
   * @note This arithmetic check does not establish that the address is mapped, DMA-accessible, or aligned.
   */
  [[nodiscard]] static constexpr bool is_valid_range(const std::uint32_t address, const std::uint32_t length) noexcept
  {
    return (length != 0U) &&
           ((static_cast<std::uint64_t>(address) + length) <= (static_cast<std::uint64_t>(UINT32_MAX) + 1U));
  }

  /**
   * @brief Initializes the DMA stream and channel selected by @p Config.
   * @return @ref status::ok on success; otherwise the backend initialization status.
   */
  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return dma_api_t<Backend>::template initialize<Config>(); });
  }

  /**
   * @brief Starts the configured DMA transfer between the source and destination ranges.
   * @tparam CompletionHandler Backend callback value following its completion-callback ABI; pass `nullptr` when no
   *                           callback is needed.
   * @param[in] src_addr Source address.
   * @param[in] dst_addr Destination address.
   * @param[in] length   Transfer length in bytes; must be non-zero.
   * @return @ref status::invalid_argument if either range is empty or wraps; otherwise @ref status::ok on success
   *         or the backend submission status.
   * @note Keep both memory ranges valid and DMA-accessible until completion or successful @ref abort. Callback
   *       support and signature are backend-specific; when invoked, the handler runs in the backend's
   *       completion/interrupt context and must be noexcept, bounded, and non-blocking.
   */
  template <auto CompletionHandler>
  [[nodiscard]] static inline status start_transfer(const std::uint32_t src_addr, const std::uint32_t dst_addr,
                                                    const std::uint32_t length) noexcept
  {
    if (!is_valid_range(src_addr, length) || !is_valid_range(dst_addr, length))
    {
      return status::invalid_argument;
    }
    return detail::as_status(
        [src_addr, dst_addr, length]
        { return dma_api_t<Backend>::template start_transfer<Config, CompletionHandler>(src_addr, dst_addr, length); });
  }

  /**
   * @brief Starts a memory-to-memory transfer when supported by the backend and configuration.
   * @tparam CompletionHandler Backend callback value following the completion-callback ABI; pass `nullptr` when no
   *                           callback is needed.
   * @param[in] src_addr Source address.
   * @param[in] dst_addr Destination address.
   * @param[in] length   Transfer length in bytes; must be non-zero.
   * @return @ref status::unsupported if the configuration direction is not memory-to-memory; otherwise the result
   *         of @ref start_transfer, including its range validation and backend errors.
   * @note Requires a backend advertising memory-to-memory DMA. Keep the ranges valid and DMA-accessible until
   *       completion or successful @ref abort. Any callback is backend-specific and must be noexcept, bounded, and
   *       non-blocking when invoked in completion/interrupt context.
   */
  template <auto CompletionHandler>
  [[nodiscard]] static inline status start_memcpy(const std::uint32_t src_addr, const std::uint32_t dst_addr,
                                                  const std::uint32_t length) noexcept
    requires(has_mem_to_mem)
  {
    if constexpr (Config::direction != dma_direction::memory_to_memory)
    {
      return status::unsupported;
    }
    return start_transfer<CompletionHandler>(src_addr, dst_addr, length);
  }

  /**
   * @brief Requests cancellation of the configured DMA transfer.
   * @return @ref status::ok when cancellation succeeds; otherwise the backend status.
   * @note Do not reuse transfer memory until completion or successful cancellation is confirmed.
   */
  [[nodiscard]] static inline status abort() noexcept
  {
    return detail::as_status([] { return dma_api_t<Backend>::template abort<Config>(); });
  }

  /**
   * @brief Queries the configured transfer's current state.
   * @return Transfer state on success, or the backend error (including unsupported status queries).
   */
  [[nodiscard]] static inline result<transfer_state> transfer_status() noexcept
  {
    return dma_api_t<Backend>::template transfer_status<Config>();
  }

  /**
   * @brief Tests whether the configured transfer has reached the complete state.
   * @return True only for @ref transfer_state::complete, false for any other reported state, or the error from
   *         @ref transfer_status.
   */
  [[nodiscard]] static inline result<bool> transfer_complete() noexcept
  {
    const result<transfer_state> current = transfer_status();
    if (!current)
    {
      return current.error();
    }
    return current.value() == transfer_state::complete;
  }
};

} // namespace microhal::dma
