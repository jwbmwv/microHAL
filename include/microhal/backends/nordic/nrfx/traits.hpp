// SPDX-License-Identifier: Apache-2.0
/**
 * @file traits.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <span>
#include <utility>

#if __has_include(<nrfx.h>)
#include <nrfx.h>
#define MICROHAL_NORDIC_NRFX_PRESENT 1
#else
#define MICROHAL_NORDIC_NRFX_PRESENT 0
#endif

#if MICROHAL_NORDIC_NRFX_PRESENT && __has_include(<nrfx_twim.h>)
#include <nrfx_twim.h>
#if defined(NRF52840_XXAA) && __has_include(<system_nrf52840.h>)
#include <system_nrf52840.h>
#define MICROHAL_NORDIC_SYSTEM_CORE_CLOCK 1
#elif defined(NRF5340_XXAA_APPLICATION) && __has_include(<system_nrf5340_application.h>)
#include <system_nrf5340_application.h>
#define MICROHAL_NORDIC_SYSTEM_CORE_CLOCK 1
#elif defined(NRF5340_XXAA_NETWORK) && __has_include(<system_nrf5340_network.h>)
#include <system_nrf5340_network.h>
#define MICROHAL_NORDIC_SYSTEM_CORE_CLOCK 1
#elif defined(NRF54L15_XXAA) && __has_include(<system_nrf54l.h>)
#include <system_nrf54l.h>
#define MICROHAL_NORDIC_SYSTEM_CORE_CLOCK 1
#elif defined(NRF54H20_XXAA) && __has_include(<system_nrf54h.h>)
#include <system_nrf54h.h>
#define MICROHAL_NORDIC_SYSTEM_CORE_CLOCK 1
#else
#define MICROHAL_NORDIC_SYSTEM_CORE_CLOCK 0
#endif
#if MICROHAL_NORDIC_SYSTEM_CORE_CLOCK
#include <lib/nrfx_coredep.h>
#endif
#if (defined(NRF_TWIM0) && defined(NRF_TWIM1)) || defined(NRF54L15_XXAA) || defined(NRF54H20_XXAA)
#define MICROHAL_NORDIC_TWIM_API_AVAILABLE 1
#else
#define MICROHAL_NORDIC_TWIM_API_AVAILABLE 0
#endif
#else
#define MICROHAL_NORDIC_SYSTEM_CORE_CLOCK 0
#define MICROHAL_NORDIC_TWIM_API_AVAILABLE 0
#endif

#if MICROHAL_NORDIC_TWIM_API_AVAILABLE && defined(NRFX_TWIM_ENABLED) && NRFX_TWIM_ENABLED
#define MICROHAL_NORDIC_HAS_TWIM 1
#else
#define MICROHAL_NORDIC_HAS_TWIM 0
#endif
#if MICROHAL_NORDIC_NRFX_PRESENT && defined(NRFX_TWIS_ENABLED) && NRFX_TWIS_ENABLED && __has_include(<nrfx_twis.h>)
#include <nrfx_twis.h>
#define MICROHAL_NORDIC_HAS_TWIS 1
#else
#define MICROHAL_NORDIC_HAS_TWIS 0
#endif

#if MICROHAL_NORDIC_NRFX_PRESENT && defined(NRF54L15_XXAA) && __has_include(<nrfx_rramc.h>)
#include <nrfx_rramc.h>
#define MICROHAL_NORDIC_RRAMC_API_AVAILABLE 1
#else
#define MICROHAL_NORDIC_RRAMC_API_AVAILABLE 0
#endif

#if MICROHAL_NORDIC_RRAMC_API_AVAILABLE && defined(NRFX_RRAMC_ENABLED) && NRFX_RRAMC_ENABLED
#define MICROHAL_NORDIC_HAS_RRAMC 1
#else
#define MICROHAL_NORDIC_HAS_RRAMC 0
#endif

#if MICROHAL_NORDIC_NRFX_PRESENT && defined(NRFX_NVMC_ENABLED) && NRFX_NVMC_ENABLED && __has_include(<nrfx_nvmc.h>) && \
    (defined(NRF52840_XXAA) || defined(NRF5340_XXAA_APPLICATION))
#include <nrfx_nvmc.h>
#define MICROHAL_NORDIC_HAS_NVMC 1
#else
#define MICROHAL_NORDIC_HAS_NVMC 0
#endif

#if MICROHAL_NORDIC_NRFX_PRESENT && defined(NRF_MRAMC) && defined(NRFX_MRAMC_ENABLED) && NRFX_MRAMC_ENABLED &&         \
    __has_include(<nrfx_mramc.h>)
#include <nrfx_mramc.h>
#define MICROHAL_NORDIC_HAS_MRAMC 1
#else
#define MICROHAL_NORDIC_HAS_MRAMC 0
#endif

#if MICROHAL_NORDIC_NRFX_PRESENT && defined(NRF54H20_XXAA) && defined(NRF_MRAMC110) && defined(NRF_MRAMC111)
#define MICROHAL_NORDIC_HAS_H20_MRAM 1
#else
#define MICROHAL_NORDIC_HAS_H20_MRAM 0
#endif

#if MICROHAL_NORDIC_NRFX_PRESENT && defined(NRF_MVDMA120) && __has_include(<haly/nrfy_mvdma.h>)
#include <haly/nrfy_mvdma.h>
#define MICROHAL_NORDIC_HAS_MVDMA 1
#else
#define MICROHAL_NORDIC_HAS_MVDMA 0
#endif

#if MICROHAL_NORDIC_NRFX_PRESENT && defined(NRF_MCAN120) && defined(NRF_CAN120)
#define MICROHAL_NORDIC_HAS_MCAN 1
#else
#define MICROHAL_NORDIC_HAS_MCAN 0
#endif

#if MICROHAL_NORDIC_NRFX_PRESENT && defined(NRF_USBD) && defined(NRFX_USBD_ENABLED) && NRFX_USBD_ENABLED &&            \
    __has_include(<nrfx_usbd.h>)
#include <nrfx_usbd.h>
#define MICROHAL_NORDIC_HAS_USBD 1
#else
#define MICROHAL_NORDIC_HAS_USBD 0
#endif

#if MICROHAL_NORDIC_NRFX_PRESENT && defined(NRF_CLOCK) && defined(NRFX_CLOCK_ENABLED) && NRFX_CLOCK_ENABLED &&         \
    __has_include(<nrfx_clock.h>)
#include <nrfx_clock.h>
#define MICROHAL_NORDIC_HAS_CLOCK 1
#else
#define MICROHAL_NORDIC_HAS_CLOCK 0
#endif

#if MICROHAL_NORDIC_NRFX_PRESENT && defined(NRF_RNG) && __has_include(<hal/nrf_rng.h>)
#include <hal/nrf_rng.h>
#define MICROHAL_NORDIC_HAS_RNG_HAL 1
#else
#define MICROHAL_NORDIC_HAS_RNG_HAL 0
#endif

#if MICROHAL_NORDIC_NRFX_PRESENT && (defined(NRF52840_XXAA) || defined(NRF5340_XXAA_APPLICATION)) &&                   \
    defined(MICROHAL_NORDIC_ENABLE_CC3XX_ENTROPY) && MICROHAL_NORDIC_ENABLE_CC3XX_ENTROPY &&                           \
    __has_include(<nrf_cc3xx_platform.h>) && \
    __has_include(<nrf_cc3xx_platform_entropy.h>)
#include <nrf_cc3xx_platform.h>
#include <nrf_cc3xx_platform_entropy.h>
#define MICROHAL_NORDIC_HAS_CC3XX_ENTROPY 1
#else
#define MICROHAL_NORDIC_HAS_CC3XX_ENTROPY 0
#endif

#if MICROHAL_NORDIC_NRFX_PRESENT && (defined(NRF54L15_XXAA) || defined(NRF54H20_XXAA)) &&                              \
    defined(MICROHAL_NORDIC_ENABLE_CRACEN_ENTROPY) && MICROHAL_NORDIC_ENABLE_CRACEN_ENTROPY &&                         \
    __has_include(<sxsymcrypt/trng.h>) && \
    __has_include(<cracen/statuscodes.h>)
#include <cracen/statuscodes.h>
#include <sxsymcrypt/trng.h>
#define MICROHAL_NORDIC_HAS_CRACEN_ENTROPY 1
#else
#define MICROHAL_NORDIC_HAS_CRACEN_ENTROPY 0
#endif

#define MICROHAL_NORDIC_HAS_RNG                                                                                        \
  (MICROHAL_NORDIC_HAS_RNG_HAL || MICROHAL_NORDIC_HAS_CC3XX_ENTROPY || MICROHAL_NORDIC_HAS_CRACEN_ENTROPY)

#if MICROHAL_NORDIC_NRFX_PRESENT && defined(NRF_OSCILLATORS) && defined(NRF54L15_XXAA) &&                              \
    __has_include(<hal/nrf_oscillators.h>)
#include <hal/nrf_oscillators.h>
#define MICROHAL_NORDIC_HAS_OSCILLATORS 1
#else
#define MICROHAL_NORDIC_HAS_OSCILLATORS 0
#endif

#if MICROHAL_NORDIC_NRFX_PRESENT && __has_include(<hal/nrf_gpio.h>)
#include <hal/nrf_gpio.h>
#define MICROHAL_NORDIC_HAS_GPIO 1
#else
#define MICROHAL_NORDIC_HAS_GPIO 0
#endif

#include "microhal/backends/nordic/nrfx/peripherals.hpp"
#include "microhal/core/traits.hpp"

namespace microhal::backends::nordic::nrfx
{

namespace detail
{

inline constexpr std::uint32_t nordic_gpio_port_count =
#if defined(NRF54L15_XXAA)
    3U;
#else
    2U;
#endif

#if MICROHAL_NORDIC_TWIM_API_AVAILABLE
using twim_transfer_state = transfer_state;

#if defined(NRF54L15_XXAA)
inline nrfx_twim_t twim0{NRF_TWIM20, {}};
inline nrfx_twim_t twim1{NRF_TWIM21, {}};
inline nrfx_twim_t twim2{NRF_TWIM22, {}};
inline nrfx_twim_t twim3{NRF_TWIM30, {}};
inline constexpr std::uint32_t twim_instance_count = 4U;
#elif defined(NRF54H20_XXAA)
inline nrfx_twim_t twim0{NRF_TWIM130, {}};
inline nrfx_twim_t twim1{NRF_TWIM131, {}};
inline nrfx_twim_t twim2{NRF_TWIM132, {}};
inline nrfx_twim_t twim3{NRF_TWIM133, {}};
inline nrfx_twim_t twim4{NRF_TWIM134, {}};
inline nrfx_twim_t twim5{NRF_TWIM135, {}};
inline nrfx_twim_t twim6{NRF_TWIM136, {}};
inline nrfx_twim_t twim7{NRF_TWIM137, {}};
inline constexpr std::uint32_t twim_instance_count = 8U;
#else
inline nrfx_twim_t twim0{NRF_TWIM0, {}};
inline nrfx_twim_t twim1{NRF_TWIM1, {}};
#if defined(NRF5340_XXAA_APPLICATION)
inline nrfx_twim_t twim2{NRF_TWIM2, {}};
inline nrfx_twim_t twim3{NRF_TWIM3, {}};
inline constexpr std::uint32_t twim_instance_count = 4U;
#else
inline constexpr std::uint32_t twim_instance_count = 2U;
#endif
#endif

inline std::array<twim_transfer_state, twim_instance_count> twim_states{};
inline nrfx_twim_t* twim_instance(const std::uint8_t index) noexcept
{
#if defined(NRF54L15_XXAA) || defined(NRF5340_XXAA_APPLICATION) || defined(NRF54H20_XXAA)
  switch (index)
  {
  case 0U:
    return &twim0;
  case 1U:
    return &twim1;
  case 2U:
    return &twim2;
  case 3U:
    return &twim3;
#if defined(NRF54H20_XXAA)
  case 4U:
    return &twim4;
  case 5U:
    return &twim5;
  case 6U:
    return &twim6;
  case 7U:
    return &twim7;
#endif
  default:
    return nullptr;
  }
#else
  switch (index)
  {
  case 0U:
    return &twim0;
  case 1U:
    return &twim1;
  default:
    return nullptr;
  }
#endif
}

inline void twim_event_handler(nrfx_twim_event_t const* event, void* context) noexcept
{
  auto* transfer = static_cast<twim_transfer_state*>(context);
  if (transfer->active.load())
    transfer->finish((event->type == NRFX_TWIM_EVT_DONE) ? status::ok : status::io_error);
}

inline nrf_twim_frequency_t twim_frequency(const std::uint32_t clock_hz) noexcept
{
  switch (clock_hz)
  {
  case 100'000U:
    return NRF_TWIM_FREQ_100K;
  case 250'000U:
    return NRF_TWIM_FREQ_250K;
  case 400'000U:
    return NRF_TWIM_FREQ_400K;
#if NRF_TWIM_HAS_1000_KHZ_FREQ
  case 1'000'000U:
    return NRF_TWIM_FREQ_1000K;
#endif
  default:
    return static_cast<nrf_twim_frequency_t>(0U);
  }
}

template <typename Config> constexpr bool twim_pins_available() noexcept
{
  return requires {
    Config::scl_pin;
    Config::sda_pin;
  } || requires {
    Config::binding::scl_pin;
    Config::binding::sda_pin;
  };
}

template <typename Config> constexpr std::uint32_t twim_scl_pin() noexcept
{
  if constexpr (requires { Config::scl_pin; })
  {
    return Config::scl_pin;
  }
  else
  {
    return Config::binding::scl_pin;
  }
}

template <typename Config> constexpr std::uint32_t twim_sda_pin() noexcept
{
  if constexpr (requires { Config::sda_pin; })
  {
    return Config::sda_pin;
  }
  else
  {
    return Config::binding::sda_pin;
  }
}

template <typename Config> status initialize_twim() noexcept
{
  if constexpr (requires { Config::address_mode; })
    if (Config::address_mode != i2c_address_mode::seven_bit)
      return status::unsupported;
  if constexpr (!twim_pins_available<Config>())
  {
    return status::unsupported;
  }
  else
  {
    if (Config::instance_index >= twim_instance_count)
    {
      return status::invalid_argument;
    }

    const nrf_twim_frequency_t frequency = twim_frequency(Config::clock_hz);
    if (frequency == static_cast<nrf_twim_frequency_t>(0U))
    {
      return status::unsupported;
    }

    nrfx_twim_t* instance = twim_instance(Config::instance_index);
    auto& transfer = twim_states[Config::instance_index];
    nrfx_twim_config_t config{};
    config.scl_pin = twim_scl_pin<Config>();
    config.sda_pin = twim_sda_pin<Config>();
    config.interrupt_priority = 7U;
    config.frequency = frequency;

    if (nrfx_twim_init_check(instance))
    {
      if (transfer.active.load())
        return status::busy;
      if (nrfx_twim_reconfigure(instance, &config) != 0)
      {
        return status::busy;
      }
      if (nrfx_twim_callback_set(instance, twim_event_handler, &transfer) != 0)
      {
        return status::busy;
      }
      return status::ok;
    }

    const auto result = serial_initialize(instance->p_twim, 4U, [&]
                                          { return nrfx_twim_init(instance, &config, twim_event_handler, &transfer); });
    if (result == status::ok)
      nrfx_twim_enable(instance);
    return result;
  }
}

inline status twim_result(int result) noexcept;

template <typename Config>
inline status twim_start_and_wait(nrfx_twim_t* instance, twim_transfer_state& transfer,
                                  const nrfx_twim_xfer_desc_t& descriptor) noexcept
{
#if MICROHAL_NORDIC_SYSTEM_CORE_CLOCK
  if (SystemCoreClock == 0U)
  {
    return status::error;
  }
  if (nrfx_twim_is_busy(instance) || !transfer.begin(nullptr))
  {
    return status::busy;
  }
  const int result = nrfx_twim_xfer(instance, &descriptor, 0U);
  if (result != 0)
  {
    transfer.finish(twim_result(result));
    return twim_result(result);
  }

  constexpr std::uint32_t timeout_ms = []() consteval -> std::uint32_t
  {
    if constexpr (requires { Config::timeout_ms; })
    {
      return static_cast<std::uint32_t>(Config::timeout_ms);
    }
    return 1000U;
  }();

  for (std::uint32_t elapsed_ms = 0U; elapsed_ms < timeout_ms; ++elapsed_ms)
  {
    if constexpr (configured_transfer_mode<Config>() == transfer_mode::polled)
      nrfx_twim_irq_handler(instance);
    if (transfer.complete)
    {
      return transfer.result;
    }
    nrfx_coredep_delay_us(1000U);
  }
  if (transfer.complete)
  {
    return transfer.result;
  }

  nrfx_twim_disable(instance);
  if (transfer.complete)
  {
    return transfer.result;
  }
  nrfx_twim_enable(instance);
  transfer.finish(status::timeout);
  return status::timeout;
#else
  (void)instance;
  (void)transfer;
  (void)descriptor;
  return status::unsupported;
#endif
}

inline status twim_result(const int result) noexcept
{
  switch (result)
  {
  case -EINVAL:
    return status::invalid_argument;
  case -EBUSY:
    return status::busy;
  case -ETIMEDOUT:
    return status::timeout;
  case -EFAULT:
  case -EAGAIN:
  case -EIO:
    return status::io_error;
#ifdef ENOTSUP
  case -ENOTSUP:
    return status::unsupported;
#endif
  default:
    break;
  }
  return status::error;
}
#else
inline constexpr std::uint32_t twim_instance_count = 0U;
#endif

#include "microhal/backends/nordic/nrfx/serial_twis_state.inc"

} // namespace detail

struct gpio_api final
{
  template <typename PinConfig> static bool configure() noexcept
  {
#if MICROHAL_NORDIC_HAS_GPIO
    if constexpr ((MICROHAL_NORDIC_HAS_GPIO != 0) && requires {
                    PinConfig::port_index;
                    PinConfig::pin_index;
                    PinConfig::mode;
                    PinConfig::pull;
                  })
    {
      constexpr std::uint32_t pin = static_cast<std::uint32_t>(PinConfig::port_index) * 32U + PinConfig::pin_index;
      if (PinConfig::pin_index >= 32U || !nrf_gpio_pin_present_check(pin))
        return false;
      if constexpr (PinConfig::mode == gpio_mode::output)
      {
        nrf_gpio_cfg_output(pin);
        return true;
      }
      else if constexpr (PinConfig::mode == gpio_mode::input)
      {
        constexpr nrf_gpio_pin_pull_t pull = (PinConfig::pull == gpio_pull::up)     ? NRF_GPIO_PIN_PULLUP
                                             : (PinConfig::pull == gpio_pull::down) ? NRF_GPIO_PIN_PULLDOWN
                                                                                    : NRF_GPIO_PIN_NOPULL;
        nrf_gpio_cfg_input(pin, pull);
        return true;
      }
      return false;
    }
#else
    (void)sizeof(PinConfig);
    return false;
#endif
  }

  template <typename PinConfig> static void write(const bool level) noexcept
  {
#if MICROHAL_NORDIC_HAS_GPIO
    if constexpr ((MICROHAL_NORDIC_HAS_GPIO != 0) && requires {
                    PinConfig::port_index;
                    PinConfig::pin_index;
                    PinConfig::mode;
                  })
    {
      constexpr std::uint32_t pin = static_cast<std::uint32_t>(PinConfig::port_index) * 32U + PinConfig::pin_index;
      if (PinConfig::pin_index >= 32U || !nrf_gpio_pin_present_check(pin))
        return;
      if constexpr (PinConfig::mode == gpio_mode::output)
      {
        level ? nrf_gpio_pin_set(pin) : nrf_gpio_pin_clear(pin);
      }
    }
#else
    (void)sizeof(PinConfig);
    (void)level;
#endif
  }

  template <typename PinConfig> static bool read() noexcept
  {
#if MICROHAL_NORDIC_HAS_GPIO
    if constexpr ((MICROHAL_NORDIC_HAS_GPIO != 0) && requires {
                    PinConfig::port_index;
                    PinConfig::pin_index;
                  })
    {
      constexpr std::uint32_t pin = static_cast<std::uint32_t>(PinConfig::port_index) * 32U + PinConfig::pin_index;
      if (PinConfig::pin_index >= 32U || !nrf_gpio_pin_present_check(pin))
        return false;
      return nrf_gpio_pin_read(pin) != 0U;
    }
    return false;
#else
    (void)sizeof(PinConfig);
    return false;
#endif
  }

  template <typename PinConfig> static status toggle() noexcept
  {
#if MICROHAL_NORDIC_HAS_GPIO
    if constexpr (PinConfig::mode != gpio_mode::output)
      return status::invalid_argument;
    const std::uint32_t pin = PinConfig::port_index * 32U + PinConfig::pin_index;
    if (!nrf_gpio_pin_present_check(pin))
      return status::invalid_argument;
    nrf_gpio_pin_toggle(pin);
    return status::ok;
#else
    return status::unsupported;
#endif
  }

  template <typename PinConfig, auto Handler> static status enable_interrupt() noexcept
  {
#if MICROHAL_NORDIC_HAS_GPIOTE
    if constexpr (PinConfig::mode != gpio_mode::input || PinConfig::interrupt_trigger == gpio_interrupt_trigger::none)
      return status::invalid_argument;
    const std::uint32_t pin = PinConfig::port_index * 32U + PinConfig::pin_index;
    if (!nrf_gpio_pin_present_check(pin))
      return status::invalid_argument;
    auto& instance = detail::gpiote_instance;
    if (!nrfx_gpiote_init_check(&instance))
    {
      const auto result = detail::driver_result(nrfx_gpiote_init(&instance, 7U));
      if (result != status::ok)
        return result;
    }
    nrfx_gpiote_trigger_config_t trigger{};
    trigger.trigger = PinConfig::interrupt_trigger == gpio_interrupt_trigger::rising    ? NRFX_GPIOTE_TRIGGER_LOTOHI
                      : PinConfig::interrupt_trigger == gpio_interrupt_trigger::falling ? NRFX_GPIOTE_TRIGGER_HITOLO
                                                                                        : NRFX_GPIOTE_TRIGGER_TOGGLE;
    nrfx_gpiote_handler_config_t handler{};
    handler.handler = [](nrfx_gpiote_pin_t, nrfx_gpiote_trigger_t, void*) { detail::notify<Handler>(status::ok); };
    const nrf_gpio_pin_pull_t pull = PinConfig::pull == gpio_pull::up     ? NRF_GPIO_PIN_PULLUP
                                     : PinConfig::pull == gpio_pull::down ? NRF_GPIO_PIN_PULLDOWN
                                                                          : NRF_GPIO_PIN_NOPULL;
    nrfx_gpiote_input_pin_config_t config{&pull, &trigger, &handler};
    const auto result = detail::driver_result(nrfx_gpiote_input_configure(&instance, pin, &config));
    if (result == status::ok)
      nrfx_gpiote_trigger_enable(&instance, pin, true);
    return result;
#else
    return status::unsupported;
#endif
  }

  static void handle_interrupt() noexcept
  {
#if MICROHAL_NORDIC_HAS_GPIOTE
    nrfx_gpiote_irq_handler(&detail::gpiote_instance);
#endif
  }

  template <typename PinConfig> static status disable_interrupt() noexcept
  {
#if MICROHAL_NORDIC_HAS_GPIOTE
    const auto pin = PinConfig::port_index * 32U + PinConfig::pin_index;
    if (PinConfig::pin_index >= 32U || !nrf_gpio_pin_present_check(pin))
      return status::invalid_argument;
    auto& instance = detail::gpiote_instance;
    if (!nrfx_gpiote_init_check(&instance))
      return status::unsupported;
    nrfx_gpiote_trigger_disable(&instance, pin);
    return detail::driver_result(nrfx_gpiote_pin_uninit(&instance, pin));
#else
    return status::unsupported;
#endif
  }
};

struct i2c_api final
{
  template <typename Config> static status initialize() noexcept
  {
#if MICROHAL_NORDIC_TWIM_API_AVAILABLE
    if constexpr (MICROHAL_NORDIC_HAS_TWIM != 0)
    {
      return detail::initialize_twim<Config>();
    }
#else
    (void)sizeof(Config);
#endif
    return status::unsupported;
  }

  template <typename Config>
  static status write_blocking(const std::span<const std::uint8_t> payload, const std::uint16_t address) noexcept
  {
#if MICROHAL_NORDIC_TWIM_API_AVAILABLE
    if constexpr (MICROHAL_NORDIC_HAS_TWIM != 0)
    {
      if (payload.empty() || !detail::serial_count_valid(payload.size()) || address > 0x7FU ||
          Config::instance_index >= detail::twim_instance_count)
      {
        return status::invalid_argument;
      }
      if (!nrfx_twim_init_check(detail::twim_instance(Config::instance_index)))
      {
        return status::unsupported;
      }

      const nrfx_twim_xfer_desc_t descriptor = NRFX_TWIM_XFER_DESC_TX(
          static_cast<std::uint8_t>(address), const_cast<std::uint8_t*>(payload.data()), payload.size());
      return detail::twim_start_and_wait<Config>(detail::twim_instance(Config::instance_index),
                                                 detail::twim_states[Config::instance_index], descriptor);
    }
#else
    (void)sizeof(Config);
    (void)payload;
    (void)address;
#endif
    return status::unsupported;
  }

  template <typename Config>
  static status read_blocking(const std::span<std::uint8_t> payload, const std::uint16_t address) noexcept
  {
#if MICROHAL_NORDIC_TWIM_API_AVAILABLE
    if constexpr (MICROHAL_NORDIC_HAS_TWIM != 0)
    {
      if (payload.empty() || !detail::serial_count_valid(payload.size()) || address > 0x7FU ||
          Config::instance_index >= detail::twim_instance_count)
      {
        return status::invalid_argument;
      }
      if (!nrfx_twim_init_check(detail::twim_instance(Config::instance_index)))
      {
        return status::unsupported;
      }

      const nrfx_twim_xfer_desc_t descriptor =
          NRFX_TWIM_XFER_DESC_RX(static_cast<std::uint8_t>(address), payload.data(), payload.size());
      return detail::twim_start_and_wait<Config>(detail::twim_instance(Config::instance_index),
                                                 detail::twim_states[Config::instance_index], descriptor);
    }
#else
    (void)sizeof(Config);
    (void)payload;
    (void)address;
#endif
    return status::unsupported;
  }

  template <typename Config, auto Handler>
  static status start_transfer(const std::span<std::uint8_t> payload, const std::uint16_t address) noexcept
  {
#if MICROHAL_NORDIC_HAS_TWIM
    if (payload.empty() || !detail::serial_count_valid(payload.size()) || address > 0x7FU ||
        Config::instance_index >= detail::twim_instance_count)
      return status::invalid_argument;
    auto* instance = detail::twim_instance(Config::instance_index);
    if (!nrfx_twim_init_check(instance))
      return status::unsupported;
    if (nrfx_twim_is_busy(instance))
      return status::busy;
    auto& transfer = detail::twim_states[Config::instance_index];
    if (!transfer.begin(detail::notify<Handler>))
      return status::busy;
    const nrfx_twim_xfer_desc_t descriptor =
        NRFX_TWIM_XFER_DESC_TX(static_cast<std::uint8_t>(address), payload.data(), payload.size());
    const int error = nrfx_twim_xfer(instance, &descriptor, 0U);
    if (error != 0)
    {
      transfer.completion = nullptr;
      transfer.finish(detail::twim_result(error));
    }
    return error == 0 ? status::ok : detail::twim_result(error);
#else
    (void)payload;
    (void)address;
    return status::unsupported;
#endif
  }

  template <typename Config> static status probe(const std::uint16_t address) noexcept
  {
#if MICROHAL_NORDIC_HAS_TWIM
    if (address > 0x7FU || Config::instance_index >= detail::twim_instance_count)
      return status::invalid_argument;
    auto* instance = detail::twim_instance(Config::instance_index);
    if (!nrfx_twim_init_check(instance))
      return status::unsupported;
    const nrfx_twim_xfer_desc_t descriptor = NRFX_TWIM_XFER_DESC_TX(static_cast<std::uint8_t>(address), nullptr, 0U);
    return detail::twim_start_and_wait<Config>(instance, detail::twim_states[Config::instance_index], descriptor);
#else
    (void)address;
    return status::unsupported;
#endif
  }

  static void handle_interrupt(const std::uint8_t index) noexcept
  {
#if MICROHAL_NORDIC_HAS_TWIM
    if (index < detail::twim_instance_count)
      nrfx_twim_irq_handler(detail::twim_instance(index));
#else
    (void)index;
#endif
  }

  template <typename Config>
  static status write_read_blocking(const std::span<const std::uint8_t> tx_payload,
                                    const std::span<std::uint8_t> rx_payload, const std::uint16_t address) noexcept
  {
#if MICROHAL_NORDIC_TWIM_API_AVAILABLE
    if constexpr (MICROHAL_NORDIC_HAS_TWIM != 0)
    {
      if ((tx_payload.empty()) || (rx_payload.empty()) || !detail::serial_count_valid(tx_payload.size()) ||
          !detail::serial_count_valid(rx_payload.size()) || (address > 0x7FU) ||
          (Config::instance_index >= detail::twim_instance_count))
      {
        return status::invalid_argument;
      }
      nrfx_twim_t* instance = detail::twim_instance(Config::instance_index);
      if (!nrfx_twim_init_check(instance))
      {
        return status::unsupported;
      }

      const nrfx_twim_xfer_desc_t descriptor =
          NRFX_TWIM_XFER_DESC_TXRX(static_cast<std::uint8_t>(address), const_cast<std::uint8_t*>(tx_payload.data()),
                                   tx_payload.size(), rx_payload.data(), rx_payload.size());
      return detail::twim_start_and_wait<Config>(instance, detail::twim_states[Config::instance_index], descriptor);
    }
#else
    (void)sizeof(Config);
    (void)tx_payload;
    (void)rx_payload;
    (void)address;
#endif
    return status::unsupported;
  }

  template <typename Config, auto Handler>
  static status write_read_async(const std::span<const std::uint8_t> tx_payload,
                                 const std::span<std::uint8_t> rx_payload, const std::uint16_t address) noexcept
  {
#if MICROHAL_NORDIC_TWIM_API_AVAILABLE
    if constexpr (MICROHAL_NORDIC_HAS_TWIM != 0)
    {
      if ((tx_payload.empty()) || (rx_payload.empty()) || !detail::serial_count_valid(tx_payload.size()) ||
          !detail::serial_count_valid(rx_payload.size()) || (address > 0x7FU) ||
          (Config::instance_index >= detail::twim_instance_count))
      {
        return status::invalid_argument;
      }
      nrfx_twim_t* instance = detail::twim_instance(Config::instance_index);
      auto& transfer = detail::twim_states[Config::instance_index];
      if (!nrfx_twim_init_check(instance))
      {
        return status::unsupported;
      }
      if (nrfx_twim_is_busy(instance))
      {
        return status::busy;
      }

      if (!transfer.begin(detail::notify<Handler>))
        return status::busy;
      const nrfx_twim_xfer_desc_t descriptor =
          NRFX_TWIM_XFER_DESC_TXRX(static_cast<std::uint8_t>(address), const_cast<std::uint8_t*>(tx_payload.data()),
                                   tx_payload.size(), rx_payload.data(), rx_payload.size());
      const int result = nrfx_twim_xfer(instance, &descriptor, 0U);
      if (result != 0)
      {
        transfer.completion = nullptr;
        transfer.finish(detail::twim_result(result));
        return detail::twim_result(result);
      }
      return status::ok;
    }
#else
    (void)sizeof(Config);
    (void)tx_payload;
    (void)rx_payload;
    (void)address;
#endif
    return status::unsupported;
  }
#include "microhal/backends/nordic/nrfx/serial_twis_api.inc"
};

struct nvmc_api final
{
  template <typename Config> static status erase(const std::uint32_t address, const std::uint32_t bytes) noexcept
  {
    (void)sizeof(Config);
#if MICROHAL_NORDIC_HAS_NVMC
    const std::uint32_t page_size = nrfx_nvmc_flash_page_size_get();
    const std::uint32_t flash_size = nrfx_nvmc_flash_size_get();
    if ((page_size == 0U) || (bytes == 0U) || ((address % page_size) != 0U) || ((bytes % page_size) != 0U) ||
        (address >= flash_size) || (static_cast<std::uint64_t>(address) + bytes > flash_size) ||
        !nrfx_nvmc_fits_memory_check(address, false, bytes))
    {
      return status::invalid_argument;
    }
    for (std::uint32_t offset = 0U; offset < bytes; offset += page_size)
    {
      if (nrfx_nvmc_page_erase(address + offset) != 0)
      {
        return status::io_error;
      }
    }
    return status::ok;
#else
    (void)address;
    (void)bytes;
    return status::unsupported;
#endif
  }

  template <typename Config>
  static status program(const std::uint32_t address, const std::span<const std::uint8_t> data) noexcept
  {
    (void)sizeof(Config);
#if MICROHAL_NORDIC_HAS_NVMC
    const std::uint32_t flash_size = nrfx_nvmc_flash_size_get();
    if (data.empty() || (data.size() > UINT32_MAX) || ((address & 0x3U) != 0U) || ((data.size() & 0x3U) != 0U) ||
        (address >= flash_size) || (static_cast<std::uint64_t>(address) + data.size() > flash_size) ||
        !nrfx_nvmc_fits_memory_check(address, false, static_cast<std::uint32_t>(data.size())))
    {
      return status::invalid_argument;
    }
    nrfx_nvmc_bytes_write(address, data.data(), static_cast<std::uint32_t>(data.size()));
    return nrfx_nvmc_write_done_check() ? status::ok : status::io_error;
#else
    (void)address;
    (void)data;
    return status::unsupported;
#endif
  }

  template <typename Config>
  static status read(const std::uint32_t address, const std::span<std::uint8_t> data) noexcept
  {
    (void)sizeof(Config);
#if MICROHAL_NORDIC_HAS_NVMC
    const std::uint32_t flash_size = nrfx_nvmc_flash_size_get();
    if (data.empty() || (data.size() > UINT32_MAX) || (address >= flash_size) ||
        (static_cast<std::uint64_t>(address) + data.size() > flash_size) ||
        !nrfx_nvmc_fits_memory_check(address, false, static_cast<std::uint32_t>(data.size())))
    {
      return status::invalid_argument;
    }
    std::memcpy(data.data(), reinterpret_cast<const void*>(static_cast<std::uintptr_t>(address)), data.size());
    return status::ok;
#else
    (void)address;
    (void)data;
    return status::unsupported;
#endif
  }

  template <typename Config> static result<flash_erase_region> erase_region(const std::uint32_t address) noexcept
  {
    (void)sizeof(Config);
#if MICROHAL_NORDIC_HAS_NVMC
    const std::uint32_t page_size = nrfx_nvmc_flash_page_size_get();
    const std::uint32_t flash_size = nrfx_nvmc_flash_size_get();
    if ((page_size == 0U) || (address >= flash_size) || !nrfx_nvmc_fits_memory_check(address, false, 1U))
    {
      return status::invalid_argument;
    }
    return flash_erase_region{.start_address = address - (address % page_size), .size_bytes = page_size};
#else
    (void)address;
    return status::unsupported;
#endif
  }
};

struct rram_api final
{
  static std::uint32_t size_bytes() noexcept
  {
#if MICROHAL_NORDIC_HAS_RRAMC
    return nrfx_rramc_memory_size_get();
#else
    return 0U;
#endif
  }

  static status initialize() noexcept
  {
#if MICROHAL_NORDIC_HAS_RRAMC
    static bool initialized = false;
    if (initialized)
    {
      return status::ok;
    }

    nrfx_rramc_config_t config = NRFX_RRAMC_DEFAULT_CONFIG(0U);
    const int result = nrfx_rramc_init(&config, nullptr);
    if (result == -EALREADY)
    {
      initialized = true;
      return status::ok;
    }
    if (result != 0)
    {
      return status::busy;
    }
    initialized = true;
    return status::ok;
#else
    return status::unsupported;
#endif
  }

  static status erase(std::uint32_t address, std::uint32_t bytes) noexcept
  {
#if MICROHAL_NORDIC_HAS_RRAMC
    if (bytes == 0U || (address & 3U) != 0U || (bytes & 3U) != 0U)
      return status::invalid_argument;
    const auto base = NRFY_RRAMC_RRAM_BASE_ADDRESS;
    if (address < base || static_cast<std::uint64_t>(address) + bytes > static_cast<std::uint64_t>(base) + size_bytes())
      return status::invalid_argument;
    if (!range_accessible(address, bytes, true))
      return status::unsupported;
    // RRAM is overwriteable; an erase facade writes the conventional erased value.
    constexpr std::array<std::uint8_t, 64U> erased = []
    {
      std::array<std::uint8_t, 64U> value{};
      value.fill(0xFFU);
      return value;
    }();
    for (std::uint32_t offset = 0U; offset < bytes;)
    {
      const auto count = (bytes - offset < erased.size()) ? bytes - offset : erased.size();
      const auto result = program(address + offset, std::span{erased}.first(count));
      if (result != status::ok)
        return result;
      offset += static_cast<std::uint32_t>(count);
    }
    return status::ok;
#else
    (void)address;
    (void)bytes;
    return status::unsupported;
#endif
  }

  static bool range_accessible(const std::uint32_t address, const std::uint32_t bytes, const bool write) noexcept
  {
#if MICROHAL_NORDIC_HAS_RRAMC
    const std::uint64_t end = static_cast<std::uint64_t>(address) + bytes;
    std::uint64_t cursor = address;
    while (cursor < end)
    {
      bool matched = false;
      std::uint64_t region_end = 0U;
      for (std::uint8_t index = 0U; index < 5U; ++index)
      {
        nrf_rramc_region_config_t region{};
        nrf_rramc_region_config_get(NRF_RRAMC, index, &region);
        const std::uint64_t start = region.address;
        const std::uint64_t limit = start + (static_cast<std::uint64_t>(region.size_kb) * 1024U);
        if ((region.size_kb == 0U) || (cursor < start) || (cursor >= limit))
        {
          continue;
        }
        if (matched)
        {
          return false;
        }

        const std::uint32_t required_permission =
            write ? NRF_RRAMC_REGION_PERM_WRITE_MASK : NRF_RRAMC_REGION_PERM_READ_MASK;
        if (((region.permissions & required_permission) == 0U) || (write && (region.lock || region.writeonce)))
        {
          return false;
        }
#if defined(NRF_TRUSTZONE_NONSECURE) && NRF_RRAMC_HAS_SECURE_CONFIG
        if ((region.permissions & NRF_RRAMC_REGION_PERM_SECURE_MASK) != 0U)
        {
          return false;
        }
#endif
        matched = true;
        region_end = limit;
      }

      if (!matched)
      {
        return false;
      }
      cursor = (region_end < end) ? region_end : end;
    }
    return true;
#else
    (void)address;
    (void)bytes;
    (void)write;
    return false;
#endif
  }

  static status program(const std::uint32_t address, const std::span<const std::uint8_t> data) noexcept
  {
#if MICROHAL_NORDIC_HAS_RRAMC
    if (data.empty() || (data.size() > UINT32_MAX))
    {
      return status::invalid_argument;
    }
    const status initialized = initialize();
    if (initialized != status::ok)
    {
      return initialized;
    }
    const std::uint32_t size = nrfx_rramc_memory_size_get();
    const std::uint32_t base = NRFY_RRAMC_RRAM_BASE_ADDRESS;
    if ((address < base) ||
        (static_cast<std::uint64_t>(address) + data.size() > static_cast<std::uint64_t>(base) + size))
    {
      return status::invalid_argument;
    }
    if (!range_accessible(address, static_cast<std::uint32_t>(data.size()), true))
    {
      return status::unsupported;
    }

    nrf_rramc_config_t saved_config{};
    nrf_rramc_config_get(NRF_RRAMC, &saved_config);
    if (!saved_config.mode_write)
    {
      nrfx_rramc_write_enable_set(true, saved_config.write_buff_size);
    }
    nrfx_rramc_bytes_write(address, data.data(), static_cast<std::uint32_t>(data.size()));
    if (saved_config.write_buff_size != 0U)
    {
      nrfx_rramc_write_buffer_commit();
    }
    while (!nrfx_rramc_ready_check())
    {
    }
    if (!saved_config.mode_write)
    {
      nrfx_rramc_write_enable_set(false, saved_config.write_buff_size);
    }

    const auto* readback = reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(address));
    return (std::memcmp(readback, data.data(), data.size()) == 0) ? status::ok : status::io_error;
#else
    (void)address;
    (void)data;
    return status::unsupported;
#endif
  }

  static status read(const std::uint32_t address, const std::span<std::uint8_t> data) noexcept
  {
#if MICROHAL_NORDIC_HAS_RRAMC
    if (data.empty() || (data.size() > UINT32_MAX))
    {
      return status::invalid_argument;
    }
    const std::uint32_t size = nrfx_rramc_memory_size_get();
    const std::uint32_t base = NRFY_RRAMC_RRAM_BASE_ADDRESS;
    if ((address < base) ||
        (static_cast<std::uint64_t>(address) + data.size() > static_cast<std::uint64_t>(base) + size))
    {
      return status::invalid_argument;
    }
    if (!range_accessible(address, static_cast<std::uint32_t>(data.size()), false))
    {
      return status::unsupported;
    }
    std::memcpy(data.data(), reinterpret_cast<const void*>(static_cast<std::uintptr_t>(address)), data.size());
    return status::ok;
#else
    (void)address;
    (void)data;
    return status::unsupported;
#endif
  }
};

#include "microhal/backends/nordic/nrfx/storage_api.inc"

struct dac_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return false;
  }

  template <typename Config> static bool write_blocking(const std::uint16_t value) noexcept
  {
    (void)sizeof(Config);
    (void)value;
    return false;
  }

  template <typename Config> static bool write_dual(const std::uint16_t a, const std::uint16_t b) noexcept
  {
    (void)sizeof(Config);
    (void)a;
    (void)b;
    return false;
  }

  template <typename Config, auto Handler>
  static bool start_dma_waveform(const std::span<const std::uint16_t> waveform) noexcept
  {
    (void)sizeof(Config);
    (void)Handler;
    (void)waveform;
    return false;
  }
};

#include "microhal/backends/nordic/nrfx/can_engine.inc"

#include "microhal/backends/nordic/nrfx/usb_engine.inc"

struct ethernet_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return false;
  }

  template <typename Config> static bool start() noexcept
  {
    (void)sizeof(Config);
    return false;
  }

  template <typename Config> static bool stop() noexcept
  {
    (void)sizeof(Config);
    return false;
  }

  template <typename Config> static bool transmit_frame(const std::span<const std::uint8_t> frame) noexcept
  {
    (void)sizeof(Config);
    (void)frame;
    return false;
  }

  template <typename Config> static std::uint32_t receive_frame(const std::span<std::uint8_t> buffer) noexcept
  {
    (void)sizeof(Config);
    (void)buffer;
    return 0U;
  }

  template <typename Config> static bool set_mac_address(const std::array<std::uint8_t, 6>& address) noexcept
  {
    (void)sizeof(Config);
    (void)address;
    return false;
  }
};

struct sd_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return false;
  }

  template <typename Config>
  static bool read_blocks(const std::uint32_t start_block, const std::span<std::uint8_t> buffer) noexcept
  {
    (void)sizeof(Config);
    (void)start_block;
    (void)buffer;
    return false;
  }

  template <typename Config>
  static bool write_blocks(const std::uint32_t start_block, const std::span<const std::uint8_t> buffer) noexcept
  {
    (void)sizeof(Config);
    (void)start_block;
    (void)buffer;
    return false;
  }

  template <typename Config>
  static bool erase_blocks(const std::uint32_t start_block, const std::uint32_t block_count) noexcept
  {
    (void)sizeof(Config);
    (void)start_block;
    (void)block_count;
    return false;
  }

  template <typename Config> static std::uint32_t capacity_blocks() noexcept
  {
    (void)sizeof(Config);
    return 0U;
  }
};

#include "microhal/backends/nordic/nrfx/dma_engine.inc"

#include "microhal/backends/nordic/nrfx/clock_engine.inc"
#include "microhal/backends/nordic/nrfx/rng_engine.inc"

struct crc_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    if constexpr (requires {
                    Config::enable_rng;
                    Config::instance_index;
                  })
      if constexpr (Config::enable_rng && Config::instance_index == 0U)
        return system_detail::entropy_driver::initialize() == status::ok;
    return false;
  }

  template <typename Config> static std::uint32_t calculate(const std::span<const std::uint8_t> data) noexcept
  {
    (void)sizeof(Config);
    (void)data;
    return 0U;
  }

  template <typename Config> static void reset() noexcept
  {
    (void)sizeof(Config);
  }

  template <typename Config> static std::uint32_t get_random() noexcept
  {
    (void)sizeof(Config);
    return read_random<Config>().value_or(0U);
  }

  template <typename Config> static result<std::uint32_t> read_random() noexcept
  {
    if constexpr (requires {
                    Config::enable_rng;
                    Config::instance_index;
                  })
      if constexpr (Config::enable_rng && Config::instance_index == 0U)
        return system_detail::entropy_driver::read();
    return status::unsupported;
  }
};

} // namespace microhal::backends::nordic::nrfx

namespace microhal
{

template <> struct backend_traits<nordic_nrf_tag> final
{
  using backend_tag = nordic_nrf_tag;
  using gpio_api = backends::nordic::nrfx::gpio_api;
  using i2c_api = backends::nordic::nrfx::i2c_api;
  using spi_api = backends::nordic::nrfx::spi_api;
  using qspi_api = backends::nordic::nrfx::qspi_api;
  using uart_api = backends::nordic::nrfx::uart_api;
  using rtc_api = backends::nordic::nrfx::rtc_api;
  using flash_api = backends::nordic::nrfx::flash_api;
  using rram_api = backends::nordic::nrfx::rram_api;
  using timer_api = backends::nordic::nrfx::timer_api;
  using adc_api = backends::nordic::nrfx::adc_api;
  using dac_api = backends::nordic::nrfx::dac_api;
  using can_api = backends::nordic::nrfx::can_api;
  using usb_api = backends::nordic::nrfx::usb_api;
  using ethernet_api = backends::nordic::nrfx::ethernet_api;
  using sd_api = backends::nordic::nrfx::sd_api;
  using dma_api = backends::nordic::nrfx::dma_api;
  using pwm_api = backends::nordic::nrfx::pwm_api;
  using watchdog_api = backends::nordic::nrfx::watchdog_api;
  using i2s_api = backends::nordic::nrfx::i2s_api;
  using rcc_api = backends::nordic::nrfx::rcc_api;
  using crc_api = backends::nordic::nrfx::crc_api;

  static constexpr vendor_kind vendor = vendor_kind::nordic_nrf;
  static constexpr backend_kind kind = backend_kind::nordic_nrfx;
  static constexpr std::string_view name{"nordic-nrfx"};
  static constexpr std::uint32_t gpio_port_count{backends::nordic::nrfx::detail::nordic_gpio_port_count};
  static constexpr std::uint32_t gpio_pins_per_port{32U};
  static constexpr std::uint32_t gpio_alternate_function_count{16U};

  static consteval struct gpio_capability gpio_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_GPIO != 0;
    return {.supported = supported,
            .blocking = supported,
            .non_blocking = MICROHAL_NORDIC_HAS_GPIOTE != 0,
            .isr = MICROHAL_NORDIC_HAS_GPIOTE != 0,
            .dma = false,
            .instance_count = supported ? gpio_port_count : 0U,
            .max_frequency_hz = 0U,
            .alternate_function = false,
            .alternate_function_count = 0U,
            .port_count = supported ? gpio_port_count : 0U,
            .pins_per_port = supported ? gpio_pins_per_port : 0U,
            .output_toggle = supported};
  }

  static consteval struct i2c_capability i2c_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_TWIM != 0;
    return {.supported = supported,
            .blocking = supported && (MICROHAL_NORDIC_SYSTEM_CORE_CLOCK != 0),
            .non_blocking = supported,
            .isr = supported,
            .dma = supported,
            .instance_count = supported ? backends::nordic::nrfx::detail::twim_instance_count : 0U,
            .max_frequency_hz =
#if defined(NRF52840_XXAA)
                supported ? 400'000U : 0U,
#else
                supported ? 1'000'000U : 0U,
#endif
            .ten_bit_addressing = false,
            .repeated_start = supported,
            .address_probe = supported && MICROHAL_NORDIC_SYSTEM_CORE_CLOCK,
            .transfer_modes = {.polled = supported && MICROHAL_NORDIC_SYSTEM_CORE_CLOCK, .dma = supported}};
  }

  static consteval struct spi_capability spi_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_SPIM != 0;
    return {.supported = supported,
            .blocking = supported && MICROHAL_NORDIC_SYSTEM_CORE_CLOCK,
            .non_blocking = supported,
            .isr = supported,
            .dma = supported,
#if MICROHAL_NORDIC_HAS_SPIM
            .instance_count = std::size(backends::nordic::nrfx::detail::spim_instances),
#endif
            .max_frequency_hz = supported ? 32'000'000U : 0U,
            .transmit_only = supported,
            .receive_only = supported,
            .async_transfer = supported,
            .transfer_modes = {.polled = supported && MICROHAL_NORDIC_SYSTEM_CORE_CLOCK, .dma = supported}};
  }

  static consteval struct qspi_capability qspi_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_QSPI != 0;
    constexpr std::uint32_t maximum =
#if defined(NRF5340_XXAA_APPLICATION)
        96'000'000U;
#else
        32'000'000U;
#endif
    return {.supported = supported,
            .blocking = supported,
            .instance_count = supported ? 1U : 0U,
            .max_frequency_hz = supported ? maximum : 0U,
            .quad_mode = supported,
            .dual_mode = supported,
            .command_transfer = supported,
            .memory_mapped = supported,
            .max_address_bytes = supported ? std::uint8_t{4U} : std::uint8_t{0U}};
  }

  static consteval struct uart_capability uart_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_UARTE != 0;
    return {.supported = supported,
            .blocking = supported && MICROHAL_NORDIC_SYSTEM_CORE_CLOCK,
            .non_blocking = supported,
            .isr = supported,
            .dma = supported,
#if MICROHAL_NORDIC_HAS_UARTE
            .instance_count = std::size(backends::nordic::nrfx::detail::uarte_instances),
#endif
            .parity = supported,
            .two_stop_bits = supported,
            .hardware_flow_control = supported,
            .max_baud_rate = supported ?
#if defined(NRF54H20_XXAA)
                                       20'000'000U
#elif defined(NRF54L15_XXAA)
                                       8'000'000U
#else
                                       1'000'000U
#endif
                                       : 0U,
            .transfer_modes = {.polled = supported && MICROHAL_NORDIC_SYSTEM_CORE_CLOCK, .dma = supported}};
  }

  static consteval struct rtc_capability rtc_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_RTC || MICROHAL_NORDIC_HAS_GRTC;
    return {.supported = supported,
            .blocking = supported,
            .non_blocking = supported,
            .isr = supported,
#if MICROHAL_NORDIC_HAS_RTC
            .instance_count = std::size(backends::nordic::nrfx::detail::rtc_instances),
#elif MICROHAL_NORDIC_HAS_GRTC
            .instance_count = 1U,
#endif
            .max_frequency_hz = supported ? (MICROHAL_NORDIC_HAS_GRTC ? 1'000'000U : 32768U) : 0U,
            .periodic = supported,
            .alarm = supported,
            .calendar = supported,
            .subsecond = supported};
  }

  static consteval struct flash_capability flash_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_NVMC || MICROHAL_NORDIC_HAS_RRAMC || MICROHAL_NORDIC_HAS_MRAMC ||
                               MICROHAL_NORDIC_HAS_H20_MRAM;
    constexpr std::uint32_t flash_kb =
#if defined(NRF52840_XXAA) || defined(NRF5340_XXAA_APPLICATION)
        1024U;
#elif defined(NRF54L15_XXAA)
        1524U;
#elif defined(NRF54H20_XXAA)
        2048U;
#else
        0U;
#endif
    return {.supported = supported,
            .blocking = supported,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = supported ? 1U : 0U,
            .max_frequency_hz = 0U,
            .alignment_bytes =
                supported ? (MICROHAL_NORDIC_HAS_H20_MRAM ? 16U : (MICROHAL_NORDIC_HAS_RRAMC ? 1U : 4U)) : 0U,
            .erase_bytes =
                supported ? (MICROHAL_NORDIC_HAS_H20_MRAM ? 16U : (MICROHAL_NORDIC_HAS_RRAMC ? 4U : 4096U)) : 0U,
            .total_size_kb = supported ? flash_kb : 0U,
            .erase_region_query = supported,
            .program_readback = supported};
  }

  static consteval struct timer_capability timer_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_TIMER != 0;
    constexpr std::uint32_t maximum_frequency =
#if defined(NRF54H20_XXAA)
        320'000'000U;
#elif defined(NRF54L15_XXAA)
        128'000'000U;
#else
        16'000'000U;
#endif
    return {.supported = supported,
            .blocking = supported,
            .non_blocking = supported,
            .isr = supported,
#if MICROHAL_NORDIC_HAS_TIMER
            .instance_count = std::size(backends::nordic::nrfx::detail::timer_instances),
#endif
            .max_frequency_hz = supported ? maximum_frequency : 0U,
            .capture_compare = supported,
            .periodic = supported,
            .channel_count = supported ?
#if defined(NRF54L15_XXAA)
                                       6U
#else
                                       4U
#endif
                                       : 0U,
            .period_readback = supported,
            .counter_reset = supported};
  }

  static consteval struct adc_capability adc_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_SAADC != 0;
    return {.supported = supported,
            .blocking = supported && MICROHAL_NORDIC_SYSTEM_CORE_CLOCK,
            .non_blocking = supported,
            .isr = supported,
            .dma = supported,
            .instance_count = supported ? 1U : 0U,
            .resolution_bits = supported ? 14U : 0U,
            .channel_count = supported ? 8U : 0U,
            .differential = supported,
            .sequential_scan = supported,
            .voltage_conversion = supported};
  }

  static consteval struct dac_capability dac_capability() noexcept
  {
    return {};
  }

  static consteval struct can_capability can_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_MCAN;
    return {.supported = supported,
            .blocking = supported,
            .non_blocking = supported,
            .isr = supported,
            .instance_count = supported ? 1U : 0U,
            .mailbox_count = supported ? 3U : 0U,
            .filter_banks = supported ? 32U : 0U,
            .extended_ids = supported,
            .remote_frames = supported,
            .rx_interrupt = supported};
  }

  static consteval struct usb_capability usb_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_USBD;
    return {.supported = supported,
            .blocking = supported,
            .non_blocking = supported,
            .isr = supported,
            .dma = supported,
            .instance_count = supported ? 1U : 0U,
            .device = supported,
            .endpoint_count = supported ? 8U : 0U,
            .endpoint_transfer = supported,
            .endpoint_stall = supported,
            .max_packet_size_bytes = supported ? std::uint16_t{64U} : std::uint16_t{0U}};
  }

  static consteval struct ethernet_capability ethernet_capability() noexcept
  {
    return {};
  }

  static consteval struct sd_capability sd_capability() noexcept
  {
    return {};
  }

  static consteval struct dma_capability dma_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_MVDMA;
    return {.supported = supported,
            .non_blocking = supported,
            .isr = supported,
            .dma = supported,
            .instance_count = supported ? 1U : 0U,
            .stream_count = supported ? 1U : 0U,
            .channel_count = supported ? 1U : 0U,
            .mem_to_mem = supported,
            .transfer_status = supported,
            .abort = supported,
            .completion_callback = supported,
            .address_width_bits = supported ? std::uint8_t{32U} : std::uint8_t{0U}};
  }

  static consteval struct pwm_capability pwm_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_PWM != 0;
    return {.supported = supported,
            .blocking = supported,
#if MICROHAL_NORDIC_HAS_PWM
            .instance_count = std::size(backends::nordic::nrfx::detail::pwm_instances),
#endif
            .max_frequency_hz = supported ? 16'000'000U : 0U,
            .channel_count = supported ? 4U : 0U,
            .runtime_duty_update = supported,
            .runtime_start_stop = supported && MICROHAL_NORDIC_SYSTEM_CORE_CLOCK};
  }

  static consteval struct watchdog_capability watchdog_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_WDT != 0;
    return {.supported = supported,
            .blocking = supported,
#if MICROHAL_NORDIC_HAS_WDT
            .instance_count = std::size(backends::nordic::nrfx::detail::wdt_instances),
#endif
            .independent = supported,
            .min_timeout_ms = supported ? 1U : 0U,
            .max_timeout_ms = supported ? 131'071'999U : 0U,
            .runtime_refresh = supported,
            .reset_on_timeout = supported};
  }

  static consteval struct i2s_capability i2s_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_I2S != 0;
    return {.supported = supported,
            .blocking = supported && MICROHAL_NORDIC_SYSTEM_CORE_CLOCK,
            .non_blocking = supported,
            .isr = supported,
            .dma = supported,
            .instance_count = supported ? 1U : 0U,
            .full_duplex = supported,
            .master_mode = supported,
            .slave_mode = supported,
            .master_clock_output = supported,
            .max_sample_rate = supported ? 48000U : 0U,
            .transmit_dma = supported,
            .receive_dma = supported,
            .sample_bits = supported ? std::uint8_t{16U} : std::uint8_t{0U},
            .transfer_modes = {.polled = supported && MICROHAL_NORDIC_SYSTEM_CORE_CLOCK, .dma = supported}};
  }

  static consteval struct rcc_capability rcc_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_SYSTEM_CORE_CLOCK != 0;
    constexpr std::uint32_t maximum =
#if defined(NRF52840_XXAA) || defined(NRF5340_XXAA_NETWORK)
        64'000'000U;
#elif defined(NRF54H20_XXAA)
        320'000'000U;
#else
        128'000'000U;
#endif
    return {.supported = supported,
            .blocking = supported,
            .instance_count = supported ? 1U : 0U,
            .hse = supported && (MICROHAL_NORDIC_HAS_NVMC != 0),
            .max_system_clock_hz = supported ? maximum : 0U,
            .clock_configuration = supported && MICROHAL_NORDIC_HAS_CLOCK,
            .peripheral_clock_gating = supported && MICROHAL_NORDIC_HAS_CLOCK,
            .system_clock_readback = supported};
  }

  static consteval struct crc_capability crc_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_RNG;
    return {.supported = supported,
            .blocking = supported,
            .instance_count = supported ? 1U : 0U,
            .random_number_generator = supported};
  }

  template <typename PinConfig> static consteval bool is_valid_gpio_pin() noexcept
  {
#if defined(NRF54H20_XXAA)
    return ((PinConfig::port_index == 0U || PinConfig::port_index == 1U || PinConfig::port_index == 2U) &&
            PinConfig::pin_index < 12U) ||
           (PinConfig::port_index == 6U && PinConfig::pin_index < 14U) ||
           (PinConfig::port_index == 7U && PinConfig::pin_index < 8U) ||
           (PinConfig::port_index == 9U && PinConfig::pin_index < 6U);
#elif defined(NRF54L15_XXAA)
    return (PinConfig::port_index == 0U && PinConfig::pin_index <= 6U) ||
           (PinConfig::port_index == 1U && PinConfig::pin_index <= 16U) ||
           (PinConfig::port_index == 2U && PinConfig::pin_index <= 10U);
#elif defined(NRF52840_XXAA) || defined(NRF5340_XXAA_APPLICATION)
    return (PinConfig::port_index == 0U && PinConfig::pin_index < 32U) ||
           (PinConfig::port_index == 1U && PinConfig::pin_index < 16U);
#else
    return (PinConfig::port_index < gpio_port_count) && (PinConfig::pin_index < gpio_pins_per_port);
#endif
  }

  template <typename PinConfig> static consteval bool is_valid_alternate_function() noexcept
  {
    return (PinConfig::alternate_function == gpio_no_alternate) ||
           (PinConfig::alternate_function < gpio_alternate_function_count);
  }
};

} // namespace microhal

#include "microhal/backends/nordic/nrfx/acquisition_modes.hpp"
#include "microhal/backends/nordic/nrfx/serial_modes.hpp"
