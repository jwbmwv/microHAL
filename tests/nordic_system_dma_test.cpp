// SPDX-License-Identifier: Apache-2.0
// Register/descriptor model: no CPU memcpy stands in for a hardware DMA transfer.
#include "microhal/core/traits.hpp"
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>

#define MICROHAL_NORDIC_HAS_MVDMA 1
#define NRF_VDMA_BUFFER_SIZE_MASK 0xFFFFFFU
#define NRF_VDMA_ATTRIBUTE_PLAIN_DATA 0U
#define NRF_MVDMA_INT_END_MASK 1U
#define NRF_MVDMA_INT_SOURCEBUSERROR_MASK 2U
#define NRF_MVDMA_INT_SINKBUSERROR_MASK 4U
#define NRFX_IRQ_PRIORITY_SET(irq, priority) ((void)0)
#define NRFX_IRQ_PENDING_CLEAR(irq) ((void)0)
#define NRFX_IRQ_ENABLE(irq) ((void)0)
#define NRFY_CACHE_INV(ptr, bytes) ((void)0)
#define NRFY_CACHE_WB(ptr, bytes) ((void)0)
#define NRFY_CACHE_WBINV(ptr, bytes) ((void)0)
enum
{
  NRF_MVDMA_EVENT_END,
  NRF_MVDMA_EVENT_SOURCEBUSERROR,
  NRF_MVDMA_EVENT_SINKBUSERROR,
  NRF_MVDMA_EVENT_PAUSED,
  NRF_MVDMA_EVENT_RESET,
  NRF_MVDMA_TASK_START0,
  NRF_MVDMA_TASK_PAUSE,
  NRF_MVDMA_TASK_RESET,
  NRF_MVDMA_MODE_SINGLE
};
struct nrf_vdma_job_t
{
  std::uint8_t* p_buffer{};
  std::uint32_t size{};
  std::uint8_t attributes{};
};
struct dma_registers
{
  bool active{};
  bool events[5]{};
  bool pause_stuck{};
  std::uint32_t interrupts{}, starts{};
  nrf_vdma_job_t const* source{};
  nrf_vdma_job_t const* sink{};
};
static dma_registers hardware{};
#define NRF_MVDMA120 (&hardware)
static std::uint32_t __get_PRIMASK()
{
  return 0U;
}
static void __disable_irq() {}
static void __set_PRIMASK(std::uint32_t) {}
static void __DMB() {}
static bool nrf_mvdma_activity_check(dma_registers* reg)
{
  return reg->active;
}
static void nrf_mvdma_int_enable(dma_registers* reg, std::uint32_t mask)
{
  reg->interrupts |= mask;
}
static void nrf_mvdma_int_disable(dma_registers* reg, std::uint32_t mask)
{
  reg->interrupts &= ~mask;
}
static void nrf_mvdma_mode_set(dma_registers*, int) {}
static void nrf_mvdma_event_clear(dma_registers* reg, int event)
{
  reg->events[event] = false;
}
static bool nrf_mvdma_event_check(dma_registers* reg, int event)
{
  return reg->events[event];
}
static void nrf_mvdma_source_list_ptr_set(dma_registers* reg, nrf_vdma_job_t const* job)
{
  reg->source = job;
}
static void nrf_mvdma_sink_list_ptr_set(dma_registers* reg, nrf_vdma_job_t const* job)
{
  reg->sink = job;
}
static void nrf_vdma_job_fill(nrf_vdma_job_t* job, void* buffer, std::uint32_t size, std::uint8_t attributes)
{
  *job = {static_cast<std::uint8_t*>(buffer), size, attributes};
}
static void nrf_vdma_job_terminate(nrf_vdma_job_t* job)
{
  job->p_buffer = nullptr;
}
static void nrf_mvdma_task_trigger(dma_registers* reg, int task)
{
  if (task == NRF_MVDMA_TASK_START0)
  {
    reg->active = true;
    ++reg->starts;
  }
  else if (task == NRF_MVDMA_TASK_PAUSE && !reg->pause_stuck)
    reg->active = false;
  else if (task == NRF_MVDMA_TASK_RESET)
    reg->events[NRF_MVDMA_EVENT_RESET] = true;
}
namespace microhal::backends::nordic::nrfx
{
namespace detail
{
template <auto Handler> void notify(status value)
{
  Handler(value);
}
} // namespace detail
#include "microhal/backends/nordic/nrfx/dma_engine.inc"
} // namespace microhal::backends::nordic::nrfx
namespace nordic = microhal::backends::nordic::nrfx;
struct config
{
  static constexpr std::uint8_t instance_index{0U}, stream{0U}, channel{0U}, direction{2U};
  static constexpr bool circular_mode{false}, double_buffer{false};
};
static int callbacks{};
static microhal::status completion{};
static void handler(microhal::status result) noexcept
{
  assert(nordic::dma_api::transfer_status<config>().value() != microhal::dma_transfer_state::active);
  ++callbacks;
  completion = result;
}
int main()
{
  using microhal::dma_transfer_state;
  using microhal::status;
  using api = nordic::dma_api;
  assert(api::transfer_status<config>().error() == status::unsupported);
  assert(api::initialize<config>() == status::ok);
  assert(api::initialize<config>() == status::busy);
  assert((api::start_transfer<config, handler>(UINT32_MAX - 3U, 0x20000000U, 8U) == status::invalid_argument));
  assert((api::start_transfer<config, handler>(0x20000000U, 0x20000004U, 8U) == status::invalid_argument));
  assert((api::start_transfer<config, handler>(0x20000000U, 0x20001000U, 16U) == status::ok));
  assert(api::transfer_status<config>().value() == dma_transfer_state::active);
  assert(hardware.starts == 1U && hardware.source[0].size == 16U && hardware.sink[0].size == 16U);
  assert(hardware.source[1].p_buffer == nullptr && hardware.sink[1].p_buffer == nullptr);
  assert((api::start_transfer<config, handler>(0x20002000U, 0x20003000U, 16U) == status::busy));
  hardware.active = false;
  hardware.events[NRF_MVDMA_EVENT_END] = true;
  api::handle_interrupt();
  assert(callbacks == 1 && completion == status::ok);
  api::handle_interrupt();
  assert(callbacks == 1);
  assert((api::start_transfer<config, handler>(0x20000000U, 0x20001000U, 16U) == status::ok));
  hardware.pause_stuck = true;
  assert(api::abort<config>() == status::timeout);
  assert(api::transfer_status<config>().value() == dma_transfer_state::active && callbacks == 1);
  hardware.pause_stuck = false;
  assert(api::abort<config>() == status::ok);
  assert(api::transfer_status<config>().value() == dma_transfer_state::idle && callbacks == 1);
  assert((api::start_transfer<config, handler>(0x20000000U, 0x20001000U, 16U) == status::ok));
  hardware.active = false;
  hardware.events[NRF_MVDMA_EVENT_SOURCEBUSERROR] = true;
  api::handle_interrupt();
  assert(callbacks == 2 && completion == status::io_error);
}
