// SPDX-License-Identifier: Apache-2.0
/**
 * @file main.cpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#include "microhal/core/backend_selector.hpp"

static_assert(__cplusplus >= 202002L, "microHAL must propagate a C++20 minimum to consumers");
#if defined(MICROHAL_TEST_CXX_STANDARD) && MICROHAL_TEST_CXX_STANDARD == 23
static_assert(__cplusplus > 202002L, "microHAL must preserve the consumer's C++23 selection");
#elif defined(MICROHAL_TEST_CXX_STANDARD) && MICROHAL_TEST_CXX_STANDARD == 26
static_assert(__cplusplus > 202302L, "microHAL must preserve the consumer's C++26 selection");
#endif

int main()
{
  return (microhal::backend_registry.size() == MICROHAL_EXPECTED_BACKEND_COUNT) ? 0 : 1;
}