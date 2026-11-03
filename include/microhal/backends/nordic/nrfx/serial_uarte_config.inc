// SPDX-License-Identifier: Apache-2.0
template <typename Config> status uarte_configure_hwfc(nrfx_uarte_config_t& config) noexcept
{
  config.rts_pin = NRF_UARTE_PSEL_DISCONNECTED;
  config.cts_pin = NRF_UARTE_PSEL_DISCONNECTED;
  config.config.hwfc = NRF_UARTE_HWFC_DISABLED;
  if constexpr (requires { Config::binding::hardware_flow_control; })
  {
    if constexpr (!std::is_same_v<std::remove_cv_t<decltype(Config::binding::hardware_flow_control)>, bool>)
      return status::invalid_argument;
    else if constexpr (Config::binding::hardware_flow_control)
    {
      if constexpr (!requires {
                      Config::binding::rts_pin;
                      Config::binding::cts_pin;
                    })
        return status::invalid_argument;
      else if constexpr (!std::is_integral_v<std::remove_cv_t<decltype(Config::binding::rts_pin)>> ||
                         !std::is_integral_v<std::remove_cv_t<decltype(Config::binding::cts_pin)>> ||
                         std::is_same_v<std::remove_cv_t<decltype(Config::binding::rts_pin)>, bool> ||
                         std::is_same_v<std::remove_cv_t<decltype(Config::binding::cts_pin)>, bool>)
        return status::invalid_argument;
      else
      {
        if constexpr (std::is_signed_v<decltype(Config::binding::rts_pin)>)
          if (Config::binding::rts_pin < 0)
            return status::invalid_argument;
        if constexpr (std::is_signed_v<decltype(Config::binding::cts_pin)>)
          if (Config::binding::cts_pin < 0)
            return status::invalid_argument;
        if (static_cast<std::uint64_t>(Config::binding::rts_pin) > UINT32_MAX ||
            static_cast<std::uint64_t>(Config::binding::cts_pin) > UINT32_MAX)
          return status::invalid_argument;
        const auto rts = static_cast<std::uint32_t>(Config::binding::rts_pin);
        const auto cts = static_cast<std::uint32_t>(Config::binding::cts_pin);
        if (!nrf_gpio_pin_present_check(rts) || !nrf_gpio_pin_present_check(cts) || rts == cts ||
            rts == config.txd_pin || rts == config.rxd_pin || cts == config.txd_pin || cts == config.rxd_pin)
          return status::invalid_argument;
        config.rts_pin = rts;
        config.cts_pin = cts;
        config.config.hwfc = NRF_UARTE_HWFC_ENABLED;
      }
    }
  }
  return status::ok;
}

template <typename Config, typename Handler>
status uarte_initialize_driver(nrfx_uarte_t& instance, nrfx_uarte_config_t& config, Handler handler) noexcept
{
  const auto configured = uarte_configure_hwfc<Config>(config);
  if (configured != status::ok)
    return configured;
  return serial_initialize(instance.p_reg, 3U, [driver = &instance, &config, handler]
                           { return nrfx_uarte_init(driver, &config, handler); });
}
