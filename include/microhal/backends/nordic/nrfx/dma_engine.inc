// SPDX-License-Identifier: Apache-2.0
// Included inside microhal::backends::nordic::nrfx.
namespace system_detail
{
#if MICROHAL_NORDIC_HAS_MVDMA
struct memory_dma_state
{
  alignas(8) nrf_vdma_job_t source[2]{};
  alignas(8) nrf_vdma_job_t sink[2]{};
  std::atomic<dma_transfer_state> state{dma_transfer_state::idle};
  void (*completion)(status) noexcept {};
  bool initialized{};
};
inline memory_dma_state memory_dma{};
inline constexpr std::uint32_t memory_dma_interrupts =
    NRF_MVDMA_INT_END_MASK | NRF_MVDMA_INT_SOURCEBUSERROR_MASK | NRF_MVDMA_INT_SINKBUSERROR_MASK;
inline void memory_dma_finish(status result) noexcept
{
  auto& state = memory_dma;
  nrf_mvdma_int_disable(NRF_MVDMA120, memory_dma_interrupts);
  NRFY_CACHE_INV(state.sink[0].p_buffer, state.sink[0].size);
  state.state.store(result == status::ok ? dma_transfer_state::complete : dma_transfer_state::error);
  const auto callback = state.completion;
  state.completion = nullptr;
  if (callback)
    callback(result);
}
#endif
} // namespace system_detail
struct dma_api final
{
  template <typename Config> static status initialize() noexcept
  {
#if MICROHAL_NORDIC_HAS_MVDMA
    if (Config::instance_index != 0U || Config::stream != 0U || Config::channel != 0U)
      return status::invalid_argument;
    if (static_cast<std::uint8_t>(Config::direction) != 2U || Config::circular_mode || Config::double_buffer)
      return status::unsupported;
    auto& state = system_detail::memory_dma;
    if (state.initialized || nrf_mvdma_activity_check(NRF_MVDMA120))
      return status::busy;
    nrf_mvdma_int_disable(NRF_MVDMA120, UINT32_MAX);
    nrf_mvdma_mode_set(NRF_MVDMA120, NRF_MVDMA_MODE_SINGLE);
    NRFX_IRQ_PRIORITY_SET(MVDMA120_IRQn, 7U);
    NRFX_IRQ_PENDING_CLEAR(MVDMA120_IRQn);
    NRFX_IRQ_ENABLE(MVDMA120_IRQn);
    state.initialized = true;
    return status::ok;
#else
    return status::unsupported;
#endif
  }
  template <typename Config, auto Handler>
  static status start_transfer(std::uint32_t source, std::uint32_t sink, std::uint32_t length) noexcept
  {
#if MICROHAL_NORDIC_HAS_MVDMA
    if (Config::instance_index != 0U || Config::stream != 0U || Config::channel != 0U)
      return status::invalid_argument;
    if (static_cast<std::uint8_t>(Config::direction) != 2U || Config::circular_mode || Config::double_buffer)
      return status::unsupported;
    if (length == 0U || length > NRF_VDMA_BUFFER_SIZE_MASK ||
        static_cast<std::uint64_t>(source) + length > (1ULL << 32U) ||
        static_cast<std::uint64_t>(sink) + length > (1ULL << 32U) ||
        (static_cast<std::uint64_t>(source) < static_cast<std::uint64_t>(sink) + length &&
         static_cast<std::uint64_t>(sink) < static_cast<std::uint64_t>(source) + length))
      return status::invalid_argument;
    auto& state = system_detail::memory_dma;
    if (!state.initialized)
      return status::unsupported;
    const auto saved = __get_PRIMASK();
    __disable_irq();
    if (state.state.load() == dma_transfer_state::active || nrf_mvdma_activity_check(NRF_MVDMA120))
    {
      __set_PRIMASK(saved);
      return status::busy;
    }
    state.state.store(dma_transfer_state::active);
    state.completion = [](status result) noexcept { detail::notify<Handler>(result); };
    nrf_vdma_job_fill(&state.source[0], reinterpret_cast<void*>(static_cast<std::uintptr_t>(source)), length,
                      NRF_VDMA_ATTRIBUTE_PLAIN_DATA);
    nrf_vdma_job_fill(&state.sink[0], reinterpret_cast<void*>(static_cast<std::uintptr_t>(sink)), length,
                      NRF_VDMA_ATTRIBUTE_PLAIN_DATA);
    nrf_vdma_job_terminate(&state.source[1]);
    nrf_vdma_job_terminate(&state.sink[1]);
    NRFY_CACHE_WB(state.source[0].p_buffer, length);
    NRFY_CACHE_WBINV(state.sink[0].p_buffer, length);
    NRFY_CACHE_WB(state.source, sizeof(state.source));
    NRFY_CACHE_WB(state.sink, sizeof(state.sink));
    nrf_mvdma_event_clear(NRF_MVDMA120, NRF_MVDMA_EVENT_END);
    nrf_mvdma_event_clear(NRF_MVDMA120, NRF_MVDMA_EVENT_SOURCEBUSERROR);
    nrf_mvdma_event_clear(NRF_MVDMA120, NRF_MVDMA_EVENT_SINKBUSERROR);
    nrf_mvdma_source_list_ptr_set(NRF_MVDMA120, state.source);
    nrf_mvdma_sink_list_ptr_set(NRF_MVDMA120, state.sink);
    nrf_mvdma_int_enable(NRF_MVDMA120, system_detail::memory_dma_interrupts);
    __DMB();
    nrf_mvdma_task_trigger(NRF_MVDMA120, NRF_MVDMA_TASK_START0);
    __set_PRIMASK(saved);
    return status::ok;
#else
    (void)source;
    (void)sink;
    (void)length;
    return status::unsupported;
#endif
  }
  template <typename Config> static status abort() noexcept
  {
#if MICROHAL_NORDIC_HAS_MVDMA
    if (Config::instance_index != 0U)
      return status::invalid_argument;
    auto& state = system_detail::memory_dma;
    if (!state.initialized)
      return status::unsupported;
    if (state.state.load() != dma_transfer_state::active)
      return status::ok;
    nrf_mvdma_int_disable(NRF_MVDMA120, system_detail::memory_dma_interrupts);
    nrf_mvdma_event_clear(NRF_MVDMA120, NRF_MVDMA_EVENT_PAUSED);
    nrf_mvdma_task_trigger(NRF_MVDMA120, NRF_MVDMA_TASK_PAUSE);
    std::uint32_t remaining = 1000000U;
    while (nrf_mvdma_activity_check(NRF_MVDMA120) && remaining)
      --remaining;
    if (!remaining)
      return status::timeout;
    nrf_mvdma_event_clear(NRF_MVDMA120, NRF_MVDMA_EVENT_RESET);
    nrf_mvdma_task_trigger(NRF_MVDMA120, NRF_MVDMA_TASK_RESET);
    remaining = 1000000U;
    while (!nrf_mvdma_event_check(NRF_MVDMA120, NRF_MVDMA_EVENT_RESET) && remaining)
      --remaining;
    if (!remaining)
      return status::timeout;
    NRFY_CACHE_INV(state.sink[0].p_buffer, state.sink[0].size);
    state.completion = nullptr;
    state.state.store(dma_transfer_state::idle);
    return status::ok;
#else
    return status::unsupported;
#endif
  }
  template <typename Config> static result<dma_transfer_state> transfer_status() noexcept
  {
#if MICROHAL_NORDIC_HAS_MVDMA
    if (Config::instance_index != 0U)
      return status::invalid_argument;
    if (!system_detail::memory_dma.initialized)
      return status::unsupported;
    return system_detail::memory_dma.state.load();
#else
    return status::unsupported;
#endif
  }
  static void handle_interrupt() noexcept
  {
#if MICROHAL_NORDIC_HAS_MVDMA
    auto& state = system_detail::memory_dma;
    if (state.state.load() != dma_transfer_state::active)
      return;
    if (nrf_mvdma_event_check(NRF_MVDMA120, NRF_MVDMA_EVENT_SOURCEBUSERROR) ||
        nrf_mvdma_event_check(NRF_MVDMA120, NRF_MVDMA_EVENT_SINKBUSERROR))
    {
      nrf_mvdma_int_disable(NRF_MVDMA120, system_detail::memory_dma_interrupts);
      nrf_mvdma_event_clear(NRF_MVDMA120, NRF_MVDMA_EVENT_RESET);
      nrf_mvdma_task_trigger(NRF_MVDMA120, NRF_MVDMA_TASK_RESET);
      std::uint32_t remaining = 1000000U;
      while (!nrf_mvdma_event_check(NRF_MVDMA120, NRF_MVDMA_EVENT_RESET) && remaining)
        --remaining;
      // Do not release the buffers or invoke completion if reset is not acknowledged.
      if (!remaining)
        return;
      nrf_mvdma_event_clear(NRF_MVDMA120, NRF_MVDMA_EVENT_SOURCEBUSERROR);
      nrf_mvdma_event_clear(NRF_MVDMA120, NRF_MVDMA_EVENT_SINKBUSERROR);
      system_detail::memory_dma_finish(status::io_error);
    }
    else if (nrf_mvdma_event_check(NRF_MVDMA120, NRF_MVDMA_EVENT_END))
    {
      nrf_mvdma_event_clear(NRF_MVDMA120, NRF_MVDMA_EVENT_END);
      system_detail::memory_dma_finish(status::ok);
    }
#endif
  }
  template <typename Config> static void handle_interrupt() noexcept
  {
    handle_interrupt();
  }
};
