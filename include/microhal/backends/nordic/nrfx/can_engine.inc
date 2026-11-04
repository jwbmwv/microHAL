// SPDX-License-Identifier: Apache-2.0
// Included inside microhal::backends::nordic::nrfx.
namespace system_detail
{
struct can_state
{
  bool initialized{};
  std::uintptr_t ram{};
  void (*receive)() noexcept {};
  std::array<std::uint32_t, 16U> standard_ids{}, extended_ids{};
  std::uint8_t standard_count{}, extended_count{};
};
inline can_state can{};
constexpr result<std::uint32_t> can_nominal_timing(std::uint32_t clock, std::uint32_t baud) noexcept
{
  if (baud == 0U || baud > 1000000U || clock == 0U)
    return status::invalid_argument;
  for (std::uint32_t quanta = 80U; quanta >= 8U; --quanta)
  {
    const auto divisor = static_cast<std::uint64_t>(baud) * quanta;
    if (clock % divisor != 0U || clock / divisor == 0U || clock / divisor > 512U)
      continue;
    const auto segment2 = quanta / 5U;
    const auto segment1 = quanta - segment2 - 1U;
    return static_cast<std::uint32_t>(((clock / divisor - 1U) << 16U) | ((segment1 - 1U) << 8U) | (segment2 - 1U));
  }
  return status::invalid_argument;
}
#if MICROHAL_NORDIC_HAS_MCAN
struct can_irq_lock
{
  const std::uint32_t saved{__get_PRIMASK()};
  can_irq_lock() noexcept
  {
    __disable_irq();
  }
  ~can_irq_lock()
  {
    __set_PRIMASK(saved);
  }
};
inline bool can_init_mode(bool enter) noexcept
{
  auto value = NRF_MCAN120->CCCR;
  NRF_MCAN120->CCCR = enter ? value | MCAN_CCCR_INIT_Msk : value & ~MCAN_CCCR_INIT_Msk;
  for (std::uint32_t remaining = 1000000U; remaining; --remaining)
    if (((NRF_MCAN120->CCCR & MCAN_CCCR_INIT_Msk) != 0U) == enter)
      return true;
  return false;
}
#endif
} // namespace system_detail
struct can_api final
{
  template <typename Config> static constexpr bool bound() noexcept
  {
    return requires {
      Config::binding::can_clock_hz;
      Config::binding::can_message_ram_address;
      Config::binding::can_message_ram_base;
      Config::binding::can_message_ram_bytes;
      Config::binding::can_prepare();
    };
  }
  template <typename Config> static status initialize() noexcept
  {
#if MICROHAL_NORDIC_HAS_MCAN
    if (Config::instance_index != 0U)
      return status::invalid_argument;
    // The common message currently carries eight bytes and no FD/BRS flags.
    if (Config::enable_can_fd)
      return status::unsupported;
    if constexpr (bound<Config>())
    {
      if (system_detail::can.initialized)
        return status::busy;
      const auto timing = system_detail::can_nominal_timing(Config::binding::can_clock_hz, Config::baudrate);
      if (!timing)
        return timing.error();
      constexpr auto address = Config::binding::can_message_ram_address;
      constexpr auto base = Config::binding::can_message_ram_base;
      if (base != 0x2FBE8000U || address < base || (address & 3U) || address - base > 0x7800U - 368U ||
          Config::binding::can_message_ram_bytes < 368U)
        return status::invalid_argument;
      // Only board pinmux, transceiver and AUXPLL ownership belong to this hook.
      const auto prepared = Config::binding::can_prepare();
      if (prepared != status::ok)
        return prepared;
      NRF_CAN120->INTENCLR = UINT32_MAX;
      NRF_CAN120->EVENTS_CORE[0] = 0U;
      NRF_CAN120->EVENTS_CORE[1] = 0U;
      NRF_CAN120->TASKS_START = 1U;
      NRF_MCAN120->CCCR = NRF_MCAN120->CCCR & ~MCAN_CCCR_CSR_Msk;
      std::uint32_t remaining = 1000000U;
      while ((NRF_MCAN120->CCCR & MCAN_CCCR_CSA_Msk) && remaining)
        --remaining;
      if (!remaining)
        return status::timeout;
      if (!system_detail::can_init_mode(true))
        return status::timeout;
      NRF_MCAN120->CCCR = NRF_MCAN120->CCCR | MCAN_CCCR_CCE_Msk;
      NRF_MCAN120->NBTP = timing.value();
      NRF_MCAN120->IE = 0U;
      NRF_MCAN120->ILE = 0U;
      NRF_MCAN120->IR = UINT32_MAX;
      auto* ram = reinterpret_cast<volatile std::uint32_t*>(address);
      for (std::uint32_t i = 0U; i < 368U / 4U; ++i)
        ram[i] = 0U;
      const auto offset = static_cast<std::uint32_t>(address - base);
      NRF_MCAN120->SIDFC = offset | (16U << 16U);
      NRF_MCAN120->XIDFC = (offset + 64U) | (16U << 16U);
      NRF_MCAN120->RXF0C = (offset + 192U) | (8U << 16U);
      NRF_MCAN120->RXF1C = 0U;
      NRF_MCAN120->RXESC = 0U;
      NRF_MCAN120->TXBC = (offset + 320U) | (3U << 24U);
      NRF_MCAN120->TXESC = 0U;
      NRF_MCAN120->TXEFC = 0U;
      NRF_MCAN120->GFC = (2U << 4U) | (2U << 2U);
      NRF_MCAN120->XIDAM = 0x1FFFFFFFU;
      auto control = NRF_MCAN120->CCCR;
      control &= ~(MCAN_CCCR_MON_Msk | MCAN_CCCR_TEST_Msk);
      if (Config::mode == can_mode::silent || Config::mode == can_mode::silent_loopback)
        control |= MCAN_CCCR_MON_Msk;
      if (Config::mode == can_mode::loopback || Config::mode == can_mode::silent_loopback)
        control |= MCAN_CCCR_TEST_Msk;
      NRF_MCAN120->CCCR = control;
      NRF_MCAN120->TEST = (control & MCAN_CCCR_TEST_Msk) ? MCAN_TEST_LBCK_Msk : 0U;
      __DMB();
      if (!system_detail::can_init_mode(false))
        return status::timeout;
      system_detail::can.ram = address;
      system_detail::can.standard_count = 0U;
      system_detail::can.extended_count = 0U;
      system_detail::can.initialized = true;
      return status::ok;
    }
#endif
    return status::unsupported;
  }
  template <typename Config> static status transmit(const microhal::can::message& message) noexcept
  {
#if MICROHAL_NORDIC_HAS_MCAN
    if (Config::instance_index != 0U || message.dlc > 8U || message.id > (message.extended_id ? 0x1FFFFFFFU : 0x7FFU))
      return status::invalid_argument;
    if (!system_detail::can.initialized)
      return status::unsupported;
    if (NRF_MCAN120->PSR & MCAN_PSR_BO_Msk)
      return status::io_error;
    if (NRF_MCAN120->TXFQS & (1U << 21U))
      return status::busy;
    const auto index = (NRF_MCAN120->TXFQS >> 16U) & 0x1FU;
    if (index >= 3U)
      return status::io_error;
    auto* entry = reinterpret_cast<volatile std::uint32_t*>(system_detail::can.ram + 320U + index * 16U);
    entry[0] = (message.extended_id ? message.id | (1U << 30U) : message.id << 18U) |
               (message.remote_frame ? (1U << 29U) : 0U);
    entry[1] = static_cast<std::uint32_t>(message.dlc) << 16U;
    std::uint32_t payload[2]{};
    std::memcpy(payload, message.data, message.dlc);
    entry[2] = payload[0];
    entry[3] = payload[1];
    __DMB();
    NRF_MCAN120->TXBAR = 1U << index;
    return status::ok;
#else
    (void)message;
    return status::unsupported;
#endif
  }
  template <typename Config> static status receive(microhal::can::message& message) noexcept
  {
#if MICROHAL_NORDIC_HAS_MCAN
    if (Config::instance_index != 0U)
      return status::invalid_argument;
    if (!system_detail::can.initialized)
      return status::unsupported;
    const auto fifo = NRF_MCAN120->RXF0S;
    if ((fifo & 0x7FU) == 0U)
      return status::busy;
    const auto index = (fifo >> 8U) & 0x3FU;
    if (index >= 8U)
      return status::io_error;
    const auto* entry = reinterpret_cast<volatile std::uint32_t const*>(system_detail::can.ram + 192U + index * 16U);
    __DMB();
    const auto header = entry[0];
    const auto info = entry[1];
    message.extended_id = (header & (1U << 30U)) != 0U;
    message.remote_frame = (header & (1U << 29U)) != 0U;
    message.id = message.extended_id ? header & 0x1FFFFFFFU : (header >> 18U) & 0x7FFU;
    message.dlc = static_cast<std::uint8_t>((info >> 16U) & 15U);
    if (message.dlc > 8U || (info & (1U << 21U)))
    {
      NRF_MCAN120->RXF0A = index;
      return status::unsupported;
    }
    const std::uint32_t payload[2]{entry[2], entry[3]};
    std::memcpy(message.data, payload, message.dlc);
    __DMB();
    NRF_MCAN120->RXF0A = index;
    return status::ok;
#else
    (void)message;
    return status::unsupported;
#endif
  }
  template <typename Config>
  static status configure_filter(std::uint32_t id, std::uint32_t mask, std::uint8_t fifo) noexcept
  {
#if MICROHAL_NORDIC_HAS_MCAN
    if (Config::instance_index != 0U || id > 0x1FFFFFFFU || mask > 0x1FFFFFFFU || fifo > 1U)
      return status::invalid_argument;
    if (!system_detail::can.initialized)
      return status::unsupported;
    // Only FIFO0 has allocated RX message RAM and a public receive path.
    if (fifo != 0U)
      return status::unsupported;
    const system_detail::can_irq_lock lock{};
    const bool standard = id <= 0x7FFU && mask <= 0x7FFU;
    auto& ids = standard ? system_detail::can.standard_ids : system_detail::can.extended_ids;
    auto& count = standard ? system_detail::can.standard_count : system_detail::can.extended_count;
    // The public API has no index: repeated IDs update their rule within the inferred ID format.
    std::uint8_t index{};
    while (index < count && ids[index] != id)
      ++index;
    if (index == ids.size())
      return status::busy;
    if (standard)
    {
      auto* filter = reinterpret_cast<volatile std::uint32_t*>(system_detail::can.ram + index * 4U);
      *filter = (2U << 30U) | (1U << 27U) | (id << 16U) | mask;
    }
    else
    {
      auto* filter = reinterpret_cast<volatile std::uint32_t*>(system_detail::can.ram + 64U + index * 8U);
      filter[0] = 0U;
      __DMB();
      filter[1] = mask | (2U << 30U);
      filter[0] = id | (1U << 29U);
    }
    __DMB();
    if (index == count)
    {
      ids[index] = id;
      ++count;
    }
    return status::ok;
#else
    (void)id;
    (void)mask;
    (void)fifo;
    return status::unsupported;
#endif
  }
  template <typename Config, auto Handler> static status enable_rx_interrupt() noexcept
  {
#if MICROHAL_NORDIC_HAS_MCAN
    if (Config::instance_index != 0U)
      return status::invalid_argument;
    if (!system_detail::can.initialized)
      return status::unsupported;
    system_detail::can.receive = []() noexcept { detail::notify<Handler>(status::ok); };
    NRF_MCAN120->ILS = 0U;
    NRF_MCAN120->IE = NRF_MCAN120->IE | 1U;
    NRF_MCAN120->ILE = 1U;
    NRF_CAN120->INTENSET = CAN_INTENSET_CORE0_Msk;
    NRFX_IRQ_PRIORITY_SET(CAN120_IRQn, 7U);
    NRFX_IRQ_PENDING_CLEAR(CAN120_IRQn);
    NRFX_IRQ_ENABLE(CAN120_IRQn);
    return status::ok;
#else
    return status::unsupported;
#endif
  }
  static void handle_interrupt() noexcept
  {
#if MICROHAL_NORDIC_HAS_MCAN
    if (!system_detail::can.initialized)
      return;
    NRF_CAN120->EVENTS_CORE[0] = 0U;
    NRF_CAN120->EVENTS_CORE[1] = 0U;
    const auto pending = NRF_MCAN120->IR & NRF_MCAN120->IE;
    NRF_MCAN120->IR = pending;
    if ((pending & 1U) && system_detail::can.receive)
      system_detail::can.receive();
#endif
  }
  template <typename Config> static void handle_interrupt() noexcept
  {
    handle_interrupt();
  }
};
