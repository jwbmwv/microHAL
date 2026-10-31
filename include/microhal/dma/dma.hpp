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

  [[nodiscard]] static constexpr bool is_valid_range(const std::uint32_t address, const std::uint32_t length) noexcept
  {
    return (length != 0U) &&
           ((static_cast<std::uint64_t>(address) + length) <= (static_cast<std::uint64_t>(UINT32_MAX) + 1U));
  }

  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return dma_api_t<Backend>::template initialize<Config>(); });
  }

  /**
   * @brief Starts a transfer and lets the backend notify completion.
   * @details The handler signature follows the backend's callback ABI. Pass `nullptr` when no callback
   *          is needed. A handler must be `noexcept` and non-blocking.
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

  [[nodiscard]] static inline status abort() noexcept
  {
    return detail::as_status([] { return dma_api_t<Backend>::template abort<Config>(); });
  }

  [[nodiscard]] static inline result<transfer_state> transfer_status() noexcept
  {
    return dma_api_t<Backend>::template transfer_status<Config>();
  }

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
