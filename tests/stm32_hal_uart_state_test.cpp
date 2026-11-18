// SPDX-License-Identifier: Apache-2.0
// Exercises the production direction/streaming engine without emulating a UART register block.
#include "microhal/backends/stm32/hal/traits.hpp"

#include <array>
#include <cassert>

enum HAL_StatusTypeDef
{
  HAL_OK,
  HAL_ERROR,
  HAL_BUSY,
  HAL_TIMEOUT
};
static std::uint32_t ticks{};
std::uint32_t HAL_GetTick()
{
  return ticks++;
}
#include "microhal/backends/stm32/hal/serial_modes.hpp"

namespace
{
using microhal::status;
using microhal::transfer_mode;
struct handle
{
  void (*tx)(handle*){};
  void (*rx)(handle*){};
  void (*error)(handle*){};
  void (*event)(handle*, std::uint16_t){};
};
struct driver
{
  using handle_type = ::handle;
  static constexpr bool callbacks = true, idle_supported = true;
  static inline handle_type h{};
  static inline bool tx_ready = true, rx_ready = true, circular_value{};
  static inline bool tx_failure{}, rx_failure{true};
  static inline int event_kind{};
  static inline status stop_result = status::ok, start_result = status::ok;
  static inline int tx_stops{}, rx_stops{}, irq_calls{}, dma_calls{};
  template <typename Config> static auto& handle()
  {
    return h;
  }
  template <typename Config> static status initialize()
  {
    return status::ok;
  }
  static bool direction_ready(handle_type&, bool tx)
  {
    return tx ? tx_ready : rx_ready;
  }
  static bool circular(handle_type&)
  {
    return circular_value;
  }
  static status error(handle_type&)
  {
    return status::io_error;
  }
  static bool transmit_error(handle_type&)
  {
    return tx_failure;
  }
  static bool receive_error(handle_type&)
  {
    return rx_failure;
  }
  static bool half_event(handle_type&)
  {
    return event_kind == 1;
  }
  static bool full_event(handle_type&)
  {
    return event_kind == 2;
  }
  static auto& tx_callback(handle_type&)
  {
    return h.tx;
  }
  static auto& rx_callback(handle_type&)
  {
    return h.rx;
  }
  static auto& error_callback(handle_type&)
  {
    return h.error;
  }
  static auto& event_callback(handle_type&)
  {
    return h.event;
  }
  static void interrupt(handle_type&)
  {
    ++irq_calls;
  }
  static void dma_interrupt(handle_type&)
  {
    ++dma_calls;
  }
  static status stop_direction(handle_type&, bool tx)
  {
    ++(tx ? tx_stops : rx_stops);
    if (h.error != nullptr)
      h.error(&h);
    return stop_result;
  }
  template <typename Config>
  static status start(handle_type&, std::span<const std::uint8_t>, std::span<std::uint8_t>, std::uint16_t, bool, bool)
  {
    return start_result;
  }
  template <typename Config> static status start_idle(handle_type&, std::span<std::uint8_t>)
  {
    return start_result;
  }
};
using engine = microhal::backends::stm32::hal::uart_engine<driver>;
template <transfer_mode Mode> struct config
{
  static constexpr auto transfer_mode_value = Mode;
  static constexpr auto timeout_ms = 3U;
};
using irq = config<transfer_mode::interrupt>;
using dma = config<transfer_mode::dma>;
int tx_done{}, rx_done{}, events{}, previous_calls{};
status tx_result{}, rx_result{}, event_result{};
std::array<std::uint8_t*, 16> starts{};
std::array<std::size_t, 16> lengths{};
void tx_handler(status result)
{
  ++tx_done;
  tx_result = result;
}
void rx_handler(status result)
{
  ++rx_done;
  rx_result = result;
}
void stream_handler(status result, std::span<std::uint8_t> bytes)
{
  assert(events < static_cast<int>(lengths.size()));
  const auto index = static_cast<std::size_t>(events++);
  starts.at(index) = bytes.data();
  lengths.at(index) = bytes.size();
  event_result = result;
}
void previous(handle*)
{
  ++previous_calls;
}
void previous_event(handle*, std::uint16_t)
{
  ++previous_calls;
}
void restore_originals()
{
  driver::h = {previous, previous, previous, previous_event};
}
} // namespace

