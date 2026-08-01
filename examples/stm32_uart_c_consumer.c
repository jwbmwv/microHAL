// SPDX-License-Identifier: Apache-2.0
/**
 * @file stm32_uart_c_consumer.c
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#include "stm32_uart_c_shim.h"

#include <stddef.h>
#include <stdint.h>

/*
 * Contractual example: proves a plain C translation unit can drive microHAL through a thin C++
 * shim, and that every call reports a status the caller can act on.
 *
 * This also runs as a host test, where no UART hardware exists, so the transport calls are
 * expected to report a failure. What would be genuinely wrong is the shim rejecting arguments that
 * were passed correctly, so that is what the test asserts against: MICROHAL_STATUS_INVALID_ARGUMENT
 * can only originate from the shim's own null/size validation.
 */
static int is_shim_bug(const microhal_status_t status)
{
  return status == MICROHAL_STATUS_INVALID_ARGUMENT;
}

int main(void)
{
  static const char banner[] = "microHAL C shim\n";
  uint8_t scratch[4] = {0U, 0U, 0U, 0U};

  if (is_shim_bug(microhal_example_console_uart_initialize()))
  {
    return 1;
  }

  if (is_shim_bug(microhal_example_console_uart_write((const uint8_t*)banner, sizeof(banner) - 1U)))
  {
    return 1;
  }

  if (is_shim_bug(microhal_example_console_uart_read(scratch, sizeof(scratch))))
  {
    return 1;
  }

  /* Null and zero-length arguments must be rejected rather than forwarded to the backend. */
  if (microhal_example_console_uart_write(NULL, sizeof(banner) - 1U) != MICROHAL_STATUS_INVALID_ARGUMENT)
  {
    return 1;
  }

  if (microhal_example_console_uart_read(scratch, 0U) != MICROHAL_STATUS_INVALID_ARGUMENT)
  {
    return 1;
  }

  return 0;
}
