// SPDX-License-Identifier: Apache-2.0
// Included in the nrfx detail namespace after the UARTE instance table.
struct uarte_state
{
  nrfx_uarte_t* instance{};
  transfer_state tx{};
  transfer_state rx{};
  alignas(4) std::array<std::uint8_t, 64U> tx_cache{};
  std::size_t tx_size{};
  std::size_t rx_size{};
  bool rx_error{};
  bool tx_cancelled{};
  bool rx_cancelled{};
  bool idle{};
  bool circular{};
  std::span<std::uint8_t> buffer{};
  std::span<std::uint8_t> received{};
  std::uint8_t next_half{};
  void (*receive)(status, std::span<std::uint8_t>) noexcept {};
  status rx_result{status::io_error};
#if MICROHAL_NORDIC_HAS_SERIAL_TIMEOUT
  nrfx_timer_t* timeout_timer{};
  nrfx_gppi_handle_t timeout_connection{};
  std::uint32_t timeout_event{};
  std::uint32_t timeout_clear{};
  std::uint32_t timeout_start{};
#if defined(NRF54H20_XXAA)
  NRF_DPPIC_Type* timeout_dppic{};
  std::uint8_t timeout_channel{};
#endif
#endif
};
inline std::array<uarte_state, std::size(uarte_instances)> uarte_states{};

inline void uarte_timeout_release(uarte_state& state) noexcept
{
#if MICROHAL_NORDIC_HAS_SERIAL_TIMEOUT
  if (state.timeout_timer != nullptr)
  {
    nrfx_timer_disable(state.timeout_timer);
#if defined(NRF54H20_XXAA)
    nrf_dppi_channels_disable(state.timeout_dppic, 1UL << state.timeout_channel);
    nrf_uarte_publish_clear(state.instance->p_reg, NRF_UARTE_EVENT_RXDRDY);
    nrf_timer_subscribe_clear(state.timeout_timer->p_reg, NRF_TIMER_TASK_CLEAR);
    nrf_timer_subscribe_clear(state.timeout_timer->p_reg, NRF_TIMER_TASK_START);
#else
    nrfx_gppi_conn_disable(state.timeout_connection);
    nrfx_gppi_ep_clear(state.timeout_start);
    nrfx_gppi_conn_free(state.timeout_event, state.timeout_clear, state.timeout_connection);
#endif
    nrfx_timer_uninit(state.timeout_timer);
    state.timeout_timer = nullptr;
  }
#else
  (void)state;
#endif
}

inline void uarte_handler(nrfx_uarte_event_t const* event, void* context) noexcept
{
  auto& state = *static_cast<uarte_state*>(context);
  if (event->type == NRFX_UARTE_EVT_TX_DONE && state.tx.active.load())
  {
    state.tx.finish(state.tx_cancelled || (event->data.tx.flags & NRFX_UARTE_TX_DONE_ABORTED) != 0U
                        ? status::error
                        : (event->data.tx.length == state.tx_size ? status::ok : status::io_error));
    return;
  }
  if (!state.rx.active.load())
    return;
  switch (event->type)
  {
  case NRFX_UARTE_EVT_RX_BUF_REQUEST:
    if (state.circular && !state.rx_cancelled && !state.rx_error)
    {
      const auto half = state.buffer.size() / 2U;
      const int error = nrfx_uarte_rx_buffer_set(state.instance, state.buffer.data() + state.next_half * half, half);
      state.next_half ^= 1U;
      if (error != 0)
      {
        state.rx_error = true;
        (void)nrfx_uarte_rx_abort(state.instance, true, false);
      }
    }
    break;
  case NRFX_UARTE_EVT_RX_DONE:
    state.received = {event->data.rx.p_buffer, event->data.rx.length};
    if (state.circular && !state.rx_cancelled && !state.rx_error)
    {
      if (!state.received.empty() && state.receive != nullptr)
        state.receive(status::ok, state.received);
      state.received = {};
    }
    else
      state.rx_result = state.idle || event->data.rx.length == state.rx_size ? status::ok : status::io_error;
    break;
  case NRFX_UARTE_EVT_ERROR:
  case NRFX_UARTE_EVT_RX_BUF_TOO_LATE:
    state.rx_error = true;
    (void)nrfx_uarte_rx_abort(state.instance, true, false);
    break;
  case NRFX_UARTE_EVT_RX_DISABLED:
  {
    const auto value = state.rx_cancelled ? status::error : (state.rx_error ? status::io_error : state.rx_result);
    const auto callback = state.receive;
    const auto bytes = state.received;
    state.receive = nullptr;
    state.circular = false;
    state.idle = false;
    uarte_timeout_release(state);
    state.rx.finish(value);
    if (callback != nullptr)
      callback(value, bytes);
    break;
  }
  default:
    break;
  }
}

#if MICROHAL_NORDIC_HAS_SERIAL_TIMEOUT
#if !defined(NRF54H20_XXAA)
inline std::atomic<nrfx_gppi_t*> uarte_gppi_resource{};
#endif
inline void uarte_timeout_handler(nrf_timer_event_t event, void* context) noexcept
{
  auto& state = *static_cast<uarte_state*>(context);
  if (event == NRF_TIMER_EVENT_COMPARE0 && state.rx.active.load())
  {
    const auto error = nrfx_uarte_rx_abort(state.instance, !state.circular, false);
    if (error != 0 && error != -EINPROGRESS)
      state.rx_error = true;
  }
}