int main()
{
  std::array<std::uint8_t, 8> bytes{};
  restore_originals();
  assert(engine::initialize<irq>() == status::ok);
  assert((engine::write<irq, tx_handler>(bytes) == status::ok));
  assert((engine::read<irq, rx_handler>(bytes) == status::ok));
  assert((engine::write<irq, tx_handler>(bytes) == status::busy));
  assert((engine::read<irq, rx_handler>(bytes) == status::busy));
  assert(engine::tx_status<irq>() == status::busy && engine::rx_status<irq>() == status::busy);
  assert(engine::initialize<irq>() == status::busy);
  assert(engine::abort_tx<irq>() == status::ok);
  assert(tx_done == 1 && tx_result == status::error && rx_done == 0);
  assert(driver::h.tx == previous && driver::h.error != previous);
  driver::h.rx(&driver::h);
  assert(rx_done == 1 && rx_result == status::ok);
  assert(driver::h.rx == previous && driver::h.error == previous);
  assert(previous_calls == 0);

  assert((engine::read<irq, rx_handler>(bytes) == status::ok));
  driver::stop_result = status::io_error;
  assert(engine::abort_rx<irq>() == status::io_error);
  assert((engine::read<irq, rx_handler>(bytes) == status::busy));
  assert(driver::h.rx != previous && rx_done == 1);
  driver::h.rx(&driver::h);
  assert(rx_done == 1); // Failed stop retains buffer ownership even if a stale completion fires.
  driver::stop_result = status::ok;
  assert(engine::abort_rx<irq>() == status::ok);
  assert(rx_done == 2 && driver::h.rx == previous);
  assert((engine::write<irq, tx_handler>(bytes) == status::ok));
  assert((engine::read<irq, rx_handler>(bytes) == status::ok));
  driver::h.error(&driver::h);
  assert(rx_done == 3 && rx_result == status::io_error && tx_done == 1);
  driver::h.tx(&driver::h);
  assert(tx_done == 2 && tx_result == status::ok && driver::h.error == previous);

  assert((engine::start_receive_to_idle<irq, stream_handler>(bytes) == status::ok));
  driver::h.event(&driver::h, 3U);
  assert(events == 1 && lengths[0] == 3 && starts[0] == bytes.data());
  assert(driver::h.event == previous_event && driver::h.rx == previous);
  assert((engine::start_receive_to_idle<irq, stream_handler>({}) == status::invalid_argument));
  assert((engine::start_circular_rx<irq, stream_handler>(bytes) == status::unsupported));

  assert(engine::initialize<dma>() == status::ok);
  assert((engine::start_circular_rx<dma, stream_handler>(bytes) == status::unsupported));
  driver::circular_value = true;
  assert((engine::read<dma, rx_handler>(bytes) == status::unsupported));
  assert((engine::start_receive_to_idle<dma, stream_handler>(bytes) == status::unsupported));
  assert((engine::start_circular_rx<dma, stream_handler>(bytes) == status::ok));
  assert((engine::start_circular_rx<dma, stream_handler>(bytes) == status::busy));
  events = 0;
  driver::h.event(&driver::h, 2U); // IDLE before half.
  driver::h.event(&driver::h, 4U); // Half.
  driver::h.event(&driver::h, 4U); // Duplicate IDLE.
  driver::h.event(&driver::h, 8U); // Full.
  driver::h.event(&driver::h, 8U); // Duplicate IDLE at full.
  driver::h.event(&driver::h, 0U); // Alternate representation of the same wrap.
  driver::h.event(&driver::h, 3U); // Wrapped IDLE.
  assert(events == 4);
  assert(lengths[0] == 2 && lengths[1] == 2 && lengths[2] == 4 && lengths[3] == 3);
  assert(starts[1] == bytes.data() + 2 && starts[2] == bytes.data() + 4 && starts[3] == bytes.data());
  assert(engine::rx_status<dma>() == status::busy);
  assert(driver::h.event != previous_event);
  driver::h.event(&driver::h, 9U); // Invalid vendor position cannot create an invalid span.
  assert(events == 4);
  assert(engine::handle_interrupt<dma>() == status::ok);
  assert(engine::handle_dma_interrupt<dma>() == status::ok);
  assert(driver::irq_calls == 1 && driver::dma_calls == 1);
  assert(engine::abort_rx<dma>() == status::ok);
  assert(events == 5 && lengths[4] == 0 && event_result == status::error);
  assert(driver::h.event == previous_event && driver::h.error == previous);

  driver::circular_value = false;
  assert((engine::start_receive_to_idle<dma, stream_handler>(bytes) == status::ok));
  driver::rx_ready = false;
  driver::h.event(&driver::h, 4U); // HT is not finite completion.
  assert(events == 5 && engine::rx_status<dma>() == status::busy);
  driver::rx_ready = true;
  driver::h.event(&driver::h, 6U);
  assert(events == 6 && lengths[5] == 6 && engine::rx_status<dma>() == status::ok);

  driver::start_result = status::busy;
  assert((engine::start_receive_to_idle<dma, stream_handler>(bytes) == status::busy));
  assert(events == 6 && driver::h.event == previous_event);
  driver::start_result = status::io_error;
  driver::stop_result = status::io_error;
  assert((engine::start_receive_to_idle<dma, stream_handler>(bytes) == status::io_error));
  assert(events == 6 && driver::h.event != previous_event);
  driver::stop_result = status::ok;
  assert(engine::abort_rx<dma>() == status::ok);
  assert(events == 6 && driver::h.event == previous_event);
  driver::start_result = status::ok;
  assert((engine::write<irq, nullptr>(bytes) == status::timeout));
  assert(engine::tx_status<irq>() == status::timeout);
  const auto previous_tx_done = tx_done;
  const auto previous_rx_done = rx_done;
  assert((engine::read<irq, rx_handler>(bytes) == status::ok));
  driver::stop_result = status::io_error;
  assert(engine::abort_rx<irq>() == status::io_error);
  assert((engine::write<irq, tx_handler>(bytes) == status::ok));
  driver::stop_result = status::ok;
  driver::tx_failure = true;
  driver::rx_failure = false;
  driver::h.error(&driver::h);
  assert(tx_done == previous_tx_done + 1 && tx_result == status::io_error && rx_done == previous_rx_done);
  assert((engine::read<irq, rx_handler>(bytes) == status::busy));
  assert(engine::abort_rx<irq>() == status::ok);
  assert(rx_done == previous_rx_done + 1);
  events = 0;
  driver::circular_value = true;
  assert((engine::start_circular_rx<dma, stream_handler>(bytes) == status::ok));
  driver::h.event(&driver::h, 6U);
  driver::event_kind = 1;
  driver::h.event(&driver::h, 4U); // Pending HT predates the IDLE range already delivered.
  assert(events == 1 && lengths[0] == 6);
  driver::event_kind = 2;
  driver::h.event(&driver::h, 8U);
  driver::event_kind = 0;
  driver::h.event(&driver::h, 2U);
  driver::h.event(&driver::h, 6U);
  driver::h.event(&driver::h, 2U); // Wrapped IDLE beats the pending full DMA interrupt.
  assert(events == 6 && lengths[4] == 2 && lengths[5] == 2);
  driver::event_kind = 1;
  driver::h.event(&driver::h, 4U); // An old-cycle half interrupt may also still be pending.
  assert(events == 6);
  driver::event_kind = 2;
  driver::h.event(&driver::h, 8U);
  assert(events == 6);
  driver::event_kind = 1;
  driver::h.event(&driver::h, 4U);
  assert(events == 7 && lengths[6] == 2);
  assert(engine::abort_rx<dma>() == status::ok);
}
