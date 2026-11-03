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
#include <cstring>
#include <type_traits>

// STM32 Cube HAL includes - conditional based on MCU family
#if defined(STM32C0)
#include "stm32c0xx_hal.h"
#elif defined(STM32F0)
#include "stm32f0xx_hal.h"
#elif defined(STM32F1)
#include "stm32f1xx_hal.h"
#elif defined(STM32F2)
#include "stm32f2xx_hal.h"
#elif defined(STM32F3)
#include "stm32f3xx_hal.h"
#elif defined(STM32F4)
#include "stm32f4xx_hal.h"
#elif defined(STM32F7)
#include "stm32f7xx_hal.h"
#elif defined(STM32G0)
#include "stm32g0xx_hal.h"
#elif defined(STM32G4)
#include "stm32g4xx_hal.h"
#elif defined(STM32H5)
#include "stm32h5xx_hal.h"
#elif defined(STM32H7RS)
#include "stm32h7rsxx_hal.h"
#elif defined(STM32L4)
#include "stm32l4xx_hal.h"
#elif defined(STM32L0)
#include "stm32l0xx_hal.h"
#elif defined(STM32L1)
#include "stm32l1xx_hal.h"
#elif defined(STM32L5)
#include "stm32l5xx_hal.h"
#elif defined(STM32N6)
#include "stm32n6xx_hal.h"
#elif defined(STM32U0)
#include "stm32u0xx_hal.h"
#elif defined(STM32U3)
#include "stm32u3xx_hal.h"
#elif defined(STM32U5)
#include "stm32u5xx_hal.h"
#elif defined(STM32H7)
#include "stm32h7xx_hal.h"
#elif defined(STM32WB)
#include "stm32wbxx_hal.h"
#else
#if !defined(MICROHAL_SUPPRESS_MISSING_MCU_WARNINGS)
#warning                                                                                                               \
    "No STM32 MCU family defined. Define one of: STM32C0, STM32F0, STM32F1, STM32F2, STM32F3, STM32F4, STM32F7, STM32G0, STM32G4, STM32H5, STM32H7, STM32H7RS, STM32L0, STM32L1, STM32L4, STM32L5, STM32N6, STM32U0, STM32U3, STM32U5, STM32WB"
#endif
#endif

#if defined(STM32C0) || defined(STM32F0) || defined(STM32F1) || defined(STM32F2) || defined(STM32F3) ||                \
    defined(STM32F4) || defined(STM32F7) || defined(STM32G0) || defined(STM32G4) || defined(STM32H5) ||                \
    defined(STM32H7) || defined(STM32H7RS) || defined(STM32L0) || defined(STM32L1) || defined(STM32L4) ||              \
    defined(STM32L5) || defined(STM32N6) || defined(STM32U0) || defined(STM32U3) || defined(STM32U5) ||                \
    defined(STM32WB)
#define MICROHAL_STM32_HAL_FAMILY_SELECTED 1
#else
#define MICROHAL_STM32_HAL_FAMILY_SELECTED 0
#endif

#if defined(STM32F4) || defined(STM32F7) || defined(STM32H5) || defined(STM32H7) || defined(STM32H7RS) ||              \
    defined(STM32N6)
#define MICROHAL_STM32_HAL_ETH_V2_RUNTIME 1
#else
#define MICROHAL_STM32_HAL_ETH_V2_RUNTIME 0
#endif

#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4) || defined(STM32F7) || defined(STM32H5) ||                \
    defined(STM32H7) || defined(STM32H7RS) || defined(STM32L1) || defined(STM32L4) || defined(STM32L5) ||              \
    defined(STM32N6) || defined(STM32U3) || defined(STM32U5)
#define MICROHAL_STM32_HAL_SD_RUNTIME 1
#else
#define MICROHAL_STM32_HAL_SD_RUNTIME 0
#endif

#if defined(STM32F2) || defined(STM32F4) || defined(STM32F7) || defined(STM32H7)
#define MICROHAL_STM32_HAL_STREAM_DMA_RUNTIME 1
#else
#define MICROHAL_STM32_HAL_STREAM_DMA_RUNTIME 0
#endif

#include "microhal/backends/stm32/adc_registers.hpp"
#include "microhal/core/traits.hpp"

namespace microhal::backends::stm32::hal
{

/// @brief Timeout applied when a configuration does not specify one.
inline constexpr std::uint32_t default_timeout_ms = 1000U;

/**
 * @brief Blocking-transfer timeout for @p Config.
 * @details Timeout is application policy, so it is read from the configuration type when present.
 *          Configurations predating `timeout_ms` fall back to @ref default_timeout_ms.
 */
template <typename Config> [[nodiscard]] consteval std::uint32_t timeout_for() noexcept
{
  if constexpr (requires { Config::timeout_ms; })
  {
    return Config::timeout_ms;
  }
  else
  {
    return default_timeout_ms;
  }
}

/**
 * @brief Casts away const for ST HAL transmit APIs that predate const-correctness.
 * @details `HAL_UART_Transmit`, `HAL_SPI_TransmitReceive`, and friends take a non-const pointer but
 *          only read from the buffer. Routing every such call through these helpers keeps the
 *          unavoidable cast in one reviewable place instead of scattering `const_cast` across the
 *          backend.
 */
/// @{
template <typename T> [[nodiscard]] inline T* hal_tx_buffer(const T* data) noexcept
{
  return const_cast<T*>(data);
}

[[nodiscard]] inline std::uint32_t* hal_tx_buffer_as_words(const std::uint8_t* data) noexcept
{
  return const_cast<std::uint32_t*>(reinterpret_cast<const std::uint32_t*>(data));
}

[[nodiscard]] inline std::uint32_t* hal_tx_buffer_as_words(const std::uint16_t* data) noexcept
{
  return const_cast<std::uint32_t*>(reinterpret_cast<const std::uint32_t*>(data));
}
/// @}

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

inline constexpr bool family_has_legacy_can =
#if defined(STM32F0) || defined(STM32F1) || defined(STM32F2) || defined(STM32F3) || defined(STM32F4) ||                \
    defined(STM32F7) || defined(STM32L4)
    true;
#else
    false;
#endif

#if defined(HAL_I2C_MODULE_ENABLED) &&                                                                                 \
    (defined(I2C1) || defined(I2C2) || defined(I2C3) || defined(I2C4) || defined(I2C5) || defined(I2C6))
#define MICROHAL_STM32_HAL_HAS_I2C 1
#else
#define MICROHAL_STM32_HAL_HAS_I2C 0
#endif

#if defined(HAL_SPI_MODULE_ENABLED) &&                                                                                 \
    (defined(SPI1) || defined(SPI2) || defined(SPI3) || defined(SPI4) || defined(SPI5) || defined(SPI6))
#define MICROHAL_STM32_HAL_HAS_SPI 1
#else
#define MICROHAL_STM32_HAL_HAS_SPI 0
#endif

#if defined(HAL_UART_MODULE_ENABLED) &&                                                                                \
    (defined(USART1) || defined(USART2) || defined(USART3) || defined(UART4) || defined(UART5) || defined(USART6) ||   \
     defined(UART7) || defined(UART8) || defined(UART9) || defined(UART10) || defined(USART10) || defined(USART11) ||  \
     defined(USART12) || defined(UART11) || defined(UART12) || defined(LPUART1) || defined(LPUART2) ||                 \
     defined(LPUART3))
#define MICROHAL_STM32_HAL_HAS_UART 1
#else
#define MICROHAL_STM32_HAL_HAS_UART 0
#endif

#if defined(TIM1) || defined(TIM2) || defined(TIM3) || defined(TIM4) || defined(TIM5) || defined(TIM6) ||              \
    defined(TIM7) || defined(TIM8) || defined(TIM9) || defined(TIM10) || defined(TIM11) || defined(TIM12) ||           \
    defined(TIM13) || defined(TIM14) || defined(TIM15) || defined(TIM16) || defined(TIM17) || defined(TIM18) ||        \
    defined(TIM19) || defined(TIM20) || defined(TIM23) || defined(TIM24)
#define MICROHAL_STM32_HAL_HAS_TIM 1
#else
#define MICROHAL_STM32_HAL_HAS_TIM 0
#endif

#if defined(ADC1) || defined(ADC2) || defined(ADC3) || defined(ADC4) || defined(ADC5)
#define MICROHAL_STM32_HAL_HAS_ADC 1
#else
#define MICROHAL_STM32_HAL_HAS_ADC 0
#endif

#if defined(DAC) || defined(DAC1) || defined(DAC2) || defined(DAC3) || defined(DAC4)
#define MICROHAL_STM32_HAL_HAS_DAC 1
#else
#define MICROHAL_STM32_HAL_HAS_DAC 0
#endif

#if defined(CAN1) || defined(CAN2) || defined(CAN3)
#define MICROHAL_STM32_HAL_HAS_LEGACY_CAN 1
#else
#define MICROHAL_STM32_HAL_HAS_LEGACY_CAN 0
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

constexpr std::uint32_t flash_sector_from_address(const std::uint32_t address) noexcept
{
#if defined(STM32H7) && defined(FLASH_BANK_SIZE) && defined(FLASH_SECTOR_SIZE)
  return ((address - FLASH_BASE) % FLASH_BANK_SIZE) / FLASH_SECTOR_SIZE;
#elif defined(FLASH_SECTOR_0)
  if (address < 0x08004000U)
  {
    return FLASH_SECTOR_0;
  }
  if (address < 0x08008000U)
  {
    return FLASH_SECTOR_1;
  }
  if (address < 0x0800C000U)
  {
    return FLASH_SECTOR_2;
  }
  if (address < 0x08010000U)
  {
    return FLASH_SECTOR_3;
  }
  if (address < 0x08020000U)
  {
    return FLASH_SECTOR_4;
  }
  if (address < 0x08040000U)
  {
    return FLASH_SECTOR_5;
  }
  if (address < 0x08060000U)
  {
    return FLASH_SECTOR_6;
  }
  if (address < 0x08080000U)
  {
    return FLASH_SECTOR_7;
  }
  if (address < 0x080A0000U)
  {
    return FLASH_SECTOR_8;
  }
  if (address < 0x080C0000U)
  {
    return FLASH_SECTOR_9;
  }
  if (address < 0x080E0000U)
  {
    return FLASH_SECTOR_10;
  }
  return FLASH_SECTOR_11;
#else
  (void)address;
  return 0U;
#endif
}

constexpr std::uint32_t flash_sector_span(const std::uint32_t address, const std::uint32_t bytes) noexcept
{
#if defined(FLASH_SECTOR_0)
  if (bytes == 0U)
  {
    return 0U;
  }

  const std::uint32_t first_sector = flash_sector_from_address(address);
  const std::uint32_t last_sector = flash_sector_from_address(address + bytes - 1U);
  return (last_sector - first_sector) + 1U;
#else
  (void)address;
  (void)bytes;
  return 0U;
#endif
}

constexpr result<flash_erase_region> flash_erase_region_at(const std::uint32_t address) noexcept
{
#if defined(FLASH_TYPEERASE_PAGES)
#if defined(FLASH_PAGE_SIZE)
  constexpr std::uint32_t page_size = FLASH_PAGE_SIZE;
#else
  constexpr std::uint32_t page_size = 2048U;
#endif
  if (address < FLASH_BASE)
  {
    return status::invalid_argument;
  }
  const std::uint32_t offset = address - FLASH_BASE;
  const std::uint32_t start = FLASH_BASE + (offset / page_size) * page_size;
  return flash_erase_region{.start_address = start, .size_bytes = page_size};
#elif defined(FLASH_TYPEERASE_SECTORS) && defined(FLASH_SECTOR_0)
  constexpr std::array<std::uint32_t, 13U> boundaries{0x08000000U, 0x08004000U, 0x08008000U, 0x0800C000U, 0x08010000U,
                                                      0x08020000U, 0x08040000U, 0x08060000U, 0x08080000U, 0x080A0000U,
                                                      0x080C0000U, 0x080E0000U, 0x08100000U};
  for (std::size_t index = 0U; index + 1U < boundaries.size(); ++index)
  {
    if ((address >= boundaries[index]) && (address < boundaries[index + 1U]))
    {
      return flash_erase_region{.start_address = boundaries[index],
                                .size_bytes = boundaries[index + 1U] - boundaries[index]};
    }
  }
  return status::invalid_argument;
#else
  (void)address;
  return status::unsupported;
#endif
}

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

inline constexpr std::uint32_t legacy_can_instance_count =
#if defined(CAN1)
    1U +
#else
    0U +
#endif
#if defined(CAN2)
    1U +
#else
    0U +
#endif
#if defined(CAN3)
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
// Helper to get GPIO port base address
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

// Helper to map microHAL gpio_mode to HAL GPIO mode
consteval std::uint32_t hal_gpio_mode(const gpio_mode mode) noexcept
{
  switch (mode)
  {
  case gpio_mode::input:
    return GPIO_MODE_INPUT;
  case gpio_mode::output:
    return GPIO_MODE_OUTPUT_PP;
  case gpio_mode::alternate:
    return GPIO_MODE_AF_PP;
  case gpio_mode::analog:
    return GPIO_MODE_ANALOG;
  default:
    return GPIO_MODE_INPUT;
  }
}

// Helper to map microHAL gpio_pull to HAL GPIO pull
consteval std::uint32_t hal_gpio_pull(const gpio_pull pull) noexcept
{
  switch (pull)
  {
  case gpio_pull::none:
    return GPIO_NOPULL;
  case gpio_pull::up:
    return GPIO_PULLUP;
  case gpio_pull::down:
    return GPIO_PULLDOWN;
  default:
    return GPIO_NOPULL;
  }
}

// Helper to map microHAL gpio_speed to HAL GPIO speed
consteval std::uint32_t hal_gpio_speed(const gpio_speed speed) noexcept
{
  switch (speed)
  {
  case gpio_speed::low:
    return GPIO_SPEED_FREQ_LOW;
  case gpio_speed::medium:
    return GPIO_SPEED_FREQ_MEDIUM;
  case gpio_speed::high:
    return GPIO_SPEED_FREQ_HIGH;
  case gpio_speed::very_high:
#if defined(GPIO_SPEED_FREQ_VERY_HIGH)
    return GPIO_SPEED_FREQ_VERY_HIGH;
#else
    return GPIO_SPEED_FREQ_HIGH;
#endif
  default:
    return GPIO_SPEED_FREQ_LOW;
  }
}

consteval std::uint32_t hal_gpio_interrupt_mode(const gpio_interrupt_trigger trigger) noexcept
{
  switch (trigger)
  {
  case gpio_interrupt_trigger::rising:
    return GPIO_MODE_IT_RISING;
  case gpio_interrupt_trigger::falling:
    return GPIO_MODE_IT_FALLING;
  case gpio_interrupt_trigger::both:
    return GPIO_MODE_IT_RISING_FALLING;
  case gpio_interrupt_trigger::none:
  default:
    return GPIO_MODE_INPUT;
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
                    "microHAL: STM32 GPIO bindings must expose binding::port_index or PinConfig::port_index.");
      return 0U;
    }
  }
  else
  {
    static_assert(dependent_false_v<PinConfig>,
                  "microHAL: STM32 GPIO bindings must expose binding::port_index or PinConfig::port_index.");
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
                    "microHAL: STM32 GPIO bindings must expose binding::pin_index or PinConfig::pin_index.");
      return 0U;
    }
  }
  else
  {
    static_assert(dependent_false_v<PinConfig>,
                  "microHAL: STM32 GPIO bindings must expose binding::pin_index or PinConfig::pin_index.");
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
                    "microHAL: STM32 peripheral configs must expose binding::base_address or Config::base_address.");
      return 0U;
    }
  }
  else
  {
    static_assert(dependent_false_v<Config>,
                  "microHAL: STM32 peripheral configs must expose binding::base_address or Config::base_address.");
    return 0U;
  }
}

