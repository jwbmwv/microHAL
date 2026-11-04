// SPDX-License-Identifier: Apache-2.0
#if MICROHAL_NORDIC_HAS_SPIS
inline nrfx_spis_t spis_instances[]{
#if defined(NRF54L15_XXAA)
    {NRF_SPIS00, {}}, {NRF_SPIS20, {}}, {NRF_SPIS21, {}}, {NRF_SPIS22, {}}, {NRF_SPIS30, {}}
#elif defined(NRF54H20_XXAA)
    {NRF_SPIS130, {}}, {NRF_SPIS131, {}}, {NRF_SPIS132, {}}, {NRF_SPIS133, {}}, {NRF_SPIS134, {}},
    {NRF_SPIS135, {}}, {NRF_SPIS136, {}}, {NRF_SPIS137, {}}, {NRF_SPIS120, {}}
#else
    {NRF_SPIS0, {}},
    {NRF_SPIS1, {}},
    {NRF_SPIS2, {}}
#if defined(NRF5340_XXAA_APPLICATION)
    ,
    {NRF_SPIS3, {}}
#endif
#endif
};
struct spis_state
{
  nrfx_spis_t* instance{};
  transfer_state transfer{};
  std::size_t tx_size{};
  std::size_t rx_size{};
};
inline std::array<spis_state, std::size(spis_instances)> spis_states{};
inline void spis_handler(nrfx_spis_event_t const* event, void* context) noexcept
{
  auto& state = *static_cast<spis_state*>(context);
  if (event->evt_type == NRFX_SPIS_XFER_DONE && state.transfer.active.load())
  {
    const auto errors = static_cast<std::uint32_t>(nrf_spis_status_get(state.instance->p_reg));
    state.instance->p_reg->STATUS = errors;
    const auto mask = (state.tx_size != 0U ? static_cast<std::uint32_t>(NRF_SPIS_STATUS_OVERREAD) : 0U) |
                      (state.rx_size != 0U ? static_cast<std::uint32_t>(NRF_SPIS_STATUS_OVERFLOW) : 0U);
    state.transfer.finish((errors & mask) == 0U && event->tx_amount == state.tx_size &&
                                  event->rx_amount == state.rx_size
                              ? status::ok
                              : status::io_error);
  }
}
#endif
