// SPDX-License-Identifier: Apache-2.0
template <typename Config> static status initialize_slave() noexcept
{
#if MICROHAL_NORDIC_HAS_TWIS
  if constexpr (requires {
                  Config::binding::scl_pin;
                  Config::binding::sda_pin;
                })
  {
    if (Config::instance_index >= std::size(detail::twis_instances) || Config::own_address == 0U ||
        Config::own_address > 0x7FU)
      return status::invalid_argument;
    if (Config::address_mode != i2c_address_mode::seven_bit)
      return status::unsupported;
    auto& instance = detail::twis_instances[Config::instance_index];
    if (nrfx_twis_init_check(&instance))
      return status::busy;
    nrfx_twis_config_t config{};
    config.scl_pin = Config::binding::scl_pin;
    config.sda_pin = Config::binding::sda_pin;
    config.addr[0] = Config::own_address;
    config.scl_pull = NRF_GPIO_PIN_PULLUP;
    config.sda_pull = NRF_GPIO_PIN_PULLUP;
    config.interrupt_priority = 7U;
    return detail::serial_initialize(
        instance.p_reg, 5U, [driver = &instance, &config]
        { return nrfx_twis_init(driver, &config, detail::twis_callbacks[Config::instance_index]); });
  }
#endif
  return status::unsupported;
}

template <typename Config, auto Handler>
static status slave_transfer(std::span<const std::uint8_t> tx, std::span<std::uint8_t> rx) noexcept
{
#if MICROHAL_NORDIC_HAS_TWIS
  if (Config::instance_index >= std::size(detail::twis_instances) || (tx.empty() == rx.empty()) ||
      !detail::serial_count_valid(tx.size()) || !detail::serial_count_valid(rx.size()))
    return status::invalid_argument;
  auto& instance = detail::twis_instances[Config::instance_index];
  if (!nrfx_twis_init_check(&instance))
    return status::unsupported;
  auto& state = detail::twis_states[Config::instance_index];
  if (!state.transfer.begin(detail::notify<Handler>))
    return status::busy;
  state.tx = tx;
  state.rx = rx;
  state.pending_complete = false;
  nrfx_twis_enable(&instance);
  return status::ok;
#else
  (void)tx;
  (void)rx;
  return status::unsupported;
#endif
}

template <typename Config, auto Handler>
static status master_transfer(std::span<const std::uint8_t> tx, std::span<std::uint8_t> rx,
                              std::uint16_t address) noexcept
{
#if MICROHAL_NORDIC_HAS_TWIM
  if (Config::instance_index >= detail::twim_instance_count || (tx.empty() && rx.empty()) ||
      !detail::serial_count_valid(tx.size()) || !detail::serial_count_valid(rx.size()) || address > 0x7FU)
    return status::invalid_argument;
  if (Config::address_mode != i2c_address_mode::seven_bit)
    return status::unsupported;
  auto* instance = detail::twim_instance(Config::instance_index);
  if (!nrfx_twim_init_check(instance))
    return status::unsupported;
  nrfx_twim_xfer_desc_t descriptor{};
  descriptor.address = static_cast<std::uint8_t>(address);
  descriptor.type = tx.empty() ? NRFX_TWIM_XFER_RX : (rx.empty() ? NRFX_TWIM_XFER_TX : NRFX_TWIM_XFER_TXRX);
  descriptor.p_primary_buf = tx.empty() ? rx.data() : const_cast<std::uint8_t*>(tx.data());
  descriptor.primary_length = tx.empty() ? rx.size() : tx.size();
  descriptor.p_secondary_buf = tx.empty() ? nullptr : rx.data();
  descriptor.secondary_length = tx.empty() ? 0U : rx.size();
  auto& state = detail::twim_states[Config::instance_index];
  if (!state.begin(detail::notify<Handler>))
    return status::busy;
  const auto result = detail::driver_result(nrfx_twim_xfer(instance, &descriptor, 0U));
  if (result != status::ok)
  {
    state.completion = nullptr;
    state.finish(result);
  }
  return result;
#else
  (void)tx;
  (void)rx;
  (void)address;
  return status::unsupported;
#endif
}

template <typename Config> static status abort() noexcept
{
  if constexpr (serial_slave_enabled<Config>())
  {
#if MICROHAL_NORDIC_HAS_TWIS
    if (Config::instance_index >= std::size(detail::twis_instances))
      return status::invalid_argument;
    auto& instance = detail::twis_instances[Config::instance_index];
    if (!nrfx_twis_init_check(&instance))
      return status::unsupported;
    auto& state = detail::twis_states[Config::instance_index].transfer;
    if (state.active.load())
    {
      nrfx_twis_disable(&instance);
      detail::twis_states[Config::instance_index].pending_complete = false;
      state.finish(status::error);
    }
    return status::ok;
#endif
  }
  else
  {
#if MICROHAL_NORDIC_HAS_TWIM
    if (Config::instance_index >= detail::twim_instance_count)
      return status::invalid_argument;
    auto* instance = detail::twim_instance(Config::instance_index);
    if (!nrfx_twim_init_check(instance))
      return status::unsupported;
    auto& state = detail::twim_states[Config::instance_index];
    if (state.active.load())
    {
      nrfx_twim_disable(instance);
      nrfx_twim_enable(instance);
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
#if MICROHAL_NORDIC_HAS_TWIS
    if (Config::instance_index >= std::size(detail::twis_instances))
      return status::invalid_argument;
    auto& state = detail::twis_states[Config::instance_index].transfer;
    if (!state.complete.load(std::memory_order_acquire) &&
        !nrfx_twis_init_check(&detail::twis_instances[Config::instance_index]))
      return status::unsupported;
    return state.active.load(std::memory_order_acquire)
               ? status::busy
               : (state.complete.load(std::memory_order_acquire) ? state.result : status::ok);
#endif
  }
  else
  {
#if MICROHAL_NORDIC_HAS_TWIM
    if (Config::instance_index >= detail::twim_instance_count)
      return status::invalid_argument;
    auto& state = detail::twim_states[Config::instance_index];
    if (!state.complete.load(std::memory_order_acquire) &&
        !nrfx_twim_init_check(detail::twim_instance(Config::instance_index)))
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
#if MICROHAL_NORDIC_HAS_TWIS
    if (Config::instance_index >= std::size(detail::twis_instances))
      return status::invalid_argument;
    auto& instance = detail::twis_instances[Config::instance_index];
    if (!nrfx_twis_init_check(&instance))
      return status::unsupported;
    nrfx_twis_irq_handler(&instance);
    // nrfx TWIS commits its state machine after invoking the driver callback.
    // Notify only after it returns, so a completion may safely arm another transfer.
    detail::twis_complete(Config::instance_index);
    return status::ok;
#endif
  }
  else
  {
#if MICROHAL_NORDIC_HAS_TWIM
    if (Config::instance_index >= detail::twim_instance_count)
      return status::invalid_argument;
    auto* instance = detail::twim_instance(Config::instance_index);
    if (!nrfx_twim_init_check(instance))
      return status::unsupported;
    nrfx_twim_irq_handler(instance);
    return status::ok;
#endif
  }
  return status::unsupported;
}
