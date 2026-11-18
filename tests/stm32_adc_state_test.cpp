// SPDX-License-Identifier: Apache-2.0
#include "microhal/backends/stm32/adc_engine.hpp"
#include <cassert>

namespace
{
using namespace microhal;
struct config
{
  static constexpr unsigned instance_index = 1;
};
struct other_config
{
  static constexpr unsigned instance_index = 1;
};
struct triggered_config
{
  static constexpr unsigned instance_index = 1;
  static constexpr auto trigger_edge = adc_trigger_edge::rising;
};
struct hardware
{
  inline static bool eoc{}, overrun{}, running{}, dma{}, continuous{};
  inline static unsigned flags{}, stops{}, configurations{};
  inline static status next_start{status::ok}, next_config{status::ok}, next_stop{status::ok};
  inline static std::span<std::uint16_t> destination{};
  inline static std::uint8_t sequence[16]{};
  template <typename C> static status initialize()
  {
    return status::ok;
  }
  template <typename C> static status configure(std::span<const std::uint8_t> channels, bool c, bool d, bool)
  {
    ++configurations;
    continuous = c;
    dma = d;
    eoc = overrun = false;
    for (unsigned i = 0; i < channels.size(); ++i)
      sequence[i] = channels[i];
    return next_config;
  }
  template <typename C> static status start()
  {
    running = next_start == status::ok;
    return next_start;
  }
  template <typename C> static status stop(bool)
  {
    if (next_stop == status::ok)
      running = false;
    ++stops;
    return next_stop;
  }
  template <typename C> static status wait()
  {
    return status::timeout;
  }
  template <typename C> static bool ready()
  {
    return eoc;
  }
  template <typename C> static bool error()
  {
    return overrun;
  }
  template <typename C> static std::uint16_t sample()
  {
    eoc = false;
    return 1234;
  }
  template <typename C> static status setup_dma(std::span<std::uint16_t> buffer, bool, bool)
  {
    destination = buffer;
    return status::ok;
  }
  template <typename C> static unsigned dma_flags()
  {
    auto f = flags;
    flags = 0;
    return f;
  }
};
using api = backends::stm32::adc_detail::engine<hardware>;
unsigned calls{}, samples{}, blocks{};
status last{};
std::span<std::uint16_t> last_block{};
void sample_done(status s, std::uint16_t value) noexcept
{
  ++calls;
  last = s;
  samples = value;
}
void other_done(status) noexcept
{
  assert(false);
}
void scan_done(status s) noexcept
{
  ++calls;
  last = s;
}
void stream_done(status s, std::span<std::uint16_t> block) noexcept
{
  ++blocks;
  last = s;
  last_block = block;
}
void legacy_done() noexcept
{
  ++calls;
}
void sample_only_done(std::uint16_t value) noexcept
{
  ++calls;
  samples = value;
}
void finite_done(status s, std::span<std::uint16_t> block) noexcept
{
  assert(!hardware::running);
  stream_done(s, block);
}
void restart_done(status s, std::span<std::uint16_t>) noexcept
{
  ++blocks;
  assert(s == status::ok);
  assert(api::stop<config>() == status::ok);
  assert((api::start_async<config, sample_done>(3) == status::ok));
}
} // namespace
int main()
{
  const std::uint8_t channels[]{3, 5};
  std::uint16_t buffer[8]{};
  assert(api::initialize<config>() == status::ok);
  assert((api::start_async<config, sample_done>(3) == status::ok));
  assert((api::start_async<other_config, other_done>(5) == status::busy));
  assert(api::read_blocking<other_config>(5).error() == status::busy);
  assert(api::initialize<other_config>() == status::busy);
  api::handle_interrupt<config>();
  assert(calls == 0);
  hardware::eoc = true;
  api::handle_interrupt<other_config>();
  assert(calls == 1 && samples == 1234 && last == status::ok && !hardware::running);
  api::handle_interrupt<config>();
  assert(calls == 1);
  assert(api::transfer_status<config>().value() == dma_transfer_state::complete);
  assert((api::start_async<config, sample_done>(3) == status::ok));
  hardware::overrun = true;
  api::handle_interrupt<config>();
  assert(calls == 2 && last == status::io_error);
  assert((api::start_dma_scan<config, scan_done>(channels, {}) == status::invalid_argument));
  assert((api::start_dma_stream<config, stream_done>(channels, {buffer, 6}) == status::invalid_argument));
  assert((api::start_dma_scan<config, scan_done>(channels, buffer) == status::ok));
  assert(hardware::continuous);
  assert(hardware::destination.data() == buffer && hardware::sequence[0] == 3 && hardware::sequence[1] == 5);
  hardware::flags = 1;
  api::handle_dma_interrupt<config>();
  assert(calls == 2);
  hardware::flags = 2;
  hardware::overrun = true;
  api::handle_dma_interrupt<config>();
  assert(calls == 3 && last == status::ok);
  assert((api::start_dma_stream<config, stream_done>(channels, buffer) == status::ok));
  assert(hardware::continuous);
  hardware::flags = 1;
  api::handle_dma_interrupt<config>();
  assert(blocks == 1 && last_block.data() == buffer && last_block.size() == 4);
  hardware::flags = 2;
  api::handle_dma_interrupt<config>();
  assert(blocks == 2 && last_block.data() == buffer + 4 && hardware::running);
  hardware::flags = 3;
  api::handle_dma_interrupt<config>();
  assert(blocks == 3 && last == status::io_error && last_block.empty() && !hardware::running);
  api::handle_dma_interrupt<config>();
  assert(blocks == 3);
  assert((api::start_dma_stream<config, stream_done>(channels, buffer) == status::ok));
  hardware::flags = 4;
  api::handle_dma_interrupt<config>();
  assert(blocks == 4 && last == status::io_error && last_block.empty() && !hardware::running);
  assert(api::transfer_status<config>().value() == dma_transfer_state::error);
  assert((api::start_async<config, sample_done>(3) == status::ok));
  assert(api::abort<config>() == status::ok && calls == 4 && last == status::error);
  assert(api::abort<config>() == status::ok && calls == 4);
  hardware::next_start = status::error;
  assert((api::start_async<config, sample_done>(3) == status::error));
  hardware::next_start = status::ok;
  assert(api::read_blocking<config>(3).error() == status::timeout);
  assert(api::start_continuous<config>(channels) == status::ok && hardware::continuous);
  assert(api::stop<config>() == status::ok && !hardware::running);
  assert((api::start_dma_stream<triggered_config, stream_done>(channels, buffer) == status::ok));
  assert(!hardware::continuous);
  assert(api::stop<triggered_config>() == status::ok);
  assert((api::start_async<config, legacy_done>(3) == status::ok));
  hardware::eoc = true;
  api::handle_interrupt<config>();
  assert(calls == 5);
  assert((api::start_dma_stream<config, restart_done>(channels, buffer, false) == status::ok));
  hardware::flags = 3;
  api::handle_dma_interrupt<config>();
  assert(blocks == 5);
  assert(api::transfer_status<config>().value() == dma_transfer_state::active);
  hardware::next_stop = status::timeout;
  assert(api::stop<config>() == status::timeout);
  assert((api::start_async<other_config, other_done>(5) == status::busy));
  hardware::next_stop = status::ok;
  assert(api::stop<config>() == status::ok);
  assert((api::start_async<config, sample_done>(3) == status::ok));
  assert(api::stop<config>() == status::ok);
  assert((api::start_dma_stream<config, stream_done>(channels, buffer, false) == status::ok));
  hardware::flags = 1;
  api::handle_dma_interrupt<config>();
  assert(blocks == 6 && last_block.data() == buffer && hardware::running);
  hardware::flags = 2;
  api::handle_dma_interrupt<config>();
  assert(blocks == 7 && last_block.data() == buffer + 4 && !hardware::running);
  assert(api::transfer_status<config>().value() == dma_transfer_state::complete);
  assert((api::start_dma_stream<config, finite_done>(channels, buffer, false) == status::ok));
  hardware::flags = 3;
  api::handle_dma_interrupt<config>();
  assert(blocks == 9 && last == status::ok && last_block.data() == buffer + 4);
  assert(api::transfer_status<config>().value() == dma_transfer_state::complete);
  assert((api::start_async<config, sample_only_done>(3) == status::ok));
  hardware::eoc = true;
  api::handle_interrupt<config>();
  assert(calls == 6 && samples == 1234);
  assert((api::start_async<config, scan_done>(3) == status::ok));
  hardware::eoc = true;
  api::handle_interrupt<config>();
  assert(calls == 7 && last == status::ok);
}
