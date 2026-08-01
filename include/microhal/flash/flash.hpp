// SPDX-License-Identifier: Apache-2.0
/**
 * @file flash.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

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
    return detail::as_status([address, bytes] { return flash_api_t<Backend>::template erase<Config>(address, bytes); });
  }

  [[nodiscard]] static inline status program(const std::uint32_t address,
                                             const std::span<const std::uint8_t> payload) noexcept
  {
    return detail::as_status([address, payload]
                             { return flash_api_t<Backend>::template program<Config>(address, payload); });
  }

  [[nodiscard]] static inline status read(const std::uint32_t address, const std::span<std::uint8_t> payload) noexcept
  {
    return detail::as_status([address, payload]
                             { return flash_api_t<Backend>::template read<Config>(address, payload); });
  }
};

} // namespace microhal::flash
