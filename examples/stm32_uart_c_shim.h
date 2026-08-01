// SPDX-License-Identifier: Apache-2.0
/**
 * @file stm32_uart_c_shim.h
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

  microhal_status_t microhal_example_console_uart_initialize(void);
  microhal_status_t microhal_example_console_uart_write(const uint8_t* data, size_t size);
  microhal_status_t microhal_example_console_uart_read(uint8_t* data, size_t size);

#ifdef __cplusplus
}
#endif
