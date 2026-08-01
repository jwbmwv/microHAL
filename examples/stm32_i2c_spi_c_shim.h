// SPDX-License-Identifier: Apache-2.0
/**
 * @file stm32_i2c_spi_c_shim.h
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "microhal_status_c.h"

#ifdef __cplusplus
extern "C"
{
#endif

  microhal_status_t microhal_example_sensor_i2c_initialize(void);
  microhal_status_t microhal_example_sensor_i2c_write(uint16_t address, const uint8_t* data, size_t size);
  microhal_status_t microhal_example_sensor_i2c_read(uint16_t address, uint8_t* data, size_t size);

  microhal_status_t microhal_example_sensor_spi_initialize(void);
  microhal_status_t microhal_example_sensor_spi_transfer(const uint8_t* tx_data, uint8_t* rx_data, size_t size);

#ifdef __cplusplus
}
#endif
