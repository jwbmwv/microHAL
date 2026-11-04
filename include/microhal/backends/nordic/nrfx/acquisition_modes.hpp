// SPDX-License-Identifier: Apache-2.0
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
template <> struct serial_mode_api<nordic_nrf_tag, peripheral_kind::i2s>
{
  using api = backends::nordic::nrfx::i2s_api;

  template <typename Config> static status initialize() noexcept
  {
    if constexpr (configured_transfer_mode<Config>() == transfer_mode::interrupt)
      return status::unsupported;
    return api::template initialize<Config>();
  }
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
  template <typename Config> static status abort() noexcept
  {
    return api::template abort<Config>();
  }
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
  template <typename Config> static status handle_interrupt() noexcept
  {
#if MICROHAL_NORDIC_HAS_I2S
    api::handle_interrupt();
    return transfer_status<Config>();
#else
    return status::unsupported;
#endif
  }
  template <typename Config> static status handle_dma_interrupt() noexcept
  {
    return handle_interrupt<Config>();
  }
};
} // namespace microhal
