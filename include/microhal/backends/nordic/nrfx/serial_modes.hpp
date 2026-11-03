// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <type_traits>

namespace microhal::backends::nordic::nrfx::detail
{
template <typename Config> constexpr bool serial_mode_supported() noexcept
{
  constexpr auto mode = configured_transfer_mode<Config>();
  return mode == transfer_mode::dma || (mode == transfer_mode::polled && !serial_slave_enabled<Config>());
}
template <typename Config, typename Register> void serial_mode_irq_config(Register* peripheral) noexcept
{
#if MICROHAL_NORDIC_NRFX_PRESENT
  if constexpr (configured_transfer_mode<Config>() == transfer_mode::polled)
    NRFX_IRQ_DISABLE(nrfx_get_irq_number(peripheral));
#else
  (void)peripheral;
#endif
}

template <typename Config, typename Poll, typename Cancel> status serial_wait(Poll poll, Cancel cancel) noexcept
{
#if MICROHAL_NORDIC_SYSTEM_CORE_CLOCK
  if (SystemCoreClock == 0U)
  {
    const auto cancelled = cancel();
    return cancelled == status::ok ? status::error : cancelled;
  }
#else
  const auto cancelled = cancel();
  return cancelled == status::ok ? status::unsupported : cancelled;
#endif
  status value = status::busy;
  const bool completed = wait_until<Config>(
      [&]
      {
        value = poll();
        return value != status::busy;
      });
  if (completed)
    return value;
  const auto abort_result = cancel();
  return abort_result == status::ok ? status::timeout : abort_result;
}
} // namespace microhal::backends::nordic::nrfx::detail

