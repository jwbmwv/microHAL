// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "microhal/core/adc_types.hpp"
#include "microhal/core/traits.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

namespace microhal::backends::stm32::adc_detail
{
struct irq_guard
{
#if defined(__arm__) || defined(__thumb__)
  std::uint32_t mask;
  irq_guard() noexcept
  {
    asm volatile("mrs %0, primask\ncpsid i" : "=r"(mask)::"memory");
  }
  ~irq_guard()
  {
    asm volatile("msr primask, %0" ::"r"(mask) : "memory");
  }
#else
  irq_guard() noexcept = default;
#endif
};
struct transfer
{
  std::atomic<bool> owned{false};
  std::atomic<dma_transfer_state> state{dma_transfer_state::idle};
  std::uint32_t generation{};
  bool dma{}, circular{}, blocks{};
  std::span<std::uint16_t> buffer{};
  void (*notify)(status, std::uint16_t, std::span<std::uint16_t>) noexcept {};
};
// Shared by every Config, callback, and backend that names the same physical ADC.
inline transfer transfers[6]{};

template <typename Hardware> struct engine
{
  template <typename C> static transfer& state() noexcept
  {
    static_assert(C::instance_index > 0 && C::instance_index < 6);
    return transfers[C::instance_index];
  }
  template <auto H, bool Stream = false>
  static void callback(status s, std::uint16_t sample, std::span<std::uint16_t> block) noexcept
  {
    if constexpr (Stream)
      H(s, block);
    else if constexpr (std::is_invocable_v<decltype(H), status, std::uint16_t>)
      H(s, sample);
    else if constexpr (std::is_invocable_v<decltype(H), status>)
      H(s);
    else if constexpr (std::is_invocable_v<decltype(H), std::uint16_t>)
    {
      if (s == status::ok)
        H(sample);
    }
    else
      H();
  }
  template <typename C> static bool acquire() noexcept
  {
#if defined(__arm__) || defined(__thumb__)
    irq_guard guard;
    if (state<C>().owned.load(std::memory_order_acquire))
      return false;
    state<C>().owned.store(true, std::memory_order_release);
#else
    bool expected = false;
    if (!state<C>().owned.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
      return false;
#endif
    ++state<C>().generation;
    state<C>().state = dma_transfer_state::active;
    return true;
  }
  template <typename C> static status release(status s) noexcept
  {
    auto& t = state<C>();
    t.notify = nullptr;
    t.dma = t.circular = t.blocks = false;
    t.buffer = {};
    t.state = s == status::ok ? dma_transfer_state::complete : dma_transfer_state::error;
    t.owned.store(false, std::memory_order_release);
    return s;
  }
  template <typename C> static status initialize() noexcept
  {
    [[maybe_unused]] irq_guard guard;
    if (!acquire<C>())
      return status::busy;
    auto s = Hardware::template initialize<C>();
    release<C>(s);
    if (s == status::ok)
      state<C>().state = dma_transfer_state::idle;
    return s;
  }
  template <typename C> static status stop() noexcept
  {
    [[maybe_unused]] irq_guard guard;
    auto& t = state<C>();
    if (!t.owned.load(std::memory_order_acquire))
      return status::ok;
    const auto s = Hardware::template stop<C>(t.dma);
    if (s != status::ok)
    {
      t.state = dma_transfer_state::error;
      return s;
    }
    return release<C>(s);
  }
  template <typename C> static status abort() noexcept
  {
    [[maybe_unused]] irq_guard guard;
    auto callback = state<C>().notify;
    const bool active = state<C>().owned.load(std::memory_order_acquire);
    auto s = stop<C>();
    state<C>().notify = nullptr;
    if (active)
      state<C>().state = dma_transfer_state::error;
    if (active && callback)
      callback(status::error, 0, {});
    return s;
  }
  template <typename C> static result<dma_transfer_state> transfer_status() noexcept
  {
    return state<C>().state.load(std::memory_order_acquire);
  }
  template <typename C> static result<std::uint16_t> read_blocking(std::uint8_t channel) noexcept
  {
    if (!acquire<C>())
      return status::busy;
    auto s = Hardware::template configure<C>({&channel, 1}, false, false, false);
    if (s == status::ok)
      s = Hardware::template start<C>();
    if (s == status::ok)
      s = Hardware::template wait<C>();
    const auto sample = s == status::ok ? Hardware::template sample<C>() : std::uint16_t{};
    const auto stopped = Hardware::template stop<C>(false);
    if (s == status::ok)
      s = stopped;
    if (stopped == status::ok)
      release<C>(s);
    else
      state<C>().state = dma_transfer_state::error;
    if (s != status::ok)
      return s;
    return sample;
  }
  template <typename C> static status start_continuous(std::span<const std::uint8_t> channels) noexcept
  {
    return begin<C>(channels, {}, true, false, false, nullptr);
  }
  template <typename C, auto H> static status start_async(std::uint8_t channel) noexcept
  {
    return begin<C>({&channel, 1}, {}, false, false, true, callback<H>);
  }
  template <typename C, auto H>
  static status start_dma_scan(std::span<const std::uint8_t> channels, std::span<std::uint16_t> buffer) noexcept
  {
    return begin<C>(channels, buffer, false, true, false, callback<H>);
  }
  template <typename C, auto H>
  static status start_dma_stream(std::span<const std::uint8_t> channels, std::span<std::uint16_t> buffer,
                                 bool circular = true) noexcept
  {
    return begin<C>(channels, buffer, circular, true, false, callback<H, true>, true, circular);
  }
  template <typename C>
  static status begin(std::span<const std::uint8_t> channels, std::span<std::uint16_t> buffer, bool continuous,
                      bool dma, bool irq, void (*notify)(status, std::uint16_t, std::span<std::uint16_t>) noexcept,
                      bool blocks = false, bool circular = false) noexcept
  {
    [[maybe_unused]] irq_guard guard;
    if (channels.empty() || channels.size() > 32 || !channels.data())
      return status::invalid_argument;
    if (dma && (buffer.empty() || !buffer.data() || buffer.size() > 65535 ||
                reinterpret_cast<std::uintptr_t>(buffer.data()) % alignof(std::uint16_t) != 0 ||
                buffer.size() % channels.size() != 0 || (blocks && (buffer.size() % (2 * channels.size()) != 0))))
      return status::invalid_argument;
    if (!acquire<C>())
      return status::busy;
    auto& t = state<C>();
    t.notify = notify;
    t.buffer = buffer;
    t.dma = dma;
    t.circular = circular;
    t.blocks = blocks;
    constexpr bool software_trigger = []
    {
      if constexpr (requires { C::trigger_edge; })
        return C::trigger_edge == adc_trigger_edge::software;
      else
        return true;
    }();
    const bool repeat = software_trigger && (continuous || (dma && buffer.size() > channels.size()));
    auto s = Hardware::template configure<C>(channels, repeat, dma, irq);
    bool dma_may_be_active = false;
    if (s == status::ok && dma)
    {
      s = Hardware::template setup_dma<C>(buffer, circular, blocks);
      dma_may_be_active = s != status::unsupported;
    }
    if (s == status::ok)
      s = Hardware::template start<C>();
    if (s != status::ok)
    {
      const auto stopped = Hardware::template stop<C>(dma_may_be_active);
      if (stopped == status::ok)
        release<C>(s);
      else
      {
        t.state = dma_transfer_state::error;
        t.notify = nullptr;
      }
    }
    return s;
  }
  template <typename C> static void fail() noexcept
  {
    auto notify = state<C>().notify;
    const auto stopped = Hardware::template stop<C>(state<C>().dma);
    if (stopped == status::ok)
      release<C>(status::io_error);
    else
    {
      state<C>().state = dma_transfer_state::error;
      state<C>().notify = nullptr;
    }
    if (notify)
      notify(status::io_error, 0, {});
  }
  template <typename C> static void handle_interrupt() noexcept
  {
    auto& t = state<C>();
    if (!t.owned.load(std::memory_order_acquire))
      return;
    if (t.dma)
    {
      handle_dma_interrupt<C>();
      return;
    }
    if (Hardware::template error<C>())
    {
      fail<C>();
      return;
    }
    if (!t.dma && t.notify && Hardware::template ready<C>())
    {
      auto notify = t.notify;
      const auto sample = Hardware::template sample<C>();
      const auto s = Hardware::template stop<C>(false);
      if (s == status::ok)
        release<C>(s);
      else
      {
        t.state = dma_transfer_state::error;
        t.notify = nullptr;
      }
      notify(s, sample, {});
    }
  }
  template <typename C> static void handle_dma_interrupt() noexcept
  {
    auto& t = state<C>();
    if (!t.owned.load(std::memory_order_acquire) || !t.dma)
      return;
    const unsigned flags = Hardware::template dma_flags<C>();
    std::atomic_thread_fence(std::memory_order_acquire);
    // Both circular events mean the producer has lapped the unserviced first half.
    if ((flags & 4U) || (t.circular && (flags & 3U) == 3U) ||
        ((t.circular || !(flags & 2U)) && Hardware::template error<C>()))
    {
      fail<C>();
      return;
    }
    auto notify = t.notify;
    const auto buffer = t.buffer;
    const auto generation = t.generation;
    if (t.blocks)
    {
      if (!t.circular && (flags & 2U))
      {
        const auto s = Hardware::template stop<C>(true);
        if (s != status::ok)
        {
          t.state = dma_transfer_state::error;
          t.notify = nullptr;
          if (notify)
            notify(s, 0, {});
          return;
        }
        release<C>(s);
        if ((flags & 1U) && notify)
          notify(status::ok, 0, buffer.first(buffer.size() / 2));
        if (state<C>().generation == generation && notify)
          notify(status::ok, 0, buffer.subspan(buffer.size() / 2));
        return;
      }
      if ((flags & 1U) && notify)
        notify(status::ok, 0, buffer.first(buffer.size() / 2));
      // A callback is allowed to stop/abort the stream.
      if ((flags & 2U) && state<C>().owned.load(std::memory_order_acquire) && state<C>().generation == generation &&
          notify)
      {
        notify(status::ok, 0, buffer.subspan(buffer.size() / 2));
      }
    }
    else if (flags & 2U)
    {
      const auto s = Hardware::template stop<C>(true);
      if (s == status::ok)
        release<C>(s);
      else
      {
        t.state = dma_transfer_state::error;
        t.notify = nullptr;
      }
      if (notify)
        notify(s, 0, buffer);
    }
  }
};
} // namespace microhal::backends::stm32::adc_detail
