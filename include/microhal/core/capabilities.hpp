// SPDX-License-Identifier: Apache-2.0
/**
 * @file capabilities.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <array>

#include "microhal/core/backend_config.hpp"
#include "microhal/core/traits.hpp"

#if MICROHAL_ENABLE_STM32_HAL
#include "microhal/backends/stm32/hal/traits.hpp"
#endif
#if MICROHAL_ENABLE_STM32_LL
#include "microhal/backends/stm32/ll/traits.hpp"
#endif
#if MICROHAL_ENABLE_KINETIS_ESPRESSO
#include "microhal/backends/kinetis/espresso/traits.hpp"
#endif
#if MICROHAL_ENABLE_ZEPHYR
#include "microhal/backends/zephyr/traits.hpp"
#endif
#if MICROHAL_ENABLE_NUTTX
#include "microhal/backends/nuttx/traits.hpp"
#endif
#if MICROHAL_ENABLE_NORDIC_NRFX
#include "microhal/backends/nordic/nrfx/traits.hpp"
#endif
#if MICROHAL_ENABLE_MOCK
#include "microhal/backends/mock/traits.hpp"
#endif

namespace microhal
{

template <typename Backend> consteval backend_descriptor make_backend_descriptor() noexcept
{
  return backend_descriptor{.name = backend_traits<Backend>::name,
                            .vendor = backend_traits<Backend>::vendor,
                            .kind = backend_traits<Backend>::kind};
}

/// @brief Descriptors for every backend compiled into this translation unit.
inline constexpr auto backend_registry = std::array{
#if MICROHAL_ENABLE_STM32_HAL
    make_backend_descriptor<stm32_hal_tag>(),
#endif
#if MICROHAL_ENABLE_STM32_LL
    make_backend_descriptor<stm32_ll_tag>(),
#endif
#if MICROHAL_ENABLE_KINETIS_ESPRESSO
    make_backend_descriptor<kinetis_espresso_tag>(),
#endif
#if MICROHAL_ENABLE_ZEPHYR
    make_backend_descriptor<zephyr_tag>(),
#endif
#if MICROHAL_ENABLE_NUTTX
    make_backend_descriptor<nuttx_tag>(),
#endif
#if MICROHAL_ENABLE_NORDIC_NRFX
  make_backend_descriptor<nordic_nrf_tag>(),
#endif
#if MICROHAL_ENABLE_MOCK
    make_backend_descriptor<mock_tag>(),
#endif
};

/**
 * @brief Projects any peripheral-specific capability struct onto the fields common to all of them.
 */
template <typename Capability> [[nodiscard]] consteval base_capability to_base_capability(const Capability cap) noexcept
{
  return base_capability{.supported = cap.supported,
                         .blocking = cap.blocking,
                         .non_blocking = cap.non_blocking,
                         .isr = cap.isr,
                         .dma = cap.dma,
                         .instance_count = cap.instance_count,
                         .max_frequency_hz = cap.max_frequency_hz};
}

/**
 * @brief Maps a @ref peripheral_kind to the matching backend capability accessor.
 * @details This is the single place where the enum-to-accessor mapping lives. Peripheral-specific
 *          fields remain available through the typed `backend_traits<B>::x_capability()` accessors;
 *          this helper exposes only the shared subset so generic code can stay peripheral-agnostic.
 */
template <typename Backend, peripheral_kind Peripheral>
[[nodiscard]] consteval base_capability capability_for() noexcept
{
  if constexpr (Peripheral == peripheral_kind::gpio)
  {
    return to_base_capability(backend_traits<Backend>::gpio_capability());
  }
  else if constexpr (Peripheral == peripheral_kind::i2c)
  {
    return to_base_capability(backend_traits<Backend>::i2c_capability());
  }
  else if constexpr (Peripheral == peripheral_kind::spi)
  {
    return to_base_capability(backend_traits<Backend>::spi_capability());
  }
  else if constexpr (Peripheral == peripheral_kind::qspi)
  {
    return to_base_capability(backend_traits<Backend>::qspi_capability());
  }
  else if constexpr (Peripheral == peripheral_kind::uart)
  {
    return to_base_capability(backend_traits<Backend>::uart_capability());
  }
  else if constexpr (Peripheral == peripheral_kind::rtc)
  {
    return to_base_capability(backend_traits<Backend>::rtc_capability());
  }
  else if constexpr (Peripheral == peripheral_kind::flash)
  {
    return to_base_capability(backend_traits<Backend>::flash_capability());
  }
  else if constexpr (Peripheral == peripheral_kind::timer)
  {
    return to_base_capability(backend_traits<Backend>::timer_capability());
  }
  else if constexpr (Peripheral == peripheral_kind::adc)
  {
    return to_base_capability(backend_traits<Backend>::adc_capability());
  }
  else if constexpr (Peripheral == peripheral_kind::dac)
  {
    return to_base_capability(backend_traits<Backend>::dac_capability());
  }
  else if constexpr (Peripheral == peripheral_kind::can)
  {
    return to_base_capability(backend_traits<Backend>::can_capability());
  }
  else if constexpr (Peripheral == peripheral_kind::usb)
  {
    return to_base_capability(backend_traits<Backend>::usb_capability());
  }
  else if constexpr (Peripheral == peripheral_kind::ethernet)
  {
    return to_base_capability(backend_traits<Backend>::ethernet_capability());
  }
  else if constexpr (Peripheral == peripheral_kind::sd)
  {
    return to_base_capability(backend_traits<Backend>::sd_capability());
  }
  else if constexpr (Peripheral == peripheral_kind::dma)
  {
    return to_base_capability(backend_traits<Backend>::dma_capability());
  }
  else if constexpr (Peripheral == peripheral_kind::pwm)
  {
    return to_base_capability(backend_traits<Backend>::pwm_capability());
  }
  else if constexpr (Peripheral == peripheral_kind::watchdog)
  {
    return to_base_capability(backend_traits<Backend>::watchdog_capability());
  }
  else if constexpr (Peripheral == peripheral_kind::i2s)
  {
    return to_base_capability(backend_traits<Backend>::i2s_capability());
  }
  else if constexpr (Peripheral == peripheral_kind::rcc)
  {
    return to_base_capability(backend_traits<Backend>::rcc_capability());
  }
  else if constexpr (Peripheral == peripheral_kind::crc)
  {
    return to_base_capability(backend_traits<Backend>::crc_capability());
  }
  else
  {
    static_assert(dependent_false_v<Backend>, "microHAL: unhandled peripheral_kind in capability_for().");
  }
}

template <typename Backend, peripheral_kind Peripheral> consteval bool peripheral_supported() noexcept
{
  return capability_for<Backend, Peripheral>().supported;
}

template <typename Backend, peripheral_kind Peripheral> consteval bool peripheral_supports_isr() noexcept
{
  return capability_for<Backend, Peripheral>().isr;
}

template <typename Backend, peripheral_kind Peripheral> consteval bool peripheral_supports_dma() noexcept
{
  return capability_for<Backend, Peripheral>().dma;
}

} // namespace microhal