template <typename Config> status uarte_timeout_acquire(uarte_state& state) noexcept
{
#if defined(NRF54H20_XXAA)
  if constexpr (requires {
                  Config::binding::rx_timeout_timer;
                  Config::binding::rx_timeout_us;
                  Config::binding::rx_timeout_dppic;
                  Config::binding::rx_timeout_channel;
                })
#else
  if constexpr (requires {
                  Config::binding::rx_timeout_timer;
                  Config::binding::rx_timeout_us;
                  Config::binding::rx_timeout_gppi;
                })
#endif
  {
    auto* timer = Config::binding::rx_timeout_timer;
#if defined(NRF54H20_XXAA)
    auto* dppic = Config::binding::rx_timeout_dppic;
    constexpr auto channel = Config::binding::rx_timeout_channel;
    if (timer == nullptr || dppic == nullptr || Config::binding::rx_timeout_us == 0U ||
        channel >= nrf_dppi_channel_number_get(dppic))
      return status::invalid_argument;
    const auto domain = reinterpret_cast<std::uintptr_t>(state.instance->p_reg) & 0xFFFF0000UL;
    if ((reinterpret_cast<std::uintptr_t>(timer->p_reg) & 0xFFFF0000UL) != domain ||
        (reinterpret_cast<std::uintptr_t>(dppic) & 0xFFFF0000UL) != domain)
      return status::unsupported;
    if (nrf_dppi_channel_check(dppic, channel))
      return status::busy;
#else
    auto* gppi = Config::binding::rx_timeout_gppi;
    if (timer == nullptr || gppi == nullptr || Config::binding::rx_timeout_us == 0U)
      return status::invalid_argument;
#endif
    if constexpr (requires {
                    Config::baud_rate;
                    Config::parity;
                    Config::stop_bits;
                  })
    {
      constexpr auto bits = 1U + 8U + (Config::parity != uart_parity::none ? 1U : 0U) +
                            (Config::stop_bits == uart_stop_bits::two ? 2U : 1U);
      if (static_cast<std::uint64_t>(Config::binding::rx_timeout_us) * Config::baud_rate <
          static_cast<std::uint64_t>(bits) * 1'000'000U)
        return status::invalid_argument;
    }
#if !defined(NRF54H20_XXAA)
    nrfx_gppi_t* expected = nullptr;
    if (!uarte_gppi_resource.compare_exchange_strong(expected, gppi) && expected != gppi)
      return status::busy;
    // Board-supplied masks/topology arbitrate real D(P)PI resources with other users.
    nrfx_gppi_init(gppi);
#endif
    if (nrfx_timer_init_check(timer))
      return status::busy;
    nrfx_timer_config_t config{};
    config.frequency = 1'000'000U;
    config.mode = NRF_TIMER_MODE_TIMER;
    config.bit_width = NRF_TIMER_BIT_WIDTH_32;
    config.interrupt_priority = 7U;
    config.p_context = &state;
    auto result = driver_result(nrfx_timer_init(timer, &config, uarte_timeout_handler));
    if (result != status::ok)
      return result;
#if defined(NRF54H20_XXAA)
    // H20's generic GPPI backend requires platform IPC callbacks. Use an exclusively
    // board-reserved local-domain channel instead; no cross-domain routing is implied.
    state.timeout_dppic = dppic;
    state.timeout_channel = channel;
    nrf_uarte_publish_set(state.instance->p_reg, NRF_UARTE_EVENT_RXDRDY, channel);
    nrf_timer_subscribe_set(timer->p_reg, NRF_TIMER_TASK_CLEAR, channel);
    nrf_timer_subscribe_set(timer->p_reg, NRF_TIMER_TASK_START, channel);
#else
    state.timeout_event = nrf_uarte_event_address_get(state.instance->p_reg, NRF_UARTE_EVENT_RXDRDY);
    state.timeout_clear = nrfx_timer_task_address_get(timer, NRF_TIMER_TASK_CLEAR);
    state.timeout_start = nrfx_timer_task_address_get(timer, NRF_TIMER_TASK_START);
    int error = nrfx_gppi_conn_alloc(state.timeout_event, state.timeout_clear, &state.timeout_connection);
    if (error != 0)
    {
      nrfx_timer_uninit(timer);
      return driver_result(error);
    }
    error = nrfx_gppi_ep_attach(state.timeout_start, state.timeout_connection);
    if (error != 0)
    {
      nrfx_gppi_conn_free(state.timeout_event, state.timeout_clear, state.timeout_connection);
      nrfx_timer_uninit(timer);
      return driver_result(error);
    }
#endif
    state.timeout_timer = timer;
    nrfx_timer_extended_compare(timer, NRF_TIMER_CC_CHANNEL0, Config::binding::rx_timeout_us,
                                NRF_TIMER_SHORT_COMPARE0_STOP_MASK, true);
#if defined(NRF54H20_XXAA)
    nrf_dppi_channels_enable(dppic, 1UL << channel);
#else
    nrfx_gppi_conn_enable(state.timeout_connection);
    nrfx_gppi_ep_enable(state.timeout_start);
#endif
    // The timer starts on the first byte, not on receive enable.
    return status::ok;
  }
  return status::unsupported;
}
#else
template <typename Config> status uarte_timeout_acquire(uarte_state&) noexcept
{
  return status::unsupported;
}
#endif
