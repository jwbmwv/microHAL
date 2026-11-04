// SPDX-License-Identifier: Apache-2.0
// Included inside microhal::backends::nordic::nrfx.
struct mram_api final
{
  static status initialize() noexcept
  {
#if MICROHAL_NORDIC_HAS_MRAMC
    static bool initialized{};
    if (initialized)
      return status::ok;
    const nrfx_mramc_config_t config = NRFX_MRAMC_DEFAULT_CONFIG();
    const auto result = nrfx_mramc_init(&config, nullptr);
    if (result != 0 && result != -EALREADY)
      return detail::driver_result(result);
    initialized = true;
    return status::ok;
#else
    return status::unsupported;
#endif
  }
  static status erase(std::uint32_t address, std::uint32_t bytes) noexcept
  {
#if MICROHAL_NORDIC_HAS_MRAMC
    if (bytes == 0U || (address & 3U) || (bytes & 3U) || !nrfx_mramc_fits_memory_check(address, false, bytes))
      return status::invalid_argument;
    const auto ready = initialize();
    if (ready != status::ok)
      return ready;
    nrf_mramc_config_t saved{};
    nrf_mramc_config_get(NRF_MRAMC, &saved);
    nrfx_mramc_config_erase_mode_set(NRF_MRAMC_MODE_ERASE_WORD);
    // The vendor primitive's size argument is in words, not bytes.
    nrfx_mramc_area_erase(address, bytes / 4U);
    nrfx_mramc_config_erase_mode_set(saved.mode_erase);
    return status::ok;
#else
    (void)address;
    (void)bytes;
    return status::unsupported;
#endif
  }
  static status program(std::uint32_t address, std::span<const std::uint8_t> data) noexcept
  {
#if MICROHAL_NORDIC_HAS_MRAMC
    if (data.empty() || data.size() > UINT32_MAX || (address & 3U) || (data.size() & 3U) ||
        !nrfx_mramc_fits_memory_check(address, false, static_cast<std::uint32_t>(data.size())))
      return status::invalid_argument;
    const auto ready = initialize();
    if (ready != status::ok)
      return ready;
    nrf_mramc_config_t saved{};
    nrf_mramc_config_get(NRF_MRAMC, &saved);
    nrfx_mramc_config_write_mode_set(NRF_MRAMC_MODE_WRITE_NORMAL);
    for (std::size_t offset = 0; offset < data.size(); offset += 4U)
    {
      std::uint32_t word{};
      std::memcpy(&word, data.data() + offset, sizeof(word));
      nrfx_mramc_word_write(address + static_cast<std::uint32_t>(offset), word);
    }
    nrfx_mramc_config_write_mode_set(saved.mode_write);
    return status::ok;
#else
    (void)address;
    (void)data;
    return status::unsupported;
#endif
  }
  static status read(std::uint32_t address, std::span<std::uint8_t> data) noexcept
  {
#if MICROHAL_NORDIC_HAS_MRAMC
    if (data.empty() || data.size() > UINT32_MAX ||
        !nrfx_mramc_fits_memory_check(address, false, static_cast<std::uint32_t>(data.size())))
      return status::invalid_argument;
    nrfx_mramc_buffer_read(data.data(), address, static_cast<std::uint32_t>(data.size()));
    return status::ok;
#else
    (void)address;
    (void)data;
    return status::unsupported;
#endif
  }
};

