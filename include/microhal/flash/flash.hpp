// SPDX-License-Identifier: Apache-2.0
/**
 * @file flash.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <functional>
#include <span>

#include "microhal/core/concepts.hpp"

namespace microhal::flash
{

/**
 * @brief Compile-time FLASH erase and program constraints.
 */
template <std::uint32_t ProgramAlignmentBytes, std::uint32_t EraseSizeBytes, bool EnableDma = false,
          bool EnableInterrupts = false>
struct flash_config final
{
  static constexpr std::uint32_t program_alignment_bytes{ProgramAlignmentBytes};
  static constexpr std::uint32_t erase_size_bytes{EraseSizeBytes};
  static constexpr bool enable_dma{EnableDma};
  static constexpr bool enable_interrupts{EnableInterrupts};
};

/**
 * @brief Zero-overhead FLASH controller facade with alignment and erase-size validation.
 */
template <typename Backend, typename Config>
  requires microhal::FlashConcept<Backend, Config>
struct controller final
{
  using backend_tag = Backend;
  using config_type = Config;

  static constexpr peripheral_kind kind{peripheral_kind::flash};

  [[nodiscard]] static inline status erase(const std::uint32_t address, const std::uint32_t bytes) noexcept
  {
    if ((bytes == 0U) || ((address % Config::erase_size_bytes) != 0U) || ((bytes % Config::erase_size_bytes) != 0U) ||
        ((static_cast<std::uint64_t>(address) + bytes) > (static_cast<std::uint64_t>(UINT32_MAX) + 1U)))
    {
      return status::invalid_argument;
    }

    const auto first_region = flash_api_t<Backend>::template erase_region<Config>(address);
    if (!first_region.has_value() && (first_region.error() != status::unsupported))
    {
      return first_region.error();
    }

    if (first_region.has_value())
    {
      std::uint64_t cursor = address;
      std::uint64_t remaining = bytes;
      while (remaining > 0U)
      {
        if (cursor > UINT32_MAX)
        {
          return status::invalid_argument;
        }

        const auto region = flash_api_t<Backend>::template erase_region<Config>(static_cast<std::uint32_t>(cursor));
        if (!region.has_value())
        {
          return region.error();
        }
        if ((region.value().start_address != cursor) || (region.value().size_bytes == 0U) ||
            (region.value().size_bytes > remaining))
        {
          return status::invalid_argument;
        }

        cursor += region.value().size_bytes;
        remaining -= region.value().size_bytes;
      }
    }

    return detail::as_status([address, bytes] { return flash_api_t<Backend>::template erase<Config>(address, bytes); });
  }

  /**
   * @brief Programs bytes that satisfy the configured hardware programming unit.
   * @details Address and payload size must both be multiples of
   *          @c Config::program_alignment_bytes. This writes raw flash and does not provide
   *          erase-before-write emulation or partition protection.
   */
  [[nodiscard]] static inline status program(const std::uint32_t address,
                                             const std::span<const std::uint8_t> payload) noexcept
  {
    if (payload.empty() || (payload.size() > UINT32_MAX) || ((address % Config::program_alignment_bytes) != 0U) ||
        ((payload.size() % Config::program_alignment_bytes) != 0U) ||
        ((static_cast<std::uint64_t>(address) + payload.size()) > (static_cast<std::uint64_t>(UINT32_MAX) + 1U)))
    {
      return status::invalid_argument;
    }

    return detail::as_status([address, payload]
                             { return flash_api_t<Backend>::template program<Config>(address, payload); });
  }

  /**
   * @brief Programs data and verifies it through the backend's read path.
   * @details The caller supplies storage so the operation does not allocate or place an
   *          unbounded temporary buffer on the stack.
   */
  [[nodiscard]] static inline status program_and_verify(const std::uint32_t address,
                                                        const std::span<const std::uint8_t> payload,
                                                        const std::span<std::uint8_t> readback) noexcept
  {
    if (payload.size() != readback.size())
    {
      return status::invalid_argument;
    }
    if (!payload.empty())
    {
      const auto* const payload_end = payload.data() + payload.size();
      const auto* const readback_end = readback.data() + readback.size();
      const std::less<const std::uint8_t*> less{};
      if (less(payload.data(), readback_end) && less(readback.data(), payload_end))
      {
        return status::invalid_argument;
      }
    }

    const status programmed = program(address, payload);
    if (programmed != status::ok)
    {
      return programmed;
    }

    const status read_status = read(address, readback);
    if (read_status != status::ok)
    {
      return read_status;
    }

    for (std::size_t index = 0U; index < payload.size(); ++index)
    {
      if (payload[index] != readback[index])
      {
        return status::io_error;
      }
    }
    return status::ok;
  }

  /**
   * @brief Returns the physical erase block containing @p address, when the backend can query it.
   * @details The returned region is hardware geometry, not an application partition.
   */
  [[nodiscard]] static inline result<flash_erase_region> erase_region(const std::uint32_t address) noexcept
  {
    return detail::as_result<flash_erase_region>(
        [address] { return flash_api_t<Backend>::template erase_region<Config>(address); });
  }

  [[nodiscard]] static inline status read(const std::uint32_t address, const std::span<std::uint8_t> payload) noexcept
  {
    return detail::as_status([address, payload]
                             { return flash_api_t<Backend>::template read<Config>(address, payload); });
  }
};

} // namespace microhal::flash
