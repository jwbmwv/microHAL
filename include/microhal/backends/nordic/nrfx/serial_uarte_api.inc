// SPDX-License-Identifier: Apache-2.0
template <typename Config> static status tx_status() noexcept
{
#if MICROHAL_NORDIC_HAS_UARTE
  if (Config::instance_index >= std::size(detail::uarte_instances))
    return status::invalid_argument;
  if (!nrfx_uarte_init_check(&detail::uarte_instances[Config::instance_index]))
    return status::unsupported;
  auto& state = detail::uarte_states[Config::instance_index].tx;
  return state.active.load(std::memory_order_acquire)
             ? status::busy
             : (state.complete.load(std::memory_order_acquire) ? state.result : status::ok);
#else
  return status::unsupported;
#endif
}

template <typename Config> static status rx_status() noexcept
{
#if MICROHAL_NORDIC_HAS_UARTE
  if (Config::instance_index >= std::size(detail::uarte_instances))
    return status::invalid_argument;
  if (!nrfx_uarte_init_check(&detail::uarte_instances[Config::instance_index]))
    return status::unsupported;
  auto& state = detail::uarte_states[Config::instance_index].rx;
  return state.active.load(std::memory_order_acquire)
             ? status::busy
             : (state.complete.load(std::memory_order_acquire) ? state.result : status::ok);
#else
  return status::unsupported;
#endif
}

template <typename Config> static status abort_tx() noexcept
{
#if MICROHAL_NORDIC_HAS_UARTE
  if (Config::instance_index >= std::size(detail::uarte_instances))
    return status::invalid_argument;
  auto& state = detail::uarte_states[Config::instance_index];
  if (state.instance == nullptr || !nrfx_uarte_init_check(state.instance))
    return status::unsupported;
  if (!state.tx.active.load())
    return status::ok;
  const auto generation = state.tx.generation.load(std::memory_order_relaxed);
  state.tx_cancelled = true;
  const auto error = nrfx_uarte_tx_abort(state.instance, true);
  if (error != 0)
  {
    state.tx_cancelled = false;
    return detail::driver_result(error);
  }
  if (state.tx.active.load() && state.tx.generation.load(std::memory_order_relaxed) == generation)
    state.tx.finish(status::error);
  return status::ok;
#else
  return status::unsupported;
#endif
}

template <typename Config> static status abort_rx() noexcept
{
#if MICROHAL_NORDIC_HAS_UARTE
  if (Config::instance_index >= std::size(detail::uarte_instances))
    return status::invalid_argument;
  auto& state = detail::uarte_states[Config::instance_index];
  if (state.instance == nullptr || !nrfx_uarte_init_check(state.instance))
    return status::unsupported;
  if (!state.rx.active.load())
    return status::ok;
  const auto generation = state.rx.generation.load(std::memory_order_relaxed);
  state.rx_cancelled = true;
  const auto error = nrfx_uarte_rx_abort(state.instance, true, true);
  if (error != 0)
  {
    state.rx_cancelled = false;
    return detail::driver_result(error);
  }
  // Synchronous abort returns ownership even when the driver has no pending event.
  if (state.rx.active.load() && state.rx.generation.load(std::memory_order_relaxed) == generation)
  {
    detail::uarte_timeout_release(state);
    const auto callback = state.receive;
    state.receive = nullptr;
    state.circular = false;
    state.idle = false;
    state.rx.finish(status::error);
    if (callback != nullptr)
      callback(status::error, state.received);
  }
  return status::ok;
#else
  return status::unsupported;
#endif
}

template <typename Config> static status abort() noexcept
{
  const auto tx = abort_tx<Config>();
  const auto rx = abort_rx<Config>();
  return tx == status::ok ? rx : tx;
}

template <typename Config> static status transfer_status() noexcept
{
  const auto tx = tx_status<Config>();
  const auto rx = rx_status<Config>();
  if (tx == status::busy || rx == status::busy)
    return status::busy;
  return tx == status::ok ? rx : tx;
}

template <typename Config, auto Handler, bool Circular>
static status start_receive(std::span<std::uint8_t> buffer) noexcept
{
#if MICROHAL_NORDIC_HAS_UARTE
  if (Config::instance_index >= std::size(detail::uarte_instances) || buffer.empty() ||
      !detail::serial_count_valid(buffer.size()) || (Circular && (buffer.size() < 2U || buffer.size() % 2U != 0U)))
    return status::invalid_argument;
  auto& instance = detail::uarte_instances[Config::instance_index];
  if (!nrfx_uarte_init_check(&instance))
    return status::unsupported;
  auto& state = detail::uarte_states[Config::instance_index];
  if (!state.rx.begin(nullptr))
    return status::busy;
  state.rx_cancelled = false;
  state.rx_error = false;
  state.rx_result = status::ok;
  state.circular = Circular;
  state.idle = !Circular;
  state.buffer = buffer;
  state.received = {};
  state.rx_size = Circular ? buffer.size() / 2U : buffer.size();
  state.next_half = 1U;
  state.receive = [](status value, std::span<std::uint8_t> bytes) noexcept { Handler(value, bytes); };
  // Circular RX is double-buffered. An optional timeout additionally emits partial buffers.
  auto result = detail::uarte_timeout_acquire<Config>(state);
  if constexpr (Circular)
    if (result == status::unsupported)
      result = status::ok;
  if (result == status::ok)
    result = detail::driver_result(nrfx_uarte_rx_buffer_set(&instance, buffer.data(), state.rx_size));
  if (result == status::ok)
    result = detail::driver_result(
        nrfx_uarte_rx_enable(&instance, Circular ? NRFX_UARTE_RX_ENABLE_CONT : NRFX_UARTE_RX_ENABLE_STOP_ON_END));
  if (result != status::ok)
  {
    state.receive = nullptr;
    detail::uarte_timeout_release(state);
    (void)nrfx_uarte_rx_abort(&instance, true, true);
    if (state.rx.active.load())
      state.rx.finish(result);
  }
  return result;
#else
  (void)buffer;
  return status::unsupported;
#endif
}

template <typename Config, auto Handler> static status start_receive_to_idle(std::span<std::uint8_t> buffer) noexcept
{
  return start_receive<Config, Handler, false>(buffer);
}
template <typename Config, auto Handler> static status start_circular_rx(std::span<std::uint8_t> buffer) noexcept
{
  return start_receive<Config, Handler, true>(buffer);
}
