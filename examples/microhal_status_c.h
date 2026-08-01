/**
 * @file microhal_status_c.h
 * @brief C mirror of microhal::status for the example C shims.
 * @details The values are kept numerically identical to the C++ enumerators; each shim
 *          static_asserts that correspondence so the two definitions cannot drift apart.
 * @copyright Copyright (c) 2026 James Baldwin
 */

// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

  typedef enum microhal_status
  {
    MICROHAL_STATUS_OK = 0,
    MICROHAL_STATUS_ERROR = 1,
    MICROHAL_STATUS_TIMEOUT = 2,
    MICROHAL_STATUS_BUSY = 3,
    MICROHAL_STATUS_UNSUPPORTED = 4,
    MICROHAL_STATUS_INVALID_ARGUMENT = 5,
    MICROHAL_STATUS_IO_ERROR = 6
  } microhal_status_t;

#ifdef __cplusplus
}
#endif