namespace microhal
{
// nrfx UARTE/SPIM/TWIM and the corresponding slaves all move payloads with EasyDMA.
// "interrupt" means bytewise non-DMA IRQ in the public contract and is not emulated.
template <> struct serial_mode_api<nordic_nrf_tag, peripheral_kind::uart>
{
  using api = backends::nordic::nrfx::uart_api;
  template <typename Config> static status initialize() noexcept
  {
    if constexpr (!backends::nordic::nrfx::detail::serial_mode_supported<Config>())
      return status::unsupported;
    const auto result = api::template initialize<Config>();
#if MICROHAL_NORDIC_HAS_UARTE
    if (result == status::ok)
      backends::nordic::nrfx::detail::serial_mode_irq_config<Config>(
          backends::nordic::nrfx::detail::uarte_instances[Config::instance_index].p_reg);
#endif
    return result;
  }
  template <typename Config, auto Handler> static status write(std::span<const std::uint8_t> bytes) noexcept
  {
    if constexpr (!backends::nordic::nrfx::detail::serial_mode_supported<Config>())
      return status::unsupported;
    else if constexpr (std::is_same_v<decltype(Handler), std::nullptr_t>)
      return api::template write_blocking<Config>(bytes);
    else if constexpr (configured_transfer_mode<Config>() == transfer_mode::dma)
      return api::template start_async_tx<Config, Handler>(bytes);
    return status::unsupported;
  }
  template <typename Config, auto Handler> static status read(std::span<std::uint8_t> bytes) noexcept
  {
    if constexpr (!backends::nordic::nrfx::detail::serial_mode_supported<Config>())
      return status::unsupported;
    else if constexpr (std::is_same_v<decltype(Handler), std::nullptr_t>)
      return api::template read_blocking<Config>(bytes);
    else if constexpr (configured_transfer_mode<Config>() == transfer_mode::dma)
      return api::template start_async_rx<Config, Handler>(bytes);
    return status::unsupported;
  }
  template <typename Config, auto Handler> static status start_receive_to_idle(std::span<std::uint8_t> bytes) noexcept
  {
    if constexpr (configured_transfer_mode<Config>() == transfer_mode::dma)
      return api::template start_receive_to_idle<Config, Handler>(bytes);
    return status::unsupported;
  }
  template <typename Config, auto Handler> static status start_circular_rx(std::span<std::uint8_t> bytes) noexcept
  {
    if constexpr (configured_transfer_mode<Config>() == transfer_mode::dma)
      return api::template start_circular_rx<Config, Handler>(bytes);
    return status::unsupported;
  }
  template <typename Config> static status abort_tx() noexcept
  {
    return api::template abort_tx<Config>();
  }
  template <typename Config> static status abort_rx() noexcept
  {
    return api::template abort_rx<Config>();
  }
  template <typename Config> static status tx_status() noexcept
  {
    return api::template tx_status<Config>();
  }
  template <typename Config> static status rx_status() noexcept
  {
    return api::template rx_status<Config>();
  }
  template <typename Config> static status abort() noexcept
  {
    return api::template abort<Config>();
  }
  template <typename Config> static status transfer_status() noexcept
  {
    return api::template transfer_status<Config>();
  }
  template <typename Config> static status handle_interrupt() noexcept
  {
#if MICROHAL_NORDIC_HAS_UARTE
    if (Config::instance_index >= std::size(backends::nordic::nrfx::detail::uarte_instances))
      return status::invalid_argument;
    if (!nrfx_uarte_init_check(&backends::nordic::nrfx::detail::uarte_instances[Config::instance_index]))
      return status::unsupported;
    api::handle_interrupt(Config::instance_index);
    return status::ok;
#else
    return status::unsupported;
#endif
  }
  template <typename Config> static status handle_dma_interrupt() noexcept
  {
    return handle_interrupt<Config>();
  }
  template <typename Config> static status handle_rx_timeout_interrupt() noexcept
  {
#if MICROHAL_NORDIC_HAS_SERIAL_TIMEOUT
    if (Config::instance_index >= std::size(backends::nordic::nrfx::detail::uarte_states))
      return status::invalid_argument;
    auto* timer = backends::nordic::nrfx::detail::uarte_states[Config::instance_index].timeout_timer;
    if (timer == nullptr)
      return status::unsupported;
    nrfx_timer_irq_handler(timer);
    return status::ok;
#else
    return status::unsupported;
#endif
  }
};

template <> struct serial_mode_api<nordic_nrf_tag, peripheral_kind::spi>
{
  using api = backends::nordic::nrfx::spi_api;
  template <typename Config> static status initialize() noexcept
  {
    if constexpr (!backends::nordic::nrfx::detail::serial_mode_supported<Config>())
      return status::unsupported;
    else if constexpr (serial_slave_enabled<Config>())
      return api::template initialize_slave<Config>();
    else
    {
      const auto result = api::template initialize<Config>();
#if MICROHAL_NORDIC_HAS_SPIM
      if (result == status::ok)
        backends::nordic::nrfx::detail::serial_mode_irq_config<Config>(
            backends::nordic::nrfx::detail::spim_instances[Config::instance_index].p_reg);
#endif
      return result;
    }
  }
  template <typename Config, auto Handler>
  static status transfer(std::span<const std::uint8_t> tx, std::span<std::uint8_t> rx) noexcept
  {
    constexpr bool sync = std::is_same_v<decltype(Handler), std::nullptr_t>;
    if constexpr (!backends::nordic::nrfx::detail::serial_mode_supported<Config>() ||
                  (!sync && configured_transfer_mode<Config>() == transfer_mode::polled))
      return status::unsupported;
    else if constexpr (serial_slave_enabled<Config>())
    {
      const auto result = api::template slave_transfer<Config, Handler>(tx, rx);
      if constexpr (sync)
      {
        if (result != status::ok)
          return result;
        return backends::nordic::nrfx::detail::serial_wait<Config>(
            [] { return api::template transfer_status<Config>(); }, [] { return api::template abort<Config>(); });
      }
      return result;
    }
    else if constexpr (sync)
      return api::template transfer<Config>(tx, rx);
    else
      return api::template transfer_async<Config, Handler>(tx, rx);
  }
  template <typename Config, auto Handler> static status write(std::span<const std::uint8_t> tx) noexcept
  {
    return transfer<Config, Handler>(tx, {});
  }
  template <typename Config, auto Handler> static status read(std::span<std::uint8_t> rx) noexcept
  {
    return transfer<Config, Handler>({}, rx);
  }
  template <typename Config> static status abort() noexcept
  {
    return api::template abort<Config>();
  }
  template <typename Config> static status transfer_status() noexcept
  {
    return api::template transfer_status<Config>();
  }
  template <typename Config> static status handle_interrupt() noexcept
  {
    return api::template dispatch<Config>();
  }
  template <typename Config> static status handle_dma_interrupt() noexcept
  {
    return handle_interrupt<Config>();
  }
};

template <> struct serial_mode_api<nordic_nrf_tag, peripheral_kind::i2c>
{
  using api = backends::nordic::nrfx::i2c_api;
  template <typename Config> static status initialize() noexcept
  {
    if constexpr (!backends::nordic::nrfx::detail::serial_mode_supported<Config>())
      return status::unsupported;
    else if constexpr (serial_slave_enabled<Config>())
      return api::template initialize_slave<Config>();
    else
    {
      const auto result = api::template initialize<Config>();
#if MICROHAL_NORDIC_HAS_TWIM
      if (result == status::ok)
        backends::nordic::nrfx::detail::serial_mode_irq_config<Config>(
            backends::nordic::nrfx::detail::twim_instance(Config::instance_index)->p_twim);
#endif
      return result;
    }
  }
  template <typename Config, auto Handler>
  static status submit(std::span<const std::uint8_t> tx, std::span<std::uint8_t> rx, std::uint16_t address) noexcept
  {
    constexpr bool sync = std::is_same_v<decltype(Handler), std::nullptr_t>;
    if constexpr (!backends::nordic::nrfx::detail::serial_mode_supported<Config>() ||
                  (!sync && configured_transfer_mode<Config>() == transfer_mode::polled))
      return status::unsupported;
    else
    {
      const auto result = [&]
      {
        if constexpr (serial_slave_enabled<Config>())
          return api::template slave_transfer<Config, Handler>(tx, rx);
        else
          return api::template master_transfer<Config, Handler>(tx, rx, address);
      }();
      if constexpr (sync)
      {
        if (result != status::ok)
          return result;
        return backends::nordic::nrfx::detail::serial_wait<Config>(
            []
            {
              if constexpr (configured_transfer_mode<Config>() == transfer_mode::polled)
                (void)api::template dispatch<Config>();
              return api::template transfer_status<Config>();
            },
            [] { return api::template abort<Config>(); });
      }
      return result;
    }
  }
  template <typename Config, auto Handler>
  static status write(std::span<const std::uint8_t> tx, std::uint16_t address = 0U) noexcept
  {
    return submit<Config, Handler>(tx, {}, address);
  }
  template <typename Config, auto Handler>
  static status read(std::span<std::uint8_t> rx, std::uint16_t address = 0U) noexcept
  {
    return submit<Config, Handler>({}, rx, address);
  }
  template <typename Config, auto Handler>
  static status write_read(std::span<const std::uint8_t> tx, std::span<std::uint8_t> rx, std::uint16_t address) noexcept
  {
    if (tx.empty() || rx.empty())
      return status::invalid_argument;
    return submit<Config, Handler>(tx, rx, address);
  }
  template <typename Config> static status probe(std::uint16_t address) noexcept
  {
    if constexpr (!backends::nordic::nrfx::detail::serial_mode_supported<Config>() || serial_slave_enabled<Config>())
      return status::unsupported;
    return api::template probe<Config>(address);
  }
  template <typename Config> static status abort() noexcept
  {
    return api::template abort<Config>();
  }
  template <typename Config> static status transfer_status() noexcept
  {
    return api::template transfer_status<Config>();
  }
  template <typename Config> static status handle_interrupt() noexcept
  {
    return api::template dispatch<Config>();
  }
  template <typename Config> static status handle_dma_interrupt() noexcept
  {
    return handle_interrupt<Config>();
  }
};
} // namespace microhal
