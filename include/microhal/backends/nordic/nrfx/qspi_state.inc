// SPDX-License-Identifier: Apache-2.0
// Included inside the nrfx backend's detail namespace.
constexpr bool qspi_range_valid(const std::uint32_t address, const std::size_t size, const std::uint8_t address_bytes,
                                const std::size_t max_count) noexcept
{
  if ((address_bytes != 3U && address_bytes != 4U) || size == 0U || size > max_count)
    return false;
  const std::uint64_t limit = std::uint64_t{1U} << (address_bytes * 8U);
  return address < limit && size <= limit - address;
}
#if MICROHAL_NORDIC_HAS_QSPI
inline nrfx_qspi_config_t qspi_configuration{};
inline std::atomic<bool> qspi_active{false};
inline bool qspi_mapped{false};
inline const void* qspi_owner{};
template <typename Config> inline constexpr std::uint8_t qspi_owner_token{};
struct qspi_operation
{
  const bool acquired{!qspi_active.exchange(true)};
  ~qspi_operation()
  {
    if (acquired)
      qspi_active.store(false);
  }
  qspi_operation() = default;
  qspi_operation(const qspi_operation&) = delete;
  qspi_operation& operator=(const qspi_operation&) = delete;
};
template <typename Config> bool qspi_owned() noexcept
{
  return nrfx_qspi_init_check() && qspi_owner == &qspi_owner_token<Config>;
}
template <typename Config> constexpr bool qspi_window_owned() noexcept
{
  // The binding owns the aperture; transitions run internally with mapped bus masters quiesced.
  if constexpr (requires { Config::binding::owns_xip_window; })
    return Config::binding::owns_xip_window;
  return false;
}
inline void qspi_sync_mapping() noexcept
{
  __DSB();
#if MICROHAL_NORDIC_QSPI_HAS_CACHE
  if (nrf_cache_enable_check(NRF_CACHE))
    nrf_cache_invalidate(NRF_CACHE);
#endif
  __DSB();
  __ISB();
}
inline bool qspi_memory_command(const qspi_command& command, const bool write) noexcept
{
  const auto& config = qspi_configuration;
  const auto address_bytes = config.prot_if.addrmode == NRF_QSPI_ADDRMODE_24BIT ? 3U : 4U;
  if (command.instruction_mode != qspi_io_mode::single || command.address_size_bytes != address_bytes ||
      (address_bytes == 3U && command.address > 0xFFFFFFU))
    return false;
  if (write)
  {
    if (command.dummy_cycles != 0U)
      return false;
    switch (config.prot_if.writeoc)
    {
    case NRF_QSPI_WRITEOC_PP:
      return command.instruction == 0x02U && command.address_mode == qspi_io_mode::single &&
             command.data_mode == qspi_io_mode::single;
    case NRF_QSPI_WRITEOC_PP2O:
      return command.instruction == 0xA2U && command.address_mode == qspi_io_mode::single &&
             command.data_mode == qspi_io_mode::dual;
    case NRF_QSPI_WRITEOC_PP4O:
      return command.instruction == 0x32U && command.address_mode == qspi_io_mode::single &&
             command.data_mode == qspi_io_mode::quad;
    case NRF_QSPI_WRITEOC_PP4IO:
      return command.instruction == 0x38U && command.address_mode == qspi_io_mode::quad &&
             command.data_mode == qspi_io_mode::quad;
    default:
      return false;
    }
  }
  switch (config.prot_if.readoc)
  {
  case NRF_QSPI_READOC_FASTREAD:
    return command.instruction == 0x0BU && command.address_mode == qspi_io_mode::single &&
           command.data_mode == qspi_io_mode::single && command.dummy_cycles == 8U;
  case NRF_QSPI_READOC_READ2O:
    return command.instruction == 0x3BU && command.address_mode == qspi_io_mode::single &&
           command.data_mode == qspi_io_mode::dual && command.dummy_cycles == 8U;
  case NRF_QSPI_READOC_READ4O:
    return command.instruction == 0x6BU && command.address_mode == qspi_io_mode::single &&
           command.data_mode == qspi_io_mode::quad && command.dummy_cycles == 8U;
  case NRF_QSPI_READOC_READ2IO:
    return command.instruction == 0xBBU && command.address_mode == qspi_io_mode::dual &&
           command.data_mode == qspi_io_mode::dual && command.dummy_cycles == 4U;
  case NRF_QSPI_READOC_READ4IO:
    return command.instruction == 0xEBU && command.address_mode == qspi_io_mode::quad &&
           command.data_mode == qspi_io_mode::quad && command.dummy_cycles == 6U;
  default:
    return false;
  }
}
#endif
