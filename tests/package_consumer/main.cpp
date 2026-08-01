// SPDX-License-Identifier: Apache-2.0
/**
 * @file main.cpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#include "microhal/core/backend_selector.hpp"

int main()
{
  return (microhal::backend_registry.size() == MICROHAL_EXPECTED_BACKEND_COUNT) ? 0 : 1;
}