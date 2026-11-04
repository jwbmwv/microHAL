// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "microhal/backends/stm32/dma_resource.hpp"
#include "microhal/core/traits.hpp"
#include <atomic>
#include <type_traits>

namespace microhal::backends::stm32::ll
{
// Board code supplies clocks, pins, IRQ routing and DMA request selection, not transfer callbacks.
template <peripheral_kind Kind, std::uint32_t Instance> struct serial_binding
{
};

using detail::channel_dma;
using detail::gpdma;
using detail::serial_dma_layout;
using detail::serial_dma_resource;
using detail::serial_gpdma_node;
using detail::stream_dma;

struct serial_guard
{
#if defined(__arm__) || defined(__thumb__)
  std::uint32_t mask{};
  serial_guard() noexcept
  {
    __asm volatile("mrs %0, primask\ncpsid i" : "=r"(mask)::"memory");
  }
  ~serial_guard()
  {
    __asm volatile("msr primask, %0" ::"r"(mask) : "memory");
  }
#else
  serial_guard() noexcept = default;
  ~serial_guard() {}
#endif
};

template <typename Config> consteval std::uint32_t serial_identity() noexcept
{
  if constexpr (requires { Config::instance_index; })
    return Config::instance_index;
  else if constexpr (requires { Config::base_address; })
    return static_cast<std::uint32_t>(Config::base_address);
  else if constexpr (requires { Config::binding::base_address; })
    return static_cast<std::uint32_t>(Config::binding::base_address);
  else
    static_assert(dependent_false_v<Config>, "STM32 LL serial requires an instance index or register binding");
}
template <peripheral_kind Kind, typename Config> using serial_board = serial_binding<Kind, serial_identity<Config>()>;
template <peripheral_kind Kind, typename Config> serial_dma_resource serial_dma(const bool tx) noexcept
{
  using board = serial_board<Kind, Config>;
  if (tx)
  {
    if constexpr (requires { board::tx_dma(); })
      return board::tx_dma();
    else if constexpr (requires { Config::binding::tx_dma(); })
      return Config::binding::tx_dma();
  }
  else
  {
    if constexpr (requires { board::rx_dma(); })
      return board::rx_dma();
    else if constexpr (requires { Config::binding::rx_dma(); })
      return Config::binding::rx_dma();
  }
  return {};
}
template <peripheral_kind Kind, typename Config> std::uint32_t serial_clock() noexcept
{
  if constexpr (requires { serial_board<Kind, Config>::source_clock_hz(); })
    return serial_board<Kind, Config>::source_clock_hz();
  else if constexpr (requires { Config::source_clock_hz; })
    return Config::source_clock_hz;
  else if constexpr (requires { Config::binding::source_clock_hz(); })
    return Config::binding::source_clock_hz();
  else
    return 0U;
}
template <peripheral_kind Kind, typename Config>
constexpr bool serial_has_time =
    requires { serial_board<Kind, Config>::now_ms(); } || requires { Config::binding::now_ms(); };
template <peripheral_kind Kind, typename Config> std::uint32_t serial_now() noexcept
{
  if constexpr (requires { serial_board<Kind, Config>::now_ms(); })
    return serial_board<Kind, Config>::now_ms();
  else if constexpr (requires { Config::binding::now_ms(); })
    return Config::binding::now_ms();
  else
    return 0U;
}
template <typename Config> constexpr bool serial_slave()
{
  if constexpr (requires { Config::is_slave; })
    return Config::is_slave;
  else
    return false;
}
template <typename Config> constexpr std::uint32_t serial_timeout()
{
  if constexpr (requires { Config::timeout_ms; })
    return Config::timeout_ms;
  else
    return 1000U;
}
template <peripheral_kind Kind, typename Peripheral, typename Config> Peripheral* serial_instance() noexcept
{
  if constexpr (requires { Config::instance(); })
    return Config::instance();
  else if constexpr (requires { Config::base_address; })
    return reinterpret_cast<Peripheral*>(Config::base_address);
  else if constexpr (requires { Config::binding::base_address; })
    return reinterpret_cast<Peripheral*>(Config::binding::base_address);
  else if constexpr (requires { serial_board<Kind, Config>::instance(); })
    return serial_board<Kind, Config>::instance();
  else if constexpr (requires { serial_board<Kind, Config>::base_address; })
    return reinterpret_cast<Peripheral*>(serial_board<Kind, Config>::base_address);
  else
    static_assert(dependent_false_v<Config>, "STM32 LL serial requires a board peripheral register binding");
}
template <auto Handler> void serial_complete(const status value) noexcept
{
  if constexpr (requires { Handler(value); })
    Handler(value);
  else if constexpr (requires { Handler(); })
    Handler();
}

struct serial_operation
{
  std::atomic<bool> active{false};
  std::atomic<status> result{status::ok};
  std::span<const std::uint8_t> tx{};
  std::span<std::uint8_t> rx{};
  std::size_t written{}, received{}, reported{};
  std::uint32_t started{}, generation{};
  std::uint16_t address{};
  std::uint8_t address_stage{};
  bool dma{}, idle{}, circular{}, addressed{}, reading{}, repeated{}, tx_done{}, rx_done{};
  void (*completion)(status) noexcept {};
  void (*data_completion)(status, std::span<std::uint8_t>) noexcept {};
  serial_dma_resource tx_dma{}, rx_dma{};
  std::uint8_t dummy{0xFFU};

