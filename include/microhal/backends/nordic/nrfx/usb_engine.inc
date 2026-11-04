// SPDX-License-Identifier: Apache-2.0
// Included inside microhal::backends::nordic::nrfx.
namespace system_detail
{
#if MICROHAL_NORDIC_HAS_USBD
struct usb_rx_state
{
  alignas(4) std::array<std::uint8_t, 64U> storage{};
  std::atomic<bool> complete{};
  std::atomic<bool> pending{};
  std::uint32_t count{};
};
inline std::array<usb_rx_state, 8U> usb_rx{};
struct usb_irq_lock
{
  const std::uint32_t saved{__get_PRIMASK()};
  usb_irq_lock() noexcept
  {
    __disable_irq();
  }
  ~usb_irq_lock()
  {
    __set_PRIMASK(saved);
  }
};
inline void (*usb_policy)(nrfx_usbd_evt_t const*) noexcept {};
inline void usb_event(nrfx_usbd_evt_t const* event) noexcept
{
  if (event->type == NRFX_USBD_EVT_RESET)
  {
    for (auto& state : usb_rx)
    {
      state.complete.store(false);
      state.pending = false;
      state.count = 0U;
    }
  }
  if (event->type == NRFX_USBD_EVT_EPTRANSFER && (static_cast<std::uint8_t>(event->data.eptransfer.ep) & 0x80U) == 0U)
  {
    const auto index = static_cast<std::uint8_t>(event->data.eptransfer.ep);
    if (index < usb_rx.size() && usb_rx[index].pending && event->data.eptransfer.status != NRFX_USBD_EP_WAITING)
    {
      auto& state = usb_rx[index];
      std::size_t count{};
      const auto result = nrfx_usbd_ep_status_get(event->data.eptransfer.ep, &count);
      state.count = result == NRFX_USBD_EP_OK ? static_cast<std::uint32_t>(count) : 0U;
      state.pending = false;
      state.complete.store(true);
    }
  }
  if (usb_policy)
    usb_policy(event);
}
#endif
} // namespace system_detail
struct usb_api final
{
  template <typename Config> static status initialize() noexcept
  {
#if MICROHAL_NORDIC_HAS_USBD
    if (Config::host_mode || Config::otg_mode || !Config::device_mode)
      return status::unsupported;
    // USBD is full-speed only; the hardware does not provide a high-speed PHY.
    if (Config::speed != usb_speed::full)
      return status::unsupported;
    if constexpr (requires(nrfx_usbd_setup_t const& setup) { Config::binding::handle_setup(setup); })
    {
      if (nrfx_usbd_init_check())
        return status::busy;
      const auto result = detail::driver_result(nrfx_usbd_init(system_detail::usb_event));
      if (result != status::ok)
        return result;
      system_detail::usb_policy = [](nrfx_usbd_evt_t const* event) noexcept
      {
        if (event->type == NRFX_USBD_EVT_SETUP)
        {
          nrfx_usbd_setup_t setup{};
          nrfx_usbd_setup_get(&setup);
          Config::binding::handle_setup(setup);
        }
        if constexpr (requires { Config::binding::usb_event(event); })
          Config::binding::usb_event(event);
      };
      return status::ok;
    }
#endif
    return status::unsupported;
  }
  template <typename Config> static status connect() noexcept
  {
#if MICROHAL_NORDIC_HAS_USBD
    if (!nrfx_usbd_init_check())
      return status::unsupported;
    if (nrfx_usbd_is_started())
      return status::busy;
#if defined(NRF52840_XXAA)
    if ((NRF_POWER->USBREGSTATUS & POWER_USBREGSTATUS_VBUSDETECT_Msk) == 0U)
      return status::busy;
#elif defined(NRF5340_XXAA_APPLICATION)
    if ((NRF_USBREGULATOR->USBREGSTATUS & USBREG_USBREGSTATUS_VBUSDETECT_Msk) == 0U)
      return status::busy;
#endif
    if constexpr (requires { Config::binding::request_usb_clock(); })
    {
      const auto requested = Config::binding::request_usb_clock();
      if (requested != status::ok)
        return requested;
    }
    else
    {
#if MICROHAL_NORDIC_HAS_CLOCK
      if (!nrfx_clock_init_check())
      {
        const auto result = detail::driver_result(nrfx_clock_init(nullptr));
        if (result != status::ok)
          return result;
      }
      nrfx_clock_start(NRF_CLOCK_DOMAIN_HFCLK);
      std::uint32_t remaining = 1000000U;
      while (!nrfx_clock_is_running(NRF_CLOCK_DOMAIN_HFCLK, nullptr) && remaining)
        --remaining;
      if (!remaining)
        return status::timeout;
#else
      return status::unsupported;
#endif
    }
    if (!nrfx_usbd_is_enabled())
      nrfx_usbd_enable();
    nrfx_usbd_start(false);
    return status::ok;
#else
    return status::unsupported;
#endif
  }
  template <typename Config> static status disconnect() noexcept
  {
#if MICROHAL_NORDIC_HAS_USBD
    if (!nrfx_usbd_init_check())
      return status::unsupported;
    if (nrfx_usbd_is_started())
      nrfx_usbd_stop();
    if (nrfx_usbd_is_enabled())
      nrfx_usbd_disable();
    for (auto& state : system_detail::usb_rx)
    {
      state.pending = false;
      state.complete.store(false);
    }
    if constexpr (requires { Config::binding::release_usb_clock(); })
      Config::binding::release_usb_clock();
    return status::ok;
#else
    return status::unsupported;
#endif
  }
  template <typename Config>
  static status endpoint_transmit(std::uint8_t endpoint, std::span<const std::uint8_t> data) noexcept
  {
#if MICROHAL_NORDIC_HAS_USBD
    if ((endpoint & 0x7FU) >= 8U || data.size() > UINT16_MAX)
      return status::invalid_argument;
    if (!nrfx_usbd_is_started())
      return status::unsupported;
    const auto ep = NRFX_USBD_EPIN(endpoint & 0x0FU);
    if (!nrfx_usbd_ep_enable_check(ep))
    {
      nrfx_usbd_ep_max_packet_size_set(ep, 64U);
      nrfx_usbd_ep_enable(ep);
    }
    // The span must remain alive until EPTRANSFER reports completion.
    NRFX_USBD_TRANSFER_IN(transfer, data.data(), data.size(), 0U);
    return detail::driver_result(nrfx_usbd_ep_transfer(ep, &transfer));
#else
    (void)endpoint;
    (void)data;
    return status::unsupported;
#endif
  }
  template <typename Config>
  static std::uint32_t endpoint_receive(std::uint8_t endpoint, std::span<std::uint8_t> buffer) noexcept
  {
#if MICROHAL_NORDIC_HAS_USBD
    if (endpoint >= 8U || buffer.empty() || !nrfx_usbd_is_started())
      return 0U;
    const system_detail::usb_irq_lock lock{};
    auto& state = system_detail::usb_rx[endpoint];
    if (state.complete.load())
    {
      if (buffer.size() < state.count)
        return 0U;
      std::memcpy(buffer.data(), state.storage.data(), state.count);
      const auto count = state.count;
      state.complete.store(false);
      return count;
    }
    if (!state.pending)
    {
      const auto ep = NRFX_USBD_EPOUT(endpoint);
      if (!nrfx_usbd_ep_enable_check(ep))
      {
        nrfx_usbd_ep_max_packet_size_set(ep, 64U);
        nrfx_usbd_ep_enable(ep);
      }
      NRFX_USBD_TRANSFER_OUT(transfer, state.storage.data(), state.storage.size());
      state.pending = true;
      if (nrfx_usbd_ep_transfer(ep, &transfer) != 0)
        state.pending = false;
    }
#else
    (void)endpoint;
    (void)buffer;
#endif
    return 0U;
  }
  static void handle_interrupt() noexcept
  {
#if MICROHAL_NORDIC_HAS_USBD
    if (nrfx_usbd_init_check())
      nrfx_usbd_irq_handler();
#endif
  }
  template <typename Config> static void handle_interrupt() noexcept
  {
    handle_interrupt();
  }
  template <typename Config> static status endpoint_status(std::uint8_t endpoint) noexcept
  {
#if MICROHAL_NORDIC_HAS_USBD
    if ((endpoint & 0x7FU) >= 8U)
      return status::invalid_argument;
    if (!nrfx_usbd_init_check())
      return status::unsupported;
    const auto ep = static_cast<nrfx_usbd_ep_t>(endpoint);
    std::size_t size{};
    const auto result = nrfx_usbd_ep_status_get(ep, &size);
    switch (result)
    {
    case NRFX_USBD_EP_OK:
      return status::ok;
    case NRFX_USBD_EP_BUSY:
    case NRFX_USBD_EP_WAITING:
      return status::busy;
    default:
      return status::io_error;
    }
#else
    (void)endpoint;
    return status::unsupported;
#endif
  }
  template <typename Config> static status endpoint_abort(std::uint8_t endpoint) noexcept
  {
#if MICROHAL_NORDIC_HAS_USBD
    if ((endpoint & 0x7FU) >= 8U)
      return status::invalid_argument;
    if (!nrfx_usbd_is_started())
      return status::unsupported;
    nrfx_usbd_ep_abort(static_cast<nrfx_usbd_ep_t>(endpoint));
    std::size_t size{};
    const auto transfer = nrfx_usbd_ep_status_get(static_cast<nrfx_usbd_ep_t>(endpoint), &size);
    if (transfer == NRFX_USBD_EP_BUSY || transfer == NRFX_USBD_EP_WAITING)
      return status::busy;
    if ((endpoint & 0x80U) == 0U)
    {
      auto& state = system_detail::usb_rx[endpoint];
      state.pending = false;
      state.complete.store(false);
    }
    return status::ok;
#else
    (void)endpoint;
    return status::unsupported;
#endif
  }
  template <typename Config> static status endpoint_stall(std::uint8_t endpoint, bool stalled) noexcept
  {
#if MICROHAL_NORDIC_HAS_USBD
    if ((endpoint & 0x7FU) >= 8U)
      return status::invalid_argument;
    if (!nrfx_usbd_is_started())
      return status::unsupported;
    const auto ep = static_cast<nrfx_usbd_ep_t>(endpoint);
    if (stalled)
      nrfx_usbd_ep_stall(ep);
    else
      nrfx_usbd_ep_stall_clear(ep);
    return nrfx_usbd_ep_stall_check(ep) == stalled ? status::ok : status::io_error;
#else
    (void)endpoint;
    (void)stalled;
    return status::unsupported;
#endif
  }
};
