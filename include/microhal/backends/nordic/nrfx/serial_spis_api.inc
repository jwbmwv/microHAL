// SPDX-License-Identifier: Apache-2.0
template <typename Config> static status initialize_slave() noexcept
{
#if MICROHAL_NORDIC_HAS_SPIS
  if constexpr (requires {
                  Config::binding::sck_pin;
                  Config::binding::ss_pin;
                })
  {
    if (Config::instance_index >= std::size(detail::spis_instances))
      return status::invalid_argument;
    if (Config::duplex_mode == spi_duplex_mode::half)
      return status::unsupported;
    constexpr auto mosi = []() -> std::uint32_t
    {
      if constexpr (requires { Config::binding::mosi_pin; })
        return Config::binding::mosi_pin;
      return NRF_SPIS_PIN_NOT_CONNECTED;
    }();
    constexpr auto miso = []() -> std::uint32_t
    {
      if constexpr (requires { Config::binding::miso_pin; })
        return Config::binding::miso_pin;
      return NRF_SPIS_PIN_NOT_CONNECTED;
    }();
    if ((Config::duplex_mode != spi_duplex_mode::simplex_tx && mosi == NRF_SPIS_PIN_NOT_CONNECTED) ||
        (Config::duplex_mode != spi_duplex_mode::simplex_rx && miso == NRF_SPIS_PIN_NOT_CONNECTED))
      return status::invalid_argument;
    auto& instance = detail::spis_instances[Config::instance_index];
    if (nrfx_spis_init_check(&instance))
      return status::busy;
    nrfx_spis_config_t config{};
    config.sck_pin = Config::binding::sck_pin;
    config.mosi_pin = Config::duplex_mode == spi_duplex_mode::simplex_tx ? NRF_SPIS_PIN_NOT_CONNECTED : mosi;
    config.miso_pin = Config::duplex_mode == spi_duplex_mode::simplex_rx ? NRF_SPIS_PIN_NOT_CONNECTED : miso;
    config.csn_pin = Config::binding::ss_pin;
    config.mode = static_cast<nrf_spis_mode_t>((Config::clock_polarity == spi_clock_polarity::idle_high ? 2U : 0U) |
                                               (Config::clock_phase == spi_clock_phase::second_edge ? 1U : 0U));
    config.bit_order =
        Config::bit_order == spi_bit_order::msb_first ? NRF_SPIS_BIT_ORDER_MSB_FIRST : NRF_SPIS_BIT_ORDER_LSB_FIRST;
    config.csn_pullup = NRF_GPIO_PIN_PULLUP;
    config.miso_drive = NRF_GPIO_PIN_S0S1;
    config.def = 0xFFU;
    config.orc = 0xFFU;
    config.irq_priority = 7U;
    detail::spis_states[Config::instance_index].instance = &instance;
    return detail::serial_initialize(instance.p_reg, 2U,
                                     [driver = &instance, &config]
                                     {
#if defined(NRF52840_XXAA)
                                       if constexpr (requires { Config::binding::spis_gpiote; })
                                         nrfx_spis_nrf52_anomaly_109_init(driver, Config::binding::spis_gpiote);
#endif
                                       return nrfx_spis_init(driver, &config, detail::spis_handler,
                                                             &detail::spis_states[Config::instance_index]);
                                     });
  }
#endif
  return status::unsupported;
}

template <typename Config, auto Handler>
static status slave_transfer(std::span<const std::uint8_t> tx, std::span<std::uint8_t> rx) noexcept
{
#if MICROHAL_NORDIC_HAS_SPIS
  if (Config::instance_index >= std::size(detail::spis_instances) || (tx.empty() && rx.empty()) ||
      !detail::serial_count_valid(tx.size()) || !detail::serial_count_valid(rx.size()))
    return status::invalid_argument;
  if ((Config::duplex_mode == spi_duplex_mode::simplex_tx && !rx.empty()) ||
      (Config::duplex_mode == spi_duplex_mode::simplex_rx && !tx.empty()))
    return status::unsupported;
  auto& instance = detail::spis_instances[Config::instance_index];
  if (!nrfx_spis_init_check(&instance))
    return status::unsupported;
  auto& state = detail::spis_states[Config::instance_index];
  if (!state.transfer.begin(detail::notify<Handler>))
    return status::busy;
  state.tx_size = tx.size();
  state.rx_size = rx.size();
  instance.p_reg->STATUS = NRF_SPIS_STATUS_OVERREAD | NRF_SPIS_STATUS_OVERFLOW;
  const auto result =
      detail::driver_result(nrfx_spis_buffers_set(&instance, tx.data(), tx.size(), rx.data(), rx.size()));
  if (result != status::ok)
  {
    state.transfer.completion = nullptr;
    state.transfer.finish(result);
  }
  return result;
#else
  (void)tx;
  (void)rx;
  return status::unsupported;
#endif
}

