// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "../sdk/fsl_common.h"
// Device symbols alone must not enable an unavailable driver.
#define I2C0 reinterpret_cast<I2C_Type*>(0x40066000U)
#define LPI2C1 reinterpret_cast<LPI2C_Type*>(0x400A0000U)
#define SPI0 reinterpret_cast<SPI_Type*>(0x4002C000U)
#define LPSPI1 reinterpret_cast<LPSPI_Type*>(0x4002D000U)
#define I2S0 reinterpret_cast<I2S_Type*>(0x4002F000U)
