// SPDX-License-Identifier: Apache-2.0
// Included inside microhal::backends::nordic::nrfx.
namespace system_detail
{
struct rtc_clock
{
  std::uint64_t high_ticks{};
  std::uint64_t last_counter{};
  std::uint64_t origin_us{};
  std::uint64_t epoch_us{};
  std::uint64_t alarm_us{};
  std::uint64_t periodic_us{};
  std::uint32_t period_us{};
  std::uint32_t period_ticks{};
  std::uint64_t periodic_ticks{};
  void (*alarm)() noexcept {};
  void (*periodic)() noexcept {};
  bool initialized{};
  bool calendar_set{};
#if MICROHAL_NORDIC_HAS_GRTC
  nrfx_grtc_channel_t channels[2]{};
#endif
};
inline rtc_clock clocks[
#if MICROHAL_NORDIC_HAS_RTC
    std::size(detail::rtc_instances)
#else
    1U
#endif
]{};
#if MICROHAL_NORDIC_HAS_RTC
inline std::uint64_t raw_ticks(std::uint8_t index) noexcept
{
  auto& state = clocks[index];
  auto& instance = detail::rtc_instances[index];
  auto high = state.high_ticks;
  auto ticks = nrfx_rtc_counter_get(&instance);
  if (nrf_rtc_event_check(instance.p_reg, NRF_RTC_EVENT_OVERFLOW))
  {
    high += 1ULL << 24U;
    ticks = nrfx_rtc_counter_get(&instance);
  }
  return high + ticks;
}
#endif
inline std::uint64_t raw_us(std::uint8_t index) noexcept
{
#if MICROHAL_NORDIC_HAS_GRTC
  constexpr std::uint64_t mask = (static_cast<std::uint64_t>(NRF_GRTC_SYSCOUNTER_CCH_MASK) << 32U) | UINT32_MAX;
  auto& state = clocks[index];
  const auto counter = nrfx_grtc_syscounter_get() & mask;
  if (counter < state.last_counter)
    state.high_ticks += mask + 1U;
  state.last_counter = counter;
  return state.high_ticks + counter;
#elif MICROHAL_NORDIC_HAS_RTC
  const auto ticks = raw_ticks(index);
  return (ticks / 32768U) * 1000000ULL + ((ticks % 32768U) * 1000000ULL) / 32768U;
#else
  (void)index;
  return 0U;
#endif
}
inline status schedule(std::uint8_t index, std::uint8_t slot, std::uint64_t when) noexcept
{
#if MICROHAL_NORDIC_HAS_GRTC
  constexpr std::uint64_t mask = (static_cast<std::uint64_t>(NRF_GRTC_SYSCOUNTER_CCH_MASK) << 32U) | UINT32_MAX;
  return detail::driver_result(nrfx_grtc_syscounter_cc_absolute_set(&clocks[index].channels[slot], when & mask, true));
#elif MICROHAL_NORDIC_HAS_RTC
  const auto ticks = (when / 1000000U) * 32768U + ((when % 1000000U) * 32768U + 999999U) / 1000000U;
  return detail::driver_result(
      nrfx_rtc_cc_set(&detail::rtc_instances[index], slot, static_cast<std::uint32_t>(ticks & 0xFFFFFFU), true));
#else
  (void)index;
  (void)slot;
  (void)when;
  return status::unsupported;
#endif
}
inline void disable(std::uint8_t index, std::uint8_t slot) noexcept
{
#if MICROHAL_NORDIC_HAS_GRTC
  (void)nrfx_grtc_syscounter_cc_disable(clocks[index].channels[slot].channel);
#elif MICROHAL_NORDIC_HAS_RTC
  (void)nrfx_rtc_cc_disable(&detail::rtc_instances[index], slot);
#else
  (void)index;
  (void)slot;
#endif
}
inline void event(std::uint8_t index, std::uint8_t slot) noexcept
{
  auto& state = clocks[index];
  const auto now = raw_us(index);
  if (slot == 0U)
  {
    if (now < state.alarm_us)
    {
      (void)schedule(index, 0U, state.alarm_us);
      return;
    }
    disable(index, 0U);
    const auto callback = state.alarm;
    state.alarm = nullptr;
    if (callback)
      callback();
  }
  else if (state.periodic)
  {
#if MICROHAL_NORDIC_HAS_RTC
    const auto ticks = raw_ticks(index);
    state.periodic_ticks += ((ticks - state.periodic_ticks) / state.period_ticks + 1U) * state.period_ticks;
    if (state.period_ticks > 1U)
      (void)nrfx_rtc_cc_set(&detail::rtc_instances[index], 1U,
                            static_cast<std::uint32_t>(state.periodic_ticks & 0xFFFFFFU), true);
#else
    state.periodic_us += ((now - state.periodic_us) / state.period_us + 1U) * state.period_us;
    (void)schedule(index, 1U, state.periodic_us);
#endif
    state.periodic();
  }
}
#if MICROHAL_NORDIC_HAS_RTC
inline void rtc_event(nrf_rtc_event_t event_id, void* context) noexcept
{
  const auto index = static_cast<std::uint8_t>(static_cast<rtc_clock*>(context) - clocks);
  if (event_id == NRF_RTC_EVENT_OVERFLOW)
    clocks[index].high_ticks += 1ULL << 24U;
  else if (event_id == NRF_RTC_EVENT_COMPARE_0)
    event(index, 0U);
  else if (event_id == NRF_RTC_EVENT_COMPARE_1 || event_id == NRF_RTC_EVENT_TICK)
    event(index, 1U);
}
#endif
} // namespace system_detail