template <typename PeripheralType, typename Config> constexpr auto peripheral_instance() noexcept -> PeripheralType*
{
  return reinterpret_cast<PeripheralType*>(peripheral_base_address<Config>());
}

consteval std::uint32_t hal_i2c_address_mode(const i2c_address_mode mode) noexcept
{
#if MICROHAL_STM32_HAL_HAS_I2C
  switch (mode)
  {
  case i2c_address_mode::ten_bit:
    return I2C_ADDRESSINGMODE_10BIT;
  case i2c_address_mode::seven_bit:
  default:
    return I2C_ADDRESSINGMODE_7BIT;
  }
#else
  (void)mode;
  return 0U;
#endif
}

template <typename Config> consteval std::uint32_t hal_i2c_timing() noexcept
{
  if constexpr (requires { Config::timing; })
  {
    return Config::timing;
  }
  else if constexpr (requires { typename Config::binding; })
  {
    using binding = typename Config::binding;
    if constexpr (requires { binding::timing; })
    {
      return binding::timing;
    }
    else
    {
      static_assert(dependent_false_v<Config>, "microHAL: STM32 HAL I2C configs targeting timing-register families "
                                               "must expose binding::timing or Config::timing.");
      return 0U;
    }
  }
  else
  {
    static_assert(dependent_false_v<Config>, "microHAL: STM32 HAL I2C configs targeting timing-register families must "
                                             "expose binding::timing or Config::timing.");
    return 0U;
  }
}

template <typename Config> consteval ethernet_media_interface ethernet_interface() noexcept
{
  if constexpr (requires { typename Config::binding; })
  {
    using binding = typename Config::binding;
    if constexpr (requires { binding::media_interface; })
    {
      return binding::media_interface;
    }
  }

  return ethernet_media_interface::rmii;
}

template <typename Config> consteval std::uint32_t ethernet_rx_buffer_size() noexcept
{
  if constexpr (requires { typename Config::binding; })
  {
    using binding = typename Config::binding;
    if constexpr (requires { binding::rx_buffer_size; })
    {
      return binding::rx_buffer_size;
    }
  }

#if defined(ETH_RX_BUF_SIZE) && defined(ETH)
  return ETH_RX_BUF_SIZE;
#else
  return 1528U;
#endif
}

template <typename Config> consteval std::size_t ethernet_rx_buffer_count() noexcept
{
  if constexpr (requires { typename Config::binding; })
  {
    using binding = typename Config::binding;
    if constexpr (requires { binding::rx_buffer_count; })
    {
      return static_cast<std::size_t>(binding::rx_buffer_count);
    }
  }

#if defined(ETH_RX_DESC_CNT)
  return static_cast<std::size_t>(ETH_RX_DESC_CNT + 2U);
#else
  return 6U;
#endif
}

