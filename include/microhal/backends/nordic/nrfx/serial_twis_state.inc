// SPDX-License-Identifier: Apache-2.0
#if MICROHAL_NORDIC_HAS_TWIS
inline nrfx_twis_t twis_instances[]{
#if defined(NRF54L15_XXAA)
    {NRF_TWIS20, {}}, {NRF_TWIS21, {}}, {NRF_TWIS22, {}}, {NRF_TWIS30, {}}
#elif defined(NRF54H20_XXAA)
    {NRF_TWIS130, {}}, {NRF_TWIS131, {}}, {NRF_TWIS132, {}}, {NRF_TWIS133, {}}, {NRF_TWIS134, {}},
    {NRF_TWIS135, {}}, {NRF_TWIS136, {}}, {NRF_TWIS137, {}}
#else
    {NRF_TWIS0, {}},
    {NRF_TWIS1, {}}
#if defined(NRF5340_XXAA_APPLICATION)
    ,
    {NRF_TWIS2, {}},
    {NRF_TWIS3, {}}
#endif
#endif
};
struct twis_state
{
  transfer_state transfer{};
  std::span<const std::uint8_t> tx{};
  std::span<std::uint8_t> rx{};
  bool pending_complete{};
  status pending_result{status::error};
};
inline std::array<twis_state, std::size(twis_instances)> twis_states{};

template <std::size_t Index> void twis_handler(nrfx_twis_event_t const* event) noexcept
{
  auto& state = twis_states[Index];
  if (!state.transfer.active.load() || state.pending_complete)
    return;
  auto& instance = twis_instances[Index];
  if (event->type == NRFX_TWIS_EVT_READ_REQ || event->type == NRFX_TWIS_EVT_WRITE_REQ)
  {
    const auto error =
        event->type == NRFX_TWIS_EVT_READ_REQ
            ? (state.tx.empty() ? -EINVAL : nrfx_twis_tx_prepare(&instance, state.tx.data(), state.tx.size()))
            : (state.rx.empty() ? -EINVAL : nrfx_twis_rx_prepare(&instance, state.rx.data(), state.rx.size()));
    if (error != 0)
    {
      nrfx_twis_disable(&instance);
      state.pending_result = driver_result(error);
      state.pending_complete = true;
    }
    return;
  }
  auto value = status::io_error;
  if (event->type == NRFX_TWIS_EVT_READ_DONE)
    value = !state.tx.empty() && event->data.tx_amount == state.tx.size() ? status::ok : status::io_error;
  else if (event->type == NRFX_TWIS_EVT_WRITE_DONE)
    value = !state.rx.empty() && event->data.rx_amount == state.rx.size() ? status::ok : status::io_error;
  nrfx_twis_disable(&instance);
  state.pending_result = value;
  state.pending_complete = true;
}
inline void twis_complete(std::size_t index) noexcept
{
  auto& state = twis_states[index];
  if (state.pending_complete)
  {
    state.pending_complete = false;
    state.transfer.finish(state.pending_result);
  }
}
template <std::size_t... Indices> constexpr auto twis_handlers(std::index_sequence<Indices...>) noexcept
{
  return std::array<nrfx_twis_event_handler_t, sizeof...(Indices)>{twis_handler<Indices>...};
}
inline constexpr auto twis_callbacks = twis_handlers(std::make_index_sequence<std::size(twis_instances)>{});
#endif