  bool claim() noexcept
  {
    serial_guard guard;
#if defined(__arm__) || defined(__thumb__)
    if (active.load(std::memory_order_relaxed))
      return false;
    active.store(true, std::memory_order_relaxed);
    return true;
#else
    return !active.exchange(true);
#endif
  }
  void reset() noexcept
  {
    ++generation;
    tx = {};
    rx = {};
    written = received = reported = 0U;
    idle = circular = addressed = reading = repeated = tx_done = rx_done = false;
    address_stage = 0U;
    completion = nullptr;
    data_completion = nullptr;
    tx_dma = {};
    rx_dma = {};
    result = status::busy;
  }
  bool stop_dma() const noexcept
  {
    const bool tx_stopped = tx_dma.stop();
    const bool rx_stopped = rx_dma.stop();
    return tx_stopped && rx_stopped;
  }
  void finish(const status value, const std::size_t count = 0U) noexcept
  {
    const auto callback = completion;
    const auto data_callback = data_completion;
    const auto data = value == status::ok ? rx.subspan(reported, count - reported) : std::span<std::uint8_t>{};
    result = value;
    tx = {};
    rx = {};
    completion = nullptr;
    data_completion = nullptr;
    active.store(false, std::memory_order_release);
    if (callback)
      callback(value);
    if (data_callback)
      data_callback(value, data);
  }
};
template <peripheral_kind Kind, std::uint32_t Instance> struct serial_storage
{
  static inline serial_operation bus{}, tx{}, rx{};
};

template <peripheral_kind Kind> struct native_serial_engine;

#if defined(USART_CR1_UE)
template <> struct native_serial_engine<peripheral_kind::uart>
{
  template <typename C> static auto& tx()
  {
    return serial_storage<peripheral_kind::uart, serial_identity<C>()>::tx;
  }
  template <typename C> static auto& rx()
  {
    return serial_storage<peripheral_kind::uart, serial_identity<C>()>::rx;
  }
  template <typename C> static auto* device()
  {
    return serial_instance<peripheral_kind::uart, USART_TypeDef, C>();
  }
  template <typename C> static status initialize() noexcept
  {
    serial_guard guard;
    if (tx<C>().active || rx<C>().active)
      return status::busy;
    auto* p = device<C>();
    const auto clock = serial_clock<peripheral_kind::uart, C>();
    if (!clock)
      return status::unsupported;
    if (C::stop_bits != uart_stop_bits::one && C::stop_bits != uart_stop_bits::two)
      return status::unsupported;
    bool low_power = false;
#if defined(LPUART1_BASE)
    low_power = reinterpret_cast<std::uintptr_t>(p) == LPUART1_BASE;
#endif
#if defined(LPUART2_BASE)
    low_power = low_power || reinterpret_cast<std::uintptr_t>(p) == LPUART2_BASE;
#endif
    if constexpr (requires { C::require_lpuart; })
      if (C::require_lpuart && !low_power)
        return status::unsupported;
    if constexpr (requires { C::require_fifo; })
      if (C::require_fifo)
      {
#if !defined(USART_CR1_FIFOEN)
        return status::unsupported;
#endif
      }
    if (C::baud_rate == 0U)
      return status::invalid_argument;
    const auto divider = low_power ? (static_cast<std::uint64_t>(clock) * 256U + C::baud_rate / 2U) / C::baud_rate
                                   : (clock + C::baud_rate / 2U) / C::baud_rate;
    if (low_power ? divider < 0x300U || divider > 0xFFFFFU : divider < 16U || divider > 65535U)
      return status::invalid_argument;
    p->CR1 = 0U;
    p->CR2 = (C::stop_bits == uart_stop_bits::two ? USART_CR2_STOP_1 : 0U);
    p->CR3 = 0U;
    p->BRR = static_cast<std::uint32_t>(divider);
#if defined(USART_PRESC_PRESCALER)
    p->PRESC = 0U;
#endif
    // Byte payloads retain eight data bits when parity is enabled.
    p->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE |
             (C::parity == uart_parity::none
                  ? 0U
                  : USART_CR1_PCE | USART_CR1_M | (C::parity == uart_parity::odd ? USART_CR1_PS : 0U));
#if defined(USART_CR1_FIFOEN)
    if constexpr (requires { C::require_fifo; })
      if (C::require_fifo)
        p->CR1 = p->CR1 | USART_CR1_FIFOEN;
#endif
    return status::ok;
  }
  static volatile std::uint32_t& flags(USART_TypeDef* p) noexcept
  {
#if defined(USART_ISR_RXNE) || defined(USART_ISR_RXNE_RXFNE)
    return p->ISR;
#else
    return p->SR;
#endif
  }
  static std::uintptr_t data_address(USART_TypeDef* p, bool transmit) noexcept
  {
#if defined(USART_ISR_RXNE) || defined(USART_ISR_RXNE_RXFNE)
    return reinterpret_cast<std::uintptr_t>(transmit ? &p->TDR : &p->RDR);
#else
    (void)transmit;
    return reinterpret_cast<std::uintptr_t>(&p->DR);
#endif
  }
  static void clear_flags(USART_TypeDef* p, const std::uint32_t mask) noexcept
  {
#if defined(USART_ICR_IDLECF)
    p->ICR = mask;
#else
    const volatile auto sr = p->SR;
    const volatile auto dr = p->DR;
    (void)sr;
    (void)dr;
    (void)mask;
#endif
  }
  template <typename C> static void disable(const bool transmit) noexcept
  {
    auto* p = device<C>();
    p->CR1 = p->CR1 & ~(transmit ? ((1U << 7U) | USART_CR1_TCIE) : ((1U << 5U) | USART_CR1_IDLEIE | USART_CR1_PEIE));
    p->CR3 = p->CR3 & ~(transmit ? USART_CR3_DMAT : USART_CR3_DMAR);
    if (transmit ? !rx<C>().active : !tx<C>().active)
      p->CR3 = p->CR3 & ~USART_CR3_EIE;
  }
  template <typename C> static status cancel(const bool transmit, const status reason) noexcept
  {
    serial_guard guard;
    auto& s = transmit ? tx<C>() : rx<C>();
    if (!s.active)
      return status::ok;
    disable<C>(transmit);
    if (!s.stop_dma())
    {
      s.result = status::io_error;
      return status::io_error;
    }
    s.finish(reason);
    return status::ok;
  }
  template <typename C> static status abort_tx() noexcept
  {
    return cancel<C>(true, status::error);
  }
  template <typename C> static status abort_rx() noexcept
  {
    return cancel<C>(false, status::error);
  }
  template <typename C> static status abort() noexcept
  {
    const auto a = abort_tx<C>(), b = abort_rx<C>();
    return a == status::ok ? b : a;
  }
  template <typename C, auto H>
  static status submit(std::span<const std::uint8_t> output, std::span<std::uint8_t> input, bool to_idle = false,
                       bool circular = false) noexcept
  {
    const bool transmit = !output.empty();
    auto& s = transmit ? tx<C>() : rx<C>();
    const auto size = transmit ? output.size() : input.size();
    if (!size || size > 65535U || (circular && (size < 2U || size % 2U)))
      return status::invalid_argument;
    constexpr auto mode = configured_transfer_mode<C>();
    if (circular && mode != transfer_mode::dma)
      return status::unsupported;
    if constexpr (mode == transfer_mode::polled || std::is_same_v<decltype(H), std::nullptr_t>)
      if constexpr (!serial_has_time<peripheral_kind::uart, C>)
        return status::unsupported;
    serial_guard guard;
    if (!s.claim())
      return status::busy;
    s.reset();
    s.tx = output;
    s.rx = input;
    s.dma = mode == transfer_mode::dma;
    s.idle = to_idle;
    s.circular = circular;
    s.started = serial_now<peripheral_kind::uart, C>();
    if constexpr (requires { H(status::ok, input); })
      s.data_completion = [](status v, std::span<std::uint8_t> data) noexcept { H(v, data); };
    else
      s.completion = serial_complete<H>;
    auto* p = device<C>();
    if (s.dma)
    {
      auto resource = serial_dma<peripheral_kind::uart, C>(transmit);
      auto& opposite = transmit ? rx<C>() : tx<C>();
      const auto other_resource = transmit ? opposite.rx_dma : opposite.tx_dma;
      if (opposite.active && resource && resource.control == other_resource.control)
      {
        s.completion = nullptr;
        s.data_completion = nullptr;
        s.finish(status::invalid_argument);
        return status::invalid_argument;
      }
      if (transmit)
        s.tx_dma = resource;
      else
        s.rx_dma = resource;
      const auto result = resource.start(data_address(p, transmit),
                                         reinterpret_cast<std::uintptr_t>(transmit ? output.data() : input.data()),
                                         size, transmit, circular);
      if (result != status::ok)
      {
        s.completion = nullptr;
        s.data_completion = nullptr;
        if (s.stop_dma())
          s.finish(result);
        else
          s.result = status::io_error;
        return result;
      }
      p->CR3 = p->CR3 | (transmit ? USART_CR3_DMAT : USART_CR3_DMAR);
    }
    else
      p->CR1 = p->CR1 | (transmit ? (1U << 7U) : (1U << 5U) | USART_CR1_PEIE);
    if (to_idle || circular)
    {
      clear_flags(p, 1U << 4U);
      p->CR1 = p->CR1 | USART_CR1_IDLEIE;
    }
    p->CR3 = p->CR3 | USART_CR3_EIE;
    return status::ok;
  }
  template <typename C> static void check_deadline(serial_operation& s, const bool transmit) noexcept
  {
    if constexpr (serial_has_time<peripheral_kind::uart, C>)
      if (s.active && !s.circular &&
          static_cast<std::uint32_t>(serial_now<peripheral_kind::uart, C>() - s.started) >= serial_timeout<C>())
        (void)cancel<C>(transmit, status::timeout);
  }
  template <typename C> static status tx_status() noexcept
  {
    check_deadline<C>(tx<C>(), true);
    return tx<C>().result;
  }
  template <typename C> static status rx_status() noexcept
  {
    check_deadline<C>(rx<C>(), false);
    return rx<C>().result;
  }
  template <typename C> static status transfer_status() noexcept
  {
    const auto a = tx_status<C>(), b = rx_status<C>();
    if (a == status::busy || b == status::busy)
      return status::busy;
    return a == status::ok ? b : a;
  }
  template <typename C> static void report(const std::size_t end) noexcept
  {
    auto& s = rx<C>();
    if (end > s.reported)
    {
      auto data = s.rx.subspan(s.reported, end - s.reported);
      s.reported = end;
      if (s.data_completion)
        s.data_completion(status::ok, data);
    }
  }
  template <typename C> static status handle_dma_interrupt() noexcept
  {
    serial_guard guard;
    auto& r = rx<C>();
    auto& t = tx<C>();
    for (auto* s : {&t, &r})
    {
      if (!s->active || !s->dma)
        continue;
      const bool transmit = s == &t;
      const auto generation = s->generation;
      const auto resource = transmit ? s->tx_dma : s->rx_dma;
      const auto f = *resource.flags;
      *resource.clear = f & (resource.complete_mask | resource.half_mask | resource.error_mask);
      if (f & resource.error_mask)
      {
        (void)cancel<C>(transmit, status::io_error);
        continue;
      }
      if (!transmit && s->circular)
      {
        if (f & resource.half_mask)
          report<C>(s->rx.size() / 2U);
        if (!s->active || s->generation != generation)
          continue;
        if (f & resource.complete_mask)
        {
          report<C>(s->rx.size());
          if (s->active && s->generation == generation)
            s->reported = 0U;
        }
      }
      else if (f & resource.complete_mask)
      {
        if (!resource.stop())
        {
          s->result = status::io_error;
          continue;
        }
        if (transmit)
        {
          p_disable_dma_tx<C>();
          device<C>()->CR1 = device<C>()->CR1 | USART_CR1_TCIE;
        }
        else
        {
          disable<C>(false);
          s->finish(status::ok, s->rx.size());
        }
      }
    }
    return transfer_status<C>();
  }
  template <typename C> static void p_disable_dma_tx() noexcept
  {
    device<C>()->CR3 = device<C>()->CR3 & ~USART_CR3_DMAT;
  }
  template <typename C> static status handle_interrupt() noexcept
  {
    serial_guard guard;
    auto* p = device<C>();
    auto& t = tx<C>();
    auto& r = rx<C>();
    auto f = flags(p);
    if (f & 0xFU)
    {
      clear_flags(p, f & 0xFU);
      // Receiver errors must not cancel an independently owned transmitter.
      if (r.active)
        (void)cancel<C>(false, status::io_error);
    }
    if (r.active && !r.dma && (f & (1U << 5U)))
    {
      r.rx[r.received++] = *reinterpret_cast<volatile std::uint8_t*>(data_address(p, false));
      if (r.received == r.rx.size())
      {
        disable<C>(false);
        r.finish(status::ok, r.received);
      }
    }
    if (r.active && (r.idle || r.circular) && (f & (1U << 4U)))
    {
      clear_flags(p, 1U << 4U);
      if (r.dma)
      {
        if (r.circular)
          report<C>(r.rx.size() - r.rx_dma.remaining());
        else
        {
          disable<C>(false);
          if (r.stop_dma())
            r.finish(status::ok, r.rx.size() - r.rx_dma.remaining());
          else
            r.result = status::io_error;
        }
      }
      else
      {
        disable<C>(false);
        r.finish(status::ok, r.received);
      }
    }
    if (t.active && !t.dma && (f & (1U << 7U)) && t.written < t.tx.size())
    {
      *reinterpret_cast<volatile std::uint8_t*>(data_address(p, true)) = t.tx[t.written++];
      if (t.written == t.tx.size())
        p->CR1 = (p->CR1 & ~(1U << 7U)) | USART_CR1_TCIE;
      // The sampled TC predates the last write.
      f = f & ~(1U << 6U);
    }
    if (t.active && (p->CR1 & USART_CR1_TCIE) && (f & (1U << 6U)))
    {
#if defined(USART_ICR_TCCF)
      p->ICR = USART_ICR_TCCF;
#endif
      disable<C>(true);
      t.finish(status::ok);
    }
    return transfer_status<C>();
  }
  template <typename C, auto H> static status write(std::span<const std::uint8_t> data, std::uint16_t = 0U) noexcept
  {
    auto value = submit<C, H>(data, {});
    constexpr bool blocking =
        configured_transfer_mode<C>() == transfer_mode::polled || std::is_same_v<decltype(H), std::nullptr_t>;
    if constexpr (blocking)
      while (value == status::ok && tx<C>().active && tx<C>().result == status::busy)
      {
        if constexpr (configured_transfer_mode<C>() == transfer_mode::dma)
          (void)handle_dma_interrupt<C>();
        (void)handle_interrupt<C>();
      }
    return value == status::ok && blocking ? tx<C>().result.load(std::memory_order_acquire) : value;
  }
  template <typename C, auto H> static status read(std::span<std::uint8_t> data, std::uint16_t = 0U) noexcept
  {
    auto value = submit<C, H>({}, data);
    constexpr bool blocking =
        configured_transfer_mode<C>() == transfer_mode::polled || std::is_same_v<decltype(H), std::nullptr_t>;
    if constexpr (blocking)
      while (value == status::ok && rx<C>().active && rx<C>().result == status::busy)
      {
        if constexpr (configured_transfer_mode<C>() == transfer_mode::dma)
          (void)handle_dma_interrupt<C>();
        (void)handle_interrupt<C>();
      }
    return value == status::ok && blocking ? rx<C>().result.load(std::memory_order_acquire) : value;
  }
  template <typename C, auto H> static status start_receive_to_idle(std::span<std::uint8_t> data) noexcept
  {
    if constexpr (configured_transfer_mode<C>() == transfer_mode::polled)
      return status::unsupported;
    else
      return submit<C, H>({}, data, true);
  }
  template <typename C, auto H> static status start_circular_rx(std::span<std::uint8_t> data) noexcept
  {
    return submit<C, H>({}, data, true, true);
  }
};
#endif
template <peripheral_kind Kind> struct native_bus_engine
{
  using hardware = native_serial_engine<Kind>;
  template <typename C> static bool stop_hardware(const bool reset = true) noexcept
  {
    if constexpr (std::is_same_v<decltype(hardware::template stop<C>(reset)), bool>)
      return hardware::template stop<C>(reset);
    else
    {
      hardware::template stop<C>(reset);
      return true;
    }
  }
  template <typename C> static auto& state()
  {
    return serial_storage<Kind, serial_identity<C>()>::bus;
  }
  template <typename C> static status initialize() noexcept
  {
    serial_guard guard;
    if (state<C>().active)
      return status::busy;
    return hardware::template setup<C>();
  }
  template <typename C> static status cancel(const status reason) noexcept
  {
    serial_guard guard;
    auto& s = state<C>();
    if (!s.active)
      return status::ok;
    const bool idle = stop_hardware<C>();
    if (!s.stop_dma() || !idle)
    {
      s.result = status::io_error;
      return status::io_error;
    }
    s.finish(reason);
    return status::ok;
  }
  template <typename C> static status abort() noexcept
  {
    return cancel<C>(status::error);
  }
  template <typename C> static status transfer_status() noexcept
  {
    auto& s = state<C>();
    if constexpr (serial_has_time<Kind, C>)
      if (s.active && static_cast<std::uint32_t>(serial_now<Kind, C>() - s.started) >= serial_timeout<C>())
        (void)cancel<C>(status::timeout);
    return s.result;
  }
  template <typename C> static status complete(const status value) noexcept
  {
    auto& s = state<C>();
    const bool idle = stop_hardware<C>(value != status::ok);
    if (!s.stop_dma() || !idle)
    {
      s.result = status::io_error;
      return status::io_error;
    }
    s.finish(value);
    return value;
  }
  template <typename C, auto H>
  static status submit(std::span<const std::uint8_t> tx, std::span<std::uint8_t> rx, std::uint16_t address = 0U,
                       bool repeated = false, bool probe = false) noexcept
  {
    if ((!probe && tx.empty() && rx.empty()) || tx.size() > 65535U || rx.size() > 65535U)
      return status::invalid_argument;
    if constexpr (Kind == peripheral_kind::spi)
      if (!tx.empty() && !rx.empty() && tx.size() != rx.size())
        return status::invalid_argument;
    if constexpr (Kind == peripheral_kind::i2c)
      if (address > (C::address_mode == i2c_address_mode::seven_bit ? 127U : 1023U))
        return status::invalid_argument;
    constexpr bool blocking =
        configured_transfer_mode<C>() == transfer_mode::polled || std::is_same_v<decltype(H), std::nullptr_t>;
    if constexpr (blocking)
      if constexpr (!serial_has_time<Kind, C>)
        return status::unsupported;
    status value;
    {
      serial_guard guard;
      auto& s = state<C>();
      if (!s.claim())
        return status::busy;
      s.reset();
      s.tx = tx;
      s.rx = rx;
      s.address = address;
      s.repeated = repeated;
      s.dma = configured_transfer_mode<C>() == transfer_mode::dma;
      s.started = serial_now<Kind, C>();
      s.completion = serial_complete<H>;
      value = hardware::template start<C>();
      if (value != status::ok)
      {
        s.completion = nullptr;
        // A failed rollback retains ownership until a later abort can stop DMA.
        if (s.stop_dma())
          s.finish(value);
        else
          s.result = status::io_error;
        return value;
      }
    }
    if constexpr (blocking)
    {
      while (state<C>().active && state<C>().result == status::busy)
      {
        if constexpr (configured_transfer_mode<C>() == transfer_mode::dma)
          (void)handle_dma_interrupt<C>();
        (void)handle_interrupt<C>();
      }
      return state<C>().result;
    }
    return value;
  }
  template <typename C, auto H> static status write(std::span<const std::uint8_t> tx, std::uint16_t a = 0U) noexcept
  {
    return submit<C, H>(tx, {}, a);
  }
  template <typename C, auto H> static status read(std::span<std::uint8_t> rx, std::uint16_t a = 0U) noexcept
  {
    return submit<C, H>({}, rx, a);
  }
  template <typename C, auto H>
  static status transfer(std::span<const std::uint8_t> tx, std::span<std::uint8_t> rx) noexcept
  {
    return submit<C, H>(tx, rx);
  }
  template <typename C, auto H>
  static status write_read(std::span<const std::uint8_t> tx, std::span<std::uint8_t> rx, std::uint16_t a) noexcept
  {
    if (tx.empty() || rx.empty())
      return status::invalid_argument;
    return submit<C, H>(tx, rx, a, true);
  }
  template <typename C> static status probe(std::uint16_t address) noexcept
  {
    if constexpr (serial_slave<C>())
      return status::unsupported;
    else
      return submit<C, nullptr>({}, {}, address, false, true);
  }
  template <typename C> static status handle_interrupt() noexcept
  {
    serial_guard guard;
    if (state<C>().active)
      hardware::template service<C>();
    return transfer_status<C>();
  }
  template <typename C> static status handle_dma_interrupt() noexcept
  {
    serial_guard guard;
    auto& s = state<C>();
    if (!s.active || !s.dma)
      return transfer_status<C>();
    const auto generation = s.generation;
    for (const bool transmit : {false, true})
    {
      auto resource = transmit ? s.tx_dma : s.rx_dma;
      if (!resource)
        continue;
      const auto flags = *resource.flags;
      *resource.clear = flags & (resource.complete_mask | resource.half_mask | resource.error_mask);
      if (flags & resource.error_mask)
        return complete<C>(status::io_error);
      if (flags & resource.complete_mask)
      {
        if (!resource.stop())
        {
          s.result = status::io_error;
          return status::io_error;
        }
        if (transmit)
        {
          s.tx_done = true;
          s.written = s.tx.size();
        }
        else
        {
          s.rx_done = true;
          s.received = s.rx.size();
        }
        hardware::template dma_complete<C>(transmit);
        if (!s.active || s.generation != generation)
          return s.result;
      }
    }
    if (s.active)
      hardware::template service<C>();
    return transfer_status<C>();
  }
};

#if defined(SPI_CR1_SPE)
template <> struct native_serial_engine<peripheral_kind::spi> : native_bus_engine<peripheral_kind::spi>
{
  template <typename C> static auto* device()
  {
    return serial_instance<peripheral_kind::spi, SPI_TypeDef, C>();
  }
  template <typename C> static status setup() noexcept
  {
    auto* p = device<C>();
    const auto clock = serial_clock<peripheral_kind::spi, C>();
    unsigned divisor = 0U;
    if constexpr (!serial_slave<C>())
    {
      if (!clock)
        return status::unsupported;
      if (C::clock_hz == 0U || C::clock_hz < clock / 256U)
        return status::invalid_argument;
      while (divisor < 7U && (clock >> (divisor + 1U)) > C::clock_hz)
        ++divisor;
    }
    if (C::duplex_mode != spi_duplex_mode::full && C::duplex_mode != spi_duplex_mode::half &&
        C::duplex_mode != spi_duplex_mode::simplex_rx && C::duplex_mode != spi_duplex_mode::simplex_tx)
      return status::invalid_argument;
    p->CR1 = 0U;
#if defined(SPI_CFG1_DSIZE)
    p->CFG1 = 7U | (divisor << 28U);
    p->CFG2 = SPI_CFG2_SSM | (serial_slave<C>() ? 0U : SPI_CFG2_MASTER) |
              (C::clock_polarity == spi_clock_polarity::idle_high ? SPI_CFG2_CPOL : 0U) |
              (C::clock_phase == spi_clock_phase::second_edge ? SPI_CFG2_CPHA : 0U) |
              (C::bit_order == spi_bit_order::lsb_first ? SPI_CFG2_LSBFRST : 0U);
    if constexpr (serial_slave<C>())
      p->CFG2 = p->CFG2 & ~SPI_CFG2_SSM;
    p->CFG2 = p->CFG2 | (C::duplex_mode == spi_duplex_mode::half         ? SPI_CFG2_COMM
                         : C::duplex_mode == spi_duplex_mode::simplex_tx ? SPI_CFG2_COMM_0
                         : C::duplex_mode == spi_duplex_mode::simplex_rx ? SPI_CFG2_COMM_1
                                                                         : 0U);
    p->CR1 = SPI_CR1_SSI;
#else
    p->CR1 = (serial_slave<C>() ? 0U : SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI) | (divisor << 3U) |
             (C::clock_polarity == spi_clock_polarity::idle_high ? SPI_CR1_CPOL : 0U) |
             (C::clock_phase == spi_clock_phase::second_edge ? SPI_CR1_CPHA : 0U) |
             (C::bit_order == spi_bit_order::lsb_first ? SPI_CR1_LSBFIRST : 0U);
    p->CR1 = p->CR1 | (C::duplex_mode == spi_duplex_mode::half         ? SPI_CR1_BIDIMODE
                       : C::duplex_mode == spi_duplex_mode::simplex_tx ? SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE
                       : C::duplex_mode == spi_duplex_mode::simplex_rx ? SPI_CR1_RXONLY
                                                                       : 0U);
    p->CR2 = 0U;
#if defined(SPI_CR2_DS)
    p->CR2 = (7U << 8U) | SPI_CR2_FRXTH;
#endif
#endif
    return status::ok;
  }
  template <typename C> static std::uintptr_t data_address(const bool tx) noexcept
  {
    auto* p = device<C>();
#if defined(SPI_CFG1_DSIZE)
    return reinterpret_cast<std::uintptr_t>(tx ? &p->TXDR : &p->RXDR);
#else
    (void)tx;
    return reinterpret_cast<std::uintptr_t>(&p->DR);
#endif
  }
#if !defined(SPI_CFG1_DSIZE)
  template <typename C> static bool wire_idle() noexcept
  {
    auto* p = device<C>();
    // Classic master RXONLY does not provide a usable end-of-transfer BSY indication.
    if constexpr (!serial_slave<C>() && C::duplex_mode == spi_duplex_mode::simplex_rx)
      return !(p->CR1 & SPI_CR1_SPE) && !(p->SR & SPI_SR_RXNE);
    else
      return !(p->SR & SPI_SR_BSY);
  }
#endif
  template <typename C> static bool stop(bool = true) noexcept
  {
    auto* p = device<C>();
#if defined(SPI_CFG1_DSIZE)
    p->IER = 0U;
    p->CFG1 = p->CFG1 & ~(SPI_CFG1_RXDMAEN | SPI_CFG1_TXDMAEN);
    if constexpr (!serial_slave<C>())
    {
      if ((p->CR1 & SPI_CR1_CSTART) && !(p->SR & (SPI_SR_EOT | SPI_SR_SUSP)))
      {
        p->CR1 = p->CR1 | SPI_CR1_CSUSP;
        unsigned budget = 1024U;
        while (!(p->SR & (SPI_SR_SUSP | SPI_SR_EOT)) && --budget)
        {
        }
        if (!budget)
          return false;
      }
    }
    p->CR1 = p->CR1 & ~SPI_CR1_SPE;
    p->IFCR = 0xFFFFFFFFU;
#else
    p->CR2 = p->CR2 & ~(SPI_CR2_TXEIE | SPI_CR2_RXNEIE | SPI_CR2_ERRIE | SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);
    p->CR1 = p->CR1 & ~SPI_CR1_SPE;
    // Four byte reads drain the largest classic SPI RX FIFO, including a late receive-only frame.
    for (unsigned n = 0; n < 4U; ++n)
    {
      const volatile auto data = *reinterpret_cast<volatile std::uint8_t*>(data_address<C>(false));
      (void)data;
    }
    unsigned budget = 1024U;
    while (!wire_idle<C>() && --budget)
    {
    }
    if (!budget)
      return false;
    // Clear OVR by DR then SR reads, never by writing the status register.
    const volatile auto dr = p->DR;
    const volatile auto sr = p->SR;
    (void)dr;
    (void)sr;
#endif
    return true;
  }
  template <typename C> static bool needs_tx() noexcept
  {
    return C::duplex_mode == spi_duplex_mode::full || !state<C>().tx.empty();
  }
  template <typename C> static bool needs_rx() noexcept
  {
    return C::duplex_mode == spi_duplex_mode::full || !state<C>().rx.empty();
  }
  template <typename C> static status start() noexcept
  {
    auto& s = state<C>();
    auto* p = device<C>();
    if ((C::duplex_mode != spi_duplex_mode::full && !s.tx.empty() && !s.rx.empty()) ||
        (C::duplex_mode == spi_duplex_mode::simplex_tx && s.tx.empty()) ||
        (C::duplex_mode == spi_duplex_mode::simplex_rx && s.rx.empty()))
      return status::invalid_argument;
    const bool transmit = needs_tx<C>(), receive = needs_rx<C>();
    if (p->CR1 & SPI_CR1_SPE)
      return status::busy;
#if defined(SPI_CFG1_DSIZE)
    if (C::duplex_mode == spi_duplex_mode::half)
      p->CR1 = transmit ? p->CR1 | SPI_CR1_HDDIR : p->CR1 & ~SPI_CR1_HDDIR;
#else
    if (C::duplex_mode == spi_duplex_mode::half)
      p->CR1 = transmit ? p->CR1 | SPI_CR1_BIDIOE : p->CR1 & ~SPI_CR1_BIDIOE;
#endif
    const auto count = s.tx.empty() ? s.rx.size() : s.tx.size();
    if (s.dma)
    {
      if (transmit)
        s.tx_dma = serial_dma<peripheral_kind::spi, C>(true);
      if (receive)
        s.rx_dma = serial_dma<peripheral_kind::spi, C>(false);
      if (transmit && receive && s.tx_dma && s.tx_dma.control == s.rx_dma.control)
        return status::invalid_argument;
      if (receive)
      {
        auto v = s.rx_dma.start(data_address<C>(false),
                                reinterpret_cast<std::uintptr_t>(s.rx.empty() ? &s.dummy : s.rx.data()), count, false,
                                false, !s.rx.empty());
        if (v != status::ok)
          return v;
      }
      if (transmit)
      {
        auto v = s.tx_dma.start(data_address<C>(true),
                                reinterpret_cast<std::uintptr_t>(s.tx.empty() ? &s.dummy : s.tx.data()), count, true,
                                false, !s.tx.empty());
        if (v != status::ok)
          return v;
      }
    }
#if defined(SPI_CFG1_DSIZE)
    p->IFCR = 0xFFFFFFFFU;
    p->CR2 = static_cast<std::uint32_t>(count);
    p->IER = SPI_IER_EOTIE | SPI_IER_OVRIE | SPI_IER_MODFIE | SPI_IER_UDRIE |
             (s.dma ? 0U : (receive ? SPI_IER_RXPIE : 0U) | (transmit ? SPI_IER_TXPIE : 0U));
    if (s.dma)
      p->CFG1 = p->CFG1 | (transmit ? SPI_CFG1_TXDMAEN : 0U) | (receive ? SPI_CFG1_RXDMAEN : 0U);
    p->CR1 = p->CR1 | SPI_CR1_SPE;
    if constexpr (!serial_slave<C>())
      p->CR1 = p->CR1 | SPI_CR1_CSTART;
#else
    p->CR2 = p->CR2 | SPI_CR2_ERRIE |
             (s.dma ? (transmit ? SPI_CR2_TXDMAEN : 0U) | (receive ? SPI_CR2_RXDMAEN : 0U)
                    : (transmit ? SPI_CR2_TXEIE : 0U) | (receive ? SPI_CR2_RXNEIE : 0U));
    p->CR1 = p->CR1 | SPI_CR1_SPE;
#endif
    return status::ok;
  }
  template <typename C> static void dma_complete(bool transmit) noexcept
  {
    auto* p = device<C>();
#if defined(SPI_CFG1_DSIZE)
    p->CFG1 = p->CFG1 & ~(transmit ? SPI_CFG1_TXDMAEN : SPI_CFG1_RXDMAEN);
#else
    p->CR2 = p->CR2 & ~(transmit ? SPI_CR2_TXDMAEN : SPI_CR2_RXDMAEN);
    // TXE is the drain interrupt until BSY drops; callback is never emitted by DMA TC alone.
    if (!transmit && !needs_tx<C>() && !serial_slave<C>())
      p->CR1 = p->CR1 & ~SPI_CR1_SPE;
    if ((!needs_tx<C>() || state<C>().tx_done) && (!needs_rx<C>() || state<C>().rx_done))
      p->CR2 = p->CR2 | SPI_CR2_TXEIE;
#endif
  }
  template <typename C> static void service() noexcept
  {
    auto& s = state<C>();
    auto* p = device<C>();
    const auto flags = p->SR;
    const auto count = s.tx.empty() ? s.rx.size() : s.tx.size();
#if defined(SPI_CFG1_DSIZE)
    constexpr auto rx_ready = SPI_SR_RXP, tx_ready = SPI_SR_TXP;
    constexpr auto errors = SPI_SR_OVR | SPI_SR_MODF | SPI_SR_UDR;
#else
    constexpr auto rx_ready = SPI_SR_RXNE, tx_ready = SPI_SR_TXE;
    constexpr auto errors = SPI_SR_OVR | SPI_SR_MODF;
#endif
    if (flags & errors)
    {
      (void)complete<C>(status::io_error);
      return;
    }
    if (!s.dma)
    {
      if (needs_rx<C>() && (flags & rx_ready) && s.received < count)
      {
#if !defined(SPI_CFG1_DSIZE)
        // Receive-only masters have no TSIZE: stop clocks before consuming the final DR byte.
        if (!needs_tx<C>() && !serial_slave<C>() && s.received + 1U == count)
          p->CR1 = p->CR1 & ~SPI_CR1_SPE;
#endif
        const auto value = *reinterpret_cast<volatile std::uint8_t*>(data_address<C>(false));
        if (!s.rx.empty())
          s.rx[s.received] = value;
        ++s.received;
      }
      if (needs_tx<C>() && (flags & tx_ready) && s.written < count)
      {
        *reinterpret_cast<volatile std::uint8_t*>(data_address<C>(true)) = s.tx.empty() ? 0xFFU : s.tx[s.written];
        ++s.written;
      }
    }
    const bool drained = (!needs_tx<C>() || (s.dma ? s.tx_done || s.tx_dma.remaining() == 0U : s.written == count)) &&
                         (!needs_rx<C>() || (s.dma ? s.rx_done || s.rx_dma.remaining() == 0U : s.received == count));
#if defined(SPI_CFG1_DSIZE)
    if (drained && (flags & SPI_SR_EOT))
      (void)complete<C>(status::ok);
#else
    if constexpr (!serial_slave<C>() && C::duplex_mode == spi_duplex_mode::simplex_rx)
      if (drained && (p->SR & SPI_SR_RXNE))
      {
        const volatile auto data = *reinterpret_cast<volatile std::uint8_t*>(data_address<C>(false));
        (void)data;
      }
    if (drained && wire_idle<C>())
      (void)complete<C>(status::ok);
    else if (!s.dma && (!needs_tx<C>() || s.written == count))
      p->CR2 = drained ? p->CR2 | SPI_CR2_TXEIE : p->CR2 & ~SPI_CR2_TXEIE;
#endif
  }
};
#endif

#if defined(I2C_CR1_PE)
template <> struct native_serial_engine<peripheral_kind::i2c> : native_bus_engine<peripheral_kind::i2c>
{
  template <typename C> static auto* device()
  {
    return serial_instance<peripheral_kind::i2c, I2C_TypeDef, C>();
  }
  template <typename C> static status setup() noexcept
  {
    auto* p = device<C>();
    p->CR1 = 0U;
#if defined(I2C_CR2_NBYTES)
    if constexpr (requires { C::timing; })
      p->TIMINGR = C::timing;
    else if constexpr (requires { C::binding::timing; })
      p->TIMINGR = C::binding::timing;
    else if constexpr (requires { serial_board<peripheral_kind::i2c, C>::timing(); })
      p->TIMINGR = serial_board<peripheral_kind::i2c, C>::timing();
    else
      return status::unsupported; // Timing depends on board rise/fall times and analog filtering.
    p->CR2 = 0U;
    if constexpr (serial_slave<C>())
    {
      if (C::own_address > (C::address_mode == i2c_address_mode::seven_bit ? 127U : 1023U))
        return status::invalid_argument;
      p->OAR1 = I2C_OAR1_OA1EN | (C::address_mode == i2c_address_mode::seven_bit ? C::own_address << 1U
                                                                                 : C::own_address | I2C_OAR1_OA1MODE);
    }
    else
      p->OAR1 = 0U;
    p->CR1 = I2C_CR1_PE;
#else
    const auto clock = serial_clock<peripheral_kind::i2c, C>();
    if (clock < 2000000U || clock > 50000000U)
      return status::unsupported;
    if constexpr (serial_slave<C>())
      if (C::own_address > (C::address_mode == i2c_address_mode::seven_bit ? 127U : 1023U))
        return status::invalid_argument;
    if (C::clock_hz == 0U || C::clock_hz > 400000U)
      return status::invalid_argument;
    p->CR2 = clock / 1000000U;
    if (C::clock_hz <= 100000U)
    {
      auto divider = clock / (2U * C::clock_hz);
      if (divider < 4U)
        divider = 4U;
      if (divider > 0xFFFU)
        return status::invalid_argument;
      p->CCR = divider;
      p->TRISE = clock / 1000000U + 1U;
    }
    else
    {
      p->CCR = I2C_CCR_FS | clock / (3U * C::clock_hz);
      p->TRISE = ((clock / 1000000U) * 300U) / 1000U + 1U;
    }
    if constexpr (serial_slave<C>())
      p->OAR1 = (1U << 14U) |
                (C::address_mode == i2c_address_mode::seven_bit ? C::own_address << 1U : 0x8000U | C::own_address);
    else
      p->OAR1 = 1U << 14U;
    p->CR1 = I2C_CR1_PE | I2C_CR1_ACK;
#endif
    return status::ok;
  }
  template <typename C> static std::uintptr_t data_address(const bool tx) noexcept
  {
    auto* p = device<C>();
#if defined(I2C_CR2_NBYTES)
    return reinterpret_cast<std::uintptr_t>(tx ? &p->TXDR : &p->RXDR);
#else
    (void)tx;
    return reinterpret_cast<std::uintptr_t>(&p->DR);
#endif
  }
  template <typename C> static void stop(bool reset = true) noexcept
  {
    auto* p = device<C>();
#if defined(I2C_CR2_NBYTES)
    p->CR1 = p->CR1 & ~(I2C_CR1_TXIE | I2C_CR1_RXIE | I2C_CR1_ADDRIE | I2C_CR1_NACKIE | I2C_CR1_STOPIE | I2C_CR1_TCIE |
                        I2C_CR1_ERRIE | I2C_CR1_TXDMAEN | I2C_CR1_RXDMAEN);
    if constexpr (!serial_slave<C>())
      if ((p->ISR & I2C_ISR_BUSY) && !(p->ISR & I2C_ISR_ARLO))
        p->CR2 = p->CR2 | I2C_CR2_STOP;
    // Cycling PE also releases a stretched clock after cancellation.
    if (reset)
      p->CR1 = p->CR1 & ~I2C_CR1_PE;
    p->ICR = 0x3F38U;
    if constexpr (serial_slave<C>())
      p->CR2 = p->CR2 & ~I2C_CR2_NACK;
    p->CR1 = p->CR1 | I2C_CR1_PE;
#else
    p->CR2 = p->CR2 & ~(I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN | I2C_CR2_DMAEN | I2C_CR2_LAST);
    if constexpr (!serial_slave<C>())
      if (p->SR2 & I2C_SR2_MSL)
        p->CR1 = p->CR1 | I2C_CR1_STOP;
    p->CR1 = (p->CR1 & ~(I2C_CR1_POS | (reset ? I2C_CR1_PE : 0U))) | I2C_CR1_ACK;
    p->SR1 = 0U;
    p->CR1 = p->CR1 | I2C_CR1_PE;
#endif
  }
  template <typename C> static status arm_dma(const bool transmit) noexcept
  {
    auto& s = state<C>();
    auto resource = serial_dma<peripheral_kind::i2c, C>(transmit);
    if (transmit)
      s.tx_dma = resource;
    else
      s.rx_dma = resource;
    const auto value = resource.start(data_address<C>(transmit),
                                      reinterpret_cast<std::uintptr_t>(transmit ? s.tx.data() : s.rx.data()),
                                      transmit ? s.tx.size() : s.rx.size(), transmit);
    if (value != status::ok)
      return value;
#if defined(I2C_CR2_NBYTES)
    device<C>()->CR1 = device<C>()->CR1 | (transmit ? I2C_CR1_TXDMAEN : I2C_CR1_RXDMAEN);
#else
    device<C>()->CR2 = (device<C>()->CR2 & ~I2C_CR2_LAST) | I2C_CR2_DMAEN |
                       (!serial_slave<C>() && !transmit && s.rx.size() > 1U ? I2C_CR2_LAST : 0U);
#endif
    return status::ok;
  }
#if defined(I2C_CR2_NBYTES)
  template <typename C> static void frame(const bool first) noexcept
  {
    auto& s = state<C>();
    auto* p = device<C>();
    const auto remaining = s.reading ? s.rx.size() - s.received : s.tx.size() - s.written;
    const auto bytes = remaining > 255U ? 255U : remaining;
    const auto address = C::address_mode == i2c_address_mode::seven_bit ? s.address << 1U : s.address;
    p->CR2 = static_cast<std::uint32_t>(address) | (C::address_mode == i2c_address_mode::ten_bit ? I2C_CR2_ADD10 : 0U) |
             (s.reading ? I2C_CR2_RD_WRN : 0U) | (static_cast<std::uint32_t>(bytes) << 16U) |
             (remaining > 255U ? I2C_CR2_RELOAD : (s.repeated && !s.reading ? 0U : I2C_CR2_AUTOEND)) |
             (first ? I2C_CR2_START : 0U);
  }
#endif
  template <typename C> static status start() noexcept
  {
    auto& s = state<C>();
    auto* p = device<C>();
    s.reading = s.tx.empty() && !s.rx.empty();
    if constexpr (serial_slave<C>())
      if (s.repeated)
        return status::unsupported;
#if defined(I2C_CR2_NBYTES)
    if constexpr (!serial_slave<C>())
      if (p->ISR & I2C_ISR_BUSY)
        return status::busy;
    p->ICR = 0x3F38U;
    p->CR1 = p->CR1 | I2C_CR1_PE | I2C_CR1_ERRIE | I2C_CR1_STOPIE | I2C_CR1_NACKIE | I2C_CR1_ADDRIE | I2C_CR1_TCIE;
    if (!s.dma)
      p->CR1 = p->CR1 | (s.reading ? I2C_CR1_RXIE : I2C_CR1_TXIE);
    if (s.dma && (!s.tx.empty() || !s.rx.empty()))
    {
      auto value = arm_dma<C>(!s.reading);
      if (value != status::ok)
      {
        stop<C>();
        return value;
      }
    }
    if constexpr (!serial_slave<C>())
      frame<C>(true);
#else
    if constexpr (!serial_slave<C>())
      if (p->SR2 & I2C_SR2_BUSY)
        return status::busy;
    p->CR1 = (p->CR1 & ~I2C_CR1_POS) | I2C_CR1_PE | I2C_CR1_ACK;
    p->CR2 = p->CR2 | I2C_CR2_ITEVTEN | I2C_CR2_ITERREN | (s.dma ? 0U : I2C_CR2_ITBUFEN);
    if (s.dma && (!s.tx.empty() || !s.rx.empty()))
    {
      const auto value = arm_dma<C>(!s.reading);
      if (value != status::ok)
      {
        stop<C>();
        return value;
      }
    }
    if constexpr (!serial_slave<C>())
      p->CR1 = p->CR1 | I2C_CR1_START;
#endif
    return status::ok;
  }
  template <typename C> static void dma_complete(const bool transmit) noexcept
  {
#if defined(I2C_CR2_NBYTES)
    device<C>()->CR1 = device<C>()->CR1 & ~(transmit ? I2C_CR1_TXDMAEN : I2C_CR1_RXDMAEN);
    if constexpr (serial_slave<C>())
      if (!transmit)
        device<C>()->CR2 = device<C>()->CR2 | I2C_CR2_NACK;
#else
    auto* p = device<C>();
    p->CR2 = p->CR2 & ~I2C_CR2_DMAEN;
    if (!transmit && !serial_slave<C>())
    {
      p->CR1 = p->CR1 | I2C_CR1_STOP;
      (void)complete<C>(status::ok);
    }
    if constexpr (serial_slave<C>())
      if (!transmit)
        p->CR1 = p->CR1 & ~I2C_CR1_ACK;
#endif
  }
  template <typename C> static void service() noexcept
  {
    auto& s = state<C>();
    auto* p = device<C>();
#if defined(I2C_CR2_NBYTES)
    const auto flags = p->ISR;
    if (flags & (I2C_ISR_BERR | I2C_ISR_ARLO | I2C_ISR_OVR))
    {
      (void)complete<C>(status::io_error);
      return;
    }
    if (flags & I2C_ISR_ADDR)
    {
      if constexpr (serial_slave<C>())
      {
        if (((flags & I2C_ISR_DIR) != 0U) == s.reading)
        {
          (void)complete<C>(status::io_error);
          return;
        }
        s.addressed = true;
      }
      p->ICR = I2C_ICR_ADDRCF;
    }
    if ((flags & I2C_ISR_NACKF) &&
        !(serial_slave<C>() && !s.reading && (s.dma ? s.tx_dma.remaining() == 0U : s.written == s.tx.size())))
    {
      (void)complete<C>(status::io_error);
      return;
    }
    if (!s.dma)
    {
      if ((flags & I2C_ISR_RXNE) && s.received < s.rx.size())
      {
        s.rx[s.received++] = static_cast<std::uint8_t>(p->RXDR);
        if constexpr (serial_slave<C>())
          if (s.received == s.rx.size())
            p->CR2 = p->CR2 | I2C_CR2_NACK;
      }
      if ((flags & I2C_ISR_TXIS) && s.written < s.tx.size())
        p->TXDR = s.tx[s.written++];
    }
    if constexpr (!serial_slave<C>())
    {
      if (flags & I2C_ISR_TCR)
      {
        // In DMA mode counters advance in hardware, including at a reload boundary.
        if (s.dma)
        {
          if (s.reading)
            s.received = s.rx.size() - s.rx_dma.remaining();
          else
            s.written = s.tx.size() - s.tx_dma.remaining();
        }
        frame<C>(false);
      }
      if ((flags & I2C_ISR_TC) && s.repeated && !s.reading)
      {
        s.reading = true;
        if (s.dma)
        {
          if (!s.tx_dma.stop())
          {
            s.result = status::io_error;
            return;
          }
          p->CR1 = p->CR1 & ~I2C_CR1_TXDMAEN;
          auto value = arm_dma<C>(false);
          if (value != status::ok)
          {
            (void)complete<C>(value);
            return;
          }
        }
        else
          p->CR1 = (p->CR1 & ~I2C_CR1_TXIE) | I2C_CR1_RXIE;
        frame<C>(true);
      }
    }
    if (flags & I2C_ISR_STOPF)
    {
      bool done = s.dma ? (s.tx.empty() || s.tx_dma.remaining() == 0U) && (s.rx.empty() || s.rx_dma.remaining() == 0U)
                        : s.written == s.tx.size() && s.received == s.rx.size();
      (void)complete<C>(done ? status::ok : status::io_error);
    }
#else
    const auto flags = p->SR1;
    if (flags & (I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_OVR))
    {
      (void)complete<C>(status::io_error);
      return;
    }
    if (flags & I2C_SR1_AF)
    {
      const bool done =
          serial_slave<C>() && !s.reading && (s.dma ? s.tx_dma.remaining() == 0U : s.written == s.tx.size());
      (void)complete<C>(done ? status::ok : status::io_error);
      return;
    }
    if (flags & I2C_SR1_SB)
    {
      if (C::address_mode == i2c_address_mode::seven_bit)
        p->DR = static_cast<std::uint32_t>((s.address << 1U) | (s.reading ? 1U : 0U));
      else
        p->DR = 0xF0U | ((s.address >> 7U) & 6U) | (s.reading && s.address_stage >= 2U ? 1U : 0U);
    }
    if (flags & I2C_SR1_ADD10)
    {
      p->DR = s.address & 0xFFU;
      s.address_stage = 1U;
    }
    if (flags & I2C_SR1_ADDR)
    {
      if constexpr (!serial_slave<C>())
      {
        if (C::address_mode == i2c_address_mode::ten_bit && s.reading && s.address_stage == 1U)
        {
          const volatile auto sr1 = p->SR1;
          const volatile auto sr2 = p->SR2;
          (void)sr1;
          (void)sr2;
          s.address_stage = 2U;
          p->CR1 = p->CR1 | I2C_CR1_START;
          return;
        }
        if (s.reading)
        {
          if (s.rx.size() == 1U)
            p->CR1 = p->CR1 & ~I2C_CR1_ACK;
          if (s.rx.size() == 2U)
            p->CR1 = (p->CR1 | I2C_CR1_POS) & ~I2C_CR1_ACK;
          if (!s.dma && (s.rx.size() == 2U || s.rx.size() == 3U))
            p->CR2 = p->CR2 & ~I2C_CR2_ITBUFEN;
        }
      }
      const volatile auto sr1 = p->SR1;
      const volatile auto sr2 = p->SR2;
      (void)sr1;
      if constexpr (serial_slave<C>())
      {
        if (((sr2 & I2C_SR2_TRA) != 0U) == s.reading)
        {
          (void)complete<C>(status::io_error);
          return;
        }
      }
      s.addressed = true;
      if constexpr (!serial_slave<C>())
      {
        if (s.reading && s.rx.size() == 1U)
          p->CR1 = p->CR1 | I2C_CR1_STOP;
        if (s.tx.empty() && s.rx.empty())
        {
          p->CR1 = p->CR1 | I2C_CR1_STOP;
          (void)complete<C>(status::ok);
          return;
        }
      }
    }
    if (s.addressed && !s.reading)
    {
      if (!s.dma && (flags & I2C_SR1_TXE) && s.written < s.tx.size())
        p->DR = s.tx[s.written++];
      if ((flags & I2C_SR1_BTF) && (s.dma ? s.tx_dma.remaining() == 0U : s.written == s.tx.size()))
      {
        if constexpr (!serial_slave<C>())
        {
          if (s.repeated)
          {
            p->CR2 = p->CR2 & ~I2C_CR2_DMAEN;
            if (s.dma)
            {
              if (!s.tx_dma.stop())
              {
                s.result = status::io_error;
                return;
              }
              const auto value = arm_dma<C>(false);
              if (value != status::ok)
              {
                (void)complete<C>(value);
                return;
              }
            }
            s.reading = true;
            s.addressed = false;
            s.address_stage = 2U;
            p->CR1 = p->CR1 | I2C_CR1_START;
          }
          else
          {
            p->CR1 = p->CR1 | I2C_CR1_STOP;
            (void)complete<C>(status::ok);
            return;
          }
        }
      }
    }
    if (s.addressed && s.reading && !s.dma)
    {
      auto remaining = s.rx.size() - s.received;
      if constexpr (serial_slave<C>())
      {
        if ((flags & I2C_SR1_RXNE) && remaining)
        {
          s.rx[s.received++] = static_cast<std::uint8_t>(p->DR);
          if (s.received == s.rx.size())
            p->CR1 = p->CR1 & ~I2C_CR1_ACK;
        }
      }
      else
      {
        if (remaining > 3U && (flags & I2C_SR1_RXNE))
        {
          s.rx[s.received++] = static_cast<std::uint8_t>(p->DR);
          if (s.rx.size() - s.received == 3U)
            p->CR2 = p->CR2 & ~I2C_CR2_ITBUFEN;
        }
        else if (remaining == 3U && (flags & I2C_SR1_BTF))
        {
          p->CR1 = p->CR1 & ~I2C_CR1_ACK;
          s.rx[s.received++] = static_cast<std::uint8_t>(p->DR);
        }
        else if (remaining == 2U && (flags & I2C_SR1_BTF))
        {
          p->CR1 = p->CR1 | I2C_CR1_STOP;
          s.rx[s.received++] = static_cast<std::uint8_t>(p->DR);
          s.rx[s.received++] = static_cast<std::uint8_t>(p->DR);
          (void)complete<C>(status::ok);
          return;
        }
        else if (remaining == 1U && (flags & I2C_SR1_RXNE))
        {
          s.rx[s.received++] = static_cast<std::uint8_t>(p->DR);
          (void)complete<C>(status::ok);
          return;
        }
      }
    }
    if constexpr (serial_slave<C>())
    {
      if (flags & I2C_SR1_STOPF)
      {
        const volatile auto sr1 = p->SR1;
        (void)sr1;
        p->CR1 = p->CR1;
        const bool done = s.dma ? (s.reading ? s.rx_dma.remaining() == 0U : s.tx_dma.remaining() == 0U)
                                : s.received == s.rx.size() && s.written == s.tx.size();
        (void)complete<C>(done ? status::ok : status::io_error);
      }
    }
    else if (s.dma && s.reading && s.rx_done && !(p->CR1 & I2C_CR1_STOP))
      (void)complete<C>(status::ok);
#endif
  }
};
#endif
} // namespace microhal::backends::stm32::ll

namespace microhal
{
#if defined(USART_CR1_UE)
template <>
struct serial_mode_api<stm32_ll_tag, peripheral_kind::uart>
    : backends::stm32::ll::native_serial_engine<peripheral_kind::uart>
{
};
#endif
#if defined(SPI_CR1_SPE)
template <>
struct serial_mode_api<stm32_ll_tag, peripheral_kind::spi>
    : backends::stm32::ll::native_serial_engine<peripheral_kind::spi>
{
};
#endif
#if defined(I2C_CR1_PE)
template <>
struct serial_mode_api<stm32_ll_tag, peripheral_kind::i2c>
    : backends::stm32::ll::native_serial_engine<peripheral_kind::i2c>
{
};
#endif
} // namespace microhal