struct h20_mram_api final
{
#if MICROHAL_NORDIC_HAS_H20_MRAM
  static bool readable_word(std::uint32_t address) noexcept
  {
    // An uncorrectable H20 MRAM word faults on read instead of returning bad data.
    // Probe without escalating the BusFault before doing ordinary readback.
    constexpr auto mask = SCB_CFSR_BUSFAULTSR_Msk;
    const auto primask = __get_PRIMASK();
    __disable_irq();
    if (SCB->CFSR & mask)
    {
      __set_PRIMASK(primask);
      return false;
    }
    const auto faultmask = __get_FAULTMASK();
    const auto control = SCB->CCR;
    __set_FAULTMASK(1U);
    SCB->CCR = control | SCB_CCR_BFHFNMIGN_Msk;
    __DSB();
    const auto value = *reinterpret_cast<volatile std::uint32_t const*>(static_cast<std::uintptr_t>(address));
    (void)value;
    __DSB();
    const auto fault = SCB->CFSR & mask;
    SCB->CFSR = fault;
    SCB->CCR = control;
    __set_FAULTMASK(faultmask);
    __DSB();
    __ISB();
    __set_PRIMASK(primask);
    return fault == 0U;
  }
#endif
  template <typename Config> static constexpr bool bound() noexcept
  {
    return requires {
      Config::binding::mram_start_address;
      Config::binding::mram_size_bytes;
      Config::binding::mram_bank11_address;
      Config::binding::mram_acquire();
      Config::binding::mram_release();
    };
  }
  template <typename Config> static bool range(std::uint32_t address, std::size_t bytes) noexcept
  {
    if constexpr (bound<Config>())
      return (Config::binding::mram_start_address & 15U) == 0U && (Config::binding::mram_size_bytes & 15U) == 0U &&
             bytes != 0U && bytes <= UINT32_MAX && address >= Config::binding::mram_start_address &&
             static_cast<std::uint64_t>(address) + bytes <=
                 static_cast<std::uint64_t>(Config::binding::mram_start_address) + Config::binding::mram_size_bytes;
    return false;
  }
  template <typename Config>
  static status write(std::uint32_t address, std::span<const std::uint8_t> data, bool erase) noexcept
  {
#if MICROHAL_NORDIC_HAS_H20_MRAM
    if constexpr (bound<Config>())
    {
      if (!range<Config>(address, data.size()) || (address & 15U) || (data.size() & 15U))
        return status::invalid_argument;
      // The secure system-controller owns MRAM latency/power configuration.
      // This hook only acquires that service; the memory engine remains native.
      const auto acquired = Config::binding::mram_acquire();
      if (acquired != status::ok)
        return acquired;
      status result = status::ok;
      for (std::size_t offset = 0U; offset < data.size(); offset += 16U)
      {
        const auto target = address + static_cast<std::uint32_t>(offset);
        auto* controller = target < Config::binding::mram_bank11_address ? NRF_MRAMC110 : NRF_MRAMC111;
        std::uint32_t remaining = 1000000U;
        while (!controller->READY && remaining)
          --remaining;
        if (!remaining)
        {
          result = status::timeout;
          break;
        }
        alignas(16) std::uint32_t words[4]{};
        if (erase)
          for (auto& word : words)
            word = 0xFFFFFFFFU;
        else
          std::memcpy(words, data.data() + offset, 16U);
        const auto saved = __get_PRIMASK();
        __disable_irq();
        auto* output = reinterpret_cast<volatile std::uint32_t*>(static_cast<std::uintptr_t>(target));
        for (std::size_t word = 0U; word < 4U; ++word)
          output[word] = words[word];
        __DSB();
        __set_PRIMASK(saved);
        remaining = 1000000U;
        while (!controller->READY && remaining)
          --remaining;
        if (!remaining)
        {
          result = status::timeout;
          break;
        }
        if (!readable_word(target) ||
            std::memcmp(reinterpret_cast<void const*>(static_cast<std::uintptr_t>(target)), words, 16U) != 0)
        {
          result = status::io_error;
          break;
        }
      }
      Config::binding::mram_release();
      return result;
    }
#else
    (void)address;
    (void)data;
    (void)erase;
#endif
    return status::unsupported;
  }
  template <typename Config> static status erase(std::uint32_t address, std::uint32_t bytes) noexcept
  {
    if (!range<Config>(address, bytes) || (address & 15U) || (bytes & 15U))
      return bound<Config>() ? status::invalid_argument : status::unsupported;
    // The erase path never dereferences this span; it writes the erased value.
    return write<Config>(address, {reinterpret_cast<std::uint8_t const*>(static_cast<std::uintptr_t>(address)), bytes},
                         true);
  }
  template <typename Config> static status program(std::uint32_t address, std::span<const std::uint8_t> data) noexcept
  {
    return write<Config>(address, data, false);
  }
  template <typename Config> static status read(std::uint32_t address, std::span<std::uint8_t> data) noexcept
  {
#if MICROHAL_NORDIC_HAS_H20_MRAM
    if (!range<Config>(address, data.size()))
      return bound<Config>() ? status::invalid_argument : status::unsupported;
    for (std::uint64_t cursor = address & ~15U; cursor < static_cast<std::uint64_t>(address) + data.size();
         cursor += 16U)
      if (!readable_word(static_cast<std::uint32_t>(cursor)))
        return status::io_error;
    std::memcpy(data.data(), reinterpret_cast<void const*>(static_cast<std::uintptr_t>(address)), data.size());
    return status::ok;
#else
    (void)address;
    (void)data;
    return status::unsupported;
#endif
  }
};

