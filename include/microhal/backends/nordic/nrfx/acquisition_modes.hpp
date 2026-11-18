// SPDX-License-Identifier: Apache-2.0
/** @file
 * @brief Nordic nrfx acquisition transfer-mode implementations.
 */
#pragma once

#include <type_traits>

#ifndef MICROHAL_NORDIC_NRFX_PRESENT
#include "microhal/backends/nordic/nrfx/traits.hpp"
#else
#include "microhal/backends/nordic/nrfx/peripherals.hpp"
#endif

namespace microhal
{
// The nrfx I2S data path always uses EasyDMA. Polling changes completion servicing,
// not the hardware data path; bytewise, non-DMA interrupt transfers do not exist.
/// @brief Selects the serial transfer-mode implementation for the backend and peripheral.
template <> struct serial_mode_api<nordic_nrf_tag, peripheral_kind::i2s>
{
  using api = backends::nordic::nrfx::i2s_api;

  /// @brief Initializes the configured backend resource.
  template <typename Config> static status initialize() noexcept
  {
    if constexpr (configured_transfer_mode<Config>() == transfer_mode::interrupt)
      return status::unsupported;
    return api::template initialize<Config>();
  }
  /// @brief Transmits data using the configured peripheral.
  template <typename Config, auto Handler> static status transmit(const std::span<const std::uint16_t> samples) noexcept
  {
    if constexpr (configured_transfer_mode<Config>() == transfer_mode::interrupt)
      return status::unsupported;
    else if constexpr (std::is_same_v<decltype(Handler), std::nullptr_t>)
      return api::template transmit_blocking<Config>(samples);
    else if constexpr (configured_transfer_mode<Config>() != transfer_mode::polled)
      return api::template start_dma_tx<Config, Handler>(samples);
    return status::unsupported;
  }
  /// @brief Receives data using the configured peripheral.
  template <typename Config, auto Handler> static status receive(const std::span<std::uint16_t> samples) noexcept
  {
    if constexpr (configured_transfer_mode<Config>() == transfer_mode::interrupt)
      return status::unsupported;
    else if constexpr (std::is_same_v<decltype(Handler), std::nullptr_t>)
      return api::template receive_blocking<Config>(samples);
    else if constexpr (configured_transfer_mode<Config>() != transfer_mode::polled)
      return api::template start_dma_rx<Config, Handler>(samples);
    return status::unsupported;
  }
  /// @brief Attempts to cancel the active peripheral operation.
  template <typename Config> static status abort() noexcept
  {
    return api::template abort<Config>();
  }
  /// @brief Transfers data using the configured peripheral.
  template <typename Config, auto Handler>
  static status transfer(const std::span<const std::uint16_t> tx, const std::span<std::uint16_t> rx) noexcept
  {
    if constexpr (configured_transfer_mode<Config>() == transfer_mode::interrupt)
      return status::unsupported;
    else if constexpr (std::is_same_v<decltype(Handler), std::nullptr_t>)
      return api::template transfer_blocking<Config>(tx, rx);
    else if constexpr (configured_transfer_mode<Config>() != transfer_mode::polled)
      return api::template start_dma_transfer<Config, Handler>(tx, rx);
    return status::unsupported;
  }
  /// @brief Queries the current or most recently completed transfer status.
  template <typename Config> static status transfer_status() noexcept
  {
#if MICROHAL_NORDIC_HAS_I2S
    const auto value = api::template transfer_status<Config>();
    if (!value)
      return value.error();
    switch (value.value())
    {
    case dma_transfer_state::active:
      return status::busy;
    case dma_transfer_state::error:
      return backends::nordic::nrfx::detail::i2s_transfer.result;
    default:
      return status::ok;
    }
#else
    return status::unsupported;
#endif
  }
  /// @brief Handles the interrupt for the I2S interface.
  template <typename Config> static status handle_interrupt() noexcept
  {
#if MICROHAL_NORDIC_HAS_I2S
    api::handle_interrupt();
    return transfer_status<Config>();
#else
    return status::unsupported;
#endif
  }
  /// @brief Handles the DMA interrupt for the I2S interface.
  template <typename Config> static status handle_dma_interrupt() noexcept
  {
    return handle_interrupt<Config>();
  }
};
} // namespace microhal