struct rtc_api final
{
  template <typename Config> static constexpr std::uint8_t index() noexcept
  {
    if constexpr (requires { Config::instance_index; })
      return Config::instance_index;
    return 0U;
  }
  template <typename Config> static status initialize() noexcept
  {
#if MICROHAL_NORDIC_HAS_RTC || MICROHAL_NORDIC_HAS_GRTC
    constexpr auto i = index<Config>();
    if (i >= std::size(system_detail::clocks))
      return status::invalid_argument;
    auto& state = system_detail::clocks[i];
    if (state.initialized)
      return status::busy;
#if MICROHAL_NORDIC_HAS_GRTC
    if (!nrfx_grtc_init_check())
    {
      auto result = detail::driver_result(nrfx_grtc_init(7U));
      if (result != status::ok)
        return result;
#if NRFY_GRTC_HAS_EXTENDED
      std::uint8_t main_channel{};
      const auto started = nrfx_grtc_syscounter_start(true, &main_channel);
      if (started != 0 && started != -EALREADY)
        return detail::driver_result(started);
#endif
    }
    nrfx_grtc_active_request_set(true);
    std::uint32_t remaining = 1000000U;
    while (!nrfx_grtc_ready_check() && remaining)
      --remaining;
    if (!remaining)
      return status::timeout;
    for (std::uint8_t slot = 0U; slot < 2U; ++slot)
    {
      auto& channel = state.channels[slot];
      const auto result = detail::driver_result(nrfx_grtc_channel_alloc(&channel.channel));
      if (result != status::ok)
      {
        if (slot != 0U)
          (void)nrfx_grtc_channel_free(state.channels[0].channel);
        return result;
      }
      channel.p_context = &state;
      channel.handler = [](std::int32_t id, std::uint64_t, void* context)
      {
        auto& current = *static_cast<system_detail::rtc_clock*>(context);
        system_detail::event(0U, id == current.channels[0].channel ? 0U : 1U);
      };
    }
#else
    nrfx_rtc_config_t config{};
    config.prescaler = 0U;
    config.interrupt_priority = 7U;
    config.p_context = &state;
    const auto result =
        detail::driver_result(nrfx_rtc_init(&detail::rtc_instances[i], &config, system_detail::rtc_event));
    if (result != status::ok)
      return result;
    nrfx_rtc_overflow_enable(&detail::rtc_instances[i], true);
    nrfx_rtc_enable(&detail::rtc_instances[i]);
#endif
    state.origin_us = system_detail::raw_us(i);
    state.initialized = true;
    return status::ok;
#else
    return status::unsupported;
#endif
  }
  template <typename Config, typename Date> static status set_calendar(const Date& date) noexcept
  {
    if (!calendar::valid(date))
      return status::invalid_argument;
    return set_epoch<Config>(calendar::seconds(date) * 1000000ULL, true);
  }
  template <typename Config>
  static status set_time(std::uint8_t hour, std::uint8_t minute, std::uint8_t second) noexcept
  {
    if (hour >= 24U || minute >= 60U || second >= 60U)
      return status::invalid_argument;
    return set_epoch<Config>((hour * 3600ULL + minute * 60ULL + second) * 1000000ULL, false);
  }
  template <typename Config, typename Date> static status get_calendar(Date& date) noexcept
  {
    const auto value = now<Config>();
    if (!value)
      return value.error();
    calendar::split(value.value() / 1000000ULL, date);
    if (!system_detail::clocks[index<Config>()].calendar_set)
      date.year = date.month = date.day = 0U;
    return status::ok;
  }
  template <typename Config>
  static status get_time(std::uint8_t& hour, std::uint8_t& minute, std::uint8_t& second) noexcept
  {
    const auto value = now<Config>();
    if (!value)
      return value.error();
    const auto seconds = value.value() / 1000000ULL;
    hour = static_cast<std::uint8_t>((seconds / 3600U) % 24U);
    minute = static_cast<std::uint8_t>((seconds / 60U) % 60U);
    second = static_cast<std::uint8_t>(seconds % 60U);
    return status::ok;
  }
  template <typename Config> static result<std::uint32_t> get_subsecond_us() noexcept
  {
    const auto value = now<Config>();
    if (!value)
      return value.error();
    return static_cast<std::uint32_t>(value.value() % 1000000ULL);
  }
  template <typename Config, auto Handler, typename Date> static status enable_alarm(const Date& date) noexcept
  {
    const auto current = now<Config>();
    if (!current)
      return current.error();
    std::uint64_t target{};
    if (date.year || date.month || date.day)
    {
      if (!calendar::valid(date) || !system_detail::clocks[index<Config>()].calendar_set)
        return status::invalid_argument;
      target = calendar::seconds(date) * 1000000ULL;
      if (target <= current.value())
        return status::invalid_argument;
    }
    else
    {
      if (date.hour >= 24U || date.minute >= 60U || date.second >= 60U)
        return status::invalid_argument;
      target = (current.value() / 86400000000ULL) * 86400000000ULL +
               (date.hour * 3600ULL + date.minute * 60ULL + date.second) * 1000000ULL;
      if (target <= current.value())
        target += 86400000000ULL;
    }
#if MICROHAL_NORDIC_HAS_RTC || MICROHAL_NORDIC_HAS_GRTC
    const auto saved = __get_PRIMASK();
    __disable_irq();
    auto& state = system_detail::clocks[index<Config>()];
    state.alarm_us = state.origin_us + target - state.epoch_us;
    state.alarm = []() noexcept { detail::notify<Handler>(status::ok); };
    const auto result = system_detail::schedule(index<Config>(), 0U, state.alarm_us);
    if (result != status::ok)
      state.alarm = nullptr;
    __set_PRIMASK(saved);
    return result;
#else
    return status::unsupported;
#endif
  }
  template <typename Config, auto Handler> static status enable_periodic(std::uint32_t period_us) noexcept
  {
    const auto current = now<Config>();
    if (!current)
      return current.error();
    if (period_us == 0U)
      return status::invalid_argument;
#if MICROHAL_NORDIC_HAS_RTC
    if ((static_cast<std::uint64_t>(period_us) * 32768U) % 1000000U != 0U)
      return status::invalid_argument;
#endif
#if MICROHAL_NORDIC_HAS_RTC || MICROHAL_NORDIC_HAS_GRTC
    const auto saved = __get_PRIMASK();
    __disable_irq();
    auto& state = system_detail::clocks[index<Config>()];
    state.period_us = period_us;
#if MICROHAL_NORDIC_HAS_RTC
    nrfx_rtc_tick_disable(&detail::rtc_instances[index<Config>()]);
    state.period_ticks = static_cast<std::uint32_t>((static_cast<std::uint64_t>(period_us) * 32768U) / 1000000U);
    if (state.period_ticks >= (1U << 24U))
    {
      __set_PRIMASK(saved);
      return status::invalid_argument;
    }
    state.periodic_ticks = system_detail::raw_ticks(index<Config>()) + state.period_ticks;
#endif
    state.periodic_us = system_detail::raw_us(index<Config>()) + period_us;
    state.periodic = []() noexcept { detail::notify<Handler>(status::ok); };
    const auto result = system_detail::schedule(index<Config>(), 1U, state.periodic_us);
    if (result != status::ok)
      state.periodic = nullptr;
    __set_PRIMASK(saved);
    return result;
#else
    return status::unsupported;
#endif
  }
  template <typename Config> static status disable_periodic() noexcept
  {
    const auto current = now<Config>();
    if (!current)
      return current.error();
    system_detail::disable(index<Config>(), 1U);
#if MICROHAL_NORDIC_HAS_RTC
    nrfx_rtc_tick_disable(&detail::rtc_instances[index<Config>()]);
#endif
    system_detail::clocks[index<Config>()].periodic = nullptr;
    return status::ok;
  }
  template <typename Config> static status set_date_time(const microhal::rtc::date_time& date) noexcept
  {
    if (date.year == 0U && date.month == 0U && date.day == 0U)
      return set_time<Config>(date.hour, date.minute, date.second);
    return set_calendar<Config>(date);
  }
  template <typename Config> static result<microhal::rtc::date_time> get_date_time() noexcept
  {
    microhal::rtc::date_time date{};
    const auto result = get_calendar<Config>(date);
    if (result != status::ok)
      return result;
    return date;
  }
  template <typename Config> static result<std::uint32_t> read_subsecond_us() noexcept
  {
    return get_subsecond_us<Config>();
  }
  template <typename Config, auto Handler> static status enable_periodic_irq(std::uint32_t frequency_hz) noexcept
  {
#if MICROHAL_NORDIC_HAS_RTC
    const auto current = now<Config>();
    if (!current)
      return current.error();
    if (frequency_hz == 0U || frequency_hz > 32768U || 32768U % frequency_hz != 0U)
      return status::invalid_argument;
    const auto saved = __get_PRIMASK();
    __disable_irq();
    auto& state = system_detail::clocks[index<Config>()];
    system_detail::disable(index<Config>(), 1U);
    nrfx_rtc_tick_disable(&detail::rtc_instances[index<Config>()]);
    state.period_ticks = 32768U / frequency_hz;
    state.periodic_ticks = system_detail::raw_ticks(index<Config>()) + state.period_ticks;
    state.periodic = []() noexcept { detail::notify<Handler>(status::ok); };
    status result = status::ok;
    if (state.period_ticks == 1U)
      nrfx_rtc_tick_enable(&detail::rtc_instances[index<Config>()], true);
    else
      result =
          detail::driver_result(nrfx_rtc_cc_set(&detail::rtc_instances[index<Config>()], 1U,
                                                static_cast<std::uint32_t>(state.periodic_ticks & 0xFFFFFFU), true));
    if (result != status::ok)
      state.periodic = nullptr;
    __set_PRIMASK(saved);
    return result;
#else
    if (frequency_hz == 0U || frequency_hz > 1000000U || 1000000U % frequency_hz != 0U)
      return status::invalid_argument;
    return enable_periodic<Config, Handler>(1000000U / frequency_hz);
#endif
  }
  template <typename Config> static status disable_periodic_irq() noexcept
  {
    return disable_periodic<Config>();
  }
  static void handle_interrupt(std::uint8_t i = 0U) noexcept
  {
#if MICROHAL_NORDIC_HAS_GRTC
    if (i == 0U)
      nrfx_grtc_irq_handler();
#elif MICROHAL_NORDIC_HAS_RTC
    if (i < std::size(detail::rtc_instances))
      nrfx_rtc_irq_handler(&detail::rtc_instances[i]);
#else
    (void)i;
#endif
  }

private:
  template <typename Config> static result<std::uint64_t> now() noexcept
  {
    constexpr auto i = index<Config>();
    if (i >= std::size(system_detail::clocks))
      return status::invalid_argument;
    const auto& state = system_detail::clocks[i];
    if (!state.initialized)
      return status::unsupported;
#if MICROHAL_NORDIC_HAS_RTC || MICROHAL_NORDIC_HAS_GRTC
    const auto saved = __get_PRIMASK();
    __disable_irq();
    const auto value = state.epoch_us + system_detail::raw_us(i) - state.origin_us;
    __set_PRIMASK(saved);
    return value;
#else
    return status::unsupported;
#endif
  }
  template <typename Config> static status set_epoch(std::uint64_t epoch, bool calendar_set) noexcept
  {
    const auto current = now<Config>();
    if (!current)
      return current.error();
#if MICROHAL_NORDIC_HAS_RTC || MICROHAL_NORDIC_HAS_GRTC
    const auto saved = __get_PRIMASK();
    __disable_irq();
    auto& state = system_detail::clocks[index<Config>()];
    system_detail::disable(index<Config>(), 0U);
    state.alarm = nullptr;
    state.origin_us = system_detail::raw_us(index<Config>());
    state.epoch_us = epoch;
    state.calendar_set = calendar_set;
    __set_PRIMASK(saved);
    return status::ok;
#else
    (void)epoch;
    (void)calendar_set;
    return status::unsupported;
#endif
  }
};
