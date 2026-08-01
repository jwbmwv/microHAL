// SPDX-License-Identifier: Apache-2.0
/**
 * @file traits.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace microhal::dma
{
enum class dma_direction : std::uint8_t;
enum class dma_priority : std::uint8_t;
} // namespace microhal::dma

// STM32 LL HAL includes - conditional based on MCU family
#if defined(STM32C0)
#include "stm32c0xx_ll_bus.h"
#include "stm32c0xx_ll_crc.h"
#include "stm32c0xx_ll_dma.h"
#include "stm32c0xx_ll_gpio.h"
#include "stm32c0xx_ll_i2c.h"
#include "stm32c0xx_ll_rcc.h"
#include "stm32c0xx_ll_rtc.h"
#include "stm32c0xx_ll_spi.h"
#include "stm32c0xx_ll_tim.h"
#include "stm32c0xx_ll_usart.h"
#elif defined(STM32F0)
#include "stm32f0xx_ll_adc.h"
#include "stm32f0xx_ll_bus.h"
#include "stm32f0xx_ll_crc.h"
#include "stm32f0xx_ll_dac.h"
#include "stm32f0xx_ll_dma.h"
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_i2c.h"
#include "stm32f0xx_ll_rcc.h"
#include "stm32f0xx_ll_rtc.h"
#include "stm32f0xx_ll_spi.h"
#include "stm32f0xx_ll_tim.h"
#include "stm32f0xx_ll_usart.h"
#elif defined(STM32F1)
#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_crc.h"
#include "stm32f1xx_ll_dma.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_i2c.h"
#include "stm32f1xx_ll_rcc.h"
#include "stm32f1xx_ll_rtc.h"
#include "stm32f1xx_ll_spi.h"
#include "stm32f1xx_ll_tim.h"
#include "stm32f1xx_ll_usart.h"
#elif defined(STM32F2)
#include "stm32f2xx_ll_bus.h"
#include "stm32f2xx_ll_crc.h"
#include "stm32f2xx_ll_dma.h"
#include "stm32f2xx_ll_gpio.h"
#include "stm32f2xx_ll_i2c.h"
#include "stm32f2xx_ll_rcc.h"
#include "stm32f2xx_ll_rng.h"
#include "stm32f2xx_ll_rtc.h"
#include "stm32f2xx_ll_spi.h"
#include "stm32f2xx_ll_tim.h"
#include "stm32f2xx_ll_usart.h"
#elif defined(STM32F3)
#include "stm32f3xx_ll_bus.h"
#include "stm32f3xx_ll_crc.h"
#include "stm32f3xx_ll_dma.h"
#include "stm32f3xx_ll_gpio.h"
#include "stm32f3xx_ll_i2c.h"
#include "stm32f3xx_ll_rcc.h"
#include "stm32f3xx_ll_rtc.h"
#include "stm32f3xx_ll_spi.h"
#include "stm32f3xx_ll_tim.h"
#include "stm32f3xx_ll_usart.h"
#elif defined(STM32F4)
#include "stm32f4xx_ll_adc.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_crc.h"
#include "stm32f4xx_ll_dac.h"
#include "stm32f4xx_ll_dma.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_i2c.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_rng.h"
#include "stm32f4xx_ll_rtc.h"
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_tim.h"
#include "stm32f4xx_ll_usart.h"
#elif defined(STM32F7)
#include "stm32f7xx_ll_bus.h"
#include "stm32f7xx_ll_crc.h"
#include "stm32f7xx_ll_dma.h"
#include "stm32f7xx_ll_gpio.h"
#include "stm32f7xx_ll_i2c.h"
#include "stm32f7xx_ll_rcc.h"
#include "stm32f7xx_ll_rng.h"
#include "stm32f7xx_ll_rtc.h"
#include "stm32f7xx_ll_spi.h"
#include "stm32f7xx_ll_tim.h"
#include "stm32f7xx_ll_usart.h"
#elif defined(STM32G0)
#include "stm32g0xx_ll_bus.h"
#include "stm32g0xx_ll_crc.h"
#include "stm32g0xx_ll_dma.h"
#include "stm32g0xx_ll_gpio.h"
#include "stm32g0xx_ll_i2c.h"
#include "stm32g0xx_ll_lpuart.h"
#include "stm32g0xx_ll_rcc.h"
#include "stm32g0xx_ll_rng.h"
#include "stm32g0xx_ll_rtc.h"
#include "stm32g0xx_ll_spi.h"
#include "stm32g0xx_ll_tim.h"
#include "stm32g0xx_ll_usart.h"
#elif defined(STM32G4)
#include "stm32g4xx_ll_adc.h"
#include "stm32g4xx_ll_bus.h"
#include "stm32g4xx_ll_crc.h"
#include "stm32g4xx_ll_dac.h"
#include "stm32g4xx_ll_dma.h"
#include "stm32g4xx_ll_gpio.h"
#include "stm32g4xx_ll_i2c.h"
#include "stm32g4xx_ll_lpuart.h"
#include "stm32g4xx_ll_rcc.h"
#include "stm32g4xx_ll_rng.h"
#include "stm32g4xx_ll_rtc.h"
#include "stm32g4xx_ll_spi.h"
#include "stm32g4xx_ll_tim.h"
#include "stm32g4xx_ll_usart.h"
#elif defined(STM32H5)
#include "stm32h5xx_ll_bus.h"
#include "stm32h5xx_ll_crc.h"
#include "stm32h5xx_ll_dma.h"
#include "stm32h5xx_ll_gpio.h"
#include "stm32h5xx_ll_i2c.h"
#include "stm32h5xx_ll_lpuart.h"
#include "stm32h5xx_ll_rcc.h"
#include "stm32h5xx_ll_rng.h"
#include "stm32h5xx_ll_rtc.h"
#include "stm32h5xx_ll_spi.h"
#include "stm32h5xx_ll_tim.h"
#include "stm32h5xx_ll_usart.h"
#elif defined(STM32H7RS)
#include "stm32h7rsxx_ll_bus.h"
#include "stm32h7rsxx_ll_crc.h"
#include "stm32h7rsxx_ll_dma.h"
#include "stm32h7rsxx_ll_gpio.h"
#include "stm32h7rsxx_ll_i2c.h"
#include "stm32h7rsxx_ll_lpuart.h"
#include "stm32h7rsxx_ll_rcc.h"
#include "stm32h7rsxx_ll_rng.h"
#include "stm32h7rsxx_ll_rtc.h"
#include "stm32h7rsxx_ll_spi.h"
#include "stm32h7rsxx_ll_tim.h"
#include "stm32h7rsxx_ll_usart.h"
#elif defined(STM32L0)
#include "stm32l0xx_ll_bus.h"
#include "stm32l0xx_ll_crc.h"
#include "stm32l0xx_ll_dma.h"
#include "stm32l0xx_ll_gpio.h"
#include "stm32l0xx_ll_i2c.h"
#include "stm32l0xx_ll_lpuart.h"
#include "stm32l0xx_ll_rcc.h"
#include "stm32l0xx_ll_rng.h"
#include "stm32l0xx_ll_rtc.h"
#include "stm32l0xx_ll_spi.h"
#include "stm32l0xx_ll_tim.h"
#include "stm32l0xx_ll_usart.h"
#elif defined(STM32L1)
#include "stm32l1xx_ll_bus.h"
#include "stm32l1xx_ll_crc.h"
#include "stm32l1xx_ll_dma.h"
#include "stm32l1xx_ll_gpio.h"
#include "stm32l1xx_ll_i2c.h"
#include "stm32l1xx_ll_rcc.h"
#include "stm32l1xx_ll_rtc.h"
#include "stm32l1xx_ll_spi.h"
#include "stm32l1xx_ll_tim.h"
#include "stm32l1xx_ll_usart.h"
#elif defined(STM32L4)
#include "stm32l4xx_ll_adc.h"
#include "stm32l4xx_ll_bus.h"
#include "stm32l4xx_ll_crc.h"
#include "stm32l4xx_ll_dac.h"
#include "stm32l4xx_ll_dma.h"
#include "stm32l4xx_ll_gpio.h"
#include "stm32l4xx_ll_i2c.h"
#include "stm32l4xx_ll_lpuart.h"
#include "stm32l4xx_ll_rcc.h"
#include "stm32l4xx_ll_rng.h"
#include "stm32l4xx_ll_rtc.h"
#include "stm32l4xx_ll_spi.h"
#include "stm32l4xx_ll_tim.h"
#include "stm32l4xx_ll_usart.h"
#elif defined(STM32L5)
#include "stm32l5xx_ll_bus.h"
#include "stm32l5xx_ll_crc.h"
#include "stm32l5xx_ll_dma.h"
#include "stm32l5xx_ll_gpio.h"
#include "stm32l5xx_ll_i2c.h"
#include "stm32l5xx_ll_lpuart.h"
#include "stm32l5xx_ll_rcc.h"
#include "stm32l5xx_ll_rng.h"
#include "stm32l5xx_ll_rtc.h"
#include "stm32l5xx_ll_spi.h"
#include "stm32l5xx_ll_tim.h"
#include "stm32l5xx_ll_usart.h"
#elif defined(STM32N6)
#include "stm32n6xx_ll_bus.h"
#include "stm32n6xx_ll_crc.h"
#include "stm32n6xx_ll_dma.h"
#include "stm32n6xx_ll_gpio.h"
#include "stm32n6xx_ll_i2c.h"
#include "stm32n6xx_ll_lpuart.h"
#include "stm32n6xx_ll_rcc.h"
#include "stm32n6xx_ll_rng.h"
#include "stm32n6xx_ll_rtc.h"
#include "stm32n6xx_ll_spi.h"
#include "stm32n6xx_ll_tim.h"
#include "stm32n6xx_ll_usart.h"
#elif defined(STM32U0)
#include "stm32u0xx_ll_bus.h"
#include "stm32u0xx_ll_crc.h"
#include "stm32u0xx_ll_dma.h"
#include "stm32u0xx_ll_gpio.h"
#include "stm32u0xx_ll_i2c.h"
#include "stm32u0xx_ll_lpuart.h"
#include "stm32u0xx_ll_rcc.h"
#include "stm32u0xx_ll_rng.h"
#include "stm32u0xx_ll_rtc.h"
#include "stm32u0xx_ll_spi.h"
#include "stm32u0xx_ll_tim.h"
#include "stm32u0xx_ll_usart.h"
#elif defined(STM32U3)
#include "stm32u3xx_ll_bus.h"
#include "stm32u3xx_ll_crc.h"
#include "stm32u3xx_ll_dma.h"
#include "stm32u3xx_ll_gpio.h"
#include "stm32u3xx_ll_i2c.h"
#include "stm32u3xx_ll_lpuart.h"
#include "stm32u3xx_ll_rcc.h"
#include "stm32u3xx_ll_rng.h"
#include "stm32u3xx_ll_rtc.h"
#include "stm32u3xx_ll_spi.h"
#include "stm32u3xx_ll_tim.h"
#include "stm32u3xx_ll_usart.h"
#elif defined(STM32U5)
#include "stm32u5xx_ll_adc.h"
#include "stm32u5xx_ll_bus.h"
#include "stm32u5xx_ll_crc.h"
#include "stm32u5xx_ll_dac.h"
#include "stm32u5xx_ll_dma.h"
#include "stm32u5xx_ll_gpio.h"
#include "stm32u5xx_ll_i2c.h"
#include "stm32u5xx_ll_lpuart.h"
#include "stm32u5xx_ll_rcc.h"
#include "stm32u5xx_ll_rng.h"
#include "stm32u5xx_ll_rtc.h"
#include "stm32u5xx_ll_spi.h"
#include "stm32u5xx_ll_tim.h"
#include "stm32u5xx_ll_usart.h"
#elif defined(STM32H7)
#include "stm32h7xx_ll_adc.h"
#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_crc.h"
#include "stm32h7xx_ll_dac.h"
#include "stm32h7xx_ll_dma.h"
#include "stm32h7xx_ll_gpio.h"
#include "stm32h7xx_ll_i2c.h"
#include "stm32h7xx_ll_lpuart.h"
#include "stm32h7xx_ll_rcc.h"
#include "stm32h7xx_ll_rng.h"
#include "stm32h7xx_ll_rtc.h"
#include "stm32h7xx_ll_spi.h"
#include "stm32h7xx_ll_tim.h"
#include "stm32h7xx_ll_usart.h"
#elif defined(STM32WB)
#include "stm32wbxx_ll_bus.h"
#include "stm32wbxx_ll_crc.h"
#include "stm32wbxx_ll_dma.h"
#include "stm32wbxx_ll_gpio.h"
#include "stm32wbxx_ll_i2c.h"
#include "stm32wbxx_ll_lpuart.h"
#include "stm32wbxx_ll_rcc.h"
#include "stm32wbxx_ll_rng.h"
#include "stm32wbxx_ll_rtc.h"
#include "stm32wbxx_ll_spi.h"
#include "stm32wbxx_ll_tim.h"
#include "stm32wbxx_ll_usart.h"
#else
#if !defined(MICROHAL_SUPPRESS_MISSING_MCU_WARNINGS)
#warning                                                                                                               \
    "No STM32 MCU family defined. Define one of: STM32C0, STM32F0, STM32F1, STM32F2, STM32F3, STM32F4, STM32F7, STM32G0, STM32G4, STM32H5, STM32H7, STM32H7RS, STM32L0, STM32L1, STM32L4, STM32L5, STM32N6, STM32U0, STM32U3, STM32U5, STM32WB"
#endif
#endif

// Optional STM32 Cube HAL module headers used to provide LL-backend runtime paths
// for peripherals without comprehensive LL APIs across families.
#if defined(STM32C0)
#if defined(HAL_SD_MODULE_ENABLED) && __has_include("stm32c0xx_hal_sd.h")
#include "stm32c0xx_hal_sd.h"
#endif
#if defined(HAL_QSPI_MODULE_ENABLED) && __has_include("stm32c0xx_hal_qspi.h")
#include "stm32c0xx_hal_qspi.h"
#endif
#elif defined(STM32F0)
#if defined(HAL_SD_MODULE_ENABLED) && __has_include("stm32f0xx_hal_sd.h")
#include "stm32f0xx_hal_sd.h"
#endif
#if defined(HAL_QSPI_MODULE_ENABLED) && __has_include("stm32f0xx_hal_qspi.h")
#include "stm32f0xx_hal_qspi.h"
#endif
#elif defined(STM32F1)
#if defined(HAL_SD_MODULE_ENABLED) && __has_include("stm32f1xx_hal_sd.h")
#include "stm32f1xx_hal_sd.h"
#endif
#if defined(HAL_QSPI_MODULE_ENABLED) && __has_include("stm32f1xx_hal_qspi.h")
#include "stm32f1xx_hal_qspi.h"
#endif
#elif defined(STM32F2)
#if defined(HAL_SD_MODULE_ENABLED) && __has_include("stm32f2xx_hal_sd.h")
#include "stm32f2xx_hal_sd.h"
#endif
#if defined(HAL_QSPI_MODULE_ENABLED) && __has_include("stm32f2xx_hal_qspi.h")
#include "stm32f2xx_hal_qspi.h"
#endif
#elif defined(STM32F3)
#if defined(HAL_SD_MODULE_ENABLED) && __has_include("stm32f3xx_hal_sd.h")
#include "stm32f3xx_hal_sd.h"
#endif
#if defined(HAL_QSPI_MODULE_ENABLED) && __has_include("stm32f3xx_hal_qspi.h")
#include "stm32f3xx_hal_qspi.h"
#endif
#elif defined(STM32F4)
#if defined(HAL_SD_MODULE_ENABLED) && __has_include("stm32f4xx_hal_sd.h")
#include "stm32f4xx_hal_sd.h"
#endif
#if defined(HAL_QSPI_MODULE_ENABLED) && __has_include("stm32f4xx_hal_qspi.h")
#include "stm32f4xx_hal_qspi.h"
#endif
#elif defined(STM32F7)
#if defined(HAL_SD_MODULE_ENABLED) && __has_include("stm32f7xx_hal_sd.h")
#include "stm32f7xx_hal_sd.h"
#endif
#if defined(HAL_QSPI_MODULE_ENABLED) && __has_include("stm32f7xx_hal_qspi.h")
#include "stm32f7xx_hal_qspi.h"
#endif
#elif defined(STM32G0)
#if defined(HAL_SD_MODULE_ENABLED) && __has_include("stm32g0xx_hal_sd.h")
#include "stm32g0xx_hal_sd.h"
#endif
#if defined(HAL_QSPI_MODULE_ENABLED) && __has_include("stm32g0xx_hal_qspi.h")
#include "stm32g0xx_hal_qspi.h"
#endif
#elif defined(STM32G4)
#if defined(HAL_SD_MODULE_ENABLED) && __has_include("stm32g4xx_hal_sd.h")
#include "stm32g4xx_hal_sd.h"
#endif
#if defined(HAL_QSPI_MODULE_ENABLED) && __has_include("stm32g4xx_hal_qspi.h")
#include "stm32g4xx_hal_qspi.h"
#endif
#elif defined(STM32H5)
#if defined(HAL_SD_MODULE_ENABLED) && __has_include("stm32h5xx_hal_sd.h")
#include "stm32h5xx_hal_sd.h"
#endif
#if defined(HAL_QSPI_MODULE_ENABLED) && __has_include("stm32h5xx_hal_qspi.h")
#include "stm32h5xx_hal_qspi.h"
#endif
#elif defined(STM32H7RS)
#if defined(HAL_SD_MODULE_ENABLED) && __has_include("stm32h7rsxx_hal_sd.h")
#include "stm32h7rsxx_hal_sd.h"
#endif
#if defined(HAL_QSPI_MODULE_ENABLED) && __has_include("stm32h7rsxx_hal_qspi.h")
#include "stm32h7rsxx_hal_qspi.h"
#endif
#elif defined(STM32L0)
#if defined(HAL_SD_MODULE_ENABLED) && __has_include("stm32l0xx_hal_sd.h")
#include "stm32l0xx_hal_sd.h"
#endif
#if defined(HAL_QSPI_MODULE_ENABLED) && __has_include("stm32l0xx_hal_qspi.h")
#include "stm32l0xx_hal_qspi.h"
#endif
#elif defined(STM32L1)
#if defined(HAL_SD_MODULE_ENABLED) && __has_include("stm32l1xx_hal_sd.h")
#include "stm32l1xx_hal_sd.h"
#endif
#if defined(HAL_QSPI_MODULE_ENABLED) && __has_include("stm32l1xx_hal_qspi.h")
#include "stm32l1xx_hal_qspi.h"
#endif
#elif defined(STM32L4)
#if defined(HAL_SD_MODULE_ENABLED) && __has_include("stm32l4xx_hal_sd.h")
#include "stm32l4xx_hal_sd.h"
#endif
#if defined(HAL_QSPI_MODULE_ENABLED) && __has_include("stm32l4xx_hal_qspi.h")
#include "stm32l4xx_hal_qspi.h"
#endif
#elif defined(STM32L5)
#if defined(HAL_SD_MODULE_ENABLED) && __has_include("stm32l5xx_hal_sd.h")
#include "stm32l5xx_hal_sd.h"
#endif
#if defined(HAL_QSPI_MODULE_ENABLED) && __has_include("stm32l5xx_hal_qspi.h")
#include "stm32l5xx_hal_qspi.h"
#endif
#elif defined(STM32N6)
#if defined(HAL_SD_MODULE_ENABLED) && __has_include("stm32n6xx_hal_sd.h")
#include "stm32n6xx_hal_sd.h"
#endif
#if defined(HAL_QSPI_MODULE_ENABLED) && __has_include("stm32n6xx_hal_qspi.h")
#include "stm32n6xx_hal_qspi.h"
#endif
#elif defined(STM32U0)
#if defined(HAL_SD_MODULE_ENABLED) && __has_include("stm32u0xx_hal_sd.h")
#include "stm32u0xx_hal_sd.h"
#endif
#if defined(HAL_QSPI_MODULE_ENABLED) && __has_include("stm32u0xx_hal_qspi.h")
#include "stm32u0xx_hal_qspi.h"
#endif
#elif defined(STM32U3)
#if defined(HAL_SD_MODULE_ENABLED) && __has_include("stm32u3xx_hal_sd.h")
#include "stm32u3xx_hal_sd.h"
#endif
#if defined(HAL_QSPI_MODULE_ENABLED) && __has_include("stm32u3xx_hal_qspi.h")
#include "stm32u3xx_hal_qspi.h"
#endif
#elif defined(STM32U5)
#if defined(HAL_SD_MODULE_ENABLED) && __has_include("stm32u5xx_hal_sd.h")
#include "stm32u5xx_hal_sd.h"
#endif
#if defined(HAL_QSPI_MODULE_ENABLED) && __has_include("stm32u5xx_hal_qspi.h")
#include "stm32u5xx_hal_qspi.h"
#endif
#elif defined(STM32H7)
#if defined(HAL_SD_MODULE_ENABLED) && __has_include("stm32h7xx_hal_sd.h")
#include "stm32h7xx_hal_sd.h"
#endif
#if defined(HAL_QSPI_MODULE_ENABLED) && __has_include("stm32h7xx_hal_qspi.h")
#include "stm32h7xx_hal_qspi.h"
#endif
#elif defined(STM32WB)
#if defined(HAL_SD_MODULE_ENABLED) && __has_include("stm32wbxx_hal_sd.h")
#include "stm32wbxx_hal_sd.h"
#endif
#if defined(HAL_QSPI_MODULE_ENABLED) && __has_include("stm32wbxx_hal_qspi.h")
#include "stm32wbxx_hal_qspi.h"
#endif
#endif

#if defined(STM32C0) || defined(STM32F0) || defined(STM32F1) || defined(STM32F2) || defined(STM32F3) ||                \
    defined(STM32F4) || defined(STM32F7) || defined(STM32G0) || defined(STM32G4) || defined(STM32H5) ||                \
    defined(STM32H7) || defined(STM32H7RS) || defined(STM32L0) || defined(STM32L1) || defined(STM32L4) ||              \
    defined(STM32L5) || defined(STM32N6) || defined(STM32U0) || defined(STM32U3) || defined(STM32U5) ||                \
    defined(STM32WB)
#define MICROHAL_STM32_LL_FAMILY_SELECTED 1
#else
#define MICROHAL_STM32_LL_FAMILY_SELECTED 0
#endif

#if defined(STM32H5) || defined(STM32H7RS) || defined(STM32N6) || defined(STM32U3)
#define MICROHAL_STM32_LL_DMA_RUNTIME 0
#else
#define MICROHAL_STM32_LL_DMA_RUNTIME 1
#endif

#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4) || defined(STM32F7) || defined(STM32H5) ||                \
  defined(STM32H7) || defined(STM32H7RS) || defined(STM32L1) || defined(STM32L4) || defined(STM32L5) ||              \
  defined(STM32N6) || defined(STM32U3) || defined(STM32U5)
#define MICROHAL_STM32_LL_SD_RUNTIME 1
#else
#define MICROHAL_STM32_LL_SD_RUNTIME 0
#endif

#if defined(STM32C0) && __has_include("stm32c0xx_ll_iwdg.h")
#include "stm32c0xx_ll_iwdg.h"
#elif defined(STM32F0) && __has_include("stm32f0xx_ll_iwdg.h")
#include "stm32f0xx_ll_iwdg.h"
#elif defined(STM32F1) && __has_include("stm32f1xx_ll_iwdg.h")
#include "stm32f1xx_ll_iwdg.h"
#elif defined(STM32F2) && __has_include("stm32f2xx_ll_iwdg.h")
#include "stm32f2xx_ll_iwdg.h"
#elif defined(STM32F3) && __has_include("stm32f3xx_ll_iwdg.h")
#include "stm32f3xx_ll_iwdg.h"
#elif defined(STM32F4) && __has_include("stm32f4xx_ll_iwdg.h")
#include "stm32f4xx_ll_iwdg.h"
#elif defined(STM32F7) && __has_include("stm32f7xx_ll_iwdg.h")
#include "stm32f7xx_ll_iwdg.h"
#elif defined(STM32G0) && __has_include("stm32g0xx_ll_iwdg.h")
#include "stm32g0xx_ll_iwdg.h"
#elif defined(STM32G4) && __has_include("stm32g4xx_ll_iwdg.h")
#include "stm32g4xx_ll_iwdg.h"
#elif defined(STM32H5) && __has_include("stm32h5xx_ll_iwdg.h")
#include "stm32h5xx_ll_iwdg.h"
#elif defined(STM32H7) && __has_include("stm32h7xx_ll_iwdg.h")
#include "stm32h7xx_ll_iwdg.h"
#elif defined(STM32H7RS) && __has_include("stm32h7rsxx_ll_iwdg.h")
#include "stm32h7rsxx_ll_iwdg.h"
#elif defined(STM32L0) && __has_include("stm32l0xx_ll_iwdg.h")
#include "stm32l0xx_ll_iwdg.h"
#elif defined(STM32L1) && __has_include("stm32l1xx_ll_iwdg.h")
#include "stm32l1xx_ll_iwdg.h"
#elif defined(STM32L4) && __has_include("stm32l4xx_ll_iwdg.h")
#include "stm32l4xx_ll_iwdg.h"
#elif defined(STM32L5) && __has_include("stm32l5xx_ll_iwdg.h")
#include "stm32l5xx_ll_iwdg.h"
#elif defined(STM32N6) && __has_include("stm32n6xx_ll_iwdg.h")
#include "stm32n6xx_ll_iwdg.h"
#elif defined(STM32U0) && __has_include("stm32u0xx_ll_iwdg.h")
#include "stm32u0xx_ll_iwdg.h"
#elif defined(STM32U3) && __has_include("stm32u3xx_ll_iwdg.h")
#include "stm32u3xx_ll_iwdg.h"
#elif defined(STM32U5) && __has_include("stm32u5xx_ll_iwdg.h")
#include "stm32u5xx_ll_iwdg.h"
#elif defined(STM32WB) && __has_include("stm32wbxx_ll_iwdg.h")
#include "stm32wbxx_ll_iwdg.h"
#endif

#include "microhal/core/traits.hpp"

namespace microhal::backends::stm32::ll
{

inline constexpr bool family_has_rng =
#if defined(STM32F2) || defined(STM32F4) || defined(STM32F7) || defined(STM32G0) || defined(STM32G4) ||                \
    defined(STM32H5) || defined(STM32H7) || defined(STM32H7RS) || defined(STM32L0) || defined(STM32L4) ||              \
    defined(STM32L5) || defined(STM32N6) || defined(STM32U0) || defined(STM32U3) || defined(STM32U5) ||                \
    defined(STM32WB)
    true;
#else
    false;
#endif

inline constexpr bool family_has_lpuart =
#if defined(STM32G0) || defined(STM32G4) || defined(STM32H5) || defined(STM32H7) || defined(STM32H7RS) ||              \
    defined(STM32L0) || defined(STM32L4) || defined(STM32L5) || defined(STM32N6) || defined(STM32U0) ||                \
    defined(STM32U3) || defined(STM32U5) || defined(STM32WB)
    true;
#else
    false;
#endif

#if defined(I2C1) || defined(I2C2) || defined(I2C3) || defined(I2C4) || defined(I2C5) || defined(I2C6)
#define MICROHAL_STM32_LL_HAS_I2C 1
#else
#define MICROHAL_STM32_LL_HAS_I2C 0
#endif

#if defined(SPI1) || defined(SPI2) || defined(SPI3) || defined(SPI4) || defined(SPI5) || defined(SPI6)
#define MICROHAL_STM32_LL_HAS_SPI 1
#else
#define MICROHAL_STM32_LL_HAS_SPI 0
#endif

#if defined(USART1) || defined(USART2) || defined(USART3) || defined(UART4) || defined(UART5) || defined(USART6) ||    \
    defined(UART7) || defined(UART8) || defined(UART9) || defined(UART10) || defined(USART10) || defined(USART11) ||   \
    defined(USART12) || defined(UART11) || defined(UART12) || defined(LPUART1) || defined(LPUART2) || defined(LPUART3)
#define MICROHAL_STM32_LL_HAS_UART 1
#else
#define MICROHAL_STM32_LL_HAS_UART 0
#endif

#if defined(TIM1) || defined(TIM2) || defined(TIM3) || defined(TIM4) || defined(TIM5) || defined(TIM6) ||              \
    defined(TIM7) || defined(TIM8) || defined(TIM9) || defined(TIM10) || defined(TIM11) || defined(TIM12) ||           \
    defined(TIM13) || defined(TIM14) || defined(TIM15) || defined(TIM16) || defined(TIM17) || defined(TIM18) ||        \
    defined(TIM19) || defined(TIM20) || defined(TIM23) || defined(TIM24)
#define MICROHAL_STM32_LL_HAS_TIM 1
#else
#define MICROHAL_STM32_LL_HAS_TIM 0
#endif

#if defined(ADC1) || defined(ADC2) || defined(ADC3) || defined(ADC4) || defined(ADC5)
#define MICROHAL_STM32_LL_HAS_ADC 1
#else
#define MICROHAL_STM32_LL_HAS_ADC 0
#endif

#if defined(DAC) || defined(DAC1) || defined(DAC2) || defined(DAC3) || defined(DAC4)
#define MICROHAL_STM32_LL_HAS_DAC 1
#else
#define MICROHAL_STM32_LL_HAS_DAC 0
#endif

#if defined(QUADSPI) && defined(HAL_QSPI_MODULE_ENABLED)
#define MICROHAL_STM32_LL_HAS_QSPI 1
#else
#define MICROHAL_STM32_LL_HAS_QSPI 0
#endif

#if (defined(SDIO) || defined(SDMMC1)) && defined(HAL_SD_MODULE_ENABLED) && MICROHAL_STM32_LL_SD_RUNTIME
#define MICROHAL_STM32_LL_HAS_SD 1
#else
#define MICROHAL_STM32_LL_HAS_SD 0
#endif

inline constexpr std::uint32_t family_gpio_port_count =
#if defined(GPIOA)
    1U +
#else
    0U +
#endif
#if defined(GPIOB)
    1U +
#else
    0U +
#endif
#if defined(GPIOC)
    1U +
#else
    0U +
#endif
#if defined(GPIOD)
    1U +
#else
    0U +
#endif
#if defined(GPIOE)
    1U +
#else
    0U +
#endif
#if defined(GPIOF)
    1U +
#else
    0U +
#endif
#if defined(GPIOG)
    1U +
#else
    0U +
#endif
#if defined(GPIOH)
    1U +
#else
    0U +
#endif
#if defined(GPIOI)
    1U +
#else
    0U +
#endif
#if defined(GPIOJ)
    1U +
#else
    0U +
#endif
#if defined(GPIOK)
    1U +
#else
    0U +
#endif
    0U;

inline constexpr std::uint32_t i2c_instance_count =
#if defined(I2C1)
    1U +
#else
    0U +
#endif
#if defined(I2C2)
    1U +
#else
    0U +
#endif
#if defined(I2C3)
    1U +
#else
    0U +
#endif
#if defined(I2C4)
    1U +
#else
    0U +
#endif
#if defined(I2C5)
    1U +
#else
    0U +
#endif
#if defined(I2C6)
    1U +
#else
    0U +
#endif
    0U;

inline constexpr std::uint32_t spi_instance_count =
#if defined(SPI1)
    1U +
#else
    0U +
#endif
#if defined(SPI2)
    1U +
#else
    0U +
#endif
#if defined(SPI3)
    1U +
#else
    0U +
#endif
#if defined(SPI4)
    1U +
#else
    0U +
#endif
#if defined(SPI5)
    1U +
#else
    0U +
#endif
#if defined(SPI6)
    1U +
#else
    0U +
#endif
    0U;

inline constexpr std::uint32_t uart_instance_count =
#if defined(USART1)
    1U +
#else
    0U +
#endif
#if defined(USART2)
    1U +
#else
    0U +
#endif
#if defined(USART3)
    1U +
#else
    0U +
#endif
#if defined(UART4)
    1U +
#else
    0U +
#endif
#if defined(UART5)
    1U +
#else
    0U +
#endif
#if defined(USART6)
    1U +
#else
    0U +
#endif
#if defined(UART7)
    1U +
#else
    0U +
#endif
#if defined(UART8)
    1U +
#else
    0U +
#endif
#if defined(UART9)
    1U +
#else
    0U +
#endif
#if defined(UART10)
    1U +
#else
    0U +
#endif
#if defined(USART10)
    1U +
#else
    0U +
#endif
#if defined(USART11)
    1U +
#else
    0U +
#endif
#if defined(USART12)
    1U +
#else
    0U +
#endif
#if defined(UART11)
    1U +
#else
    0U +
#endif
#if defined(UART12)
    1U +
#else
    0U +
#endif
#if defined(LPUART1)
    1U +
#else
    0U +
#endif
#if defined(LPUART2)
    1U +
#else
    0U +
#endif
#if defined(LPUART3)
    1U +
#else
    0U +
#endif
    0U;

inline constexpr std::uint32_t timer_instance_count =
#if defined(TIM1)
    1U +
#else
    0U +
#endif
#if defined(TIM2)
    1U +
#else
    0U +
#endif
#if defined(TIM3)
    1U +
#else
    0U +
#endif
#if defined(TIM4)
    1U +
#else
    0U +
#endif
#if defined(TIM5)
    1U +
#else
    0U +
#endif
#if defined(TIM6)
    1U +
#else
    0U +
#endif
#if defined(TIM7)
    1U +
#else
    0U +
#endif
#if defined(TIM8)
    1U +
#else
    0U +
#endif
#if defined(TIM9)
    1U +
#else
    0U +
#endif
#if defined(TIM10)
    1U +
#else
    0U +
#endif
#if defined(TIM11)
    1U +
#else
    0U +
#endif
#if defined(TIM12)
    1U +
#else
    0U +
#endif
#if defined(TIM13)
    1U +
#else
    0U +
#endif
#if defined(TIM14)
    1U +
#else
    0U +
#endif
#if defined(TIM15)
    1U +
#else
    0U +
#endif
#if defined(TIM16)
    1U +
#else
    0U +
#endif
#if defined(TIM17)
    1U +
#else
    0U +
#endif
#if defined(TIM18)
    1U +
#else
    0U +
#endif
#if defined(TIM19)
    1U +
#else
    0U +
#endif
#if defined(TIM20)
    1U +
#else
    0U +
#endif
#if defined(TIM23)
    1U +
#else
    0U +
#endif
#if defined(TIM24)
    1U +
#else
    0U +
#endif
    0U;

inline constexpr std::uint32_t adc_instance_count =
#if defined(ADC1)
    1U +
#else
    0U +
#endif
#if defined(ADC2)
    1U +
#else
    0U +
#endif
#if defined(ADC3)
    1U +
#else
    0U +
#endif
#if defined(ADC4)
    1U +
#else
    0U +
#endif
#if defined(ADC5)
    1U +
#else
    0U +
#endif
    0U;

inline constexpr std::uint32_t dac_instance_count =
#if defined(DAC1)
    1U +
#else
    0U +
#endif
#if defined(DAC2)
    1U +
#else
    0U +
#endif
#if defined(DAC3)
    1U +
#else
    0U +
#endif
#if defined(DAC4)
    1U +
#else
    0U +
#endif
#if !defined(DAC1) && !defined(DAC2) && !defined(DAC3) && !defined(DAC4) && defined(DAC)
    1U +
#else
    0U +
#endif
    0U;

inline constexpr std::uint32_t dma_controller_count =
#if defined(DMA1)
    1U +
#else
    0U +
#endif
#if defined(DMA2)
    1U +
#else
    0U +
#endif
    0U;

#if defined(GPIOA)
// Helper to get GPIO port
template <std::uint8_t PortIndex> consteval GPIO_TypeDef* gpio_port() noexcept
{
  constexpr std::array<GPIO_TypeDef*, 11> ports = {GPIOA,
#if defined(GPIOB)
                                                   GPIOB,
#else
                                                   nullptr,
#endif
#if defined(GPIOC)
                                                   GPIOC,
#else
                                                   nullptr,
#endif
#if defined(GPIOD)
                                                   GPIOD,
#else
                                                   nullptr,
#endif
#if defined(GPIOE)
                                                   GPIOE,
#else
                                                   nullptr,
#endif
#if defined(GPIOF)
                                                   GPIOF,
#else
                                                   nullptr,
#endif
#if defined(GPIOG)
                                                   GPIOG,
#else
                                                   nullptr,
#endif
#if defined(GPIOH)
                                                   GPIOH,
#else
                                                   nullptr,
#endif
#if defined(GPIOI)
                                                   GPIOI,
#else
                                                   nullptr,
#endif
#if defined(GPIOJ)
                                                   GPIOJ,
#else
                                                   nullptr,
#endif
#if defined(GPIOK)
                                                   GPIOK
#else
                                                   nullptr
#endif
  };
  return ports[PortIndex];
}

// Map microHAL gpio_mode to LL mode
consteval std::uint32_t ll_gpio_mode(const gpio_mode mode) noexcept
{
  switch (mode)
  {
  case gpio_mode::input:
    return LL_GPIO_MODE_INPUT;
  case gpio_mode::output:
    return LL_GPIO_MODE_OUTPUT;
  case gpio_mode::alternate:
    return LL_GPIO_MODE_ALTERNATE;
  case gpio_mode::analog:
    return LL_GPIO_MODE_ANALOG;
  default:
    return LL_GPIO_MODE_INPUT;
  }
}

// Map microHAL gpio_pull to LL pull
consteval std::uint32_t ll_gpio_pull(const gpio_pull pull) noexcept
{
  switch (pull)
  {
  case gpio_pull::none:
    return LL_GPIO_PULL_NO;
  case gpio_pull::up:
    return LL_GPIO_PULL_UP;
  case gpio_pull::down:
    return LL_GPIO_PULL_DOWN;
  default:
    return LL_GPIO_PULL_NO;
  }
}

// Map microHAL gpio_speed to LL speed
consteval std::uint32_t ll_gpio_speed(const gpio_speed speed) noexcept
{
  switch (speed)
  {
  case gpio_speed::low:
    return LL_GPIO_SPEED_FREQ_LOW;
  case gpio_speed::medium:
    return LL_GPIO_SPEED_FREQ_MEDIUM;
  case gpio_speed::high:
    return LL_GPIO_SPEED_FREQ_HIGH;
  case gpio_speed::very_high:
    return LL_GPIO_SPEED_FREQ_VERY_HIGH;
  default:
    return LL_GPIO_SPEED_FREQ_LOW;
  }
}

template <typename PinConfig> consteval std::uint8_t gpio_port_index() noexcept
{
  if constexpr (requires { PinConfig::port_index; })
  {
    return PinConfig::port_index;
  }
  else if constexpr (requires { typename PinConfig::binding; })
  {
    using binding = typename PinConfig::binding;
    if constexpr (requires { binding::port_index; })
    {
      return binding::port_index;
    }
    else
    {
      static_assert(dependent_false_v<PinConfig>,
                    "microHAL: STM32 LL GPIO bindings must expose binding::port_index or PinConfig::port_index.");
      return 0U;
    }
  }
  else
  {
    static_assert(dependent_false_v<PinConfig>,
                  "microHAL: STM32 LL GPIO bindings must expose binding::port_index or PinConfig::port_index.");
    return 0U;
  }
}

template <typename PinConfig> consteval std::uint8_t gpio_pin_index() noexcept
{
  if constexpr (requires { PinConfig::pin_index; })
  {
    return PinConfig::pin_index;
  }
  else if constexpr (requires { typename PinConfig::binding; })
  {
    using binding = typename PinConfig::binding;
    if constexpr (requires { binding::pin_index; })
    {
      return binding::pin_index;
    }
    else
    {
      static_assert(dependent_false_v<PinConfig>,
                    "microHAL: STM32 LL GPIO bindings must expose binding::pin_index or PinConfig::pin_index.");
      return 0U;
    }
  }
  else
  {
    static_assert(dependent_false_v<PinConfig>,
                  "microHAL: STM32 LL GPIO bindings must expose binding::pin_index or PinConfig::pin_index.");
    return 0U;
  }
}

template <typename PinConfig> consteval std::uint8_t gpio_alternate_function() noexcept
{
  if constexpr (requires { typename PinConfig::binding; })
  {
    using binding = typename PinConfig::binding;
    if constexpr (requires { binding::alternate_function; })
    {
      return binding::alternate_function;
    }
  }
  else if constexpr (requires { PinConfig::alternate_function; })
  {
    return PinConfig::alternate_function;
  }

  return gpio_no_alternate;
}

template <typename Config> consteval std::uintptr_t peripheral_base_address() noexcept
{
  if constexpr (requires { Config::base_address; })
  {
    return Config::base_address;
  }
  else if constexpr (requires { typename Config::binding; })
  {
    using binding = typename Config::binding;
    if constexpr (requires { binding::base_address; })
    {
      return binding::base_address;
    }
    else
    {
      static_assert(dependent_false_v<Config>,
                    "microHAL: STM32 LL peripheral configs must expose binding::base_address or Config::base_address.");
      return 0U;
    }
  }
  else
  {
    static_assert(dependent_false_v<Config>,
                  "microHAL: STM32 LL peripheral configs must expose binding::base_address or Config::base_address.");
    return 0U;
  }
}

template <typename PeripheralType, typename Config> constexpr auto peripheral_instance() noexcept -> PeripheralType*
{
  return reinterpret_cast<PeripheralType*>(peripheral_base_address<Config>());
}

template <typename T> [[nodiscard]] inline T* hal_tx_buffer(const T* data) noexcept
{
  return const_cast<T*>(data);
}

template <typename Config> consteval std::uint32_t sd_clock_divider() noexcept
{
  if constexpr (requires { typename Config::binding; })
  {
    using binding = typename Config::binding;
    if constexpr (requires { binding::clock_divider; })
    {
      return binding::clock_divider;
    }
  }

  constexpr std::uint32_t source_clock_hz = 48'000'000U;
  constexpr std::uint32_t divider =
      (Config::clock_hz == 0U) ? source_clock_hz : ((source_clock_hz + Config::clock_hz - 1U) / Config::clock_hz);
  return (divider > 2U) ? (divider - 2U) : 0U;
}

template <typename Config> consteval bool sd_hardware_flow_control() noexcept
{
  if constexpr (requires { typename Config::binding; })
  {
    using binding = typename Config::binding;
    if constexpr (requires { binding::hardware_flow_control; })
    {
      return binding::hardware_flow_control;
    }
  }

  return false;
}

#if defined(SDIO)
template <typename Config> constexpr auto stm32_sd_instance() noexcept -> decltype(SDIO)
{
  if constexpr (requires { Config::base_address; })
  {
    return reinterpret_cast<decltype(SDIO)>(Config::base_address);
  }
  else if constexpr (requires { typename Config::binding; })
  {
    using binding = typename Config::binding;
    if constexpr (requires { binding::base_address; })
    {
      return reinterpret_cast<decltype(SDIO)>(binding::base_address);
    }
    else
    {
      static_assert(dependent_false_v<Config>,
                    "microHAL: STM32 LL SD configs must expose binding::base_address or instance_index.");
      return nullptr;
    }
  }
  else if constexpr (requires { Config::instance_index; })
  {
    static_assert(Config::instance_index == 0U,
                  "microHAL: STM32 LL SD currently maps only instance_index 0 to SDIO.");
    return SDIO;
  }
  else
  {
    static_assert(dependent_false_v<Config>,
                  "microHAL: STM32 LL SD configs must expose binding::base_address or instance_index.");
    return nullptr;
  }
}
#elif defined(SDMMC1)
template <typename Config> constexpr auto stm32_sd_instance() noexcept -> decltype(SDMMC1)
{
  if constexpr (requires { Config::base_address; })
  {
    return reinterpret_cast<decltype(SDMMC1)>(Config::base_address);
  }
  else if constexpr (requires { typename Config::binding; })
  {
    using binding = typename Config::binding;
    if constexpr (requires { binding::base_address; })
    {
      return reinterpret_cast<decltype(SDMMC1)>(binding::base_address);
    }
    else
    {
      static_assert(dependent_false_v<Config>,
                    "microHAL: STM32 LL SD configs must expose binding::base_address or instance_index.");
      return nullptr;
    }
  }
  else if constexpr (requires { Config::instance_index; })
  {
    static_assert(Config::instance_index == 0U,
                  "microHAL: STM32 LL SD currently maps only instance_index 0 to SDMMC1.");
    return SDMMC1;
  }
  else
  {
    static_assert(dependent_false_v<Config>,
                  "microHAL: STM32 LL SD configs must expose binding::base_address or instance_index.");
    return nullptr;
  }
}
#endif

template <typename Config> consteval std::uint32_t stm32_sd_bus_width() noexcept
{
#if defined(SDIO_BUS_WIDE_1B)
  constexpr std::uint32_t one_bit = SDIO_BUS_WIDE_1B;
  constexpr std::uint32_t four_bit = SDIO_BUS_WIDE_4B;
  constexpr std::uint32_t eight_bit = SDIO_BUS_WIDE_8B;
#elif defined(SDMMC_BUS_WIDE_1B)
  constexpr std::uint32_t one_bit = SDMMC_BUS_WIDE_1B;
  constexpr std::uint32_t four_bit = SDMMC_BUS_WIDE_4B;
  constexpr std::uint32_t eight_bit = SDMMC_BUS_WIDE_8B;
#else
  constexpr std::uint32_t one_bit = 0U;
  constexpr std::uint32_t four_bit = 0U;
  constexpr std::uint32_t eight_bit = 0U;
#endif

  switch (Config::bus_width)
  {
  case sd_bus_width::one:
    return one_bit;
  case sd_bus_width::four:
    return four_bit;
  case sd_bus_width::eight:
    return eight_bit;
  default:
    return one_bit;
  }
}

#if defined(QUADSPI)
template <typename Config> constexpr auto stm32_qspi_instance() noexcept -> decltype(QUADSPI)
{
  if constexpr (requires { Config::base_address; })
  {
    return reinterpret_cast<decltype(QUADSPI)>(Config::base_address);
  }
  else if constexpr (requires { typename Config::binding; })
  {
    using binding = typename Config::binding;
    if constexpr (requires { binding::base_address; })
    {
      return reinterpret_cast<decltype(QUADSPI)>(binding::base_address);
    }
    else
    {
      static_assert(dependent_false_v<Config>,
                    "microHAL: STM32 LL QSPI configs must expose binding::base_address or instance_index.");
      return nullptr;
    }
  }
  else if constexpr (requires { Config::instance_index; })
  {
    static_assert(Config::instance_index == 0U,
                  "microHAL: STM32 LL QSPI currently maps only instance_index 0 to QUADSPI.");
    return QUADSPI;
  }
  else
  {
    static_assert(dependent_false_v<Config>,
                  "microHAL: STM32 LL QSPI configs must expose binding::base_address or instance_index.");
    return nullptr;
  }
}
#endif

template <typename Config> consteval std::uint8_t dma_instance_index() noexcept
{
  if constexpr (requires { Config::instance_index; })
  {
    return Config::instance_index;
  }
  else if constexpr (requires { typename Config::binding; })
  {
    using binding = typename Config::binding;
    if constexpr (requires { binding::instance_index; })
    {
      return binding::instance_index;
    }
    else
    {
      static_assert(dependent_false_v<Config>,
                    "microHAL: STM32 LL DMA configs must expose binding::instance_index or Config::instance_index.");
      return 0U;
    }
  }
  else
  {
    static_assert(dependent_false_v<Config>,
                  "microHAL: STM32 LL DMA configs must expose binding::instance_index or Config::instance_index.");
    return 0U;
  }
}

template <typename Config> consteval std::uint8_t dma_stream_index() noexcept
{
  if constexpr (requires { Config::stream; })
  {
    return Config::stream;
  }
  else if constexpr (requires { typename Config::binding; })
  {
    using binding = typename Config::binding;
    if constexpr (requires { binding::stream; })
    {
      return binding::stream;
    }
    else
    {
      static_assert(dependent_false_v<Config>,
                    "microHAL: STM32 LL DMA configs must expose binding::stream or Config::stream.");
      return 0U;
    }
  }
  else
  {
    static_assert(dependent_false_v<Config>,
                  "microHAL: STM32 LL DMA configs must expose binding::stream or Config::stream.");
    return 0U;
  }
}

template <typename Config> consteval std::uint8_t dma_channel_index() noexcept
{
  if constexpr (requires { Config::channel; })
  {
    return Config::channel;
  }
  else if constexpr (requires { typename Config::binding; })
  {
    using binding = typename Config::binding;
    if constexpr (requires { binding::channel; })
    {
      return binding::channel;
    }
    else
    {
      static_assert(dependent_false_v<Config>,
                    "microHAL: STM32 LL DMA configs must expose binding::channel or Config::channel.");
      return 0U;
    }
  }
  else
  {
    static_assert(dependent_false_v<Config>,
                  "microHAL: STM32 LL DMA configs must expose binding::channel or Config::channel.");
    return 0U;
  }
}

template <typename Config> consteval DMA_TypeDef* dma_controller() noexcept
{
#if defined(DMA2)
  constexpr std::array<DMA_TypeDef*, 2U> controllers{DMA1, DMA2};
  static_assert((dma_instance_index<Config>() >= 1U) && (dma_instance_index<Config>() <= controllers.size()),
                "microHAL: STM32 LL DMA instance_index must select DMA1 or DMA2.");
  return controllers[dma_instance_index<Config>() - 1U];
#elif defined(DMA1)
  static_assert(dma_instance_index<Config>() == 1U, "microHAL: STM32 LL backend exposes DMA1 only on this MCU family.");
  return DMA1;
#else
  return nullptr;
#endif
}

template <typename Config> consteval std::uint32_t dma_ll_target() noexcept
{
#if defined(DMA1_Stream0)
  static_assert(dma_stream_index<Config>() < 8U, "microHAL: STM32 LL DMA stream index must be in the range [0, 7].");
  switch (dma_stream_index<Config>())
  {
  case 0U:
    return LL_DMA_STREAM_0;
  case 1U:
    return LL_DMA_STREAM_1;
  case 2U:
    return LL_DMA_STREAM_2;
  case 3U:
    return LL_DMA_STREAM_3;
  case 4U:
    return LL_DMA_STREAM_4;
  case 5U:
    return LL_DMA_STREAM_5;
  case 6U:
    return LL_DMA_STREAM_6;
  case 7U:
    return LL_DMA_STREAM_7;
  default:
    break;
  }
#elif defined(DMA1_Channel1)
  static_assert(dma_stream_index<Config>() < 8U,
                "microHAL: STM32 LL DMA channel-backed stream index must be in the range [0, 7].");
  switch (dma_stream_index<Config>())
  {
  case 0U:
    return LL_DMA_CHANNEL_1;
#if defined(LL_DMA_CHANNEL_2)
  case 1U:
    return LL_DMA_CHANNEL_2;
#endif
#if defined(LL_DMA_CHANNEL_3)
  case 2U:
    return LL_DMA_CHANNEL_3;
#endif
#if defined(LL_DMA_CHANNEL_4)
  case 3U:
    return LL_DMA_CHANNEL_4;
#endif
#if defined(LL_DMA_CHANNEL_5)
  case 4U:
    return LL_DMA_CHANNEL_5;
#endif
#if defined(LL_DMA_CHANNEL_6)
  case 5U:
    return LL_DMA_CHANNEL_6;
#endif
#if defined(LL_DMA_CHANNEL_7)
  case 6U:
    return LL_DMA_CHANNEL_7;
#endif
#if defined(LL_DMA_CHANNEL_8)
  case 7U:
    return LL_DMA_CHANNEL_8;
#endif
  default:
    break;
  }
#elif defined(DMA1_Channel0)
  static_assert(dma_stream_index<Config>() < 16U,
                "microHAL: STM32 LL DMA channel-backed stream index must be in the range [0, 15].");
  switch (dma_stream_index<Config>())
  {
  case 0U:
    return LL_DMA_CHANNEL_0;
#if defined(LL_DMA_CHANNEL_1)
  case 1U:
    return LL_DMA_CHANNEL_1;
#endif
#if defined(LL_DMA_CHANNEL_2)
  case 2U:
    return LL_DMA_CHANNEL_2;
#endif
#if defined(LL_DMA_CHANNEL_3)
  case 3U:
    return LL_DMA_CHANNEL_3;
#endif
#if defined(LL_DMA_CHANNEL_4)
  case 4U:
    return LL_DMA_CHANNEL_4;
#endif
#if defined(LL_DMA_CHANNEL_5)
  case 5U:
    return LL_DMA_CHANNEL_5;
#endif
#if defined(LL_DMA_CHANNEL_6)
  case 6U:
    return LL_DMA_CHANNEL_6;
#endif
#if defined(LL_DMA_CHANNEL_7)
  case 7U:
    return LL_DMA_CHANNEL_7;
#endif
#if defined(LL_DMA_CHANNEL_8)
  case 8U:
    return LL_DMA_CHANNEL_8;
#endif
#if defined(LL_DMA_CHANNEL_9)
  case 9U:
    return LL_DMA_CHANNEL_9;
#endif
#if defined(LL_DMA_CHANNEL_10)
  case 10U:
    return LL_DMA_CHANNEL_10;
#endif
#if defined(LL_DMA_CHANNEL_11)
  case 11U:
    return LL_DMA_CHANNEL_11;
#endif
#if defined(LL_DMA_CHANNEL_12)
  case 12U:
    return LL_DMA_CHANNEL_12;
#endif
#if defined(LL_DMA_CHANNEL_13)
  case 13U:
    return LL_DMA_CHANNEL_13;
#endif
#if defined(LL_DMA_CHANNEL_14)
  case 14U:
    return LL_DMA_CHANNEL_14;
#endif
#if defined(LL_DMA_CHANNEL_15)
  case 15U:
    return LL_DMA_CHANNEL_15;
#endif
  default:
    break;
  }
#endif

  return 0U;
}

template <typename Config> consteval std::uint32_t dma_ll_direction() noexcept
{
  switch (static_cast<std::uint8_t>(Config::direction))
  {
  case 0U:
    return LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
  case 1U:
    return LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
  case 2U:
    return LL_DMA_DIRECTION_MEMORY_TO_MEMORY;
  default:
    return LL_DMA_DIRECTION_MEMORY_TO_MEMORY;
  }
}

template <typename Config> consteval std::uint32_t dma_ll_priority() noexcept
{
  switch (static_cast<std::uint8_t>(Config::priority))
  {
  case 0U:
    return LL_DMA_PRIORITY_LOW;
  case 1U:
    return LL_DMA_PRIORITY_MEDIUM;
  case 2U:
    return LL_DMA_PRIORITY_HIGH;
  case 3U:
    return LL_DMA_PRIORITY_VERYHIGH;
  default:
    return LL_DMA_PRIORITY_MEDIUM;
  }
}

template <typename Config> consteval std::uint32_t dma_ll_request() noexcept
{
#if defined(DMA1_Stream0)
  static_assert(dma_channel_index<Config>() < 8U, "microHAL: STM32 LL DMA channel index must be in the range [0, 7].");
  switch (dma_channel_index<Config>())
  {
  case 0U:
    return LL_DMA_CHANNEL_0;
  case 1U:
    return LL_DMA_CHANNEL_1;
  case 2U:
    return LL_DMA_CHANNEL_2;
  case 3U:
    return LL_DMA_CHANNEL_3;
  case 4U:
    return LL_DMA_CHANNEL_4;
  case 5U:
    return LL_DMA_CHANNEL_5;
  case 6U:
    return LL_DMA_CHANNEL_6;
  case 7U:
    return LL_DMA_CHANNEL_7;
  default:
    break;
  }

  return LL_DMA_CHANNEL_0;
#elif defined(LL_DMA_REQUEST_0)
  return static_cast<std::uint32_t>(LL_DMA_REQUEST_0 + dma_channel_index<Config>());
#else
  return dma_channel_index<Config>();
#endif
}

template <typename Config> consteval std::uint32_t dma_source_increment_mode() noexcept
{
  if constexpr (static_cast<std::uint8_t>(Config::direction) == 0U)
  {
    return LL_DMA_PERIPH_NOINCREMENT;
  }

  return LL_DMA_PERIPH_INCREMENT;
}

template <typename Config> consteval std::uint32_t dma_destination_increment_mode() noexcept
{
  if constexpr (static_cast<std::uint8_t>(Config::direction) == 1U)
  {
    return LL_DMA_MEMORY_NOINCREMENT;
  }

  return LL_DMA_MEMORY_INCREMENT;
}

template <typename Config>
static auto dma_init(std::uint32_t src_addr, std::uint32_t dst_addr, std::uint32_t length) noexcept
    -> LL_DMA_InitTypeDef
{
#if MICROHAL_STM32_LL_DMA_RUNTIME == 0
  (void)src_addr;
  (void)dst_addr;
  (void)length;
  return {};
#else
  LL_DMA_InitTypeDef init{};
  init.PeriphOrM2MSrcAddress = src_addr;
  init.MemoryOrM2MDstAddress = dst_addr;
  init.Direction = dma_ll_direction<Config>();
  init.Mode = Config::circular_mode ? LL_DMA_MODE_CIRCULAR : LL_DMA_MODE_NORMAL;
  init.PeriphOrM2MSrcIncMode = dma_source_increment_mode<Config>();
  init.MemoryOrM2MDstIncMode = dma_destination_increment_mode<Config>();
  init.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;
  init.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;
  init.NbData = length;
  init.Priority = dma_ll_priority<Config>();

#if defined(STM32F2) || defined(STM32F4) || defined(STM32F7)
  init.Channel = dma_ll_request<Config>();
  init.FIFOMode = LL_DMA_FIFOMODE_DISABLE;
#if defined(LL_DMA_MBURST_SINGLE)
  init.MemBurst = LL_DMA_MBURST_SINGLE;
#endif
#if defined(LL_DMA_PBURST_SINGLE)
  init.PeriphBurst = LL_DMA_PBURST_SINGLE;
#endif
#elif defined(STM32H7)
  init.PeriphRequest = dma_ll_request<Config>();
  init.FIFOMode = LL_DMA_FIFOMODE_DISABLE;
#if defined(LL_DMA_MBURST_SINGLE)
  init.MemBurst = LL_DMA_MBURST_SINGLE;
#endif
#if defined(LL_DMA_PBURST_SINGLE)
  init.PeriphBurst = LL_DMA_PBURST_SINGLE;
#endif
#elif defined(STM32U5)
  init.Request = dma_ll_request<Config>();
#elif defined(STM32C0) || defined(STM32F0) || defined(STM32G0) || defined(STM32G4) || defined(STM32L0) ||              \
    defined(STM32L4) || defined(STM32L5) || defined(STM32U0) || defined(STM32WB)
  init.PeriphRequest = dma_ll_request<Config>();
#endif

  return init;
#endif
}

template <typename Config> static void dma_enable() noexcept
{
#if defined(DMA1_Stream0)
  LL_DMA_EnableStream(dma_controller<Config>(), dma_ll_target<Config>());
#elif defined(DMA1_Channel1) || defined(DMA1_Channel0)
  LL_DMA_EnableChannel(dma_controller<Config>(), dma_ll_target<Config>());
#endif
}

template <typename Config> static void dma_disable() noexcept
{
#if defined(DMA1_Stream0)
  LL_DMA_DisableStream(dma_controller<Config>(), dma_ll_target<Config>());
#elif defined(DMA1_Channel1) || defined(DMA1_Channel0)
  LL_DMA_DisableChannel(dma_controller<Config>(), dma_ll_target<Config>());
#endif
}
#endif

struct gpio_api final
{
  template <typename PinConfig> static void configure() noexcept
  {
#if defined(GPIOA)
    LL_GPIO_InitTypeDef init{};
    init.Pin = (1U << gpio_pin_index<PinConfig>());
    init.Mode = ll_gpio_mode(PinConfig::mode);
    init.Pull = ll_gpio_pull(PinConfig::pull);
    init.Speed = ll_gpio_speed(PinConfig::speed);
    init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    if constexpr (PinConfig::mode == gpio_mode::alternate)
    {
      init.Alternate = gpio_alternate_function<PinConfig>();
    }
    LL_GPIO_Init(gpio_port<gpio_port_index<PinConfig>()>(), &init);
#endif
  }

  template <typename PinConfig> static void write(const bool level) noexcept
  {
#if defined(GPIOA)
    if (level) [[likely]]
    {
      LL_GPIO_SetOutputPin(gpio_port<gpio_port_index<PinConfig>()>(), (1U << gpio_pin_index<PinConfig>()));
    }
    else [[unlikely]]
    {
      LL_GPIO_ResetOutputPin(gpio_port<gpio_port_index<PinConfig>()>(), (1U << gpio_pin_index<PinConfig>()));
    }
#else
    (void)level;
#endif
  }

  template <typename PinConfig> static bool read() noexcept
  {
#if defined(GPIOA)
    return LL_GPIO_IsInputPinSet(gpio_port<gpio_port_index<PinConfig>()>(), (1U << gpio_pin_index<PinConfig>())) != 0U;
#else
    return false;
#endif
  }

  template <typename PinConfig, auto Handler> static bool enable_interrupt() noexcept
  {
    (void)Handler;
#if defined(GPIOA)
    (void)sizeof(PinConfig);
    return false;
#else
    return false;
#endif
  }
};

struct i2c_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if MICROHAL_STM32_LL_HAS_I2C
    LL_I2C_InitTypeDef init{};
    init.PeripheralMode = LL_I2C_MODE_I2C;

#if defined(LL_I2C_ANALOGFILTER_ENABLE)
    init.AnalogFilter = LL_I2C_ANALOGFILTER_ENABLE;
#endif
#if defined(I2C_FLTR_DNF)
    init.DigitalFilter = 0U;
#endif

    init.OwnAddress1 = 0U;
    init.TypeAcknowledge = LL_I2C_ACK;
    init.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT;

    I2C_TypeDef* instance = peripheral_instance<I2C_TypeDef, Config>();
    if (LL_I2C_Init(instance, &init) == SUCCESS) [[likely]]
    {
      LL_I2C_Enable(instance);
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config>
  static bool write_blocking(const std::uint8_t addr, const std::uint8_t* data, const std::size_t length) noexcept
  {
#if MICROHAL_STM32_LL_HAS_I2C
    I2C_TypeDef* instance = peripheral_instance<I2C_TypeDef, Config>();
#if defined(LL_I2C_MODE_AUTOEND) && defined(LL_I2C_GENERATE_START_WRITE) && defined(LL_I2C_ADDRSLAVE_7BIT)
    LL_I2C_HandleTransfer(instance, addr, LL_I2C_ADDRSLAVE_7BIT, static_cast<std::uint32_t>(length),
                          LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);

    for (std::size_t i = 0U; i < length; ++i)
    {
      while (!LL_I2C_IsActiveFlag_TXIS(instance))
      {
      }
      LL_I2C_TransmitData8(instance, data[i]);
    }

    while (!LL_I2C_IsActiveFlag_STOP(instance))
    {
    }
    LL_I2C_ClearFlag_STOP(instance);
    return true;
#else
    LL_I2C_AcknowledgeNextData(instance, LL_I2C_ACK);
    LL_I2C_GenerateStartCondition(instance);
    while (!LL_I2C_IsActiveFlag_SB(instance))
    {
    }

    LL_I2C_TransmitData8(instance, static_cast<std::uint8_t>(addr << 1U));
    while (!LL_I2C_IsActiveFlag_ADDR(instance))
    {
    }
    LL_I2C_ClearFlag_ADDR(instance);

    for (std::size_t i = 0U; i < length; ++i)
    {
      while (!LL_I2C_IsActiveFlag_TXE(instance))
      {
      }
      LL_I2C_TransmitData8(instance, data[i]);
    }

    while (!LL_I2C_IsActiveFlag_BTF(instance))
    {
    }
    LL_I2C_GenerateStopCondition(instance);
    return true;
#endif
#else
    (void)addr;
    (void)data;
    (void)length;
    return false;
#endif
  }

  template <typename Config>
  static bool write_blocking(const std::span<const std::uint8_t> payload, const std::uint16_t address) noexcept
  {
    return write_blocking<Config>(static_cast<std::uint8_t>(address), payload.data(), payload.size());
  }

  template <typename Config>
  static bool read_blocking(const std::span<std::uint8_t> payload, const std::uint16_t address) noexcept
  {
#if MICROHAL_STM32_LL_HAS_I2C
    I2C_TypeDef* instance = peripheral_instance<I2C_TypeDef, Config>();
#if defined(LL_I2C_MODE_AUTOEND) && defined(LL_I2C_GENERATE_START_READ) && defined(LL_I2C_ADDRSLAVE_7BIT)
    LL_I2C_HandleTransfer(instance, static_cast<std::uint8_t>(address), LL_I2C_ADDRSLAVE_7BIT,
                          static_cast<std::uint32_t>(payload.size()), LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);

    for (std::size_t i = 0U; i < payload.size(); ++i)
    {
      while (!LL_I2C_IsActiveFlag_RXNE(instance))
      {
      }
      payload[i] = LL_I2C_ReceiveData8(instance);
    }

    while (!LL_I2C_IsActiveFlag_STOP(instance))
    {
    }
    LL_I2C_ClearFlag_STOP(instance);
    return true;
#else
    LL_I2C_AcknowledgeNextData(instance, LL_I2C_ACK);
    LL_I2C_GenerateStartCondition(instance);
    while (!LL_I2C_IsActiveFlag_SB(instance))
    {
    }

    LL_I2C_TransmitData8(instance, static_cast<std::uint8_t>((address << 1U) | 0x01U));
    while (!LL_I2C_IsActiveFlag_ADDR(instance))
    {
    }
    LL_I2C_ClearFlag_ADDR(instance);

    for (std::size_t i = 0U; i < payload.size(); ++i)
    {
      while (!LL_I2C_IsActiveFlag_RXNE(instance))
      {
      }
      payload[i] = LL_I2C_ReceiveData8(instance);
    }

    LL_I2C_GenerateStopCondition(instance);
    return true;
#endif
#else
    (void)payload;
    (void)address;
    return false;
#endif
  }

  template <typename Config, auto Handler>
  static bool start_transfer(const std::span<std::uint8_t> payload, const std::uint16_t address) noexcept
  {
    (void)Handler;
    return write_blocking<Config>(std::span<const std::uint8_t>{payload.data(), payload.size()}, address);
  }
};

struct spi_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if MICROHAL_STM32_LL_HAS_SPI
    LL_SPI_InitTypeDef init{};
    init.TransferDirection = LL_SPI_FULL_DUPLEX;
    init.Mode = LL_SPI_MODE_MASTER;
    init.DataWidth = LL_SPI_DATAWIDTH_8BIT;
    init.ClockPolarity = LL_SPI_POLARITY_LOW;
    init.ClockPhase = LL_SPI_PHASE_1EDGE;
    init.NSS = LL_SPI_NSS_SOFT;
    init.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV16;
    init.BitOrder = LL_SPI_MSB_FIRST;
    init.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;

    SPI_TypeDef* instance = peripheral_instance<SPI_TypeDef, Config>();
    if (LL_SPI_Init(instance, &init) == SUCCESS) [[likely]]
    {
      LL_SPI_Enable(instance);
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config>
  static bool transfer(const std::uint8_t* tx_data, std::uint8_t* rx_data, const std::size_t length) noexcept
  {
#if MICROHAL_STM32_LL_HAS_SPI
    SPI_TypeDef* instance = peripheral_instance<SPI_TypeDef, Config>();
    for (std::size_t i = 0U; i < length; ++i)
    {
      while (!LL_SPI_IsActiveFlag_TXE(instance))
      {
      }
      LL_SPI_TransmitData8(instance, tx_data[i]);

      while (!LL_SPI_IsActiveFlag_RXNE(instance))
      {
      }
      rx_data[i] = LL_SPI_ReceiveData8(instance);
    }
    return true;
#else
    (void)tx_data;
    (void)rx_data;
    (void)length;
    return false;
#endif
  }

  template <typename Config>
  static bool transfer(const std::span<const std::uint8_t> tx_payload,
                       const std::span<std::uint8_t> rx_payload) noexcept
  {
    const std::size_t length = (tx_payload.size() < rx_payload.size()) ? tx_payload.size() : rx_payload.size();
    return transfer<Config>(tx_payload.data(), rx_payload.data(), length);
  }

  template <typename Config, auto Handler>
  static bool transfer_async(const std::span<const std::uint8_t> tx_payload,
                             const std::span<std::uint8_t> rx_payload) noexcept
  {
    (void)tx_payload;
    (void)rx_payload;
    (void)Handler;
    return false;
  }
};

struct qspi_api final
{
#if MICROHAL_STM32_LL_HAS_QSPI
  template <typename Config> static auto& handle() noexcept
  {
    static QSPI_HandleTypeDef handle_storage{};
    return handle_storage;
  }
#endif

  template <typename Config> static bool initialize() noexcept
  {
#if MICROHAL_STM32_LL_HAS_QSPI
    auto& handle_storage = handle<Config>();
    handle_storage.Instance = stm32_qspi_instance<Config>();
    handle_storage.Init.ClockPrescaler = 1U;
    handle_storage.Init.FifoThreshold = 4U;
    handle_storage.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    handle_storage.Init.FlashSize = 23U;
    handle_storage.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE;
    handle_storage.Init.ClockMode = QSPI_CLOCK_MODE_0;

    return HAL_QSPI_Init(&handle_storage) == HAL_OK;
#else
    return false;
#endif
  }
};

struct uart_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if MICROHAL_STM32_LL_HAS_UART
    LL_USART_InitTypeDef init{};
#if defined(LL_USART_PRESCALER_DIV1)
    init.PrescalerValue = LL_USART_PRESCALER_DIV1;
#endif
    init.BaudRate = Config::baud_rate;
    init.DataWidth = LL_USART_DATAWIDTH_8B;
    init.StopBits = LL_USART_STOPBITS_1;
    init.Parity = LL_USART_PARITY_NONE;
    init.TransferDirection = LL_USART_DIRECTION_TX_RX;
    init.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    init.OverSampling = LL_USART_OVERSAMPLING_16;

    USART_TypeDef* instance = peripheral_instance<USART_TypeDef, Config>();
    if (LL_USART_Init(instance, &init) == SUCCESS) [[likely]]
    {
      LL_USART_Enable(instance);
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config> static bool write_blocking(std::span<const std::uint8_t> payload) noexcept
  {
#if MICROHAL_STM32_LL_HAS_UART
    USART_TypeDef* instance = peripheral_instance<USART_TypeDef, Config>();
    for (const std::uint8_t byte : payload)
    {
      while (!LL_USART_IsActiveFlag_TXE(instance))
      {
      }
      LL_USART_TransmitData8(instance, byte);
    }
    while (!LL_USART_IsActiveFlag_TC(instance))
    {
    }
    return true;
#else
    (void)payload;
    return false;
#endif
  }

  template <typename Config> static bool read_blocking(std::span<std::uint8_t> payload) noexcept
  {
#if MICROHAL_STM32_LL_HAS_UART
    USART_TypeDef* instance = peripheral_instance<USART_TypeDef, Config>();
    for (std::uint8_t& byte : payload)
    {
      while (!LL_USART_IsActiveFlag_RXNE(instance))
      {
      }
      byte = LL_USART_ReceiveData8(instance);
    }
    return true;
#else
    (void)payload;
    return false;
#endif
  }

  template <typename Config, auto Handler> static bool start_async_rx(std::span<std::uint8_t> payload) noexcept
  {
#if MICROHAL_STM32_LL_HAS_UART
    USART_TypeDef* instance = peripheral_instance<USART_TypeDef, Config>();
    LL_USART_EnableIT_RXNE(instance);
    return true;
#else
    (void)payload;
    (void)Handler;
    return false;
#endif
  }

  template <typename Config, auto Handler> static bool start_async_tx(std::span<const std::uint8_t> payload) noexcept
  {
#if MICROHAL_STM32_LL_HAS_UART
    USART_TypeDef* instance = peripheral_instance<USART_TypeDef, Config>();
    LL_USART_EnableIT_TXE(instance);
    (void)payload;
    (void)Handler;
    return true;
#else
    (void)payload;
    (void)Handler;
    return false;
#endif
  }

  template <typename UartConfig, typename AutobaudConfig> static bool arm_autobaud() noexcept
  {
    (void)sizeof(UartConfig);
    (void)sizeof(AutobaudConfig);
    return false;
  }

  template <typename UartConfig> static bool apply_autobaud(const std::uint32_t baud_rate) noexcept
  {
    (void)sizeof(UartConfig);
    (void)baud_rate;
    return false;
  }
};

struct rtc_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(RTC)
    LL_RTC_InitTypeDef init{};
    init.HourFormat = LL_RTC_HOURFORMAT_24HOUR;
    init.AsynchPrescaler = 127U;
    init.SynchPrescaler = 255U;

    if (LL_RTC_Init(RTC, &init) == SUCCESS) [[likely]]
    {
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config>
  static bool set_time(const std::uint8_t hours, const std::uint8_t minutes, const std::uint8_t seconds) noexcept
  {
#if defined(RTC)
    LL_RTC_TimeTypeDef time{};
    time.Hours = hours;
    time.Minutes = minutes;
    time.Seconds = seconds;
    time.TimeFormat = LL_RTC_TIME_FORMAT_AM_OR_24;

    if (LL_RTC_TIME_Init(RTC, LL_RTC_FORMAT_BIN, &time) == SUCCESS) [[likely]]
    {
      return true;
    }
    return false;
#else
    (void)hours;
    (void)minutes;
    (void)seconds;
    return false;
#endif
  }

  template <typename Config>
  static bool get_time(std::uint8_t& hours, std::uint8_t& minutes, std::uint8_t& seconds) noexcept
  {
#if defined(RTC)
    hours = LL_RTC_TIME_GetHour(RTC);
    minutes = LL_RTC_TIME_GetMinute(RTC);
    seconds = LL_RTC_TIME_GetSecond(RTC);
    return true;
#else
    (void)hours;
    (void)minutes;
    (void)seconds;
    return false;
#endif
  }

  template <typename Config, auto Handler, typename AlarmType>
  static bool enable_alarm(const AlarmType& alarm_time) noexcept
  {
    (void)sizeof(Config);
    (void)Handler;
    (void)alarm_time;
    return false;
  }
};

struct flash_api final
{
  template <typename Config> static bool erase(const std::uint32_t address, const std::uint32_t bytes) noexcept
  {
#if defined(FLASH)
    // Note: LL Flash typically requires HAL or manual register access
    // This is a simplified placeholder
    (void)address;
    (void)bytes;
    return false;
#else
    (void)address;
    (void)bytes;
    return false;
#endif
  }

  template <typename Config>
  static bool program(const std::uint32_t address, const std::span<const std::uint8_t> data) noexcept
  {
#if defined(FLASH)
    // LL Flash programming requires careful register manipulation
    (void)address;
    (void)data;
    return false;
#else
    (void)address;
    (void)data;
    return false;
#endif
  }

  template <typename Config> static bool read(const std::uint32_t address, const std::span<std::uint8_t> data) noexcept
  {
    (void)sizeof(Config);
    (void)address;
    (void)data;
    return false;
  }
};

struct timer_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if MICROHAL_STM32_LL_HAS_TIM
    LL_TIM_InitTypeDef init{};
    init.Prescaler = Config::prescaler;
    init.CounterMode = LL_TIM_COUNTERMODE_UP;
    init.Autoreload = Config::period;
    init.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;

    TIM_TypeDef* instance = peripheral_instance<TIM_TypeDef, Config>();
    if (LL_TIM_Init(instance, &init) == SUCCESS) [[likely]]
    {
      LL_TIM_EnableCounter(instance);
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config> static std::uint32_t get_counter() noexcept
  {
#if MICROHAL_STM32_LL_HAS_TIM
    TIM_TypeDef* instance = peripheral_instance<TIM_TypeDef, Config>();
    return LL_TIM_GetCounter(instance);
#else
    return 0U;
#endif
  }

  template <typename Config> static std::uint32_t counter() noexcept
  {
    return get_counter<Config>();
  }
};

struct adc_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if MICROHAL_STM32_LL_HAS_ADC
    LL_ADC_InitTypeDef init{};
    init.Resolution = LL_ADC_RESOLUTION_12B;
    init.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;
#if defined(LL_ADC_LP_MODE_NONE)
    init.LowPowerMode = LL_ADC_LP_MODE_NONE;
#endif

    ADC_TypeDef* instance = peripheral_instance<ADC_TypeDef, Config>();
    if (LL_ADC_Init(instance, &init) == SUCCESS) [[likely]]
    {
      LL_ADC_Enable(instance);
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config> static std::uint16_t read_blocking(const std::uint8_t channel) noexcept
  {
#if MICROHAL_STM32_LL_HAS_ADC
    ADC_TypeDef* instance = peripheral_instance<ADC_TypeDef, Config>();
    LL_ADC_REG_SetSequencerRanks(instance, LL_ADC_REG_RANK_1, channel);
#if defined(STM32F2) || defined(STM32F4) || defined(STM32F7)
    LL_ADC_REG_StartConversionSWStart(instance);
#else
    LL_ADC_REG_StartConversion(instance);
#endif
#if defined(STM32F2) || defined(STM32F4) || defined(STM32F7)
    while (!LL_ADC_IsActiveFlag_EOCS(instance))
    {
    }
#else
    while (!LL_ADC_IsActiveFlag_EOC(instance))
    {
    }
#endif
    return static_cast<std::uint16_t>(LL_ADC_REG_ReadConversionData12(instance));
#else
    (void)channel;
    return 0U;
#endif
  }

  template <typename Config, auto Handler> static bool start_interrupt(const std::uint8_t channel) noexcept
  {
#if MICROHAL_STM32_LL_HAS_ADC
    ADC_TypeDef* instance = peripheral_instance<ADC_TypeDef, Config>();
    LL_ADC_REG_SetSequencerRanks(instance, LL_ADC_REG_RANK_1, channel);
#if defined(STM32F2) || defined(STM32F4) || defined(STM32F7)
    LL_ADC_EnableIT_EOCS(instance);
    LL_ADC_REG_StartConversionSWStart(instance);
#else
    LL_ADC_EnableIT_EOC(instance);
    LL_ADC_REG_StartConversion(instance);
#endif
    return true;
#else
    (void)channel;
    (void)Handler;
    return false;
#endif
  }

  template <typename Config, auto Handler>
  static bool start_dma(const std::uint8_t channel, std::span<std::uint16_t> buffer) noexcept
  {
#if MICROHAL_STM32_LL_HAS_ADC
    ADC_TypeDef* instance = peripheral_instance<ADC_TypeDef, Config>();
    LL_ADC_REG_SetSequencerRanks(instance, LL_ADC_REG_RANK_1, channel);
    LL_ADC_REG_SetDMATransfer(instance, LL_ADC_REG_DMA_TRANSFER_LIMITED);
#if defined(STM32F2) || defined(STM32F4) || defined(STM32F7)
    LL_ADC_REG_StartConversionSWStart(instance);
#else
    LL_ADC_REG_StartConversion(instance);
#endif
    return true;
#else
    (void)channel;
    (void)buffer;
    (void)Handler;
    return false;
#endif
  }

  template <typename Config> static bool start_continuous(const std::span<const std::uint8_t> channels) noexcept
  {
    (void)channels;
#if MICROHAL_STM32_LL_HAS_ADC
    ADC_TypeDef* instance = peripheral_instance<ADC_TypeDef, Config>();
#if defined(STM32F2) || defined(STM32F4) || defined(STM32F7)
    LL_ADC_REG_StartConversionSWStart(instance);
#else
    LL_ADC_REG_StartConversion(instance);
#endif
    return true;
#else
    return false;
#endif
  }

  template <typename Config, auto Handler> static bool start_async(const std::uint8_t channel) noexcept
  {
    return start_interrupt<Config, Handler>(channel);
  }

  template <typename Config, auto Handler>
  static bool start_dma_scan(const std::span<const std::uint8_t> channels, const std::span<std::uint16_t> results) noexcept
  {
    if (channels.empty())
    {
      return false;
    }
    return start_dma<Config, Handler>(channels.front(), std::span<std::uint16_t>{results.data(), results.size()});
  }
};

struct dac_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if MICROHAL_STM32_LL_HAS_DAC
    LL_DAC_InitTypeDef init{};
    init.TriggerSource = LL_DAC_TRIG_SOFTWARE;
    init.WaveAutoGeneration = LL_DAC_WAVE_AUTO_GENERATION_NONE;
    init.OutputBuffer = LL_DAC_OUTPUT_BUFFER_ENABLE;

    if (LL_DAC_Init(DAC1, LL_DAC_CHANNEL_1, &init) == SUCCESS) [[likely]]
    {
      LL_DAC_Enable(DAC1, LL_DAC_CHANNEL_1);
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config> static void write(const std::uint16_t value, const std::uint8_t channel) noexcept
  {
#if MICROHAL_STM32_LL_HAS_DAC
    const std::uint32_t ch = (channel == 1U) ? LL_DAC_CHANNEL_1 : LL_DAC_CHANNEL_2;
    LL_DAC_ConvertData12RightAligned(DAC1, ch, value);
    LL_DAC_TrigSWConversion(DAC1, ch);
#else
    (void)value;
    (void)channel;
#endif
  }

  template <typename Config> static void write_dual(const std::uint16_t value1, const std::uint16_t value2) noexcept
  {
#if MICROHAL_STM32_LL_HAS_DAC
    LL_DAC_ConvertDualData12RightAligned(DAC1, value1, value2);
    LL_DAC_TrigSWConversion(DAC1, LL_DAC_CHANNEL_1);
#else
    (void)value1;
    (void)value2;
#endif
  }

  template <typename Config, auto Handler>
  static bool start_dma(const std::span<const std::uint16_t> data, const std::uint8_t channel) noexcept
  {
#if MICROHAL_STM32_LL_HAS_DAC
    const std::uint32_t ch = (channel == 1U) ? LL_DAC_CHANNEL_1 : LL_DAC_CHANNEL_2;
    LL_DAC_EnableDMAReq(DAC1, ch);
    return true;
#else
    (void)data;
    (void)channel;
    (void)Handler;
    return false;
#endif
  }

  template <typename Config> static bool write_blocking(const std::uint16_t value) noexcept
  {
    write<Config>(value, Config::channel);
    return true;
  }

  template <typename Config, auto Handler>
  static bool start_dma_waveform(const std::span<const std::uint16_t> samples) noexcept
  {
    return start_dma<Config, Handler>(samples, Config::channel);
  }
};

struct can_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    // Note: CAN typically uses HAL, not LL in STM32 ecosystem
#if defined(CAN1)
    return false; // LL_CAN not commonly available
#else
    return false;
#endif
  }

  template <typename Config> static bool transmit(const std::uint8_t* data, std::size_t length) noexcept
  {
#if defined(CAN1)
    (void)data;
    (void)length;
    return false;
#else
    (void)data;
    (void)length;
    return false;
#endif
  }

  template <typename Config> static bool receive(std::uint8_t* data, std::size_t& length) noexcept
  {
#if defined(CAN1)
    (void)data;
    (void)length;
    return false;
#else
    (void)data;
    (void)length;
    return false;
#endif
  }

  template <typename Config>
  static bool configure_filter(const std::uint32_t id, const std::uint32_t mask, const std::uint8_t fifo) noexcept
  {
#if defined(CAN1)
    (void)id;
    (void)mask;
    (void)fifo;
    return false;
#else
    (void)id;
    (void)mask;
    (void)fifo;
    return false;
#endif
  }

  template <typename Config, auto Handler> static bool enable_rx_interrupt() noexcept
  {
#if defined(CAN1)
    (void)Handler;
    return false;
#else
    (void)Handler;
    return false;
#endif
  }
};

struct usb_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    // Note: USB typically uses HAL, not LL
#if defined(USB_OTG_FS) || defined(USB)
    return false; // LL_USB not commonly available
#else
    return false;
#endif
  }

  template <typename Config> static bool connect() noexcept
  {
#if defined(USB_OTG_FS) || defined(USB)
    return false;
#else
    return false;
#endif
  }

  template <typename Config> static bool disconnect() noexcept
  {
#if defined(USB_OTG_FS) || defined(USB)
    return false;
#else
    return false;
#endif
  }

  template <typename Config>
  static bool endpoint_transmit(const std::uint8_t endpoint, const std::span<const std::uint8_t> data) noexcept
  {
#if defined(USB_OTG_FS) || defined(USB)
    (void)endpoint;
    (void)data;
    return false;
#else
    (void)endpoint;
    (void)data;
    return false;
#endif
  }

  template <typename Config>
  static std::uint32_t endpoint_receive(const std::uint8_t endpoint, const std::span<std::uint8_t> buffer) noexcept
  {
#if defined(USB_OTG_FS) || defined(USB)
    (void)endpoint;
    (void)buffer;
    return 0U;
#else
    (void)endpoint;
    (void)buffer;
    return 0U;
#endif
  }
};

struct ethernet_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    return false;
  }

  template <typename Config> static bool start() noexcept
  {
    return false;
  }

  template <typename Config> static bool stop() noexcept
  {
    return false;
  }

  template <typename Config> static bool transmit_frame(const std::span<const std::uint8_t> frame) noexcept
  {
    (void)frame;
    return false;
  }

  template <typename Config> static std::uint32_t receive_frame(const std::span<std::uint8_t> buffer) noexcept
  {
    (void)buffer;
    return 0U;
  }

  template <typename Config> static bool set_mac_address(const std::array<std::uint8_t, 6>& address) noexcept
  {
    (void)address;
    return false;
  }
};

struct sd_api final
{
#if MICROHAL_STM32_LL_HAS_SD
  template <typename Config> static auto& handle() noexcept
  {
    static SD_HandleTypeDef handle_storage{};
    return handle_storage;
  }

  template <typename Config> static auto& card_info() noexcept
  {
    static HAL_SD_CardInfoTypeDef card_info_storage{};
    return card_info_storage;
  }
#endif

  template <typename Config> static bool initialize() noexcept
  {
#if MICROHAL_STM32_LL_HAS_SD
    auto& handle_storage = handle<Config>();
    auto& card = card_info<Config>();

    handle_storage.Instance = stm32_sd_instance<Config>();
#if defined(SDIO_CLOCK_EDGE_RISING)
    handle_storage.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
    handle_storage.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
    handle_storage.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
    handle_storage.Init.HardwareFlowControl =
        sd_hardware_flow_control<Config>() ? SDIO_HARDWARE_FLOW_CONTROL_ENABLE : SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
#else
    handle_storage.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
    handle_storage.Init.ClockBypass = SDMMC_CLOCK_BYPASS_DISABLE;
    handle_storage.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
    handle_storage.Init.HardwareFlowControl = sd_hardware_flow_control<Config>() ? SDMMC_HARDWARE_FLOW_CONTROL_ENABLE
                                                                                  : SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
#endif
    handle_storage.Init.BusWide = stm32_sd_bus_width<Config>();
    handle_storage.Init.ClockDiv = sd_clock_divider<Config>();

    if (HAL_SD_Init(&handle_storage) != HAL_OK) [[unlikely]]
    {
      return false;
    }

    if (HAL_SD_ConfigWideBusOperation(&handle_storage, stm32_sd_bus_width<Config>()) != HAL_OK) [[unlikely]]
    {
      return false;
    }

    return HAL_SD_GetCardInfo(&handle_storage, &card) == HAL_OK;
#else
    return false;
#endif
  }

  template <typename Config>
  static bool read_blocks(const std::uint32_t start_block, const std::span<std::uint8_t> buffer) noexcept
  {
#if MICROHAL_STM32_LL_HAS_SD
    auto& handle_storage = handle<Config>();
    constexpr std::uint32_t block_size = 512U;

    if (buffer.empty() || ((buffer.size() % block_size) != 0U))
    {
      return false;
    }

    return HAL_SD_ReadBlocks(&handle_storage, buffer.data(), start_block,
                             static_cast<std::uint32_t>(buffer.size() / block_size), 1000U) == HAL_OK;
#else
    (void)start_block;
    (void)buffer;
    return false;
#endif
  }

  template <typename Config>
  static bool write_blocks(const std::uint32_t start_block, const std::span<const std::uint8_t> buffer) noexcept
  {
#if MICROHAL_STM32_LL_HAS_SD
    auto& handle_storage = handle<Config>();
    constexpr std::uint32_t block_size = 512U;

    if (buffer.empty() || ((buffer.size() % block_size) != 0U))
    {
      return false;
    }

    return HAL_SD_WriteBlocks(&handle_storage, hal_tx_buffer(buffer.data()), start_block,
                              static_cast<std::uint32_t>(buffer.size() / block_size), 1000U) == HAL_OK;
#else
    (void)start_block;
    (void)buffer;
    return false;
#endif
  }

  template <typename Config>
  static bool erase_blocks(const std::uint32_t start_block, const std::uint32_t block_count) noexcept
  {
#if MICROHAL_STM32_LL_HAS_SD
    auto& handle_storage = handle<Config>();

    if (block_count == 0U)
    {
      return false;
    }

    return HAL_SD_Erase(&handle_storage, start_block, start_block + block_count - 1U) == HAL_OK;
#else
    (void)start_block;
    (void)block_count;
    return false;
#endif
  }

  template <typename Config> static std::uint32_t capacity_blocks() noexcept
  {
#if MICROHAL_STM32_LL_HAS_SD
    auto& handle_storage = handle<Config>();
    auto& card = card_info<Config>();

    if (HAL_SD_GetCardInfo(&handle_storage, &card) != HAL_OK) [[unlikely]]
    {
      return 0U;
    }

    return card.LogBlockNbr;
#else
    return 0U;
#endif
  }
};

struct dma_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(DMA1) && MICROHAL_STM32_LL_DMA_RUNTIME
    if constexpr (Config::double_buffer)
    {
      return false;
    }

    LL_DMA_InitTypeDef init = dma_init<Config>(0U, 0U, 0U);
    return LL_DMA_Init(dma_controller<Config>(), dma_ll_target<Config>(), &init) == SUCCESS;
#else
    (void)sizeof(Config);
    return false;
#endif
  }

  template <typename Config, auto Handler>
  static bool start_transfer(const std::uint32_t src_addr, const std::uint32_t dst_addr,
                             const std::uint32_t length) noexcept
  {
#if defined(DMA1) && MICROHAL_STM32_LL_DMA_RUNTIME
    if constexpr (Config::double_buffer)
    {
      (void)Handler;
      return false;
    }

    if (length == 0U)
    {
      (void)Handler;
      return false;
    }

    dma_disable<Config>();

    LL_DMA_InitTypeDef init = dma_init<Config>(src_addr, dst_addr, length);
    if (LL_DMA_Init(dma_controller<Config>(), dma_ll_target<Config>(), &init) != SUCCESS) [[unlikely]]
    {
      (void)Handler;
      return false;
    }

    dma_enable<Config>();
    (void)Handler;
    return true;
#else
    (void)src_addr;
    (void)dst_addr;
    (void)length;
    (void)Handler;
    return false;
#endif
  }

  template <typename Config> static bool abort() noexcept
  {
#if defined(DMA1) && MICROHAL_STM32_LL_DMA_RUNTIME
    dma_disable<Config>();
    return true;
#else
    return false;
#endif
  }
};

struct pwm_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if MICROHAL_STM32_LL_HAS_TIM
    // Timer base init
    LL_TIM_InitTypeDef init{};
    init.Prescaler = Config::prescaler;
    init.CounterMode = LL_TIM_COUNTERMODE_UP;
    init.Autoreload = Config::period;
    init.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;

    TIM_TypeDef* instance = peripheral_instance<TIM_TypeDef, Config>();
    if (LL_TIM_Init(instance, &init) == SUCCESS) [[likely]]
    {
      // PWM mode setup
      LL_TIM_OC_InitTypeDef oc_init{};
      oc_init.OCMode = LL_TIM_OCMODE_PWM1;
      oc_init.OCState = LL_TIM_OCSTATE_ENABLE;
      oc_init.CompareValue = 0U;
      oc_init.OCPolarity = LL_TIM_OCPOLARITY_HIGH;

      if (LL_TIM_OC_Init(instance, LL_TIM_CHANNEL_CH1, &oc_init) == SUCCESS) [[likely]]
      {
        return true;
      }
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config> static void set_duty_cycle(const std::uint16_t duty_permille) noexcept
  {
#if MICROHAL_STM32_LL_HAS_TIM
    TIM_TypeDef* instance = peripheral_instance<TIM_TypeDef, Config>();
    const std::uint32_t period = LL_TIM_GetAutoReload(instance);
    const std::uint32_t compare_value = (period * duty_permille) / 1000U;
    LL_TIM_OC_SetCompareCH1(instance, compare_value);
#else
    (void)duty_permille;
#endif
  }

  template <typename Config> static bool start() noexcept
  {
#if MICROHAL_STM32_LL_HAS_TIM
    TIM_TypeDef* instance = peripheral_instance<TIM_TypeDef, Config>();
    LL_TIM_CC_EnableChannel(instance, LL_TIM_CHANNEL_CH1);
    LL_TIM_EnableCounter(instance);
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static bool stop() noexcept
  {
#if MICROHAL_STM32_LL_HAS_TIM
    TIM_TypeDef* instance = peripheral_instance<TIM_TypeDef, Config>();
    LL_TIM_CC_DisableChannel(instance, LL_TIM_CHANNEL_CH1);
    LL_TIM_DisableCounter(instance);
    return true;
#else
    return false;
#endif
  }
};

struct watchdog_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(IWDG) && defined(LL_IWDG_PRESCALER_32)
    LL_IWDG_EnableWriteAccess(IWDG);
    LL_IWDG_SetPrescaler(IWDG, LL_IWDG_PRESCALER_32);
    LL_IWDG_SetReloadCounter(IWDG, 4095U);
    while (LL_IWDG_IsReady(IWDG) != 1U)
    {
    }
    LL_IWDG_Enable(IWDG);
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static bool start() noexcept
  {
#if defined(IWDG) && defined(LL_IWDG_PRESCALER_32)
    LL_IWDG_Enable(IWDG);
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static bool refresh() noexcept
  {
#if defined(IWDG) && defined(LL_IWDG_PRESCALER_32)
    LL_IWDG_ReloadCounter(IWDG);
    return true;
#else
    return false;
#endif
  }
};

struct i2s_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if MICROHAL_STM32_LL_HAS_SPI && defined(SPI_I2SCFGR_I2SMOD)
    LL_I2S_InitTypeDef init{};
    init.Mode = LL_I2S_MODE_MASTER_TX;
    init.Standard = LL_I2S_STANDARD_PHILIPS;
    init.DataFormat = LL_I2S_DATAFORMAT_16B;
    init.MCLKOutput = LL_I2S_MCLK_OUTPUT_DISABLE;
    init.AudioFreq = LL_I2S_AUDIOFREQ_48K;
    init.ClockPolarity = LL_I2S_POLARITY_LOW;

    SPI_TypeDef* instance = peripheral_instance<SPI_TypeDef, Config>();
    if (LL_I2S_Init(instance, &init) == SUCCESS) [[likely]]
    {
      LL_I2S_Enable(instance);
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config> static bool transmit_blocking(const std::span<const std::uint16_t> samples) noexcept
  {
#if MICROHAL_STM32_LL_HAS_SPI && defined(SPI_I2SCFGR_I2SMOD)
    SPI_TypeDef* instance = peripheral_instance<SPI_TypeDef, Config>();
    for (const std::uint16_t sample : samples)
    {
      while (!LL_I2S_IsActiveFlag_TXE(instance))
      {
      }
      LL_I2S_TransmitData16(instance, sample);
    }
    return true;
#else
    (void)samples;
    return false;
#endif
  }

  template <typename Config> static bool receive_blocking(const std::span<std::uint16_t> samples) noexcept
  {
#if MICROHAL_STM32_LL_HAS_SPI && defined(SPI_I2SCFGR_I2SMOD)
    SPI_TypeDef* instance = peripheral_instance<SPI_TypeDef, Config>();
    for (std::uint16_t& sample : samples)
    {
      while (!LL_I2S_IsActiveFlag_RXNE(instance))
      {
      }
      sample = LL_I2S_ReceiveData16(instance);
    }
    return true;
#else
    (void)samples;
    return false;
#endif
  }

  template <typename Config, auto Handler>
  static bool start_dma_tx(const std::span<const std::uint16_t> samples) noexcept
  {
#if MICROHAL_STM32_LL_HAS_SPI && defined(SPI_I2SCFGR_I2SMOD)
    SPI_TypeDef* instance = peripheral_instance<SPI_TypeDef, Config>();
    LL_I2S_EnableDMAReq_TX(instance);
    return true;
#else
    (void)samples;
    (void)Handler;
    return false;
#endif
  }
};

struct rcc_api final
{
  template <typename Config> static bool configure() noexcept
  {
#if defined(RCC) && defined(LL_RCC_PLLSOURCE_HSE) && defined(LL_RCC_PLLM_DIV_8) && defined(LL_RCC_PLLP_DIV_2) &&       \
    defined(LL_RCC_APB2_DIV_2) && defined(LL_RCC_SYS_CLKSOURCE_PLL) && defined(LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
    // Example HSE + PLL configuration using LL
    LL_RCC_HSE_Enable();
    while (LL_RCC_HSE_IsReady() != 1U)
    {
    }

    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_8, 336U, LL_RCC_PLLP_DIV_2);
    LL_RCC_PLL_Enable();
    while (LL_RCC_PLL_IsReady() != 1U)
    {
    }

    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_4);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);

    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
    {
    }

    return true;
#else
    return false;
#endif
  }

  template <typename Config> static std::uint32_t get_system_clock_hz() noexcept
  {
#if defined(RCC)
    LL_RCC_ClocksTypeDef clocks{};
    LL_RCC_GetSystemClocksFreq(&clocks);
    return clocks.SYSCLK_Frequency;
#else
    return 0U;
#endif
  }

  template <typename Config>
  static void enable_peripheral_clock(const peripheral_kind peripheral, const std::uint8_t instance) noexcept
  {
#if defined(RCC)
    // Example: map peripheral to LL clock enable calls
    // __LL_RCC_GPIOA_CLK_ENABLE(), etc.
    (void)peripheral;
    (void)instance;
#else
    (void)peripheral;
    (void)instance;
#endif
  }

  template <typename Config>
  static void disable_peripheral_clock(const peripheral_kind peripheral, const std::uint8_t instance) noexcept
  {
#if defined(RCC)
    (void)peripheral;
    (void)instance;
#else
    (void)peripheral;
    (void)instance;
#endif
  }
};

struct crc_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(CRC)
    LL_CRC_ResetCRCCalculationUnit(CRC);

#if defined(RNG)
    if constexpr (Config::enable_rng)
    {
      LL_RNG_Enable(RNG);
    }
#endif

    return true;
#else
    (void)sizeof(Config);
    return false;
#endif
  }

  template <typename Config> static std::uint32_t calculate(const std::span<const std::uint8_t> data) noexcept
  {
#if defined(CRC)
    LL_CRC_ResetCRCCalculationUnit(CRC);
    for (std::size_t i = 0U; i < data.size(); i += 4U)
    {
      std::uint32_t word = 0U;
      for (std::size_t j = 0U; j < 4U && (i + j) < data.size(); ++j)
      {
        word |= static_cast<std::uint32_t>(data[i + j]) << (j * 8U);
      }
      LL_CRC_FeedData32(CRC, word);
    }
    return LL_CRC_ReadData32(CRC);
#else
    (void)data;
    return 0U;
#endif
  }

  template <typename Config> static void reset() noexcept
  {
#if defined(CRC)
    LL_CRC_ResetCRCCalculationUnit(CRC);
#endif
  }

  template <typename Config> static std::uint32_t get_random() noexcept
  {
#if defined(RNG)
    if constexpr (!Config::enable_rng)
    {
      return 0U;
    }

    while (!LL_RNG_IsActiveFlag_DRDY(RNG))
    {
      if (LL_RNG_IsActiveFlag_CECS(RNG) || LL_RNG_IsActiveFlag_SECS(RNG)) [[unlikely]]
      {
        return 0U;
      }
    }

    return LL_RNG_ReadRandData32(RNG);
#else
    (void)sizeof(Config);
    return 0U;
#endif
  }
};

} // namespace microhal::backends::stm32::ll

namespace microhal
{

template <> struct backend_traits<stm32_ll_tag> final
{
  using backend_tag = stm32_ll_tag;
  using gpio_api = backends::stm32::ll::gpio_api;
  using i2c_api = backends::stm32::ll::i2c_api;
  using spi_api = backends::stm32::ll::spi_api;
  using qspi_api = backends::stm32::ll::qspi_api;
  using uart_api = backends::stm32::ll::uart_api;
  using rtc_api = backends::stm32::ll::rtc_api;
  using flash_api = backends::stm32::ll::flash_api;
  using timer_api = backends::stm32::ll::timer_api;
  using adc_api = backends::stm32::ll::adc_api;
  using dac_api = backends::stm32::ll::dac_api;
  using can_api = backends::stm32::ll::can_api;
  using usb_api = backends::stm32::ll::usb_api;
  using ethernet_api = backends::stm32::ll::ethernet_api;
  using sd_api = backends::stm32::ll::sd_api;
  using dma_api = backends::stm32::ll::dma_api;
  using pwm_api = backends::stm32::ll::pwm_api;
  using watchdog_api = backends::stm32::ll::watchdog_api;
  using i2s_api = backends::stm32::ll::i2s_api;
  using rcc_api = backends::stm32::ll::rcc_api;
  using crc_api = backends::stm32::ll::crc_api;

  static constexpr vendor_kind vendor = vendor_kind::stm32;
  static constexpr backend_kind kind = backend_kind::stm32_cube_ll;
  static constexpr std::string_view name{"stm32-cube-ll"};
  static constexpr std::uint32_t gpio_port_count{
      (MICROHAL_STM32_LL_FAMILY_SELECTED != 0) ? backends::stm32::ll::family_gpio_port_count : 11U};
  static constexpr std::uint32_t gpio_pins_per_port{16U};
  static constexpr std::uint32_t gpio_alternate_function_count{16U};

  static consteval struct gpio_capability gpio_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = gpio_port_count,
            .max_frequency_hz = 0U,
            .alternate_function = true};
  }

  static consteval struct i2c_capability i2c_capability() noexcept
  {
    return {.supported =
                (MICROHAL_STM32_LL_FAMILY_SELECTED != 0) ? (backends::stm32::ll::i2c_instance_count != 0U) : true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = (MICROHAL_STM32_LL_FAMILY_SELECTED != 0) ? backends::stm32::ll::i2c_instance_count : 4U,
            .max_frequency_hz = 1'000'000U};
  }

  static consteval struct spi_capability spi_capability() noexcept
  {
    return {.supported =
                (MICROHAL_STM32_LL_FAMILY_SELECTED != 0) ? (backends::stm32::ll::spi_instance_count != 0U) : true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = (MICROHAL_STM32_LL_FAMILY_SELECTED != 0) ? backends::stm32::ll::spi_instance_count : 6U,
            .max_frequency_hz = 100'000'000U};
  }

  static consteval struct qspi_capability qspi_capability() noexcept
  {
      return {.supported = (MICROHAL_STM32_LL_HAS_QSPI != 0),
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
        .instance_count = (MICROHAL_STM32_LL_HAS_QSPI != 0) ? 1U : 0U,
            .max_frequency_hz = 133'000'000U,
            .quad_mode = true};
  }

  static consteval struct uart_capability uart_capability() noexcept
  {
    if constexpr (MICROHAL_STM32_LL_FAMILY_SELECTED != 0)
    {
      return {.supported = true,
              .blocking = true,
              .non_blocking = true,
              .isr = true,
              .dma = true,
              .instance_count = backends::stm32::ll::uart_instance_count,
              .max_frequency_hz = 12'500'000U,
              .fifo = true,
              .lpuart = backends::stm32::ll::family_has_lpuart,
              .autobaud = true,
              .fifo_depth = 16U};
    }

    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 10U,
            .max_frequency_hz = 12'500'000U,
            .fifo = true,
            .lpuart = true,
            .autobaud = true,
            .fifo_depth = 16U};
  }

  static consteval struct rtc_capability rtc_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .periodic = true};
  }

  static consteval struct flash_capability flash_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .alignment_bytes = 8U,
            .erase_bytes = 2'048U};
  }

  static consteval struct timer_capability timer_capability() noexcept
  {
    return {.supported =
                (MICROHAL_STM32_LL_FAMILY_SELECTED != 0) ? (backends::stm32::ll::timer_instance_count != 0U) : true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count =
                (MICROHAL_STM32_LL_FAMILY_SELECTED != 0) ? backends::stm32::ll::timer_instance_count : 14U,
            .max_frequency_hz = 200'000'000U,
            .capture_compare = true,
            .periodic = true};
  }

  static consteval struct adc_capability adc_capability() noexcept
  {
    return {.supported =
                (MICROHAL_STM32_LL_FAMILY_SELECTED != 0) ? (backends::stm32::ll::adc_instance_count != 0U) : true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = (MICROHAL_STM32_LL_FAMILY_SELECTED != 0) ? backends::stm32::ll::adc_instance_count : 3U,
            .max_frequency_hz = 36'000'000U,
            .resolution_bits = 12U,
            .channel_count = 16U,
            .differential = true,
            .oversampling = true};
  }

  static consteval struct dac_capability dac_capability() noexcept
  {
    return {.supported =
                (MICROHAL_STM32_LL_FAMILY_SELECTED != 0) ? (backends::stm32::ll::dac_instance_count != 0U) : true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = (MICROHAL_STM32_LL_FAMILY_SELECTED != 0) ? backends::stm32::ll::dac_instance_count : 1U,
            .max_frequency_hz = 0U,
            .resolution_bits = 12U,
            .dual_mode = true,
            .waveform_generation = true};
  }

  static consteval struct can_capability can_capability() noexcept
  {
    return {};
  }

  static consteval struct usb_capability usb_capability() noexcept
  {
    return {};
  }

  static consteval struct ethernet_capability ethernet_capability() noexcept
  {
    return {.supported = false,
            .blocking = false,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 0U,
            .max_frequency_hz = 0U,
            .checksum_offload = false,
            .ptp_timestamp = false,
            .vlan_filter = false,
            .gigabit = false,
            .mtu_bytes = 0U};
  }

  static consteval struct sd_capability sd_capability() noexcept
  {
    if constexpr (MICROHAL_STM32_LL_FAMILY_SELECTED != 0)
    {
      if constexpr (MICROHAL_STM32_LL_HAS_SD == 0)
      {
        return {};
      }

      return {.supported = true,
              .blocking = true,
              .non_blocking = true,
              .isr = true,
              .dma = true,
              .instance_count = 1U,
              .max_frequency_hz = 50'000'000U,
              .high_speed = true,
              .uhs = false,
              .bus_width_4 = true,
#if defined(SDIO_BUS_WIDE_8B) || defined(SDMMC_BUS_WIDE_8B)
              .bus_width_8 = true,
#else
              .bus_width_8 = false,
#endif
              .block_size_bytes = 512U};
    }

    return {};
  }

  static consteval struct dma_capability dma_capability() noexcept
  {
    if constexpr (MICROHAL_STM32_LL_FAMILY_SELECTED != 0)
    {
      if constexpr (MICROHAL_STM32_LL_DMA_RUNTIME == 0)
      {
        return {};
      }

      return {.supported = true,
              .blocking = false,
              .non_blocking = true,
              .isr = true,
              .dma = false,
              .instance_count = backends::stm32::ll::dma_controller_count,
              .max_frequency_hz = 0U,
              .stream_count = 8U,
              .channel_count = 8U,
              .mem_to_mem = true,
              .fifo = true,
              .double_buffer = true};
    }

    return {.supported = true,
            .blocking = false,
            .non_blocking = true,
            .isr = true,
            .dma = false,
            .instance_count = 2U,
            .max_frequency_hz = 0U,
            .stream_count = 8U,
            .channel_count = 8U,
            .mem_to_mem = true,
            .fifo = true,
            .double_buffer = true};
  }

  static consteval struct pwm_capability pwm_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 14U,
            .max_frequency_hz = 200'000'000U,
            .channel_count = 4U,
            .complementary_output = true,
            .dead_time = true};
  }

  static consteval struct watchdog_capability watchdog_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = true,
            .dma = false,
            .instance_count = 2U,
            .max_frequency_hz = 0U,
            .independent = true,
            .window = true,
            .min_timeout_ms = 1U,
            .max_timeout_ms = 32'768U};
  }

  static consteval struct i2s_capability i2s_capability() noexcept
  {
    if constexpr (MICROHAL_STM32_LL_FAMILY_SELECTED != 0)
    {
#if defined(SPI_I2SCFGR_I2SMOD)
      return {.supported = true,
              .blocking = true,
              .non_blocking = true,
              .isr = true,
              .dma = true,
              .instance_count = 3U,
              .max_frequency_hz = 42'000'000U,
              .full_duplex = true,
              .master_clock_output = true,
              .max_sample_rate = 192'000U};
#else
      return {};
#endif
    }

    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 3U,
            .max_frequency_hz = 42'000'000U,
            .full_duplex = true,
            .master_clock_output = true,
            .max_sample_rate = 192'000U};
  }

  static consteval struct rcc_capability rcc_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .pll_config = true,
            .hse = true,
            .lse = true,
            .max_system_clock_hz = 180'000'000U};
  }

  static consteval struct crc_capability crc_capability() noexcept
  {
    if constexpr (MICROHAL_STM32_LL_FAMILY_SELECTED != 0)
    {
      return {.supported = true,
              .blocking = true,
              .non_blocking = false,
              .isr = false,
              .dma = false,
              .instance_count = 1U,
              .max_frequency_hz = 0U,
              .hardware_crc32 = true,
              .programmable_polynomial = true,
              .random_number_generator = backends::stm32::ll::family_has_rng};
    }

    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .hardware_crc32 = true,
            .programmable_polynomial = true,
            .random_number_generator = true};
  }

  template <typename PinConfig> static consteval bool is_valid_gpio_pin() noexcept
  {
    return (PinConfig::port_index < gpio_port_count) && (PinConfig::pin_index < gpio_pins_per_port);
  }

  template <typename PinConfig> static consteval bool is_valid_alternate_function() noexcept
  {
    return (PinConfig::alternate_function == gpio_no_alternate) ||
           (PinConfig::alternate_function < gpio_alternate_function_count);
  }
};

} // namespace microhal
