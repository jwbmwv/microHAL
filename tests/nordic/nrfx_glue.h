// SPDX-License-Identifier: Apache-2.0
// Bare-metal glue for the compile-only nrfx driver validation target.
#pragma once
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#define NRFX_ASSERT(expression) assert(expression)
#ifdef __cplusplus
#define NRFX_STATIC_ASSERT(expression) static_assert(expression)
#else
#define NRFX_STATIC_ASSERT(expression) _Static_assert(expression, #expression)
#endif
#define NRFX_IRQ_PRIORITY_SET(irq, priority) NVIC_SetPriority((IRQn_Type)(irq), priority)
#define NRFX_IRQ_ENABLE(irq) NVIC_EnableIRQ((IRQn_Type)(irq))
#define NRFX_IRQ_DISABLE(irq) NVIC_DisableIRQ((IRQn_Type)(irq))
#define NRFX_IRQ_IS_ENABLED(irq) NVIC_GetEnableIRQ((IRQn_Type)(irq))
#define NRFX_IRQ_PENDING_SET(irq) NVIC_SetPendingIRQ((IRQn_Type)(irq))
#define NRFX_IRQ_PENDING_CLEAR(irq) NVIC_ClearPendingIRQ((IRQn_Type)(irq))
#define NRFX_IRQ_IS_PENDING(irq) NVIC_GetPendingIRQ((IRQn_Type)(irq))
#define NRFX_CRITICAL_SECTION_ENTER()                                                                                  \
  {                                                                                                                    \
    uint32_t microhal_saved_primask = __get_PRIMASK();                                                                 \
    __disable_irq();
#define NRFX_CRITICAL_SECTION_EXIT()                                                                                   \
  __set_PRIMASK(microhal_saved_primask);                                                                               \
  }
#define NRFX_COREDEP_DELAY_DWT_BASED 0
#define NRFX_DELAY_US(time) nrfx_coredep_delay_us(time)
#define nrfx_atomic_t uint32_t
#define NRFX_ATOMIC_FETCH_STORE(ptr, value) __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST)
#define NRFX_ATOMIC_FETCH_OR(ptr, value) __atomic_fetch_or(ptr, value, __ATOMIC_SEQ_CST)
#define NRFX_ATOMIC_FETCH_AND(ptr, value) __atomic_fetch_and(ptr, value, __ATOMIC_SEQ_CST)
#define NRFX_ATOMIC_FETCH_XOR(ptr, value) __atomic_fetch_xor(ptr, value, __ATOMIC_SEQ_CST)
#define NRFX_ATOMIC_FETCH_ADD(ptr, value) __atomic_fetch_add(ptr, value, __ATOMIC_SEQ_CST)
#define NRFX_ATOMIC_FETCH_SUB(ptr, value) __atomic_fetch_sub(ptr, value, __ATOMIC_SEQ_CST)
static inline bool microhal_nrfx_atomic_cas(uint32_t* ptr, uint32_t expected, uint32_t desired)
{
  return __atomic_compare_exchange_n(ptr, &expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}
#define NRFX_ATOMIC_CAS(ptr, expected, desired) microhal_nrfx_atomic_cas(ptr, expected, desired)
#define NRFX_CLZ(value) __builtin_clz(value)
#define NRFX_CTZ(value) __builtin_ctz(value)
#define NRFX_EVENT_READBACK_ENABLED 1
#define NRFY_CACHE_WB(buffer, size)                                                                                    \
  do                                                                                                                   \
  {                                                                                                                    \
    (void)(buffer);                                                                                                    \
    (void)(size);                                                                                                      \
  } while (0)
#define NRFY_CACHE_INV(buffer, size)                                                                                   \
  do                                                                                                                   \
  {                                                                                                                    \
    (void)(buffer);                                                                                                    \
    (void)(size);                                                                                                      \
  } while (0)
#define NRFY_CACHE_WBINV(buffer, size)                                                                                 \
  do                                                                                                                   \
  {                                                                                                                    \
    (void)(buffer);                                                                                                    \
    (void)(size);                                                                                                      \
  } while (0)

#include <lib/nrfx_coredep.h>
