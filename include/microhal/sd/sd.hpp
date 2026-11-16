// SPDX-License-Identifier: Apache-2.0
/**
 * @file sd.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <limits>
#include <span>

#include "microhal/core/concepts.hpp"

namespace microhal::sd
{

template <std::uint8_t InstanceIndex, std::uint32_t ClockHz = 25'000'000U, sd_bus_width BusWidth = sd_bus_width::four,
          sd_timing_mode TimingMode = sd_timing_mode::default_speed, bool EnableDma = true,
          bool EnableInterrupts = true>
struct host_config final
{
  static constexpr std::uint8_t instance_index{InstanceIndex};
  static constexpr std::uint32_t clock_hz{ClockHz};
  static constexpr sd_bus_width bus_width{BusWidth};
  static constexpr sd_timing_mode timing_mode{TimingMode};
  static constexpr bool enable_dma{EnableDma};
  static constexpr bool enable_interrupts{EnableInterrupts};
};

template <typename Binding, std::uint32_t ClockHz = 25'000'000U, sd_bus_width BusWidth = sd_bus_width::four,
          sd_timing_mode TimingMode = sd_timing_mode::default_speed, bool EnableDma = true,
          bool EnableInterrupts = true>
struct bound_host_config final
{
  using binding = Binding;

  static constexpr std::uint32_t clock_hz{ClockHz};
  static constexpr sd_bus_width bus_width{BusWidth};
  static constexpr sd_timing_mode timing_mode{TimingMode};
  static constexpr bool enable_dma{EnableDma};
  static constexpr bool enable_interrupts{EnableInterrupts};
};

template <typename Backend, typename Config>
  requires SdConcept<Backend, Config>
struct card final
{
  using backend_tag = Backend;
  using config_type = Config;

  static constexpr peripheral_kind kind{peripheral_kind::sd};
  static constexpr bool has_high_speed{backend_traits<Backend>::sd_capability().high_speed};
  static constexpr bool has_uhs{backend_traits<Backend>::sd_capability().uhs};
  static constexpr bool has_4bit_bus{backend_traits<Backend>::sd_capability().bus_width_4};
  static constexpr bool has_8bit_bus{backend_traits<Backend>::sd_capability().bus_width_8};
  static constexpr std::uint32_t block_size_bytes{backend_traits<Backend>::sd_capability().block_size_bytes};

  /**
   * @brief Initializes the configured SD card host.
   * @return Backend initialization status.
   */
  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return sd_api_t<Backend>::template initialize<Config>(); });
  }

  /**
   * @brief Reads one or more contiguous SD card blocks.
   * @param[in]  start_block Index of the first block to read.
   * @param[out] buffer      Destination storage; its size must be a nonzero multiple of block_size_bytes.
   * @return @ref microhal::status::invalid_argument for an empty, non-integral, or overflowing block range;
   *         otherwise the backend read status.
   */
  [[nodiscard]] static inline status read_blocks(const std::uint32_t start_block,
                                                 const std::span<std::uint8_t> buffer) noexcept
  {
    const std::uint32_t block_size = block_size_bytes;
    if ((block_size == 0U) || buffer.empty() || ((buffer.size() % block_size) != 0U))
    {
      return status::invalid_argument;
    }

    const std::size_t blocks = buffer.size() / block_size;
    if ((blocks > std::numeric_limits<std::uint32_t>::max()) ||
        (static_cast<std::uint32_t>(blocks) > (std::numeric_limits<std::uint32_t>::max() - start_block)))
    {
      return status::invalid_argument;
    }

    return detail::as_status([start_block, buffer]
                             { return sd_api_t<Backend>::template read_blocks<Config>(start_block, buffer); });
  }

  /**
   * @brief Writes one or more contiguous SD card blocks.
   * @param[in] start_block Index of the first block to write.
   * @param[in] buffer      Source bytes; its size must be a nonzero multiple of block_size_bytes and storage is used
   *                        for the duration of this call.
   * @return @ref microhal::status::invalid_argument for an empty, non-integral, or overflowing block range;
   *         otherwise the backend write status.
   */
  [[nodiscard]] static inline status write_blocks(const std::uint32_t start_block,
                                                  const std::span<const std::uint8_t> buffer) noexcept
  {
    const std::uint32_t block_size = block_size_bytes;
    if ((block_size == 0U) || buffer.empty() || ((buffer.size() % block_size) != 0U))
    {
      return status::invalid_argument;
    }

    const std::size_t blocks = buffer.size() / block_size;
    if ((blocks > std::numeric_limits<std::uint32_t>::max()) ||
        (static_cast<std::uint32_t>(blocks) > (std::numeric_limits<std::uint32_t>::max() - start_block)))
    {
      return status::invalid_argument;
    }

    return detail::as_status([start_block, buffer]
                             { return sd_api_t<Backend>::template write_blocks<Config>(start_block, buffer); });
  }

  /**
   * @brief Erases a contiguous range of SD card blocks.
   * @param[in] start_block Index of the first block to erase.
   * @param[in] block_count Number of blocks to erase; must be nonzero.
   * @return @ref microhal::status::invalid_argument for a zero or overflowing range,
   *         @ref microhal::status::unsupported when the backend does not advertise erase, or the backend status.
   */
  [[nodiscard]] static inline status erase_blocks(const std::uint32_t start_block,
                                                  const std::uint32_t block_count) noexcept
  {
    if ((block_count == 0U) || (block_count > (std::numeric_limits<std::uint32_t>::max() - start_block)))
    {
      return status::invalid_argument;
    }
    if (!backend_traits<Backend>::sd_capability().erase)
    {
      return status::unsupported;
    }

    return detail::as_status([start_block, block_count]
                             { return sd_api_t<Backend>::template erase_blocks<Config>(start_block, block_count); });
  }

  /**
   * @brief Queries the card capacity in blocks.
   * @return Block count on success, or the backend capacity-query error.
   */
  [[nodiscard]] static inline result<std::uint32_t> capacity_blocks() noexcept
  {
    return detail::as_result<std::uint32_t>([] { return sd_api_t<Backend>::template capacity_blocks<Config>(); });
  }

  /**
   * @brief Checks whether a nonempty block range fits within the queried card capacity.
   * @param[in] start_block Index of the first block.
   * @param[in] block_count Number of blocks in the range.
   * @return True when the range is nonzero, does not overflow, and is within a successfully queried capacity.
   * @note Returns false if the backend capacity query fails.
   */
  [[nodiscard]] static inline bool is_valid_range(const std::uint32_t start_block,
                                                  const std::uint32_t block_count) noexcept
  {
    if ((block_count == 0U) || (block_count > (std::numeric_limits<std::uint32_t>::max() - start_block)))
    {
      return false;
    }

    const result<std::uint32_t> capacity = capacity_blocks();
    return capacity && (start_block <= capacity.value()) && (block_count <= (capacity.value() - start_block));
  }

  /**
   * @brief Reads exactly one SD card block.
   * @param[in]  block  Index of the block to read.
   * @param[out] buffer Destination storage; its size must equal block_size_bytes.
   * @return @ref microhal::status::invalid_argument for a buffer of the wrong size, or the read_blocks result.
   */
  [[nodiscard]] static inline status read_block(const std::uint32_t block,
                                                const std::span<std::uint8_t> buffer) noexcept
  {
    return (buffer.size() == block_size_bytes) ? read_blocks(block, buffer) : status::invalid_argument;
  }

  /**
   * @brief Writes exactly one SD card block.
   * @param[in] block  Index of the block to write.
   * @param[in] buffer Source bytes; its size must equal block_size_bytes and storage is used for this call.
   * @return @ref microhal::status::invalid_argument for a buffer of the wrong size, or the write_blocks result.
   */
  [[nodiscard]] static inline status write_block(const std::uint32_t block,
                                                 const std::span<const std::uint8_t> buffer) noexcept
  {
    return (buffer.size() == block_size_bytes) ? write_blocks(block, buffer) : status::invalid_argument;
  }
};

} // namespace microhal::sd
