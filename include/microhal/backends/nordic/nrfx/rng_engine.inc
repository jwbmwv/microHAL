// SPDX-License-Identifier: Apache-2.0
// Included inside microhal::backends::nordic::nrfx.
namespace system_detail
{
struct entropy_driver
{
  static inline std::atomic<bool> initialized{};
  static inline std::atomic<bool> active{};
  static status initialize() noexcept
  {
    if (initialized.load())
      return status::ok;
    if (active.exchange(true))
      return status::busy;
    const auto result = initialize_hardware();
    if (result == status::ok)
      initialized.store(true);
    active.store(false);
    return result;
  }
  static result<std::uint32_t> read() noexcept
  {
    if (!initialized.load())
      return status::unsupported;
    if (active.exchange(true))
      return status::busy;
    const auto result = generate();
    active.store(false);
    return result;
  }

private:
  static status initialize_hardware() noexcept
  {
#if MICROHAL_NORDIC_HAS_RNG_HAL
    if (nrf_rng_int_enable_check(NRF_RNG, NRF_RNG_INT_VALRDY_MASK))
      return status::busy;
    nrf_rng_error_correction_enable(NRF_RNG);
#elif MICROHAL_NORDIC_HAS_CC3XX_ENTROPY
    if (!nrf_cc3xx_platform_is_initialized() && nrf_cc3xx_platform_init() != 0)
      return status::io_error;
#elif MICROHAL_NORDIC_HAS_CRACEN_ENTROPY
    sx_trng context{};
    if (sx_trng_open(&context, nullptr) != SX_OK)
      return status::io_error;
    if (sx_trng_close(&context) != SX_OK)
      return status::io_error;
#else
    return status::unsupported;
#endif
    return status::ok;
  }
  static result<std::uint32_t> generate() noexcept
  {
#if MICROHAL_NORDIC_HAS_RNG_HAL
    std::uint32_t value{};
    const auto shorts = NRF_RNG->SHORTS;
    nrf_rng_shorts_enable(NRF_RNG, NRF_RNG_SHORT_VALRDY_STOP_MASK);
    for (std::uint8_t byte = 0U; byte < 4U; ++byte)
    {
      nrf_rng_event_clear(NRF_RNG, NRF_RNG_EVENT_VALRDY);
      nrf_rng_task_trigger(NRF_RNG, NRF_RNG_TASK_START);
      std::uint32_t remaining = 1000000U;
      while (!nrf_rng_event_check(NRF_RNG, NRF_RNG_EVENT_VALRDY) && remaining)
        --remaining;
      if (!remaining)
      {
        nrf_rng_task_trigger(NRF_RNG, NRF_RNG_TASK_STOP);
        NRF_RNG->SHORTS = shorts;
        return status::timeout;
      }
      value |= static_cast<std::uint32_t>(nrf_rng_random_value_get(NRF_RNG)) << (byte * 8U);
    }
    NRF_RNG->SHORTS = shorts;
    return value;
#elif MICROHAL_NORDIC_HAS_CC3XX_ENTROPY
    std::uint32_t value{};
    std::size_t written{};
    if (nrf_cc3xx_platform_entropy_get(reinterpret_cast<std::uint8_t*>(&value), sizeof(value), &written) != 0 ||
        written != sizeof(value))
      return status::io_error;
    return value;
#elif MICROHAL_NORDIC_HAS_CRACEN_ENTROPY
    sx_trng context{};
    if (sx_trng_open(&context, nullptr) != SX_OK)
      return status::io_error;
    std::uint32_t value{};
    std::uint32_t remaining = 1000000U;
    int result{};
    do
    {
      result = sx_trng_get(&context, reinterpret_cast<std::uint8_t*>(&value), sizeof(value));
    } while (result == SX_ERR_HW_PROCESSING && --remaining);
    const auto closed = sx_trng_close(&context);
    if (result == SX_ERR_HW_PROCESSING)
      return status::timeout;
    if (result != SX_OK || closed != SX_OK)
      return status::io_error;
    return value;
#else
    return status::unsupported;
#endif
  }
};
} // namespace system_detail