template <typename Config> consteval std::array<std::uint8_t, 6U> ethernet_initial_mac_address() noexcept
{
  if constexpr (requires { typename Config::binding; })
  {
    using binding = typename Config::binding;
    if constexpr (requires { binding::mac_address; })
    {
      return binding::mac_address;
    }
  }
  else if constexpr (requires { Config::instance_index; })
  {
    return {0x02U, 0x00U, 0x00U, 0x00U, 0x00U, static_cast<std::uint8_t>(Config::instance_index + 1U)};
  }

  return {0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U};
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

#if defined(ETH)
template <typename Config> constexpr auto stm32_ethernet_instance() noexcept -> decltype(ETH)
{
  if constexpr (requires { Config::base_address; })
  {
    return reinterpret_cast<decltype(ETH)>(Config::base_address);
  }
  else if constexpr (requires { typename Config::binding; })
  {
    using binding = typename Config::binding;
    if constexpr (requires { binding::base_address; })
    {
      return reinterpret_cast<decltype(ETH)>(binding::base_address);
    }
    else
    {
      static_assert(dependent_false_v<Config>,
                    "microHAL: STM32 Ethernet configs must expose binding::base_address or instance_index.");
      return nullptr;
    }
  }
  else if constexpr (requires { Config::instance_index; })
  {
    static_assert(Config::instance_index == 0U,
                  "microHAL: STM32 HAL Ethernet currently maps only instance_index 0 to ETH.");
    return ETH;
  }
  else
  {
    static_assert(dependent_false_v<Config>,
                  "microHAL: STM32 Ethernet configs must expose binding::base_address or instance_index.");
    return nullptr;
  }
}
#endif

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
                    "microHAL: STM32 SD configs must expose binding::base_address or instance_index.");
      return nullptr;
    }
  }
  else if constexpr (requires { Config::instance_index; })
  {
    static_assert(Config::instance_index == 0U, "microHAL: STM32 HAL SD currently maps only instance_index 0 to SDIO.");
    return SDIO;
  }
  else
  {
    static_assert(dependent_false_v<Config>,
                  "microHAL: STM32 SD configs must expose binding::base_address or instance_index.");
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
                    "microHAL: STM32 SD configs must expose binding::base_address or instance_index.");
      return nullptr;
    }
  }
  else if constexpr (requires { Config::instance_index; })
  {
    static_assert(Config::instance_index == 0U,
                  "microHAL: STM32 HAL SD currently maps only instance_index 0 to SDMMC1.");
    return SDMMC1;
  }
  else
  {
    static_assert(dependent_false_v<Config>,
                  "microHAL: STM32 SD configs must expose binding::base_address or instance_index.");
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

#if defined(ETH) && defined(HAL_ETH_MODULE_ENABLED) && MICROHAL_STM32_HAL_ETH_V2_RUNTIME
template <typename Config> struct stm32_ethernet_state final
{
  static inline ETH_HandleTypeDef handle_storage{};
  static inline std::array<ETH_DMADescTypeDef, ETH_TX_DESC_CNT> tx_descriptors{};
  static inline std::array<ETH_DMADescTypeDef, ETH_RX_DESC_CNT> rx_descriptors{};
  static inline std::array<std::uint8_t, 6U> mac_address = ethernet_initial_mac_address<Config>();

  static constexpr std::size_t rx_buffer_bytes = static_cast<std::size_t>(ethernet_rx_buffer_size<Config>());
  static constexpr std::size_t rx_packet_count = ethernet_rx_buffer_count<Config>();

  struct rx_packet final
  {
    rx_packet* next{nullptr};
    std::uint16_t length{0U};
    bool in_use{false};
    alignas(32) std::array<std::uint8_t, rx_buffer_bytes> data{};
  };

  static inline std::array<rx_packet, rx_packet_count> rx_pool{};

  static void reset_rx_pool() noexcept
  {
    for (auto& packet : rx_pool)
    {
      packet.next = nullptr;
      packet.length = 0U;
      packet.in_use = false;
    }
  }

  static auto packet_from_buffer(std::uint8_t* buffer) noexcept -> rx_packet*
  {
    if (buffer == nullptr)
    {
      return nullptr;
    }

    const auto raw = reinterpret_cast<std::uintptr_t>(buffer) - offsetof(rx_packet, data);
    return reinterpret_cast<rx_packet*>(raw);
  }

  static void rx_allocate_callback(std::uint8_t** buffer)
  {
    if (buffer == nullptr)
    {
      return;
    }

    for (auto& packet : rx_pool)
    {
      if (!packet.in_use)
      {
        packet.in_use = true;
        packet.next = nullptr;
        packet.length = 0U;
        *buffer = packet.data.data();
        return;
      }
    }

    *buffer = nullptr;
  }

  static void rx_link_callback(void** start, void** end, std::uint8_t* buffer, const std::uint16_t length)
  {
    if ((start == nullptr) || (end == nullptr))
    {
      return;
    }

    rx_packet* packet = packet_from_buffer(buffer);
    if (packet == nullptr)
    {
      return;
    }

    packet->next = nullptr;
    packet->length = length;

    auto** first = reinterpret_cast<rx_packet**>(start);
    auto** last = reinterpret_cast<rx_packet**>(end);
    if (*first == nullptr)
    {
      *first = packet;
    }
    else
    {
      (*last)->next = packet;
    }
    *last = packet;
  }

  static void release_packet_chain(rx_packet* packet) noexcept
  {
    while (packet != nullptr)
    {
      rx_packet* next = packet->next;
      packet->next = nullptr;
      packet->length = 0U;
      packet->in_use = false;
      packet = next;
    }
  }
};
#endif

#if (defined(SDIO) || defined(SDMMC1)) && defined(HAL_SD_MODULE_ENABLED) && MICROHAL_STM32_HAL_SD_RUNTIME
template <typename Config> struct stm32_sd_state final
{
  static inline SD_HandleTypeDef handle_storage{};
  static inline HAL_SD_CardInfoTypeDef card_info{};
};
#endif
#endif

struct gpio_api final
{
  template <typename PinConfig> static void configure() noexcept
  {
#if defined(GPIOA)
    GPIO_InitTypeDef init{};
    init.Pin = (1U << gpio_pin_index<PinConfig>());
    init.Mode = hal_gpio_mode(PinConfig::mode);
    init.Pull = hal_gpio_pull(PinConfig::pull);
    init.Speed = hal_gpio_speed(PinConfig::speed);
    if constexpr (PinConfig::mode == gpio_mode::alternate)
    {
#if !defined(STM32F1)
      init.Alternate = gpio_alternate_function<PinConfig>();
#endif
    }
    HAL_GPIO_Init(gpio_port<gpio_port_index<PinConfig>()>(), &init);
#endif
  }

  template <typename PinConfig> static void write(const bool level) noexcept
  {
#if defined(GPIOA)
    HAL_GPIO_WritePin(gpio_port<gpio_port_index<PinConfig>()>(), (1U << gpio_pin_index<PinConfig>()),
                      level ? GPIO_PIN_SET : GPIO_PIN_RESET);
#else
    (void)level;
#endif
  }

  template <typename PinConfig> static bool read() noexcept
  {
#if defined(GPIOA)
    return HAL_GPIO_ReadPin(gpio_port<gpio_port_index<PinConfig>()>(), (1U << gpio_pin_index<PinConfig>())) ==
           GPIO_PIN_SET;
#else
    return false;
#endif
  }

  template <typename PinConfig> static status toggle() noexcept
  {
#if defined(GPIOA)
    HAL_GPIO_TogglePin(gpio_port<gpio_port_index<PinConfig>()>(), (1U << gpio_pin_index<PinConfig>()));
    return status::ok;
#else
    (void)sizeof(PinConfig);
    return status::unsupported;
#endif
  }

  template <typename PinConfig, auto Handler> static void enable_interrupt() noexcept
  {
#if defined(GPIOA)
    static_assert(PinConfig::interrupt_trigger != gpio_interrupt_trigger::none,
                  "microHAL: STM32 GPIO interrupt pins must select a non-none interrupt_trigger.");
    GPIO_InitTypeDef init{};
    init.Pin = (1U << gpio_pin_index<PinConfig>());
    init.Mode = hal_gpio_interrupt_mode(PinConfig::interrupt_trigger);
    init.Pull = hal_gpio_pull(PinConfig::pull);
    HAL_GPIO_Init(gpio_port<gpio_port_index<PinConfig>()>(), &init);

    if constexpr (
        requires { typename PinConfig::binding; } &&
        requires { PinConfig::binding::template enable_interrupt<Handler>(); })
    {
      PinConfig::binding::template enable_interrupt<Handler>();
    }
    else
    {
      static_assert(dependent_false_v<PinConfig>,
                    "microHAL: STM32 GPIO interrupt bindings must provide binding::enable_interrupt<Handler>() to own "
                    "NVIC and vector dispatch.");
    }
#else
    (void)Handler;
#endif
  }
};

struct i2c_api final
{
#if MICROHAL_STM32_HAL_HAS_I2C
  static status error_status(I2C_HandleTypeDef& handle_storage) noexcept
  {
    const std::uint32_t error = HAL_I2C_GetError(&handle_storage);
#if defined(HAL_I2C_ERROR_AF)
    if ((error & HAL_I2C_ERROR_AF) != 0U)
    {
      return status::io_error;
    }
#endif
#if defined(HAL_I2C_ERROR_TIMEOUT)
    if ((error & HAL_I2C_ERROR_TIMEOUT) != 0U)
    {
      return status::timeout;
    }
#endif
#if defined(HAL_I2C_ERROR_BERR) || defined(HAL_I2C_ERROR_ARLO) || defined(HAL_I2C_ERROR_OVR)
    if (
#if defined(HAL_I2C_ERROR_BERR)
        ((error & HAL_I2C_ERROR_BERR) != 0U) ||
#endif
#if defined(HAL_I2C_ERROR_ARLO)
        ((error & HAL_I2C_ERROR_ARLO) != 0U) ||
#endif
#if defined(HAL_I2C_ERROR_OVR)
        ((error & HAL_I2C_ERROR_OVR) != 0U) ||
#endif
        false)
    {
      return status::io_error;
    }
#endif
    return (error == HAL_I2C_ERROR_NONE) ? status::ok : status::error;
  }

  static status transfer_status(const HAL_StatusTypeDef result, I2C_HandleTypeDef& handle_storage) noexcept
  {
    if (result == HAL_OK)
    {
      return status::ok;
    }
    if (result == HAL_TIMEOUT)
    {
      return status::timeout;
    }
    if (result == HAL_BUSY)
    {
      return status::busy;
    }
    const status error = error_status(handle_storage);
    return (error == status::ok) ? status::error : error;
  }

  template <typename Config> static auto& handle() noexcept
  {
    static I2C_HandleTypeDef handle_storage{};
    return handle_storage;
  }

#if defined(I2C_FIRST_FRAME) && defined(I2C_LAST_FRAME)
  static void abort_and_wait(I2C_HandleTypeDef& handle_storage, const std::uint16_t device_address,
                             const std::uint32_t timeout_ms) noexcept
  {
    if (HAL_I2C_Master_Abort_IT(&handle_storage, device_address) != HAL_OK)
    {
      return;
    }

    const std::uint32_t start = HAL_GetTick();
    while ((HAL_I2C_GetState(&handle_storage) != HAL_I2C_STATE_READY) && ((HAL_GetTick() - start) < timeout_ms))
    {
    }
  }
#endif

#if defined(I2C_FIRST_FRAME) && defined(I2C_LAST_FRAME) && defined(USE_HAL_I2C_REGISTER_CALLBACKS) &&                  \
    (USE_HAL_I2C_REGISTER_CALLBACKS == 1)
  template <typename Config, auto Handler> struct async_sequence_state final
  {
    std::span<const std::uint8_t> tx_payload{};
    std::span<std::uint8_t> rx_payload{};
    std::uint16_t device_address{0U};
    bool active{false};
    void (*previous_tx_callback)(I2C_HandleTypeDef*){nullptr};
    void (*previous_rx_callback)(I2C_HandleTypeDef*){nullptr};
    void (*previous_error_callback)(I2C_HandleTypeDef*){nullptr};
  };

  template <typename Config, auto Handler> static auto& async_state() noexcept
  {
    static async_sequence_state<Config, Handler> state{};
    return state;
  }

  template <typename Config, auto Handler> static void notify_completion(const status result) noexcept
  {
    if constexpr (requires { Handler(result); })
    {
      Handler(result);
    }
    else if constexpr (requires { Handler(); })
    {
      Handler();
    }
  }

  template <typename Config, auto Handler> static void restore_callbacks() noexcept
  {
    auto& state = async_state<Config, Handler>();
    auto& handle_storage = handle<Config>();
    handle_storage.MasterTxCpltCallback = state.previous_tx_callback;
    handle_storage.MasterRxCpltCallback = state.previous_rx_callback;
    handle_storage.ErrorCallback = state.previous_error_callback;
    state.active = false;
  }

  template <typename Config, auto Handler>
  static void async_read_complete(I2C_HandleTypeDef* const handle_storage) noexcept
  {
    auto& state = async_state<Config, Handler>();
    if (!state.active || (handle_storage != &handle<Config>()))
    {
      return;
    }

    const auto previous_callback = state.previous_rx_callback;
    const status result = error_status(*handle_storage);
    restore_callbacks<Config, Handler>();
    if (previous_callback != nullptr)
    {
      previous_callback(handle_storage);
    }
    notify_completion<Config, Handler>(result);
  }

  template <typename Config, auto Handler> static void async_error(I2C_HandleTypeDef* const handle_storage) noexcept
  {
    auto& state = async_state<Config, Handler>();
    if (!state.active || (handle_storage != &handle<Config>()))
    {
      return;
    }

    const auto previous_callback = state.previous_error_callback;
    const status error = error_status(*handle_storage);
    const status result = (error == status::ok) ? status::error : error;
    restore_callbacks<Config, Handler>();
    if (previous_callback != nullptr)
    {
      previous_callback(handle_storage);
    }
    notify_completion<Config, Handler>(result);
  }

  template <typename Config, auto Handler>
  static void async_write_complete(I2C_HandleTypeDef* const handle_storage) noexcept
  {
    auto& state = async_state<Config, Handler>();
    if (!state.active || (handle_storage != &handle<Config>()))
    {
      return;
    }

    const auto previous_callback = state.previous_tx_callback;
    const HAL_StatusTypeDef receive_status =
        HAL_I2C_Master_Seq_Receive_IT(handle_storage, state.device_address, state.rx_payload.data(),
                                      static_cast<std::uint16_t>(state.rx_payload.size()), I2C_LAST_FRAME);
    if (previous_callback != nullptr)
    {
      previous_callback(handle_storage);
    }
    if (receive_status != HAL_OK)
    {
      const status result = transfer_status(receive_status, *handle_storage);
      restore_callbacks<Config, Handler>();
      notify_completion<Config, Handler>(result);
    }
  }
#endif
#endif

  template <typename Config> static bool initialize() noexcept
  {
#if MICROHAL_STM32_HAL_HAS_I2C
    auto& handle_storage = handle<Config>();
    handle_storage.Instance = peripheral_instance<I2C_TypeDef, Config>();

#if defined(I2C_CCR_FS)
    handle_storage.Init.ClockSpeed = Config::clock_hz;
    handle_storage.Init.DutyCycle = I2C_DUTYCYCLE_2;
#else
    handle_storage.Init.Timing = hal_i2c_timing<Config>();
#endif

    if constexpr (serial_slave_enabled<Config>())
      handle_storage.Init.OwnAddress1 = Config::address_mode == i2c_address_mode::seven_bit
                                            ? static_cast<std::uint32_t>(Config::own_address << 1U)
                                            : Config::own_address;
    else
      handle_storage.Init.OwnAddress1 = 0U;
    handle_storage.Init.AddressingMode = hal_i2c_address_mode(Config::address_mode);
    handle_storage.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;

    handle_storage.Init.OwnAddress2 = 0U;
#if defined(I2C_OA2_NOMASK)
    handle_storage.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
#endif
    handle_storage.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    handle_storage.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&handle_storage) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config>
  static status write_blocking(std::span<const std::uint8_t> payload, const std::uint16_t address) noexcept
  {
#if MICROHAL_STM32_HAL_HAS_I2C
    auto& handle_storage = handle<Config>();
    constexpr std::uint32_t timeout_ms = timeout_for<Config>();
    const std::uint16_t device_address =
        (Config::address_mode == i2c_address_mode::seven_bit) ? static_cast<std::uint16_t>(address << 1U) : address;
    return transfer_status(HAL_I2C_Master_Transmit(&handle_storage, device_address, hal_tx_buffer(payload.data()),
                                                   static_cast<std::uint16_t>(payload.size()), timeout_ms),
                           handle_storage);
#else
    (void)payload;
    (void)address;
    return status::unsupported;
#endif
  }

  template <typename Config>
  static status read_blocking(std::span<std::uint8_t> payload, const std::uint16_t address) noexcept
  {
#if MICROHAL_STM32_HAL_HAS_I2C
    auto& handle_storage = handle<Config>();
    constexpr std::uint32_t timeout_ms = timeout_for<Config>();
    const std::uint16_t device_address =
        (Config::address_mode == i2c_address_mode::seven_bit) ? static_cast<std::uint16_t>(address << 1U) : address;
    return transfer_status(HAL_I2C_Master_Receive(&handle_storage, device_address, payload.data(),
                                                  static_cast<std::uint16_t>(payload.size()), timeout_ms),
                           handle_storage);
#else
    (void)payload;
    (void)address;
    return status::unsupported;
#endif
  }

  template <typename Config> static status probe(const std::uint16_t address) noexcept
  {
#if MICROHAL_STM32_HAL_HAS_I2C
    auto& handle_storage = handle<Config>();
    constexpr std::uint32_t timeout_ms = timeout_for<Config>();
    const std::uint16_t device_address =
        (Config::address_mode == i2c_address_mode::seven_bit) ? static_cast<std::uint16_t>(address << 1U) : address;
    return transfer_status(HAL_I2C_IsDeviceReady(&handle_storage, device_address, 1U, timeout_ms), handle_storage);
#else
    (void)address;
    return status::unsupported;
#endif
  }

  template <typename Config>
  static status write_read_blocking(const std::span<const std::uint8_t> tx_payload,
                                    const std::span<std::uint8_t> rx_payload, const std::uint16_t address) noexcept
  {
#if MICROHAL_STM32_HAL_HAS_I2C && defined(I2C_FIRST_FRAME) && defined(I2C_LAST_FRAME)
    if ((tx_payload.size() > UINT16_MAX) || (rx_payload.size() > UINT16_MAX))
    {
      return status::invalid_argument;
    }

    auto& handle_storage = handle<Config>();
    if (HAL_I2C_GetState(&handle_storage) != HAL_I2C_STATE_READY)
    {
      return status::busy;
    }

    const std::uint16_t device_address =
        (Config::address_mode == i2c_address_mode::seven_bit) ? static_cast<std::uint16_t>(address << 1U) : address;
    const std::uint32_t timeout_ms = timeout_for<Config>();
    const std::uint32_t operation_start = HAL_GetTick();
    handle_storage.ErrorCode = HAL_I2C_ERROR_NONE;

    const HAL_StatusTypeDef transmit_status =
        HAL_I2C_Master_Seq_Transmit_IT(&handle_storage, device_address, hal_tx_buffer(tx_payload.data()),
                                       static_cast<std::uint16_t>(tx_payload.size()), I2C_FIRST_FRAME);
    const status transmit_result = transfer_status(transmit_status, handle_storage);
    if (transmit_result != status::ok)
    {
      return transmit_result;
    }

    while (handle_storage.XferCount != 0U)
    {
      const status error = error_status(handle_storage);
      if (error != status::ok)
      {
        return error;
      }
      if ((HAL_GetTick() - operation_start) >= timeout_ms)
      {
        abort_and_wait(handle_storage, device_address, timeout_ms);
        return status::timeout;
      }
    }

    const HAL_StatusTypeDef receive_status =
        HAL_I2C_Master_Seq_Receive_IT(&handle_storage, device_address, rx_payload.data(),
                                      static_cast<std::uint16_t>(rx_payload.size()), I2C_LAST_FRAME);
    const status receive_result = transfer_status(receive_status, handle_storage);
    if (receive_result != status::ok)
    {
      return receive_result;
    }

    while (handle_storage.XferCount != 0U)
    {
      const status error = error_status(handle_storage);
      if (error != status::ok)
      {
        return error;
      }
      if ((HAL_GetTick() - operation_start) >= timeout_ms)
      {
        abort_and_wait(handle_storage, device_address, timeout_ms);
        return status::timeout;
      }
    }
    return error_status(handle_storage);
#else
    (void)tx_payload;
    (void)rx_payload;
    (void)address;
    return status::unsupported;
#endif
  }

  template <typename Config, auto Handler>
  static status write_read_async(const std::span<const std::uint8_t> tx_payload,
                                 const std::span<std::uint8_t> rx_payload, const std::uint16_t address) noexcept
  {
#if MICROHAL_STM32_HAL_HAS_I2C && defined(I2C_FIRST_FRAME) && defined(I2C_LAST_FRAME) &&                               \
    defined(USE_HAL_I2C_REGISTER_CALLBACKS) && (USE_HAL_I2C_REGISTER_CALLBACKS == 1)
    if ((tx_payload.size() > UINT16_MAX) || (rx_payload.size() > UINT16_MAX))
    {
      return status::invalid_argument;
    }

    auto& state = async_state<Config, Handler>();
    if (state.active)
    {
      return status::busy;
    }

    auto& handle_storage = handle<Config>();
    state.tx_payload = tx_payload;
    state.rx_payload = rx_payload;
    state.device_address =
        (Config::address_mode == i2c_address_mode::seven_bit) ? static_cast<std::uint16_t>(address << 1U) : address;
    state.previous_tx_callback = handle_storage.MasterTxCpltCallback;
    state.previous_rx_callback = handle_storage.MasterRxCpltCallback;
    state.previous_error_callback = handle_storage.ErrorCallback;
    state.active = true;

    handle_storage.MasterTxCpltCallback = async_write_complete<Config, Handler>;
    handle_storage.MasterRxCpltCallback = async_read_complete<Config, Handler>;
    handle_storage.ErrorCallback = async_error<Config, Handler>;

    const HAL_StatusTypeDef result =
        HAL_I2C_Master_Seq_Transmit_IT(&handle_storage, state.device_address, hal_tx_buffer(tx_payload.data()),
                                       static_cast<std::uint16_t>(tx_payload.size()), I2C_FIRST_FRAME);
    if (result != HAL_OK)
    {
      restore_callbacks<Config, Handler>();
      return transfer_status(result, handle_storage);
    }
    return status::ok;
#else
    (void)tx_payload;
    (void)rx_payload;
    (void)address;
    (void)Handler;
    return status::unsupported;
#endif
  }

  template <typename Config, auto Handler>
  static bool start_transfer(std::span<std::uint8_t> payload, const std::uint16_t address) noexcept
  {
#if MICROHAL_STM32_HAL_HAS_I2C
    auto& handle_storage = handle<Config>();
    if (HAL_I2C_Master_Transmit_IT(&handle_storage, static_cast<std::uint16_t>(address << 1U), payload.data(),
                                   static_cast<std::uint16_t>(payload.size())) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    (void)payload;
    (void)address;
    (void)Handler;
    return false;
#endif
  }
};

struct spi_api final
{
#if MICROHAL_STM32_HAL_HAS_SPI
  static status transfer_status(const HAL_StatusTypeDef result) noexcept
  {
    if (result == HAL_OK)
    {
      return status::ok;
    }
    if (result == HAL_TIMEOUT)
    {
      return status::timeout;
    }
    if (result == HAL_BUSY)
    {
      return status::busy;
    }
    return status::error;
  }

  template <typename Config> static auto& handle() noexcept
  {
    static SPI_HandleTypeDef handle_storage{};
    return handle_storage;
  }

#if defined(USE_HAL_SPI_REGISTER_CALLBACKS) && (USE_HAL_SPI_REGISTER_CALLBACKS == 1)
  template <typename Config, auto Handler> struct async_state final
  {
    bool active{false};
    void (*previous_complete_callback)(SPI_HandleTypeDef*){nullptr};
    void (*previous_error_callback)(SPI_HandleTypeDef*){nullptr};
  };

  template <typename Config, auto Handler> static auto& state() noexcept
  {
    static async_state<Config, Handler> transfer_state{};
    return transfer_state;
  }

  template <typename Config, auto Handler> static void notify_completion(const status result) noexcept
  {
    if constexpr (requires { Handler(result); })
    {
      Handler(result);
    }
    else if constexpr (requires { Handler(); })
    {
      Handler();
    }
  }

  template <typename Config, auto Handler> static void restore_callbacks() noexcept
  {
    auto& transfer_state = state<Config, Handler>();
    auto& handle_storage = handle<Config>();
    handle_storage.TxRxCpltCallback = transfer_state.previous_complete_callback;
    handle_storage.ErrorCallback = transfer_state.previous_error_callback;
    transfer_state.active = false;
  }

  template <typename Config, auto Handler> static void async_complete(SPI_HandleTypeDef* const handle_storage) noexcept
  {
    auto& transfer_state = state<Config, Handler>();
    if (!transfer_state.active || (handle_storage != &handle<Config>()))
    {
      return;
    }

    const auto previous_callback = transfer_state.previous_complete_callback;
    restore_callbacks<Config, Handler>();
    if (previous_callback != nullptr)
    {
      previous_callback(handle_storage);
    }
    notify_completion<Config, Handler>(status::ok);
  }

  template <typename Config, auto Handler> static void async_error(SPI_HandleTypeDef* const handle_storage) noexcept
  {
    auto& transfer_state = state<Config, Handler>();
    if (!transfer_state.active || (handle_storage != &handle<Config>()))
    {
      return;
    }

    const auto previous_callback = transfer_state.previous_error_callback;
    const status result = (HAL_SPI_GetError(handle_storage) == HAL_SPI_ERROR_NONE) ? status::error : status::io_error;
    restore_callbacks<Config, Handler>();
    if (previous_callback != nullptr)
    {
      previous_callback(handle_storage);
    }
    notify_completion<Config, Handler>(result);
  }
#endif
#endif

  template <typename Config> static bool initialize() noexcept
  {
#if MICROHAL_STM32_HAL_HAS_SPI
    auto& handle_storage = handle<Config>();
    handle_storage.Instance = peripheral_instance<SPI_TypeDef, Config>();
    handle_storage.Init.Mode = serial_slave_enabled<Config>() ? SPI_MODE_SLAVE : SPI_MODE_MASTER;
    handle_storage.Init.Direction = Config::duplex_mode == spi_duplex_mode::half         ? SPI_DIRECTION_1LINE
                                    : Config::duplex_mode == spi_duplex_mode::simplex_rx ? SPI_DIRECTION_2LINES_RXONLY
                                                                                         : SPI_DIRECTION_2LINES;
    handle_storage.Init.DataSize = SPI_DATASIZE_8BIT;
    handle_storage.Init.CLKPolarity =
        Config::clock_polarity == spi_clock_polarity::idle_high ? SPI_POLARITY_HIGH : SPI_POLARITY_LOW;
    handle_storage.Init.CLKPhase =
        Config::clock_phase == spi_clock_phase::second_edge ? SPI_PHASE_2EDGE : SPI_PHASE_1EDGE;
    handle_storage.Init.NSS = serial_slave_enabled<Config>() ? SPI_NSS_HARD_INPUT : SPI_NSS_SOFT;
    handle_storage.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    handle_storage.Init.FirstBit = Config::bit_order == spi_bit_order::lsb_first ? SPI_FIRSTBIT_LSB : SPI_FIRSTBIT_MSB;
    handle_storage.Init.TIMode = SPI_TIMODE_DISABLE;
    handle_storage.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;

    if (HAL_SPI_Init(&handle_storage) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config>
  static bool transfer(std::span<const std::uint8_t> tx_payload, std::span<std::uint8_t> rx_payload) noexcept
  {
#if MICROHAL_STM32_HAL_HAS_SPI
    auto& handle_storage = handle<Config>();
    constexpr std::uint32_t timeout_ms = timeout_for<Config>();
    if (HAL_SPI_TransmitReceive(&handle_storage, hal_tx_buffer(tx_payload.data()), rx_payload.data(),
                                static_cast<std::uint16_t>(tx_payload.size()), timeout_ms) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    (void)tx_payload;
    (void)rx_payload;
    return false;
#endif
  }

  template <typename Config> static status write_blocking(const std::span<const std::uint8_t> payload) noexcept
  {
#if MICROHAL_STM32_HAL_HAS_SPI
    if (payload.size() > UINT16_MAX)
    {
      return status::invalid_argument;
    }
    auto& handle_storage = handle<Config>();
    return transfer_status(HAL_SPI_Transmit(&handle_storage, hal_tx_buffer(payload.data()),
                                            static_cast<std::uint16_t>(payload.size()), timeout_for<Config>()));
#else
    (void)payload;
    return status::unsupported;
#endif
  }

  template <typename Config> static status read_blocking(const std::span<std::uint8_t> payload) noexcept
  {
#if MICROHAL_STM32_HAL_HAS_SPI
    if (payload.size() > UINT16_MAX)
    {
      return status::invalid_argument;
    }
    auto& handle_storage = handle<Config>();
    return transfer_status(HAL_SPI_Receive(&handle_storage, payload.data(), static_cast<std::uint16_t>(payload.size()),
                                           timeout_for<Config>()));
#else
    (void)payload;
    return status::unsupported;
#endif
  }

  template <typename Config, auto Handler>
  static status transfer_async(std::span<const std::uint8_t> tx_payload, std::span<std::uint8_t> rx_payload) noexcept
  {
#if MICROHAL_STM32_HAL_HAS_SPI && defined(USE_HAL_SPI_REGISTER_CALLBACKS) && (USE_HAL_SPI_REGISTER_CALLBACKS == 1)
    if ((tx_payload.size() > UINT16_MAX) || (rx_payload.size() > UINT16_MAX))
    {
      return status::invalid_argument;
    }

    auto& transfer_state = state<Config, Handler>();
    if (transfer_state.active)
    {
      return status::busy;
    }

    auto& handle_storage = handle<Config>();
    transfer_state.previous_complete_callback = handle_storage.TxRxCpltCallback;
    transfer_state.previous_error_callback = handle_storage.ErrorCallback;
    transfer_state.active = true;
    handle_storage.TxRxCpltCallback = async_complete<Config, Handler>;
    handle_storage.ErrorCallback = async_error<Config, Handler>;

    const HAL_StatusTypeDef result =
        HAL_SPI_TransmitReceive_IT(&handle_storage, hal_tx_buffer(tx_payload.data()), rx_payload.data(),
                                   static_cast<std::uint16_t>(tx_payload.size()));
    if (result == HAL_OK) [[likely]]
    {
      return status::ok;
    }
    restore_callbacks<Config, Handler>();
    return transfer_status(result);
#else
    (void)tx_payload;
    (void)rx_payload;
    (void)Handler;
    return status::unsupported;
#endif
  }
};

struct qspi_api final
{
#if defined(QUADSPI)
  template <typename Config> static auto& handle() noexcept
  {
    static QSPI_HandleTypeDef handle_storage{};
    return handle_storage;
  }

  static bool instruction_mode(const qspi_io_mode mode, std::uint32_t& hal_mode) noexcept
  {
    switch (mode)
    {
    case qspi_io_mode::none:
      hal_mode = QSPI_INSTRUCTION_NONE;
      return true;
    case qspi_io_mode::single:
      hal_mode = QSPI_INSTRUCTION_1_LINE;
      return true;
    case qspi_io_mode::dual:
      hal_mode = QSPI_INSTRUCTION_2_LINES;
      return true;
    case qspi_io_mode::quad:
      hal_mode = QSPI_INSTRUCTION_4_LINES;
      return true;
    case qspi_io_mode::octo:
      return false;
    }
    return false;
  }

  static bool address_mode(const qspi_io_mode mode, std::uint32_t& hal_mode) noexcept
  {
    switch (mode)
    {
    case qspi_io_mode::none:
      hal_mode = QSPI_ADDRESS_NONE;
      return true;
    case qspi_io_mode::single:
      hal_mode = QSPI_ADDRESS_1_LINE;
      return true;
    case qspi_io_mode::dual:
      hal_mode = QSPI_ADDRESS_2_LINES;
      return true;
    case qspi_io_mode::quad:
      hal_mode = QSPI_ADDRESS_4_LINES;
      return true;
    case qspi_io_mode::octo:
      return false;
    }
    return false;
  }

  static bool data_mode(const qspi_io_mode mode, std::uint32_t& hal_mode) noexcept
  {
    switch (mode)
    {
    case qspi_io_mode::none:
      hal_mode = QSPI_DATA_NONE;
      return true;
    case qspi_io_mode::single:
      hal_mode = QSPI_DATA_1_LINE;
      return true;
    case qspi_io_mode::dual:
      hal_mode = QSPI_DATA_2_LINES;
      return true;
    case qspi_io_mode::quad:
      hal_mode = QSPI_DATA_4_LINES;
      return true;
    case qspi_io_mode::octo:
      return false;
    }
    return false;
  }

  static bool address_size(const std::uint8_t bytes, std::uint32_t& hal_size) noexcept
  {
    switch (bytes)
    {
    case 0U:
      hal_size = QSPI_ADDRESS_NONE;
      return true;
    case 1U:
      hal_size = QSPI_ADDRESS_8_BITS;
      return true;
    case 2U:
      hal_size = QSPI_ADDRESS_16_BITS;
      return true;
    case 3U:
      hal_size = QSPI_ADDRESS_24_BITS;
      return true;
    case 4U:
      hal_size = QSPI_ADDRESS_32_BITS;
      return true;
    default:
      return false;
    }
  }

  static bool prepare_command(const qspi_command& request, const std::uint32_t data_length,
                              QSPI_CommandTypeDef& hal_command) noexcept
  {
    if (!instruction_mode(request.instruction_mode, hal_command.InstructionMode) ||
        !address_mode(request.address_mode, hal_command.AddressMode) ||
        !data_mode(request.data_mode, hal_command.DataMode) ||
        !address_size(request.address_size_bytes, hal_command.AddressSize))
    {
      return false;
    }

    hal_command.Instruction = request.instruction;
    hal_command.Address = request.address;
    hal_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    hal_command.AlternateBytes = 0U;
    hal_command.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
    hal_command.DummyCycles = request.dummy_cycles;
    hal_command.NbData = data_length;
    hal_command.DdrMode = QSPI_DDR_MODE_DISABLE;
#if defined(QSPI_DDR_HHC_ANALOG_DELAY)
    hal_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
#endif
    hal_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
    return true;
  }
#endif

  template <typename Config> static bool initialize() noexcept
  {
#if defined(QUADSPI)
    auto& handle_storage = handle<Config>();
    handle_storage.Instance = QUADSPI;
    handle_storage.Init.ClockPrescaler = 1U;
    handle_storage.Init.FifoThreshold = 4U;
    handle_storage.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    handle_storage.Init.FlashSize = 23U; // 2^(23+1) = 16 MB
    handle_storage.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE;
    handle_storage.Init.ClockMode = QSPI_CLOCK_MODE_0;

    if (HAL_QSPI_Init(&handle_storage) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config>
  static status command_write(const qspi_command& request, const std::span<const std::uint8_t> payload) noexcept
  {
#if defined(QUADSPI)
    auto& handle_storage = handle<Config>();
    QSPI_CommandTypeDef hal_command{};
    if (!prepare_command(request, static_cast<std::uint32_t>(payload.size()), hal_command))
    {
      return status::unsupported;
    }
    if (HAL_QSPI_Command(&handle_storage, &hal_command, timeout_for<Config>()) != HAL_OK)
    {
      return status::error;
    }
    if (payload.empty() || (HAL_QSPI_Transmit(&handle_storage, const_cast<std::uint8_t*>(payload.data()),
                                              timeout_for<Config>()) == HAL_OK))
    {
      return status::ok;
    }
    return status::error;
#else
    (void)request;
    (void)payload;
    return status::unsupported;
#endif
  }

  template <typename Config>
  static status command_read(const qspi_command& request, const std::span<std::uint8_t> payload) noexcept
  {
#if defined(QUADSPI)
    auto& handle_storage = handle<Config>();
    QSPI_CommandTypeDef hal_command{};
    if (!prepare_command(request, static_cast<std::uint32_t>(payload.size()), hal_command))
    {
      return status::unsupported;
    }
    if (HAL_QSPI_Command(&handle_storage, &hal_command, timeout_for<Config>()) != HAL_OK)
    {
      return status::error;
    }
    if (payload.empty() || (HAL_QSPI_Receive(&handle_storage, payload.data(), timeout_for<Config>()) == HAL_OK))
    {
      return status::ok;
    }
    return status::error;
#else
    (void)request;
    (void)payload;
    return status::unsupported;
#endif
  }

  template <typename Config> static status enter_memory_mapped(const qspi_command& request) noexcept
  {
#if defined(QUADSPI)
    auto& handle_storage = handle<Config>();
    QSPI_CommandTypeDef hal_command{};
    if (!prepare_command(request, 0U, hal_command))
    {
      return status::unsupported;
    }

    QSPI_MemoryMappedTypeDef memory_mapped{};
    memory_mapped.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
    memory_mapped.TimeOutPeriod = 0U;
    return (HAL_QSPI_MemoryMapped(&handle_storage, &hal_command, &memory_mapped) == HAL_OK) ? status::ok
                                                                                            : status::error;
#else
    (void)request;
    return status::unsupported;
#endif
  }

  template <typename Config> static status exit_memory_mapped() noexcept
  {
#if defined(QUADSPI)
    return (HAL_QSPI_Abort(&handle<Config>()) == HAL_OK) ? status::ok : status::error;
#else
    return status::unsupported;
#endif
  }
};

struct uart_api final
{
#if MICROHAL_STM32_HAL_HAS_UART
  template <typename Config> static auto& handle() noexcept
  {
    static UART_HandleTypeDef handle_storage{};
    return handle_storage;
  }
#endif

  template <typename Config> static bool initialize() noexcept
  {
#if MICROHAL_STM32_HAL_HAS_UART
    auto& handle_storage = handle<Config>();
    handle_storage.Instance = peripheral_instance<USART_TypeDef, Config>();
    handle_storage.Init.BaudRate = Config::baud_rate;
    if constexpr (Config::parity == uart_parity::none)
    {
      handle_storage.Init.WordLength = UART_WORDLENGTH_8B;
      handle_storage.Init.Parity = UART_PARITY_NONE;
    }
    else if constexpr (Config::parity == uart_parity::even)
    {
      handle_storage.Init.WordLength = UART_WORDLENGTH_9B;
      handle_storage.Init.Parity = UART_PARITY_EVEN;
    }
    else
    {
      handle_storage.Init.WordLength = UART_WORDLENGTH_9B;
      handle_storage.Init.Parity = UART_PARITY_ODD;
    }

    if constexpr (Config::stop_bits == uart_stop_bits::one)
    {
      handle_storage.Init.StopBits = UART_STOPBITS_1;
    }
    else if constexpr (Config::stop_bits == uart_stop_bits::two)
    {
      handle_storage.Init.StopBits = UART_STOPBITS_2;
    }
    else
    {
#if defined(UART_STOPBITS_1_5)
      handle_storage.Init.StopBits = UART_STOPBITS_1_5;
#else
      return false;
#endif
    }
    handle_storage.Init.Mode = UART_MODE_TX_RX;
    handle_storage.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    handle_storage.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&handle_storage) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config> static bool write_blocking(std::span<const std::uint8_t> payload) noexcept
  {
#if MICROHAL_STM32_HAL_HAS_UART
    auto& handle_storage = handle<Config>();
    constexpr std::uint32_t timeout_ms = timeout_for<Config>();
    if (HAL_UART_Transmit(&handle_storage, hal_tx_buffer(payload.data()), static_cast<std::uint16_t>(payload.size()),
                          timeout_ms) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    (void)payload;
    return false;
#endif
  }

  template <typename Config> static bool read_blocking(std::span<std::uint8_t> payload) noexcept
  {
#if MICROHAL_STM32_HAL_HAS_UART
    auto& handle_storage = handle<Config>();
    constexpr std::uint32_t timeout_ms = timeout_for<Config>();
    if (HAL_UART_Receive(&handle_storage, payload.data(), static_cast<std::uint16_t>(payload.size()), timeout_ms) ==
        HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    (void)payload;
    return false;
#endif
  }

  template <typename Config, auto Handler> static bool start_async_rx(std::span<std::uint8_t> payload) noexcept
  {
#if MICROHAL_STM32_HAL_HAS_UART
    auto& handle_storage = handle<Config>();
    if (HAL_UART_Receive_IT(&handle_storage, payload.data(), static_cast<std::uint16_t>(payload.size())) == HAL_OK)
        [[likely]]
    {
      return true;
    }
    return false;
#else
    (void)payload;
    (void)Handler;
    return false;
#endif
  }

  template <typename Config, auto Handler> static bool start_async_tx(std::span<const std::uint8_t> payload) noexcept
  {
#if MICROHAL_STM32_HAL_HAS_UART
    auto& handle_storage = handle<Config>();
    if (HAL_UART_Transmit_IT(&handle_storage, hal_tx_buffer(payload.data()),
                             static_cast<std::uint16_t>(payload.size())) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
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
#if defined(RTC)
  static inline RTC_HandleTypeDef handle_storage{};
#endif

  template <typename Config> static bool initialize() noexcept
  {
#if defined(RTC)
    handle_storage.Instance = RTC;
#if defined(STM32F1)
    handle_storage.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
    handle_storage.Init.OutPut = RTC_OUTPUTSOURCE_NONE;
#else
    handle_storage.Init.HourFormat = RTC_HOURFORMAT_24;
    handle_storage.Init.AsynchPrediv = 127U;
    handle_storage.Init.SynchPrediv = 255U;
    handle_storage.Init.OutPut = RTC_OUTPUT_DISABLE;
    handle_storage.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    handle_storage.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
#endif

    if (HAL_RTC_Init(&handle_storage) == HAL_OK) [[likely]]
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
    RTC_TimeTypeDef time{};
    time.Hours = hours;
    time.Minutes = minutes;
    time.Seconds = seconds;
#if !defined(STM32F1)
    time.TimeFormat = RTC_HOURFORMAT12_AM;
    time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    time.StoreOperation = RTC_STOREOPERATION_RESET;
#endif

    if (HAL_RTC_SetTime(&handle_storage, &time, RTC_FORMAT_BIN) == HAL_OK) [[likely]]
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
    RTC_TimeTypeDef time{};
    RTC_DateTypeDef date{}; // Required by HAL but not used

    if (HAL_RTC_GetTime(&handle_storage, &time, RTC_FORMAT_BIN) == HAL_OK) [[likely]]
    {
      HAL_RTC_GetDate(&handle_storage, &date, RTC_FORMAT_BIN); // Must read date after time
      hours = time.Hours;
      minutes = time.Minutes;
      seconds = time.Seconds;
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
#if defined(FLASH_TYPEERASE_PAGES)
  template <typename EraseConfig>
  static void configure_page_erase(EraseConfig& erase_init, const std::uint32_t address,
                                   const std::uint32_t bytes) noexcept
  {
#if defined(FLASH_PAGE_SIZE)
    constexpr std::uint32_t page_size = FLASH_PAGE_SIZE;
#else
    constexpr std::uint32_t page_size = 2048U;
#endif

    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.NbPages = (bytes + page_size - 1U) / page_size;
    if constexpr (requires(EraseConfig config) { config.Page; })
    {
      erase_init.Page = (address - FLASH_BASE) / page_size;
    }
    else
    {
      erase_init.PageAddress = address;
    }
  }
#endif

  template <typename Config> static result<flash_erase_region> erase_region(const std::uint32_t address) noexcept
  {
#if defined(FLASH)
    (void)sizeof(Config);
    return flash_erase_region_at(address);
#else
    (void)sizeof(Config);
    (void)address;
    return status::unsupported;
#endif
  }

  template <typename Config> static bool erase(const std::uint32_t address, const std::uint32_t bytes) noexcept
  {
#if defined(FLASH)
    std::uint64_t cursor = address;
    std::uint64_t remaining = bytes;
    while (remaining > 0U)
    {
      if (cursor > UINT32_MAX)
      {
        return false;
      }
      const auto region = erase_region<Config>(static_cast<std::uint32_t>(cursor));
      if (!region.has_value() || (region.value().start_address != cursor) || (region.value().size_bytes == 0U) ||
          (region.value().size_bytes > remaining))
      {
        return false;
      }
      cursor += region.value().size_bytes;
      remaining -= region.value().size_bytes;
    }

    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase_init{};
#if defined(FLASH_TYPEERASE_PAGES)
    configure_page_erase(erase_init, address, bytes);
#elif defined(FLASH_TYPEERASE_SECTORS)
    erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_init.Sector = flash_sector_from_address(address);
    erase_init.NbSectors = flash_sector_span(address, bytes);
#if defined(FLASH_VOLTAGE_RANGE_3)
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;
#endif
#else
    HAL_FLASH_Lock();
    return false;
#endif

    std::uint32_t error = 0U;
    if (HAL_FLASHEx_Erase(&erase_init, &error) == HAL_OK) [[likely]]
    {
      HAL_FLASH_Lock();
      return true;
    }
    HAL_FLASH_Lock();
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
    if (data.empty())
    {
      return false;
    }

    HAL_FLASH_Unlock();

#if defined(FLASH_TYPEPROGRAM_WORD)
    if (((address % sizeof(std::uint32_t)) != 0U) || ((data.size() % sizeof(std::uint32_t)) != 0U))
    {
      HAL_FLASH_Lock();
      return false;
    }
    for (std::size_t i = 0U; i < data.size(); i += 4U)
    {
      std::uint32_t word = 0U;
      for (std::size_t j = 0U; j < 4U && (i + j) < data.size(); ++j)
      {
        word |= static_cast<std::uint32_t>(data[i + j]) << (j * 8U);
      }

      if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address + i, word) != HAL_OK) [[unlikely]]
      {
        HAL_FLASH_Lock();
        return false;
      }
    }
#elif defined(FLASH_TYPEPROGRAM_DOUBLEWORD)
    if (((address % sizeof(std::uint64_t)) != 0U) || ((data.size() % sizeof(std::uint64_t)) != 0U))
    {
      HAL_FLASH_Lock();
      return false;
    }
    for (std::size_t i = 0U; i < data.size(); i += 8U)
    {
      std::uint64_t double_word = 0U;
      for (std::size_t j = 0U; j < 8U && (i + j) < data.size(); ++j)
      {
        double_word |= static_cast<std::uint64_t>(data[i + j]) << (j * 8U);
      }

      if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address + i, double_word) != HAL_OK) [[unlikely]]
      {
        HAL_FLASH_Lock();
        return false;
      }
    }
#else
    HAL_FLASH_Lock();
    return false;
#endif

    HAL_FLASH_Lock();
    return true;
#else
    (void)address;
    (void)data;
    return false;
#endif
  }

  template <typename Config> static bool read(const std::uint32_t address, const std::span<std::uint8_t> data) noexcept
  {
#if defined(FLASH)
    const auto* flash = reinterpret_cast<const std::uint8_t*>(address);
    for (std::size_t i = 0U; i < data.size(); ++i)
    {
      data[i] = flash[i];
    }
    (void)sizeof(Config);
    return true;
#else
    (void)address;
    (void)data;
    (void)sizeof(Config);
    return false;
#endif
  }
};

struct timer_api final
{
#if MICROHAL_STM32_HAL_HAS_TIM
  template <typename Config> static auto& handle() noexcept
  {
    static TIM_HandleTypeDef handle_storage{};
    return handle_storage;
  }
#endif

  template <typename Config> static bool initialize() noexcept
  {
#if MICROHAL_STM32_HAL_HAS_TIM
    auto& handle_storage = handle<Config>();
    handle_storage.Instance = peripheral_instance<TIM_TypeDef, Config>();
    handle_storage.Init.Prescaler = Config::prescaler;
    handle_storage.Init.CounterMode = TIM_COUNTERMODE_UP;
    handle_storage.Init.Period = Config::period - 1U;
    handle_storage.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    handle_storage.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&handle_storage) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config> static status start() noexcept
  {
#if MICROHAL_STM32_HAL_HAS_TIM
    return (HAL_TIM_Base_Start(&handle<Config>()) == HAL_OK) ? status::ok : status::error;
#else
    return status::unsupported;
#endif
  }

  template <typename Config> static status stop() noexcept
  {
#if MICROHAL_STM32_HAL_HAS_TIM
    return (HAL_TIM_Base_Stop(&handle<Config>()) == HAL_OK) ? status::ok : status::error;
#else
    return status::unsupported;
#endif
  }

  template <typename Config> static status set_period(const std::uint32_t period) noexcept
  {
#if MICROHAL_STM32_HAL_HAS_TIM
    if (period == 0U)
    {
      return status::invalid_argument;
    }
    __HAL_TIM_SET_AUTORELOAD(&handle<Config>(), period - 1U);
    return status::ok;
#else
    (void)period;
    return status::unsupported;
#endif
  }

  template <typename Config> static std::uint32_t counter() noexcept
  {
#if MICROHAL_STM32_HAL_HAS_TIM
    auto& handle_storage = handle<Config>();
    return __HAL_TIM_GET_COUNTER(&handle_storage);
#else
    return 0U;
#endif
  }

  template <typename Config> static status reset_counter() noexcept
  {
#if MICROHAL_STM32_HAL_HAS_TIM
    __HAL_TIM_SET_COUNTER(&handle<Config>(), 0U);
    return status::ok;
#else
    return status::unsupported;
#endif
  }

  template <typename Config> static result<std::uint32_t> period() noexcept
  {
#if MICROHAL_STM32_HAL_HAS_TIM
    return __HAL_TIM_GET_AUTORELOAD(&handle<Config>()) + 1U;
#else
    return status::unsupported;
#endif
  }

  template <typename Config, typename ChannelConfig> static result<std::uint32_t> capture_value() noexcept
  {
#if MICROHAL_STM32_HAL_HAS_TIM
    if constexpr (ChannelConfig::channel_index == 1U)
    {
      return HAL_TIM_ReadCapturedValue(&handle<Config>(), TIM_CHANNEL_1);
    }
    else if constexpr (ChannelConfig::channel_index == 2U)
    {
      return HAL_TIM_ReadCapturedValue(&handle<Config>(), TIM_CHANNEL_2);
    }
    else if constexpr (ChannelConfig::channel_index == 3U)
    {
      return HAL_TIM_ReadCapturedValue(&handle<Config>(), TIM_CHANNEL_3);
    }
    else if constexpr (ChannelConfig::channel_index == 4U)
    {
      return HAL_TIM_ReadCapturedValue(&handle<Config>(), TIM_CHANNEL_4);
    }
    else
    {
      return status::invalid_argument;
    }
#else
    (void)sizeof(Config);
    (void)sizeof(ChannelConfig);
    return status::unsupported;
#endif
  }
};

#include "microhal/backends/stm32/adc_api.inc"

struct dac_api final
{
#if MICROHAL_STM32_HAL_HAS_DAC
  template <typename Config> static auto& handle() noexcept
  {
    static DAC_HandleTypeDef handle_storage{};
    return handle_storage;
  }

  template <typename Config> static constexpr auto instance() noexcept -> DAC_TypeDef*
  {
    if constexpr (requires { Config::base_address; })
    {
      return reinterpret_cast<DAC_TypeDef*>(Config::base_address);
    }
    else if constexpr (requires { typename Config::binding; })
    {
      using binding = typename Config::binding;
      if constexpr (requires { binding::base_address; })
      {
        return reinterpret_cast<DAC_TypeDef*>(binding::base_address);
      }
      else
      {
        static_assert(dependent_false_v<Config>,
                      "microHAL: STM32 DAC bindings must expose binding::base_address or Config::base_address.");
        return nullptr;
      }
    }
    else if constexpr (requires { Config::instance_index; })
    {
#if defined(DAC1)
      if constexpr (Config::instance_index == 1U)
      {
        return DAC1;
      }
#endif
#if defined(DAC2)
      else if constexpr (Config::instance_index == 2U)
      {
        return DAC2;
      }
#endif
#if defined(DAC3)
      else if constexpr (Config::instance_index == 3U)
      {
        return DAC3;
      }
#endif
#if defined(DAC4)
      else if constexpr (Config::instance_index == 4U)
      {
        return DAC4;
      }
#endif
#if defined(DAC) && !defined(DAC1)
      else if constexpr (Config::instance_index == 1U)
      {
        return DAC;
      }
#endif
      else
      {
        static_assert(dependent_false_v<Config>,
                      "microHAL: STM32 DAC instance_index does not identify an available DAC instance.");
        return nullptr;
      }
    }
    else
    {
      static_assert(dependent_false_v<Config>,
                    "microHAL: STM32 DAC configs must expose instance_index or a nested binding with base_address.");
      return nullptr;
    }
  }

  template <typename Config> static consteval std::uint32_t channel() noexcept
  {
    if constexpr (Config::channel == 1U)
    {
      return DAC_CHANNEL_1;
    }
#if defined(DAC_CHANNEL_2)
    else if constexpr (Config::channel == 2U)
    {
      return DAC_CHANNEL_2;
    }
#endif
    else
    {
      static_assert(dependent_false_v<Config>, "microHAL: STM32 DAC channel is not available for this device.");
      return DAC_CHANNEL_1;
    }
  }
#endif

  template <typename Config> static bool initialize() noexcept
  {
#if MICROHAL_STM32_HAL_HAS_DAC
    auto& handle_storage = handle<Config>();
    handle_storage.Instance = instance<Config>();
    if (HAL_DAC_Init(&handle_storage) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config> static status write_blocking(const std::uint16_t value) noexcept
  {
#if MICROHAL_STM32_HAL_HAS_DAC
    auto& handle_storage = handle<Config>();
    if (HAL_DAC_SetValue(&handle_storage, channel<Config>(), DAC_ALIGN_12B_R, value) != HAL_OK)
    {
      return status::error;
    }
    return (HAL_DAC_Start(&handle_storage, channel<Config>()) == HAL_OK) ? status::ok : status::error;
#else
    (void)value;
    return status::unsupported;
#endif
  }

  template <typename Config> static status write_dual(const std::uint16_t value1, const std::uint16_t value2) noexcept
  {
#if MICROHAL_STM32_HAL_HAS_DAC
    auto& handle_storage = handle<Config>();
    return (HAL_DACEx_DualSetValue(&handle_storage, DAC_ALIGN_12B_R, value1, value2) == HAL_OK) ? status::ok
                                                                                                : status::error;
#else
    (void)value1;
    (void)value2;
    return status::unsupported;
#endif
  }

  template <typename Config, auto Handler>
  static bool start_dma_waveform(const std::span<const std::uint16_t> samples) noexcept
  {
#if MICROHAL_STM32_HAL_HAS_DAC
    auto& handle_storage = handle<Config>();
    if (HAL_DAC_Start_DMA(&handle_storage, channel<Config>(), hal_tx_buffer_as_words(samples.data()),
                          static_cast<std::uint32_t>(samples.size()), DAC_ALIGN_12B_R) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    (void)samples;
    (void)Handler;
    return false;
#endif
  }
};

struct can_api final
{
#if MICROHAL_STM32_HAL_HAS_LEGACY_CAN
  template <typename Config> static auto& handle() noexcept
  {
    static CAN_HandleTypeDef handle_storage{};
    return handle_storage;
  }
#endif

  template <typename Config> static bool initialize() noexcept
  {
#if MICROHAL_STM32_HAL_HAS_LEGACY_CAN
    auto& handle_storage = handle<Config>();
    handle_storage.Instance = peripheral_instance<CAN_TypeDef, Config>();
    handle_storage.Init.Prescaler = 16U;
    handle_storage.Init.Mode = CAN_MODE_NORMAL;
    handle_storage.Init.SyncJumpWidth = CAN_SJW_1TQ;
    handle_storage.Init.TimeSeg1 = CAN_BS1_1TQ;
    handle_storage.Init.TimeSeg2 = CAN_BS2_1TQ;
    handle_storage.Init.TimeTriggeredMode = DISABLE;
    handle_storage.Init.AutoBusOff = DISABLE;
    handle_storage.Init.AutoWakeUp = DISABLE;
    handle_storage.Init.AutoRetransmission = ENABLE;
    handle_storage.Init.ReceiveFifoLocked = DISABLE;
    handle_storage.Init.TransmitFifoPriority = DISABLE;

    if (HAL_CAN_Init(&handle_storage) == HAL_OK) [[likely]]
    {
      if (HAL_CAN_Start(&handle_storage) == HAL_OK) [[likely]]
      {
        return true;
      }
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config> static bool transmit(const microhal::can::message& msg) noexcept
  {
#if MICROHAL_STM32_HAL_HAS_LEGACY_CAN
    auto& handle_storage = handle<Config>();
    CAN_TxHeaderTypeDef header{};
    header.StdId = msg.extended_id ? 0U : static_cast<std::uint32_t>(msg.id & 0x7FFU);
    header.ExtId = msg.extended_id ? msg.id : 0U;
    header.RTR = msg.remote_frame ? CAN_RTR_REMOTE : CAN_RTR_DATA;
    header.IDE = msg.extended_id ? CAN_ID_EXT : CAN_ID_STD;
    header.DLC = static_cast<std::uint32_t>((msg.dlc > 8U) ? 8U : msg.dlc);
    header.TransmitGlobalTime = DISABLE;

    std::uint32_t mailbox = 0U;
    if (HAL_CAN_AddTxMessage(&handle_storage, &header, hal_tx_buffer(msg.data), &mailbox) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    (void)msg;
    return false;
#endif
  }

  template <typename Config> static bool receive(microhal::can::message& msg) noexcept
  {
#if MICROHAL_STM32_HAL_HAS_LEGACY_CAN
    auto& handle_storage = handle<Config>();
    CAN_RxHeaderTypeDef header{};
    if (HAL_CAN_GetRxMessage(&handle_storage, CAN_RX_FIFO0, &header, msg.data) == HAL_OK) [[likely]]
    {
      msg.id = (header.IDE == CAN_ID_EXT) ? header.ExtId : header.StdId;
      msg.dlc = static_cast<std::uint8_t>((header.DLC > 8U) ? 8U : header.DLC);
      msg.extended_id = (header.IDE == CAN_ID_EXT);
      msg.remote_frame = (header.RTR == CAN_RTR_REMOTE);
      return true;
    }
    return false;
#else
    (void)msg;
    return false;
#endif
  }

  template <typename Config>
  static bool configure_filter(const std::uint32_t id, const std::uint32_t mask, const std::uint8_t fifo) noexcept
  {
#if MICROHAL_STM32_HAL_HAS_LEGACY_CAN
    auto& handle_storage = handle<Config>();
    CAN_FilterTypeDef filter{};
    filter.FilterIdHigh = static_cast<std::uint16_t>(id << 5U);
    filter.FilterIdLow = 0x0000U;
    filter.FilterMaskIdHigh = static_cast<std::uint16_t>(mask << 5U);
    filter.FilterMaskIdLow = 0x0000U;
    filter.FilterFIFOAssignment = (fifo == 0U) ? CAN_RX_FIFO0 : CAN_RX_FIFO1;
    filter.FilterBank = 0U;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterActivation = CAN_FILTER_ENABLE;

    if (HAL_CAN_ConfigFilter(&handle_storage, &filter) == HAL_OK) [[likely]]
    {
      return true;
    }
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
#if MICROHAL_STM32_HAL_HAS_LEGACY_CAN
    auto& handle_storage = handle<Config>();
    if (HAL_CAN_ActivateNotification(&handle_storage, CAN_IT_RX_FIFO0_MSG_PENDING) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    (void)Handler;
    return false;
#endif
  }
};

struct usb_api final
{
#if defined(USB_OTG_FS) || defined(USB_OTG_HS) || defined(USB_DRD_FS) || defined(USB_DRD_HS) || defined(USB)
  static inline PCD_HandleTypeDef handle_storage{};

  template <typename PcdInit> static void configure_init(PcdInit& init) noexcept
  {
    if constexpr (requires(PcdInit value) { value.dev_endpoints; })
    {
      init.dev_endpoints = 4U;
    }

    if constexpr (requires(PcdInit value) { value.speed; })
    {
#if defined(USB_OTG_HS) || defined(USB_DRD_HS)
      init.speed = PCD_SPEED_HIGH;
#else
      init.speed = PCD_SPEED_FULL;
#endif
    }

    if constexpr (requires(PcdInit value) { value.phy_itface; })
    {
      init.phy_itface = PCD_PHY_EMBEDDED;
    }

    if constexpr (requires(PcdInit value) { value.Sof_enable; })
    {
      init.Sof_enable = DISABLE;
    }

    if constexpr (requires(PcdInit value) { value.low_power_enable; })
    {
      init.low_power_enable = DISABLE;
    }

    if constexpr (requires(PcdInit value) { value.lpm_enable; })
    {
      init.lpm_enable = DISABLE;
    }

    if constexpr (requires(PcdInit value) { value.vbus_sensing_enable; })
    {
      init.vbus_sensing_enable = DISABLE;
    }

    if constexpr (requires(PcdInit value) { value.use_dedicated_ep1; })
    {
      init.use_dedicated_ep1 = DISABLE;
    }
  }
#endif

  template <typename Config> static bool initialize() noexcept
  {
#if defined(USB_OTG_FS) || defined(USB_OTG_HS) || defined(USB_DRD_FS) || defined(USB_DRD_HS) || defined(USB)
#if defined(USB_OTG_FS)
    handle_storage.Instance = USB_OTG_FS;
#elif defined(USB_OTG_HS)
    handle_storage.Instance = USB_OTG_HS;
#elif defined(USB_DRD_FS)
    handle_storage.Instance = USB_DRD_FS;
#elif defined(USB_DRD_HS)
    handle_storage.Instance = USB_DRD_HS;
#else
    handle_storage.Instance = USB;
#endif

    configure_init(handle_storage.Init);

    if (HAL_PCD_Init(&handle_storage) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config> static bool connect() noexcept
  {
#if defined(USB_OTG_FS) || defined(USB_OTG_HS) || defined(USB_DRD_FS) || defined(USB_DRD_HS) || defined(USB)
    if (HAL_PCD_Start(&handle_storage) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config> static bool disconnect() noexcept
  {
#if defined(USB_OTG_FS) || defined(USB_OTG_HS) || defined(USB_DRD_FS) || defined(USB_DRD_HS) || defined(USB)
    if (HAL_PCD_Stop(&handle_storage) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config>
  static bool endpoint_transmit(const std::uint8_t endpoint, const std::span<const std::uint8_t> data) noexcept
  {
#if defined(USB_OTG_FS) || defined(USB_OTG_HS) || defined(USB_DRD_FS) || defined(USB_DRD_HS) || defined(USB)
    if (HAL_PCD_EP_Transmit(&handle_storage, endpoint, hal_tx_buffer(data.data()),
                            static_cast<std::uint16_t>(data.size())) == HAL_OK) [[likely]]
    {
      return true;
    }
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
#if defined(USB_OTG_FS) || defined(USB_OTG_HS) || defined(USB_DRD_FS) || defined(USB_DRD_HS) || defined(USB)
    if (HAL_PCD_EP_Receive(&handle_storage, endpoint, buffer.data(), static_cast<std::uint16_t>(buffer.size())) ==
        HAL_OK) [[likely]]
    {
      return HAL_PCD_EP_GetRxCount(&handle_storage, endpoint);
    }
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
#if defined(ETH) && defined(HAL_ETH_MODULE_ENABLED) && MICROHAL_STM32_HAL_ETH_V2_RUNTIME
    using state = stm32_ethernet_state<Config>;

    state::reset_rx_pool();
    state::handle_storage.Instance = stm32_ethernet_instance<Config>();
    state::handle_storage.Init.MACAddr = state::mac_address.data();
    state::handle_storage.Init.MediaInterface =
        (ethernet_interface<Config>() == ethernet_media_interface::mii) ? HAL_ETH_MII_MODE : HAL_ETH_RMII_MODE;
    state::handle_storage.Init.TxDesc = state::tx_descriptors.data();
    state::handle_storage.Init.RxDesc = state::rx_descriptors.data();
    state::handle_storage.Init.RxBuffLen = static_cast<std::uint32_t>(state::rx_buffer_bytes);

    if (HAL_ETH_Init(&state::handle_storage) != HAL_OK) [[unlikely]]
    {
      return false;
    }

    if (HAL_ETH_RegisterRxAllocateCallback(&state::handle_storage, state::rx_allocate_callback) != HAL_OK) [[unlikely]]
    {
      return false;
    }

    if (HAL_ETH_RegisterRxLinkCallback(&state::handle_storage, state::rx_link_callback) != HAL_OK) [[unlikely]]
    {
      return false;
    }

    ETH_MACConfigTypeDef mac_config{};
    if (HAL_ETH_GetMACConfig(&state::handle_storage, &mac_config) != HAL_OK) [[unlikely]]
    {
      return false;
    }

    mac_config.Speed = (Config::link_speed == ethernet_link_speed::mbps10) ? ETH_SPEED_10M : ETH_SPEED_100M;
    mac_config.DuplexMode = (Config::duplex == ethernet_duplex::full) ? ETH_FULLDUPLEX_MODE : ETH_HALFDUPLEX_MODE;
    mac_config.ChecksumOffload = Config::enable_checksum_offload ? ENABLE : DISABLE;

    return HAL_ETH_SetMACConfig(&state::handle_storage, &mac_config) == HAL_OK;
#else
    return false;
#endif
  }

  template <typename Config> static bool start() noexcept
  {
#if defined(ETH) && defined(HAL_ETH_MODULE_ENABLED) && MICROHAL_STM32_HAL_ETH_V2_RUNTIME
    using state = stm32_ethernet_state<Config>;

    if constexpr (Config::enable_interrupts)
    {
      return HAL_ETH_Start_IT(&state::handle_storage) == HAL_OK;
    }

    return HAL_ETH_Start(&state::handle_storage) == HAL_OK;
#else
    return false;
#endif
  }

  template <typename Config> static bool stop() noexcept
  {
#if defined(ETH) && defined(HAL_ETH_MODULE_ENABLED) && MICROHAL_STM32_HAL_ETH_V2_RUNTIME
    using state = stm32_ethernet_state<Config>;
    return HAL_ETH_Stop(&state::handle_storage) == HAL_OK;
#else
    return false;
#endif
  }

  template <typename Config> static bool transmit_frame(const std::span<const std::uint8_t> frame) noexcept
  {
#if defined(ETH) && defined(HAL_ETH_MODULE_ENABLED) && MICROHAL_STM32_HAL_ETH_V2_RUNTIME
    using state = stm32_ethernet_state<Config>;

    if (frame.empty())
    {
      return false;
    }

    ETH_BufferTypeDef tx_buffer{};
    tx_buffer.buffer = hal_tx_buffer(frame.data());
    tx_buffer.len = static_cast<std::uint32_t>(frame.size());
    tx_buffer.next = nullptr;

    ETH_TxPacketConfigTypeDef tx_config{};
    tx_config.Length = static_cast<std::uint32_t>(frame.size());
    tx_config.TxBuffer = &tx_buffer;
    tx_config.pData = nullptr;
    tx_config.Attributes = Config::enable_checksum_offload
                               ? (ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD)
                               : ETH_TX_PACKETS_FEATURES_CRCPAD;
    tx_config.ChecksumCtrl =
        Config::enable_checksum_offload ? ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC : ETH_CHECKSUM_DISABLE;
    tx_config.CRCPadCtrl = ETH_CRC_PAD_INSERT;

    return HAL_ETH_Transmit(&state::handle_storage, &tx_config, 20U) == HAL_OK;
#else
    (void)frame;
    return false;
#endif
  }

  template <typename Config> static std::uint32_t receive_frame(const std::span<std::uint8_t> buffer) noexcept
  {
#if defined(ETH) && defined(HAL_ETH_MODULE_ENABLED) && MICROHAL_STM32_HAL_ETH_V2_RUNTIME
    using state = stm32_ethernet_state<Config>;
    using packet_type = typename state::rx_packet;

    void* packet_start = nullptr;
    if (HAL_ETH_ReadData(&state::handle_storage, &packet_start) != HAL_OK) [[unlikely]]
    {
      return 0U;
    }

    auto* packet = static_cast<packet_type*>(packet_start);
    if (packet == nullptr)
    {
      return 0U;
    }

    std::uint32_t total_length = 0U;
    for (packet_type* current = packet; current != nullptr; current = current->next)
    {
      total_length += current->length;
    }

    if (total_length > buffer.size())
    {
      state::release_packet_chain(packet);
      return 0U;
    }

    std::uint8_t* dst = buffer.data();
    for (packet_type* current = packet; current != nullptr; current = current->next)
    {
      std::memcpy(dst, current->data.data(), current->length);
      dst += current->length;
    }

    state::release_packet_chain(packet);
    return total_length;
#else
    (void)buffer;
    return 0U;
#endif
  }

  template <typename Config> static bool set_mac_address(const std::array<std::uint8_t, 6>& address) noexcept
  {
#if defined(ETH) && defined(HAL_ETH_MODULE_ENABLED) && MICROHAL_STM32_HAL_ETH_V2_RUNTIME
    using state = stm32_ethernet_state<Config>;

    state::mac_address = address;

    const bool was_started = HAL_ETH_GetState(&state::handle_storage) == HAL_ETH_STATE_STARTED;
    if (HAL_ETH_DeInit(&state::handle_storage) != HAL_OK) [[unlikely]]
    {
      return false;
    }

    if (!initialize<Config>())
    {
      return false;
    }

    return !was_started || start<Config>();
#else
    (void)address;
    return false;
#endif
  }
};

struct sd_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if (defined(SDIO) || defined(SDMMC1)) && defined(HAL_SD_MODULE_ENABLED) && MICROHAL_STM32_HAL_SD_RUNTIME
    using state = stm32_sd_state<Config>;

    state::handle_storage.Instance = stm32_sd_instance<Config>();
#if defined(SDIO_CLOCK_EDGE_RISING)
    state::handle_storage.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
    state::handle_storage.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
    state::handle_storage.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
    state::handle_storage.Init.HardwareFlowControl =
        sd_hardware_flow_control<Config>() ? SDIO_HARDWARE_FLOW_CONTROL_ENABLE : SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
#else
    state::handle_storage.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
#if defined(SDMMC_CLOCK_BYPASS_DISABLE)
    state::handle_storage.Init.ClockBypass = SDMMC_CLOCK_BYPASS_DISABLE;
#endif
    state::handle_storage.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
    state::handle_storage.Init.HardwareFlowControl =
        sd_hardware_flow_control<Config>() ? SDMMC_HARDWARE_FLOW_CONTROL_ENABLE : SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
#endif
    state::handle_storage.Init.BusWide = stm32_sd_bus_width<Config>();
    state::handle_storage.Init.ClockDiv = sd_clock_divider<Config>();

    if (HAL_SD_Init(&state::handle_storage) != HAL_OK) [[unlikely]]
    {
      return false;
    }

    if (HAL_SD_ConfigWideBusOperation(&state::handle_storage, stm32_sd_bus_width<Config>()) != HAL_OK) [[unlikely]]
    {
      return false;
    }

    return HAL_SD_GetCardInfo(&state::handle_storage, &state::card_info) == HAL_OK;
#else
    return false;
#endif
  }

  template <typename Config>
  static bool read_blocks(const std::uint32_t start_block, const std::span<std::uint8_t> buffer) noexcept
  {
#if (defined(SDIO) || defined(SDMMC1)) && defined(HAL_SD_MODULE_ENABLED) && MICROHAL_STM32_HAL_SD_RUNTIME
    using state = stm32_sd_state<Config>;
    constexpr std::uint32_t block_size = 512U;

    if (buffer.empty() || ((buffer.size() % block_size) != 0U))
    {
      return false;
    }

    return HAL_SD_ReadBlocks(&state::handle_storage, buffer.data(), start_block,
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
#if (defined(SDIO) || defined(SDMMC1)) && defined(HAL_SD_MODULE_ENABLED) && MICROHAL_STM32_HAL_SD_RUNTIME
    using state = stm32_sd_state<Config>;
    constexpr std::uint32_t block_size = 512U;

    if (buffer.empty() || ((buffer.size() % block_size) != 0U))
    {
      return false;
    }

    return HAL_SD_WriteBlocks(&state::handle_storage, hal_tx_buffer(buffer.data()), start_block,
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
#if (defined(SDIO) || defined(SDMMC1)) && defined(HAL_SD_MODULE_ENABLED) && MICROHAL_STM32_HAL_SD_RUNTIME
    using state = stm32_sd_state<Config>;

    if (block_count == 0U)
    {
      return false;
    }

    return HAL_SD_Erase(&state::handle_storage, start_block, start_block + block_count - 1U) == HAL_OK;
#else
    (void)start_block;
    (void)block_count;
    return false;
#endif
  }

  template <typename Config> static std::uint32_t capacity_blocks() noexcept
  {
#if (defined(SDIO) || defined(SDMMC1)) && defined(HAL_SD_MODULE_ENABLED) && MICROHAL_STM32_HAL_SD_RUNTIME
    using state = stm32_sd_state<Config>;

    if (HAL_SD_GetCardInfo(&state::handle_storage, &state::card_info) != HAL_OK) [[unlikely]]
    {
      return 0U;
    }

    return state::card_info.LogBlockNbr;
#else
    return 0U;
#endif
  }
};

struct dma_api final
{
#if defined(DMA1) && MICROHAL_STM32_HAL_STREAM_DMA_RUNTIME
  template <typename Config> static auto& handle() noexcept
  {
    static DMA_HandleTypeDef handle_storage{};
    return handle_storage;
  }

  template <typename Config> static bool& completed() noexcept
  {
    static bool transfer_completed{false};
    return transfer_completed;
  }

  template <typename Config, auto Handler>
  static void completion_callback(DMA_HandleTypeDef* const handle_storage) noexcept
  {
    completed<Config>() = true;
    if constexpr (requires { Handler(); })
    {
      Handler();
    }
    else if constexpr (requires { Handler(handle_storage); })
    {
      Handler(handle_storage);
    }
  }
#endif

  template <typename Config> static bool initialize() noexcept
  {
#if defined(DMA1) && MICROHAL_STM32_HAL_STREAM_DMA_RUNTIME
    auto& handle_storage = handle<Config>();
    completed<Config>() = false;
    handle_storage.Instance = peripheral_instance<DMA_Stream_TypeDef, Config>();
#if defined(STM32H7)
    handle_storage.Init.Request = DMA_REQUEST_MEM2MEM;
#else
    handle_storage.Init.Channel = DMA_CHANNEL_0;
#endif
    handle_storage.Init.Direction = DMA_MEMORY_TO_MEMORY;
    handle_storage.Init.PeriphInc = DMA_PINC_DISABLE;
    handle_storage.Init.MemInc = DMA_MINC_ENABLE;
    handle_storage.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    handle_storage.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    handle_storage.Init.Mode = DMA_NORMAL;
    handle_storage.Init.Priority = DMA_PRIORITY_LOW;
    handle_storage.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    if (HAL_DMA_Init(&handle_storage) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config, auto Handler>
  static bool start_transfer(const std::uint32_t src_addr, const std::uint32_t dst_addr,
                             const std::uint32_t length) noexcept
  {
#if defined(DMA1) && MICROHAL_STM32_HAL_STREAM_DMA_RUNTIME
    auto& handle_storage = handle<Config>();
    completed<Config>() = false;
    handle_storage.XferCpltCallback = completion_callback<Config, Handler>;
    if (HAL_DMA_Start_IT(&handle_storage, src_addr, dst_addr, length) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
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
#if defined(DMA1) && MICROHAL_STM32_HAL_STREAM_DMA_RUNTIME
    auto& handle_storage = handle<Config>();
    if (HAL_DMA_Abort(&handle_storage) == HAL_OK) [[likely]]
    {
      completed<Config>() = false;
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config> static result<dma_transfer_state> transfer_status() noexcept
  {
#if defined(DMA1) && MICROHAL_STM32_HAL_STREAM_DMA_RUNTIME
    const HAL_DMA_StateTypeDef dma_state = HAL_DMA_GetState(&handle<Config>());
    if (dma_state == HAL_DMA_STATE_BUSY)
    {
      return dma_transfer_state::active;
    }
    if (dma_state == HAL_DMA_STATE_ERROR)
    {
      return dma_transfer_state::error;
    }
#if defined(STM32F2) || defined(STM32F4) || defined(STM32F7)
    if (dma_state == HAL_DMA_STATE_TIMEOUT)
    {
      return dma_transfer_state::error;
    }
#endif
    return completed<Config>() ? dma_transfer_state::complete : dma_transfer_state::idle;
#else
    return status::unsupported;
#endif
  }
};

struct pwm_api final
{
#if MICROHAL_STM32_HAL_HAS_TIM
  template <typename Config> static auto& handle() noexcept
  {
    static TIM_HandleTypeDef handle_storage{};
    return handle_storage;
  }
#endif

  template <typename Config> static bool initialize() noexcept
  {
#if MICROHAL_STM32_HAL_HAS_TIM
    auto& handle_storage = handle<Config>();
    handle_storage.Instance = peripheral_instance<TIM_TypeDef, Config>();
    handle_storage.Init.Prescaler = Config::prescaler;
    handle_storage.Init.CounterMode = TIM_COUNTERMODE_UP;
    handle_storage.Init.Period = Config::period;
    handle_storage.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    handle_storage.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_PWM_Init(&handle_storage) == HAL_OK) [[likely]]
    {
      TIM_OC_InitTypeDef oc_config{};
      oc_config.OCMode = TIM_OCMODE_PWM1;
      oc_config.Pulse = 0U;
      oc_config.OCPolarity = TIM_OCPOLARITY_HIGH;
      oc_config.OCFastMode = TIM_OCFAST_DISABLE;
      if (HAL_TIM_PWM_ConfigChannel(&handle_storage, &oc_config, TIM_CHANNEL_1) == HAL_OK) [[likely]]
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
#if MICROHAL_STM32_HAL_HAS_TIM
    auto& handle_storage = handle<Config>();
    const std::uint32_t compare_value =
        (static_cast<std::uint32_t>(handle_storage.Init.Period) * duty_permille) / 1000U;
    __HAL_TIM_SET_COMPARE(&handle_storage, TIM_CHANNEL_1, compare_value);
#else
    (void)duty_permille;
#endif
  }

  template <typename Config> static bool start() noexcept
  {
#if MICROHAL_STM32_HAL_HAS_TIM
    auto& handle_storage = handle<Config>();
    if (HAL_TIM_PWM_Start(&handle_storage, TIM_CHANNEL_1) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config> static bool stop() noexcept
  {
#if MICROHAL_STM32_HAL_HAS_TIM
    auto& handle_storage = handle<Config>();
    if (HAL_TIM_PWM_Stop(&handle_storage, TIM_CHANNEL_1) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    return false;
#endif
  }
};

struct watchdog_api final
{
#if defined(IWDG)
  static inline IWDG_HandleTypeDef handle_storage{};
#endif

  template <typename Config> static bool initialize() noexcept
  {
#if defined(IWDG)
    handle_storage.Instance = IWDG;
    handle_storage.Init.Prescaler = IWDG_PRESCALER_32;
    handle_storage.Init.Reload = 4095U;

    if (HAL_IWDG_Init(&handle_storage) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config> static bool start() noexcept
  {
#if defined(IWDG)
    if (HAL_IWDG_Init(&handle_storage) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config> static bool refresh() noexcept
  {
#if defined(IWDG)
    if (HAL_IWDG_Refresh(&handle_storage) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    return false;
#endif
  }
};

struct i2s_api final
{
#if MICROHAL_STM32_HAL_HAS_SPI && defined(HAL_I2S_MODULE_ENABLED) && defined(I2S_STANDARD_PHILIPS)
  template <typename Config> static auto& handle() noexcept
  {
    static I2S_HandleTypeDef handle_storage{};
    return handle_storage;
  }

#if defined(USE_HAL_I2S_REGISTER_CALLBACKS) && (USE_HAL_I2S_REGISTER_CALLBACKS == 1)
  template <typename Config, auto Handler> struct dma_state final
  {
    bool active{false};
    void (*previous_tx_callback)(I2S_HandleTypeDef*){nullptr};
    void (*previous_error_callback)(I2S_HandleTypeDef*){nullptr};
  };

  template <typename Config, auto Handler> static auto& state() noexcept
  {
    static dma_state<Config, Handler> transfer_state{};
    return transfer_state;
  }

  template <typename Config, auto Handler> static void notify_completion(const status result) noexcept
  {
    if constexpr (requires { Handler(result); })
    {
      Handler(result);
    }
    else if constexpr (requires { Handler(); })
    {
      Handler();
    }
  }

  template <typename Config, auto Handler> static void restore_callbacks() noexcept
  {
    auto& transfer_state = state<Config, Handler>();
    auto& handle_storage = handle<Config>();
    handle_storage.TxCpltCallback = transfer_state.previous_tx_callback;
    handle_storage.ErrorCallback = transfer_state.previous_error_callback;
    transfer_state.active = false;
  }

  template <typename Config, auto Handler> static void dma_complete(I2S_HandleTypeDef* const handle_storage) noexcept
  {
    auto& transfer_state = state<Config, Handler>();
    if (!transfer_state.active || (handle_storage != &handle<Config>()))
    {
      return;
    }

    const auto previous_callback = transfer_state.previous_tx_callback;
    restore_callbacks<Config, Handler>();
    if (previous_callback != nullptr)
    {
      previous_callback(handle_storage);
    }
    notify_completion<Config, Handler>(status::ok);
  }

  template <typename Config, auto Handler> static void dma_error(I2S_HandleTypeDef* const handle_storage) noexcept
  {
    auto& transfer_state = state<Config, Handler>();
    if (!transfer_state.active || (handle_storage != &handle<Config>()))
    {
      return;
    }

    const auto previous_callback = transfer_state.previous_error_callback;
    const status result = (handle_storage->ErrorCode == HAL_I2S_ERROR_NONE) ? status::error : status::io_error;
    restore_callbacks<Config, Handler>();
    if (previous_callback != nullptr)
    {
      previous_callback(handle_storage);
    }
    notify_completion<Config, Handler>(result);
  }
#endif
#endif

  template <typename Config> static bool initialize() noexcept
  {
#if MICROHAL_STM32_HAL_HAS_SPI && defined(HAL_I2S_MODULE_ENABLED) && defined(I2S_STANDARD_PHILIPS)
    auto& handle_storage = handle<Config>();
    handle_storage.Instance = peripheral_instance<SPI_TypeDef, Config>();
    handle_storage.Init.Mode = I2S_MODE_MASTER_TX;
    handle_storage.Init.Standard = I2S_STANDARD_PHILIPS;
    handle_storage.Init.DataFormat = I2S_DATAFORMAT_16B;
    handle_storage.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
    handle_storage.Init.AudioFreq = I2S_AUDIOFREQ_48K;
    handle_storage.Init.CPOL = I2S_CPOL_LOW;
#if defined(I2S_CLOCK_PLL)
    handle_storage.Init.ClockSource = I2S_CLOCK_PLL;
#endif

    if (HAL_I2S_Init(&handle_storage) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config> static bool transmit_blocking(const std::span<const std::uint16_t> samples) noexcept
  {
#if MICROHAL_STM32_HAL_HAS_SPI && defined(HAL_I2S_MODULE_ENABLED) && defined(I2S_STANDARD_PHILIPS)
    auto& handle_storage = handle<Config>();
    constexpr std::uint32_t timeout_ms = timeout_for<Config>();
    if (HAL_I2S_Transmit(&handle_storage, hal_tx_buffer(samples.data()), static_cast<std::uint16_t>(samples.size()),
                         timeout_ms) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    (void)samples;
    return false;
#endif
  }

  template <typename Config> static bool receive_blocking(const std::span<std::uint16_t> samples) noexcept
  {
#if MICROHAL_STM32_HAL_HAS_SPI && defined(HAL_I2S_MODULE_ENABLED) && defined(I2S_STANDARD_PHILIPS)
    auto& handle_storage = handle<Config>();
    constexpr std::uint32_t timeout_ms = timeout_for<Config>();
    if (HAL_I2S_Receive(&handle_storage, samples.data(), static_cast<std::uint16_t>(samples.size()), timeout_ms) ==
        HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    (void)samples;
    return false;
#endif
  }

  template <typename Config, auto Handler>
  static bool start_dma_tx(const std::span<const std::uint16_t> samples) noexcept
  {
#if MICROHAL_STM32_HAL_HAS_SPI && defined(HAL_I2S_MODULE_ENABLED) && defined(I2S_STANDARD_PHILIPS) &&                  \
    defined(USE_HAL_I2S_REGISTER_CALLBACKS) && (USE_HAL_I2S_REGISTER_CALLBACKS == 1)
    if (samples.size() > UINT16_MAX)
    {
      return false;
    }

    auto& transfer_state = state<Config, Handler>();
    if (transfer_state.active)
    {
      return false;
    }

    auto& handle_storage = handle<Config>();
    transfer_state.previous_tx_callback = handle_storage.TxCpltCallback;
    transfer_state.previous_error_callback = handle_storage.ErrorCallback;
    transfer_state.active = true;
    handle_storage.TxCpltCallback = dma_complete<Config, Handler>;
    handle_storage.ErrorCallback = dma_error<Config, Handler>;

    if (HAL_I2S_Transmit_DMA(&handle_storage, hal_tx_buffer(samples.data()),
                             static_cast<std::uint16_t>(samples.size())) == HAL_OK) [[likely]]
    {
      return true;
    }
    restore_callbacks<Config, Handler>();
    return false;
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
#if defined(RCC) && defined(RCC_PLL_ON) && defined(RCC_PLLSOURCE_HSE) && defined(RCC_PLLP_DIV2) &&                     \
    defined(RCC_CLOCKTYPE_PCLK2) && defined(RCC_SYSCLKSOURCE_PLLCLK) && defined(FLASH_LATENCY_5)
    RCC_OscInitTypeDef osc_init{};
    osc_init.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc_init.HSEState = RCC_HSE_ON;
    osc_init.PLL.PLLState = RCC_PLL_ON;
    osc_init.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc_init.PLL.PLLM = 8U;
    osc_init.PLL.PLLN = 336U;
    osc_init.PLL.PLLP = RCC_PLLP_DIV2;
    osc_init.PLL.PLLQ = 7U;

    if (HAL_RCC_OscConfig(&osc_init) != HAL_OK) [[unlikely]]
    {
      return false;
    }

    RCC_ClkInitTypeDef clk_init{};
    clk_init.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk_init.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk_init.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk_init.APB1CLKDivider = RCC_HCLK_DIV4;
    clk_init.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&clk_init, FLASH_LATENCY_5) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config> static std::uint32_t get_system_clock_hz() noexcept
  {
#if defined(RCC)
    return HAL_RCC_GetSysClockFreq();
#else
    return 0U;
#endif
  }

  template <typename Config>
  static status enable_peripheral_clock(const peripheral_kind peripheral, const std::uint8_t instance) noexcept
  {
#if defined(RCC)
    if constexpr (
        requires { typename Config::binding; } &&
        requires { Config::binding::enable_peripheral_clock(peripheral, instance); })
    {
      return microhal::detail::as_status([peripheral, instance]
                                         { return Config::binding::enable_peripheral_clock(peripheral, instance); });
    }
    return status::unsupported;
#else
    (void)peripheral;
    (void)instance;
    return status::unsupported;
#endif
  }

  template <typename Config>
  static status disable_peripheral_clock(const peripheral_kind peripheral, const std::uint8_t instance) noexcept
  {
#if defined(RCC)
    if constexpr (
        requires { typename Config::binding; } &&
        requires { Config::binding::disable_peripheral_clock(peripheral, instance); })
    {
      return microhal::detail::as_status([peripheral, instance]
                                         { return Config::binding::disable_peripheral_clock(peripheral, instance); });
    }
    return status::unsupported;
#else
    (void)peripheral;
    (void)instance;
    return status::unsupported;
#endif
  }
};

struct crc_api final
{
#if defined(CRC)
  static inline CRC_HandleTypeDef handle_storage{};
#endif

  template <typename Config> static bool initialize() noexcept
  {
#if defined(CRC)
    handle_storage.Instance = CRC;
#if defined(DEFAULT_POLYNOMIAL_ENABLE) && defined(DEFAULT_INIT_VALUE_ENABLE)
    handle_storage.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
    handle_storage.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
#endif
#if defined(CRC_INPUTDATA_INVERSION_NONE)
    handle_storage.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
#endif
#if defined(CRC_OUTPUTDATA_INVERSION_DISABLE)
    handle_storage.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
#endif
#if defined(CRC_INPUTDATA_FORMAT_BYTES)
    handle_storage.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
#endif

    if (HAL_CRC_Init(&handle_storage) == HAL_OK) [[likely]]
    {
      return true;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config> static std::uint32_t calculate(const std::span<const std::uint8_t> data) noexcept
  {
#if defined(CRC)
    return HAL_CRC_Calculate(&handle_storage, hal_tx_buffer_as_words(data.data()),
                             static_cast<std::uint32_t>(data.size()));
#else
    (void)data;
    return 0U;
#endif
  }

  template <typename Config> static void reset() noexcept
  {
#if defined(CRC)
    __HAL_CRC_DR_RESET(&handle_storage);
#endif
  }

  template <typename Config> static std::uint32_t get_random() noexcept
  {
#if defined(RNG)
    // Note: This should use RNG_HandleTypeDef, not CRC
    // Placeholder showing concept
    return 0U;
#else
    return 0U;
#endif
  }
};

} // namespace microhal::backends::stm32::hal

namespace microhal
{

namespace backends::stm32::hal
{
template <peripheral_kind Kind> consteval transfer_mode_capability serial_transfer_modes() noexcept
{
  if constexpr (Kind == peripheral_kind::i2c)
  {
#if MICROHAL_STM32_HAL_HAS_I2C
#if defined(USE_HAL_I2C_REGISTER_CALLBACKS) && USE_HAL_I2C_REGISTER_CALLBACKS == 1
    return {true, true, true};
#else
    return {true, false, false};
#endif
#endif
  }
  if constexpr (Kind == peripheral_kind::spi)
  {
#if MICROHAL_STM32_HAL_HAS_SPI
#if defined(USE_HAL_SPI_REGISTER_CALLBACKS) && USE_HAL_SPI_REGISTER_CALLBACKS == 1
    return {true, true, true};
#else
    return {true, false, false};
#endif
#endif
  }
  if constexpr (Kind == peripheral_kind::uart)
  {
#if MICROHAL_STM32_HAL_HAS_UART
#if defined(USE_HAL_UART_REGISTER_CALLBACKS) && USE_HAL_UART_REGISTER_CALLBACKS == 1
    return {true, true, true};
#else
    return {true, false, false};
#endif
#endif
  }
  if constexpr (Kind == peripheral_kind::i2s)
  {
#if MICROHAL_STM32_HAL_HAS_SPI && defined(HAL_I2S_MODULE_ENABLED) && defined(I2S_STANDARD_PHILIPS)
#if defined(USE_HAL_I2S_REGISTER_CALLBACKS) && USE_HAL_I2S_REGISTER_CALLBACKS == 1
    return {true, true, true};
#else
    return {true, false, false};
#endif
#endif
  }
  return {};
}
} // namespace backends::stm32::hal

template <> struct backend_traits<stm32_hal_tag> final
{
  using backend_tag = stm32_hal_tag;
  using gpio_api = backends::stm32::hal::gpio_api;
  using i2c_api = backends::stm32::hal::i2c_api;
  using spi_api = backends::stm32::hal::spi_api;
  using qspi_api = backends::stm32::hal::qspi_api;
  using uart_api = backends::stm32::hal::uart_api;
  using rtc_api = backends::stm32::hal::rtc_api;
  using flash_api = backends::stm32::hal::flash_api;
  using timer_api = backends::stm32::hal::timer_api;
  using adc_api = backends::stm32::hal::adc_api;
  using dac_api = backends::stm32::hal::dac_api;
  using can_api = backends::stm32::hal::can_api;
  using usb_api = backends::stm32::hal::usb_api;
  using ethernet_api = backends::stm32::hal::ethernet_api;
  using sd_api = backends::stm32::hal::sd_api;
  using dma_api = backends::stm32::hal::dma_api;
  using pwm_api = backends::stm32::hal::pwm_api;
  using watchdog_api = backends::stm32::hal::watchdog_api;
  using i2s_api = backends::stm32::hal::i2s_api;
  using rcc_api = backends::stm32::hal::rcc_api;
  using crc_api = backends::stm32::hal::crc_api;

  static constexpr vendor_kind vendor = vendor_kind::stm32;
  static constexpr backend_kind kind = backend_kind::stm32_cube_hal;
  static constexpr std::string_view name{"stm32-cube-hal"};
  static constexpr std::uint32_t gpio_port_count{
      (MICROHAL_STM32_HAL_FAMILY_SELECTED != 0) ? backends::stm32::hal::family_gpio_port_count : 11U};
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
            .alternate_function = true,
            .output_toggle = true};
  }

  static consteval struct i2c_capability i2c_capability() noexcept
  {
    return {.supported =
                (MICROHAL_STM32_HAL_FAMILY_SELECTED != 0) ? (backends::stm32::hal::i2c_instance_count != 0U) : true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = (MICROHAL_STM32_HAL_FAMILY_SELECTED != 0) ? backends::stm32::hal::i2c_instance_count : 4U,
            .max_frequency_hz = 1'000'000U,
            .ten_bit_addressing = true,
#if MICROHAL_STM32_HAL_HAS_I2C && defined(I2C_FIRST_FRAME) && defined(I2C_LAST_FRAME)
            .repeated_start = true,
#else
            .repeated_start = false,
#endif
            .address_probe = true,
            .transfer_modes = backends::stm32::hal::serial_transfer_modes<peripheral_kind::i2c>()};
  }

  static consteval struct spi_capability spi_capability() noexcept
  {
    return {.supported =
                (MICROHAL_STM32_HAL_FAMILY_SELECTED != 0) ? (backends::stm32::hal::spi_instance_count != 0U) : true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = (MICROHAL_STM32_HAL_FAMILY_SELECTED != 0) ? backends::stm32::hal::spi_instance_count : 6U,
            .max_frequency_hz = 100'000'000U,
            .transmit_only = true,
            .receive_only = true,
#if MICROHAL_STM32_HAL_HAS_SPI && defined(USE_HAL_SPI_REGISTER_CALLBACKS) && (USE_HAL_SPI_REGISTER_CALLBACKS == 1)
            .async_transfer = true,
#else
            .async_transfer = false,
#endif
            .transfer_modes = backends::stm32::hal::serial_transfer_modes<peripheral_kind::spi>()};
  }

  static consteval struct qspi_capability qspi_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 1U,
            .max_frequency_hz = 133'000'000U,
            .quad_mode = true,
            .dual_mode = true,
            .command_transfer = true,
            .memory_mapped = true,
            .max_address_bytes = 4U};
  }

  static consteval struct uart_capability uart_capability() noexcept
  {
    if constexpr (MICROHAL_STM32_HAL_FAMILY_SELECTED != 0)
    {
      return {.supported = true,
              .blocking = true,
              .non_blocking = true,
              .isr = true,
              .dma = true,
              .instance_count = backends::stm32::hal::uart_instance_count,
              .max_frequency_hz = 12'500'000U,
              .fifo = true,
              .lpuart = backends::stm32::hal::family_has_lpuart,
              .autobaud = true,
              .fifo_depth = 16U,
              .parity = true,
              .two_stop_bits = true,
#if defined(UART_STOPBITS_1_5)
              .one_and_half_stop_bits = true,
#else
              .one_and_half_stop_bits = false,
#endif
              .hardware_flow_control = false,
              .max_baud_rate = 12'500'000U,
              .transfer_modes = backends::stm32::hal::serial_transfer_modes<peripheral_kind::uart>()};
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
            .fifo_depth = 16U,
            .parity = true,
            .two_stop_bits = true,
#if defined(UART_STOPBITS_1_5)
            .one_and_half_stop_bits = true,
#else
            .one_and_half_stop_bits = false,
#endif
            .hardware_flow_control = false,
            .max_baud_rate = 12'500'000U};
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
            .periodic = true,
            .alarm = true,
            .calendar = false,
            .subsecond = false};
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
            .erase_bytes = 2'048U,
            .erase_region_query = true,
            .program_readback = true};
  }

  static consteval struct timer_capability timer_capability() noexcept
  {
    return {.supported =
                (MICROHAL_STM32_HAL_FAMILY_SELECTED != 0) ? (backends::stm32::hal::timer_instance_count != 0U) : true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count =
                (MICROHAL_STM32_HAL_FAMILY_SELECTED != 0) ? backends::stm32::hal::timer_instance_count : 14U,
            .max_frequency_hz = 200'000'000U,
            .capture_compare = true,
            .periodic = true,
            .channel_count = 4U,
            .period_readback = true,
            .counter_reset = true};
  }

  static consteval struct adc_capability adc_capability() noexcept
  {
    return {.supported =
                (MICROHAL_STM32_HAL_FAMILY_SELECTED != 0) ? (backends::stm32::hal::adc_instance_count != 0U) : true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = (MICROHAL_STM32_HAL_FAMILY_SELECTED != 0) ? backends::stm32::hal::adc_instance_count : 3U,
            .max_frequency_hz = 36'000'000U,
            .resolution_bits = 12U,
            .channel_count = 16U,
            .differential = true,
            .oversampling = true,
            .sequential_scan = true,
            .voltage_conversion = true};
  }

  static consteval struct dac_capability dac_capability() noexcept
  {
    return {.supported =
                (MICROHAL_STM32_HAL_FAMILY_SELECTED != 0) ? (backends::stm32::hal::dac_instance_count != 0U) : true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = (MICROHAL_STM32_HAL_FAMILY_SELECTED != 0) ? backends::stm32::hal::dac_instance_count : 1U,
            .max_frequency_hz = 0U,
            .resolution_bits = 12U,
            .channel_count = 2U,
            .dual_mode = true,
            .waveform_generation = true,
            .output_buffer = true,
            .voltage_conversion = true,
            .sequence_write = true};
  }

  static consteval struct can_capability can_capability() noexcept
  {
    if constexpr (MICROHAL_STM32_HAL_FAMILY_SELECTED != 0)
    {
      if constexpr (!backends::stm32::hal::family_has_legacy_can)
      {
        return {};
      }

      return {.supported = true,
              .blocking = true,
              .non_blocking = true,
              .isr = true,
              .dma = false,
              .instance_count = backends::stm32::hal::legacy_can_instance_count,
              .max_frequency_hz = 1'000'000U,
              .can_fd = false,
              .time_triggered = false,
              .mailbox_count = 3U,
              .filter_banks = 14U,
              .extended_ids = true,
              .remote_frames = true,
              .rx_interrupt = true};
    }

    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = false,
            .instance_count = 2U,
            .max_frequency_hz = 1'000'000U,
            .can_fd = true,
            .time_triggered = false,
            .mailbox_count = 3U,
            .filter_banks = 14U,
            .extended_ids = true,
            .remote_frames = true,
            .rx_interrupt = true};
  }

  static consteval struct usb_capability usb_capability() noexcept
  {
    if constexpr (MICROHAL_STM32_HAL_FAMILY_SELECTED != 0)
    {
      return {.supported = true,
              .blocking = true,
              .non_blocking = true,
              .isr = true,
              .dma = true,
              .instance_count = 1U,
              .max_frequency_hz = 0U,
              .device = true,
              .host = false,
              .otg = false,
              .high_speed = false,
              .endpoint_count = 4U,
              .endpoint_transfer = true,
              .endpoint_stall = true,
              .max_packet_size_bytes = 64U};
    }

    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .device = true,
            .host = true,
            .otg = true,
            .high_speed = true,
            .endpoint_count = 6U,
            .endpoint_transfer = true,
            .endpoint_stall = true,
            .max_packet_size_bytes = 512U};
  }

  static consteval struct ethernet_capability ethernet_capability() noexcept
  {
    if constexpr (MICROHAL_STM32_HAL_FAMILY_SELECTED != 0)
    {
      if constexpr (MICROHAL_STM32_HAL_ETH_V2_RUNTIME == 0)
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
              .checksum_offload = true,
              .ptp_timestamp = true,
              .vlan_filter = false,
              .gigabit = false,
              .mtu_bytes = 1522U};
    }

    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 1U,
            .max_frequency_hz = 50'000'000U,
            .checksum_offload = true,
            .ptp_timestamp = true,
            .vlan_filter = false,
            .gigabit = false,
            .mtu_bytes = 1522U};
  }

  static consteval struct sd_capability sd_capability() noexcept
  {
    if constexpr (MICROHAL_STM32_HAL_FAMILY_SELECTED != 0)
    {
      if constexpr (MICROHAL_STM32_HAL_SD_RUNTIME == 0)
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
              .bus_width_8 = false,
              .block_size_bytes = 512U,
              .block_io = true,
              .erase = true,
              .capacity_query = true,
              .bus_width_1 = true};
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
            .bus_width_8 = false,
            .block_size_bytes = 512U,
            .block_io = true,
            .erase = true,
            .capacity_query = true,
            .bus_width_1 = true};
  }

  static consteval struct dma_capability dma_capability() noexcept
  {
    if constexpr (MICROHAL_STM32_HAL_FAMILY_SELECTED != 0)
    {
      if constexpr (MICROHAL_STM32_HAL_STREAM_DMA_RUNTIME == 0)
      {
        return {};
      }

      return {.supported = true,
              .blocking = false,
              .non_blocking = true,
              .isr = true,
              .dma = false,
              .instance_count = backends::stm32::hal::dma_controller_count,
              .max_frequency_hz = 0U,
              .stream_count = 8U,
              .channel_count = 8U,
              .mem_to_mem = true,
              .fifo = true,
              .double_buffer = true,
              .transfer_status = true,
              .abort = true,
              .completion_callback = true,
              .address_width_bits = 32U};
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
            .double_buffer = true,
            .transfer_status = true,
            .abort = true,
            .completion_callback = true,
            .address_width_bits = 32U};
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
            .dead_time = true,
            .runtime_duty_update = true,
            .runtime_start_stop = true};
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
            .max_timeout_ms = 32'768U,
            .runtime_refresh = true,
            .reset_on_timeout = true};
  }

  static consteval struct i2s_capability i2s_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 3U,
            .max_frequency_hz = 42'000'000U,
            .full_duplex = true,
            .master_mode = true,
            .slave_mode = true,
            .master_clock_output = true,
            .max_sample_rate = 192'000U,
#if MICROHAL_STM32_HAL_FAMILY_SELECTED == 0 ||                                                                         \
    (MICROHAL_STM32_HAL_HAS_SPI && defined(USE_HAL_I2S_REGISTER_CALLBACKS) && (USE_HAL_I2S_REGISTER_CALLBACKS == 1))
            .transmit_dma = true,
#else
            .transmit_dma = false,
#endif
            .receive_dma = backends::stm32::hal::serial_transfer_modes<peripheral_kind::i2s>().dma,
            .sample_bits = 16U,
            .transfer_modes = backends::stm32::hal::serial_transfer_modes<peripheral_kind::i2s>()};
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
            .max_system_clock_hz = 180'000'000U,
            .clock_configuration = true,
            .peripheral_clock_gating = false,
            .system_clock_readback = true};
  }

  static consteval struct crc_capability crc_capability() noexcept
  {
    if constexpr (MICROHAL_STM32_HAL_FAMILY_SELECTED != 0)
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
              .random_number_generator = backends::stm32::hal::family_has_rng,
              .seed_configurable = false,
              .output_xor_configurable = false,
              .checksum_width_bits = 32U};
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
            .random_number_generator = true,
            .seed_configurable = false,
            .output_xor_configurable = false,
            .checksum_width_bits = 32U};
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

#if MICROHAL_STM32_HAL_FAMILY_SELECTED
#include "microhal/backends/stm32/hal/serial_modes.hpp"
#endif
