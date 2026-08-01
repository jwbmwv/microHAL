// SPDX-License-Identifier: Apache-2.0
/**
 * @file zero_overhead_probe.cpp
 * @brief Evidence for the zero-overhead claim, checked by `scripts/check_zero_overhead.sh`.
 * @details The claim "calling through microHAL costs nothing versus calling the backend directly"
 *          is easy to assert and easy to get wrong. This translation unit compiles two functions
 *          that must produce byte-identical machine code:
 *
 *          - `via_microhal` goes through the full stack: concept checks, `backend_traits`, the
 *            `*_api_t` alias, and `detail::as_status`.
 *          - `via_backend_directly` calls the same backend struct with no microHAL layer at all.
 *
 *          The probe backend below writes to a `volatile` register so the compiler cannot delete
 *          the work, and the two functions are marked `noinline` so they survive as comparable
 *          symbols. The script disassembles both and diffs them.
 */

#include "microhal/core/backend_selector.hpp"
#include "microhal/uart/uart.hpp"

#include <cstdint>

namespace probe
{

/// @brief Stand-in for a memory-mapped transmit data register.
inline volatile std::uint32_t tx_register = 0U;

struct uart_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    tx_register = Config::baud_rate;
    return true;
  }

  template <typename Config> static bool write_blocking(const std::span<const std::uint8_t> payload) noexcept
  {
    for (const std::uint8_t byte : payload)
    {
      tx_register = byte;
    }
    return true;
  }

  template <typename Config> static bool read_blocking(std::span<std::uint8_t>) noexcept
  {
    return true;
  }
};

} // namespace probe

namespace microhal
{

/// @brief Out-of-tree backend tag. Declaring it here also proves a backend can live outside the library.
struct probe_tag final
{
};

template <> struct backend_traits<probe_tag> final
{
  using backend_tag = probe_tag;
  using uart_api = probe::uart_api;

  static constexpr vendor_kind vendor = vendor_kind::generic;
  static constexpr backend_kind kind = backend_kind::custom;
  static constexpr std::string_view name{"zero-overhead-probe"};

  static consteval struct uart_capability uart_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 4U,
            .max_frequency_hz = 10'000'000U,
            .fifo = false,
            .lpuart = false,
            .autobaud = false,
            .fifo_depth = 0U};
  }
};

} // namespace microhal

namespace
{

using config = microhal::uart::port_config<1U, 115'200U>;

} // namespace

extern "C" [[gnu::noinline]] int via_microhal(const std::uint8_t* const data, const std::size_t length)
{
  using port = microhal::uart::port<microhal::probe_tag, config>;
  if (!microhal::is_ok(port::initialize()))
  {
    return 1;
  }
  return microhal::is_ok(port::write_blocking(std::span<const std::uint8_t>{data, length})) ? 0 : 1;
}

extern "C" [[gnu::noinline]] int via_backend_directly(const std::uint8_t* const data, const std::size_t length)
{
  if (!probe::uart_api::initialize<config>())
  {
    return 1;
  }
  return probe::uart_api::write_blocking<config>(std::span<const std::uint8_t>{data, length}) ? 0 : 1;
}
