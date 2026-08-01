// SPDX-License-Identifier: Apache-2.0
/**
 * @file stm32_i2c_spi_c_consumer.c
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#include "stm32_i2c_spi_c_shim.h"

#include <stddef.h>
#include <stdint.h>

int main(void)
{
  static const uint8_t command[] = {0x20U, 0x01U};
  uint8_t response[4] = {0U, 0U, 0U, 0U};

  /*
   * Runs as a host test with no sensor attached, so transport failures are expected. A
   * MICROHAL_STATUS_INVALID_ARGUMENT, however, could only come from the shim's own argument
   * validation, which would be a real defect.
   */
  if (microhal_example_sensor_i2c_initialize() == MICROHAL_STATUS_INVALID_ARGUMENT)
  {
    return 1;
  }

  if (microhal_example_sensor_i2c_write(0x1DU, command, sizeof(command)) == MICROHAL_STATUS_INVALID_ARGUMENT)
  {
    return 1;
  }

  if (microhal_example_sensor_i2c_read(0x1DU, response, sizeof(response)) == MICROHAL_STATUS_INVALID_ARGUMENT)
  {
    return 1;
  }

  if (microhal_example_sensor_spi_initialize() == MICROHAL_STATUS_INVALID_ARGUMENT)
  {
    return 1;
  }

  if (microhal_example_sensor_spi_transfer(command, response, sizeof(command)) == MICROHAL_STATUS_INVALID_ARGUMENT)
  {
    return 1;
  }

  /* Bad arguments must be rejected by the shim instead of reaching the backend. */
  if (microhal_example_sensor_i2c_write(0x1DU, NULL, sizeof(command)) != MICROHAL_STATUS_INVALID_ARGUMENT)
  {
    return 1;
  }

  if (microhal_example_sensor_spi_transfer(command, NULL, sizeof(command)) != MICROHAL_STATUS_INVALID_ARGUMENT)
  {
    return 1;
  }

  return 0;
}
