// SPDX-License-Identifier: Apache-2.0
// Included inside microhal::backends::nordic::nrfx.
struct qspi_api final
{
  template <typename Config> static status initialize() noexcept
  {
#if MICROHAL_NORDIC_HAS_QSPI
    const detail::qspi_operation operation{};
    if (!operation.acquired)
      return status::busy;
    if constexpr (requires { Config::binding::configuration; })
    {
      if (nrfx_qspi_init_check())
        return status::busy;
      auto config = Config::binding::configuration;
#if defined(NRF5340_XXAA_APPLICATION)
      constexpr std::uint32_t base_clock = 96'000'000U;
#else
      constexpr std::uint32_t base_clock = 32'000'000U;
#endif
      const auto divider = static_cast<std::uint32_t>(config.phy_if.sck_freq);
      if (divider > 15U || Config::clock_hz != base_clock / (divider + 1U) || (config.xip_offset & 3U) != 0U ||
          (config.prot_if.addrmode == NRF_QSPI_ADDRMODE_24BIT && config.xip_offset > 0xFFFFFFU))
        return status::invalid_argument;
      config.timeout = detail::timeout_ms<Config>();
      const auto result = detail::driver_result(nrfx_qspi_init(&config, nullptr, nullptr));
      if (result == status::ok)
      {
#if NRF_QSPI_HAS_XIPEN
        nrf_qspi_xip_set(NRF_QSPI, false);
        if (nrf_qspi_xip_check(NRF_QSPI))
        {
          nrfx_qspi_uninit();
          return status::io_error;
        }
#endif
        detail::qspi_sync_mapping();
        detail::qspi_configuration = config;
        detail::qspi_owner = &detail::qspi_owner_token<Config>;
        detail::qspi_mapped = false;
      }
      return result;
    }
#endif
    return status::unsupported;
  }
  template <typename Config>
  static status command_write(const qspi_command& request, const std::span<const std::uint8_t> payload) noexcept
  {
#if MICROHAL_NORDIC_HAS_QSPI
    const detail::qspi_operation operation{};
    if (!operation.acquired)
      return status::busy;
    if (!detail::qspi_owned<Config>())
      return status::unsupported;
    if (detail::qspi_mapped)
      return status::busy;
    const auto result =
        detail::qspi_memory_command(request, true)
            ? (detail::qspi_range_valid(request.address, payload.size(), request.address_size_bytes,
                                        QSPI_WRITE_CNT_CNT_Msk >> QSPI_WRITE_CNT_CNT_Pos)
                   ? detail::driver_result(nrfx_qspi_write(payload.data(), payload.size(), request.address))
                   : status::invalid_argument)
            : custom_command(request, payload.data(), nullptr, payload.size());
    return finish(result);
#else
    (void)request;
    (void)payload;
    return status::unsupported;
#endif
  }
  template <typename Config>
  static status command_read(const qspi_command& request, const std::span<std::uint8_t> payload) noexcept
  {
#if MICROHAL_NORDIC_HAS_QSPI
    const detail::qspi_operation operation{};
    if (!operation.acquired)
      return status::busy;
    if (!detail::qspi_owned<Config>())
      return status::unsupported;
    if (detail::qspi_mapped)
      return status::busy;
    if (payload.empty())
      return status::invalid_argument;
    const auto result =
        detail::qspi_memory_command(request, false)
            ? (detail::qspi_range_valid(request.address, payload.size(), request.address_size_bytes,
                                        QSPI_READ_CNT_CNT_Msk >> QSPI_READ_CNT_CNT_Pos)
                   ? detail::driver_result(nrfx_qspi_read(payload.data(), payload.size(), request.address))
                   : status::invalid_argument)
            : custom_command(request, nullptr, payload.data(), payload.size());
    return finish(result);
#else
    (void)request;
    (void)payload;
    return status::unsupported;
#endif
  }
  template <typename Config> static status enter_memory_mapped(const qspi_command& command) noexcept
  {
#if MICROHAL_NORDIC_HAS_QSPI
    const detail::qspi_operation operation{};
    if (!operation.acquired)
      return status::busy;
    if (!detail::qspi_owned<Config>() || !detail::qspi_window_owned<Config>())
      return status::unsupported;
    if (detail::qspi_mapped)
      return status::busy;
    // The hardware window uses the initialized protocol/XIPOFFSET, not an arbitrary command waveform.
    if (!detail::qspi_memory_command(command, false) || command.address != 0U)
      return status::unsupported;
    const auto activated = nrfx_qspi_activate(true);
    if (activated != 0 && activated != -EALREADY)
      return finish(detail::driver_result(activated));
    // Activation alone does not wait for a preceding NOR program when already active.
    const auto ready = detail::driver_result(nrfx_qspi_mem_busy_check());
    if (ready != status::ok)
      return finish(ready);
    nrf_qspi_xip_offset_set(NRF_QSPI, detail::qspi_configuration.xip_offset);
#if NRF_QSPI_HAS_XIPEN
    nrf_qspi_xip_set(NRF_QSPI, true);
    if (!nrf_qspi_xip_check(NRF_QSPI))
      return status::io_error;
#endif
    detail::qspi_sync_mapping();
    detail::qspi_mapped = true;
    return status::ok;
#else
    (void)command;
    return status::unsupported;
#endif
  }
  template <typename Config> static status exit_memory_mapped() noexcept
  {
#if MICROHAL_NORDIC_HAS_QSPI
    const detail::qspi_operation operation{};
    if (!operation.acquired)
      return status::busy;
    if (!detail::qspi_owned<Config>() || !detail::qspi_mapped)
      return status::unsupported;
    // The owner must quiesce every mapped reader/ISR and execute this path from internal memory.
    detail::qspi_sync_mapping();
    const auto result = detail::driver_result(nrfx_qspi_deactivate());
    if (result != status::ok)
      return result;
#if NRF_QSPI_HAS_XIPEN
    nrf_qspi_xip_set(NRF_QSPI, false);
    if (nrf_qspi_xip_check(NRF_QSPI))
      return status::io_error;
#endif
    // On nRF52, deactivation is the only hardware exit: there is no XIPEN register.
    detail::qspi_sync_mapping();
    detail::qspi_mapped = false;
    return status::ok;
#else
    return status::unsupported;
#endif
  }

private:
#if MICROHAL_NORDIC_HAS_QSPI
  static status finish(status result) noexcept
  {
    if (result == status::timeout && nrfx_qspi_init_check())
    {
      nrfx_qspi_uninit();
      detail::qspi_owner = nullptr;
      detail::qspi_mapped = false;
    }
    return result;
  }
  static status custom_command(const qspi_command& request, const std::uint8_t* tx, std::uint8_t* rx,
                               const std::size_t size) noexcept
  {
    if (request.instruction_mode != qspi_io_mode::single ||
        (request.address_size_bytes != 0U && request.address_size_bytes != 3U && request.address_size_bytes != 4U) ||
        request.address_mode != (request.address_size_bytes == 0U ? qspi_io_mode::none : qspi_io_mode::single) ||
        (request.dummy_cycles % 8U) != 0U ||
        request.data_mode != (size == 0U ? qspi_io_mode::none : qspi_io_mode::single))
      return status::unsupported;
    if (request.address_size_bytes == 3U && request.address > 0xFFFFFFU)
      return status::invalid_argument;
    nrf_qspi_cinstr_conf_t config{};
    config.opcode = request.instruction;
    config.io2_level = true;
    config.io3_level = true;
    if (request.address_size_bytes == 0U && request.dummy_cycles == 0U && size <= 8U)
    {
      config.length = static_cast<nrf_qspi_cinstr_len_t>(size + 1U);
      return detail::driver_result(nrfx_qspi_cinstr_xfer(&config, tx, rx));
    }
    config.length = NRF_QSPI_CINSTR_LEN_1B;
    auto result = detail::driver_result(nrfx_qspi_lfm_start(&config));
    if (result != status::ok)
      return result;
    std::array<std::uint8_t, 36U> prefix{};
    for (std::uint8_t i = 0U; i < request.address_size_bytes; ++i)
      prefix[i] = static_cast<std::uint8_t>(request.address >> (8U * (request.address_size_bytes - i - 1U)));
    const auto prefix_size = request.address_size_bytes + request.dummy_cycles / 8U;
    if (prefix_size != 0U)
    {
      result = detail::driver_result(nrfx_qspi_lfm_xfer(prefix.data(), nullptr, prefix_size, size == 0U));
      if (result != status::ok)
        return discard_frame(result);
      if (size == 0U)
        return status::ok;
    }
    result = detail::driver_result(nrfx_qspi_lfm_xfer(tx, rx, size, true));
    return result == status::ok ? result : discard_frame(result);
  }
  static status discard_frame(status result) noexcept
  {
    // A failed long command can leave CINSTR asserted; do not expose that state to XIP.
    nrfx_qspi_uninit();
    detail::qspi_owner = nullptr;
    return result;
  }
#endif
};