template <typename Config> static status abort() noexcept
{
  if constexpr (serial_slave_enabled<Config>())
  {
#if MICROHAL_NORDIC_HAS_SPIS
    if (Config::instance_index >= std::size(detail::spis_instances))
      return status::invalid_argument;
    auto& instance = detail::spis_instances[Config::instance_index];
    auto& state = detail::spis_states[Config::instance_index].transfer;
    if (!nrfx_spis_init_check(&instance))
      return state.complete.load(std::memory_order_acquire) && !state.active.load() ? status::ok : status::unsupported;
    if (state.active.load())
    {
      // SPIS has no abort primitive; uninit stops DMA and releases the semaphore.
      nrfx_spis_uninit(&instance);
      detail::serial_release(instance.p_reg, 2U);
      state.finish(status::error);
      // Reinitialization is required after cancelling an armed slave transaction.
    }
    return status::ok;
#endif
  }
  else
  {
#if MICROHAL_NORDIC_HAS_SPIM
    if (Config::instance_index >= std::size(detail::spim_instances))
      return status::invalid_argument;
    auto& instance = detail::spim_instances[Config::instance_index];
    if (!nrfx_spim_init_check(&instance))
      return status::unsupported;
    auto& state = detail::spim_states[Config::instance_index];
    if (state.active.load())
    {
      nrfx_spim_abort(&instance);
      state.finish(status::error);
    }
    return status::ok;
#endif
  }
  return status::unsupported;
}

template <typename Config> static status transfer_status() noexcept
{
  if constexpr (serial_slave_enabled<Config>())
  {
#if MICROHAL_NORDIC_HAS_SPIS
    if (Config::instance_index >= std::size(detail::spis_instances))
      return status::invalid_argument;
    auto& state = detail::spis_states[Config::instance_index].transfer;
    if (!state.complete.load(std::memory_order_acquire) &&
        !nrfx_spis_init_check(&detail::spis_instances[Config::instance_index]))
      return status::unsupported;
    return state.active.load(std::memory_order_acquire)
               ? status::busy
               : (state.complete.load(std::memory_order_acquire) ? state.result : status::ok);
#endif
  }
  else
  {
#if MICROHAL_NORDIC_HAS_SPIM
    if (Config::instance_index >= std::size(detail::spim_instances))
      return status::invalid_argument;
    auto& state = detail::spim_states[Config::instance_index];
    if (!state.complete.load(std::memory_order_acquire) &&
        !nrfx_spim_init_check(&detail::spim_instances[Config::instance_index]))
      return status::unsupported;
    return state.active.load(std::memory_order_acquire)
               ? status::busy
               : (state.complete.load(std::memory_order_acquire) ? state.result : status::ok);
#endif
  }
  return status::unsupported;
}

template <typename Config> static status dispatch() noexcept
{
  if constexpr (serial_slave_enabled<Config>())
  {
#if MICROHAL_NORDIC_HAS_SPIS
    if (Config::instance_index >= std::size(detail::spis_instances))
      return status::invalid_argument;
    auto& instance = detail::spis_instances[Config::instance_index];
    if (!nrfx_spis_init_check(&instance))
      return status::unsupported;
    nrfx_spis_irq_handler(&instance);
    return status::ok;
#endif
  }
  else
  {
#if MICROHAL_NORDIC_HAS_SPIM
    if (Config::instance_index >= std::size(detail::spim_instances))
      return status::invalid_argument;
    auto& instance = detail::spim_instances[Config::instance_index];
    if (!nrfx_spim_init_check(&instance))
      return status::unsupported;
    nrfx_spim_irq_handler(&instance);
    return status::ok;
#endif
  }
  return status::unsupported;
}

template <typename Config> static status write_blocking(std::span<const std::uint8_t> bytes) noexcept
{
  return transfer<Config>(bytes, {});
}
template <typename Config> static status read_blocking(std::span<std::uint8_t> bytes) noexcept
{
  return transfer<Config>({}, bytes);
}
