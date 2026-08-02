// SPDX-License-Identifier: Apache-2.0
/**
 * @file backend_config.hpp
 * @brief Compile-time selection of which microHAL backends are built into a translation unit.
 * @details By default every backend is available, which keeps host builds and the multi-backend
 *          tests working out of the box. Applications that care about compile time or that must
 *          not see a foreign vendor SDK can define `MICROHAL_BACKENDS_EXPLICIT` and then enable
 *          only the backends they need, for example:
 *          @code
 *          #define MICROHAL_BACKENDS_EXPLICIT 1
 *          #define MICROHAL_ENABLE_STM32_HAL 1
 *          @endcode
 *          The equivalent CMake option is `MICROHAL_BACKENDS`, which forwards these definitions.
 * @copyright Copyright (c) 2026 James Baldwin
 * @author microHAL contributors
 * @ingroup backend_support
 */
#pragma once

#if !defined(MICROHAL_BACKENDS_EXPLICIT)
#define MICROHAL_BACKENDS_EXPLICIT 0
#endif

#if MICROHAL_BACKENDS_EXPLICIT
// Opt-in mode: anything not explicitly enabled defaults to off.
#if !defined(MICROHAL_ENABLE_STM32_HAL)
#define MICROHAL_ENABLE_STM32_HAL 0
#endif
#if !defined(MICROHAL_ENABLE_STM32_LL)
#define MICROHAL_ENABLE_STM32_LL 0
#endif
#if !defined(MICROHAL_ENABLE_KINETIS_ESPRESSO)
#define MICROHAL_ENABLE_KINETIS_ESPRESSO 0
#endif
#if !defined(MICROHAL_ENABLE_ZEPHYR)
#define MICROHAL_ENABLE_ZEPHYR 0
#endif
#if !defined(MICROHAL_ENABLE_NUTTX)
#define MICROHAL_ENABLE_NUTTX 0
#endif
#if !defined(MICROHAL_ENABLE_NORDIC_NRFX)
#define MICROHAL_ENABLE_NORDIC_NRFX 0
#endif
#if !defined(MICROHAL_ENABLE_MOCK)
#define MICROHAL_ENABLE_MOCK 0
#endif
#else
// Default mode: every backend is compiled so capability queries work across all of them.
#if !defined(MICROHAL_ENABLE_STM32_HAL)
#define MICROHAL_ENABLE_STM32_HAL 1
#endif
#if !defined(MICROHAL_ENABLE_STM32_LL)
#define MICROHAL_ENABLE_STM32_LL 1
#endif
#if !defined(MICROHAL_ENABLE_KINETIS_ESPRESSO)
#define MICROHAL_ENABLE_KINETIS_ESPRESSO 1
#endif
#if !defined(MICROHAL_ENABLE_ZEPHYR)
#define MICROHAL_ENABLE_ZEPHYR 1
#endif
#if !defined(MICROHAL_ENABLE_NUTTX)
#define MICROHAL_ENABLE_NUTTX 1
#endif
#if !defined(MICROHAL_ENABLE_NORDIC_NRFX)
#define MICROHAL_ENABLE_NORDIC_NRFX 1
#endif
#if !defined(MICROHAL_ENABLE_MOCK)
#define MICROHAL_ENABLE_MOCK 1
#endif
#endif

#if !(MICROHAL_ENABLE_STM32_HAL || MICROHAL_ENABLE_STM32_LL || MICROHAL_ENABLE_KINETIS_ESPRESSO ||                     \
      MICROHAL_ENABLE_ZEPHYR || MICROHAL_ENABLE_NUTTX || MICROHAL_ENABLE_NORDIC_NRFX || MICROHAL_ENABLE_MOCK)
#error                                                                                                                 \
    "microHAL: at least one backend must be enabled. Enable one of MICROHAL_ENABLE_* or unset MICROHAL_BACKENDS_EXPLICIT."
#endif
