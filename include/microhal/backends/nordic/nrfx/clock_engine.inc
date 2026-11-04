// SPDX-License-Identifier: Apache-2.0
// Included inside microhal::backends::nordic::nrfx.
struct rcc_api final
{
  template <typename Config> static constexpr bool owned() noexcept
  {
    if constexpr (requires { Config::binding::owns_clock; })
      return Config::binding::owns_clock;
    return false;
  }
  template <typename Config> static status configure() noexcept
  {
    if constexpr (!owned<Config>())
      return status::unsupported;
    else
    {
      // Generic PLL M/N/P parameters have no meaning on Nordic clock hardware.
      if (Config::enable_pll || static_cast<std::uint8_t>(Config::source) > 1U)
        return status::unsupported;
#if MICROHAL_NORDIC_HAS_CLOCK && (defined(NRF52840_XXAA) || defined(NRF5340_XXAA_APPLICATION))
#if defined(NRF52840_XXAA)
      if (Config::system_clock_hz != 64000000U)
        return status::invalid_argument;
#else
      if (Config::system_clock_hz != 64000000U && Config::system_clock_hz != 128000000U)
        return status::invalid_argument;
#endif
      if (!nrfx_clock_init_check())
      {
        const auto result = detail::driver_result(nrfx_clock_init(nullptr));
        if (result != status::ok)
          return result;
      }
      if (static_cast<std::uint8_t>(Config::source) == 1U)
      {
        nrfx_clock_start(NRF_CLOCK_DOMAIN_HFCLK);
        for (std::uint32_t remaining = 1000000U; !nrfx_clock_is_running(NRF_CLOCK_DOMAIN_HFCLK, nullptr); --remaining)
        {
          if (remaining == 0U)
            return status::timeout;
        }
      }
      else
        nrfx_clock_stop(NRF_CLOCK_DOMAIN_HFCLK);
#if defined(NRF5340_XXAA_APPLICATION)
      const auto result = detail::driver_result(nrfx_clock_divider_set(
          NRF_CLOCK_DOMAIN_HFCLK,
          Config::system_clock_hz == 128000000U ? NRF_CLOCK_HFCLK_DIV_1 : NRF_CLOCK_HFCLK_DIV_2));
      if (result != status::ok)
        return result;
#endif
      SystemCoreClockUpdate();
      return SystemCoreClock == Config::system_clock_hz ? status::ok : status::io_error;
#elif MICROHAL_NORDIC_HAS_OSCILLATORS
      if (Config::system_clock_hz != 64000000U && Config::system_clock_hz != 128000000U)
        return status::invalid_argument;
      // nRF54L's CPU is PLL clocked; selecting the external crystal as a CPU source is not equivalent.
      if (static_cast<std::uint8_t>(Config::source) != 0U)
        return status::unsupported;
      nrf_oscillators_pll_freq_set(NRF_OSCILLATORS, Config::system_clock_hz == 128000000U
                                                        ? NRF_OSCILLATORS_PLL_FREQ_128M
                                                        : NRF_OSCILLATORS_PLL_FREQ_64M);
      for (std::uint32_t remaining = 1000000U; remaining != 0U; --remaining)
      {
        SystemCoreClockUpdate();
        if (SystemCoreClock == Config::system_clock_hz)
          return status::ok;
      }
      return status::timeout;
#else
      return status::unsupported;
#endif
    }
  }
  template <typename Config> static std::uint32_t get_system_clock_hz() noexcept
  {
#if MICROHAL_NORDIC_SYSTEM_CORE_CLOCK
    SystemCoreClockUpdate();
    return SystemCoreClock;
#else
    return 0U;
#endif
  }
  template <typename Config>
  static status enable_peripheral_clock(peripheral_kind peripheral, std::uint8_t instance) noexcept
  {
    return domain<Config>(peripheral, instance, true);
  }
  template <typename Config>
  static status disable_peripheral_clock(peripheral_kind peripheral, std::uint8_t instance) noexcept
  {
    return domain<Config>(peripheral, instance, false);
  }
  static void handle_interrupt() noexcept
  {
#if MICROHAL_NORDIC_HAS_CLOCK
    nrfx_clock_irq_handler();
#endif
  }

private:
  template <typename Config>
  static status domain(peripheral_kind peripheral, std::uint8_t instance, bool enable) noexcept
  {
    if constexpr (!owned<Config>())
      return status::unsupported;
    if (instance != 0U)
      return status::invalid_argument;
#if MICROHAL_NORDIC_HAS_CLOCK
    // Nordic has shared clock domains, not independently gateable peripheral bus clocks.
    // RTC is the only public peripheral mapped to an independently startable LF domain.
    if (peripheral != peripheral_kind::rtc)
      return status::unsupported;
    if (!nrfx_clock_init_check())
    {
      const auto result = detail::driver_result(nrfx_clock_init(nullptr));
      if (result != status::ok)
        return result;
    }
    if (enable)
      nrfx_clock_start(NRF_CLOCK_DOMAIN_LFCLK);
    else
      nrfx_clock_stop(NRF_CLOCK_DOMAIN_LFCLK);
    return status::ok;
#else
    (void)peripheral;
    (void)enable;
    return status::unsupported;
#endif
  }
};
