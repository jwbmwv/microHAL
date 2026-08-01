// SPDX-License-Identifier: Apache-2.0
/**
 * @file preset_capability_expectations_test.cpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#include "microhal/core/backend_selector.hpp"

namespace
{

#if defined(CONFIG_I2C)
constexpr bool nuttx_i2c_expected{true};
#else
constexpr bool nuttx_i2c_expected{false};
#endif

#if defined(CONFIG_SPI)
constexpr bool nuttx_spi_expected{true};
#else
constexpr bool nuttx_spi_expected{false};
#endif

#if defined(CONFIG_MMCSD)
constexpr bool nuttx_sd_expected{true};
#else
constexpr bool nuttx_sd_expected{false};
#endif

#if defined(CONFIG_FLASH)
constexpr bool zephyr_qspi_expected{true};
#else
constexpr bool zephyr_qspi_expected{false};
#endif

} // namespace

#if !defined(MICROHAL_BACKENDS_EXPLICIT) || MICROHAL_ENABLE_ZEPHYR
static_assert(microhal::backend_traits<microhal::zephyr_tag>::qspi_capability().supported == zephyr_qspi_expected);
#endif

#if !defined(MICROHAL_BACKENDS_EXPLICIT) || MICROHAL_ENABLE_NUTTX
static_assert(microhal::backend_traits<microhal::nuttx_tag>::i2c_capability().supported == nuttx_i2c_expected);
static_assert(microhal::backend_traits<microhal::nuttx_tag>::spi_capability().supported == nuttx_spi_expected);
static_assert(microhal::backend_traits<microhal::nuttx_tag>::sd_capability().supported == nuttx_sd_expected);
#endif

int main()
{
  return 0;
}