struct flash_api final
{
  template <typename Config> static status erase(std::uint32_t address, std::uint32_t bytes) noexcept
  {
#if MICROHAL_NORDIC_HAS_RRAMC
    return rram_api::erase(address, bytes);
#elif MICROHAL_NORDIC_HAS_H20_MRAM
    return h20_mram_api::erase<Config>(address, bytes);
#elif MICROHAL_NORDIC_HAS_MRAMC
    return mram_api::erase(address, bytes);
#else
    return nvmc_api::erase<Config>(address, bytes);
#endif
  }
  template <typename Config> static status program(std::uint32_t address, std::span<const std::uint8_t> data) noexcept
  {
#if MICROHAL_NORDIC_HAS_RRAMC
    return rram_api::program(address, data);
#elif MICROHAL_NORDIC_HAS_H20_MRAM
    return h20_mram_api::program<Config>(address, data);
#elif MICROHAL_NORDIC_HAS_MRAMC
    return mram_api::program(address, data);
#else
    return nvmc_api::program<Config>(address, data);
#endif
  }
  template <typename Config> static status read(std::uint32_t address, std::span<std::uint8_t> data) noexcept
  {
#if MICROHAL_NORDIC_HAS_RRAMC
    return rram_api::read(address, data);
#elif MICROHAL_NORDIC_HAS_H20_MRAM
    return h20_mram_api::read<Config>(address, data);
#elif MICROHAL_NORDIC_HAS_MRAMC
    return mram_api::read(address, data);
#else
    return nvmc_api::read<Config>(address, data);
#endif
  }
  template <typename Config> static result<flash_erase_region> erase_region(std::uint32_t address) noexcept
  {
#if MICROHAL_NORDIC_HAS_RRAMC
    const auto base = NRFY_RRAMC_RRAM_BASE_ADDRESS;
    if (address < base ||
        static_cast<std::uint64_t>(address) >= static_cast<std::uint64_t>(base) + rram_api::size_bytes())
      return status::invalid_argument;
    if (!rram_api::range_accessible(address & ~3U, 4U, true))
      return status::unsupported;
    return flash_erase_region{.start_address = address & ~3U, .size_bytes = 4U};
#elif MICROHAL_NORDIC_HAS_H20_MRAM
    if (!h20_mram_api::range<Config>(address & ~15U, 16U))
      return h20_mram_api::bound<Config>() ? status::invalid_argument : status::unsupported;
    return flash_erase_region{.start_address = address & ~15U, .size_bytes = 16U};
#elif MICROHAL_NORDIC_HAS_MRAMC
    if (!nrfx_mramc_fits_memory_check(address & ~3U, false, 4U))
      return status::invalid_argument;
    return flash_erase_region{.start_address = address & ~3U, .size_bytes = 4U};
#else
    return nvmc_api::erase_region<Config>(address);
#endif
  }
};
