// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "microhal/backends/stm32/adc_engine.hpp"
#include "microhal/backends/stm32/dma_resource.hpp"

namespace microhal::backends::stm32
{
/// Optional board specialization for an unmodified adc_config.
template <std::uint8_t Instance> struct adc_binding
{
};
namespace adc_detail
{
template <typename C, typename = void> struct binding_for
{
  using type = adc_binding<C::instance_index>;
};
template <typename C> struct binding_for<C, std::void_t<typename C::binding>>
{
  using type = typename C::binding;
};
template <typename C> using binding_t = typename binding_for<C>::type;

/// Bind typed register resources; the shared native endpoint implements transfers.
template <unsigned Instance> struct dma_resource_state
{
  inline static detail::serial_gpdma_node node{};
  inline static detail::serial_dma_resource endpoint{};
};

template <typename C> struct dma_registers
{
  using board = binding_t<C>;
  static_assert(C::instance_index > 0 && C::instance_index < 6);
  using state = dma_resource_state<C::instance_index>;
  static constexpr bool available = requires {
    board::dma_channel();
    board::dma_controller();
    board::dma_index;
  };

  template <typename D, typename Controller> static detail::serial_dma_resource resource(D* d, Controller* c) noexcept
  {
    if constexpr (requires {
                    d->CTR1;
                    d->CLLR;
                  })
      return detail::gpdma(*d, &state::node);
    else if constexpr (requires {
                         d->NDTR;
                         c->LISR;
                       })
    {
      constexpr unsigned index = board::dma_index;
      static_assert(index < 8);
      constexpr unsigned shifts[]{0, 6, 16, 22};
      if constexpr (index < 4)
        return detail::stream_dma(*d, c->LISR, c->LIFCR, shifts[index % 4]);
      else
        return detail::stream_dma(*d, c->HISR, c->HIFCR, shifts[index % 4]);
    }
    else
      return detail::channel_dma(*d, *c, board::dma_index);
  }
  static unsigned flags() noexcept
  {
    if (!state::endpoint)
      return 0;
    const auto flags = *state::endpoint.flags;
    const auto relevant = state::endpoint.complete_mask | state::endpoint.half_mask | state::endpoint.error_mask;
    *state::endpoint.clear = flags & relevant;
    return ((flags & state::endpoint.half_mask) ? 1U : 0U) | ((flags & state::endpoint.complete_mask) ? 2U : 0U) |
           ((flags & state::endpoint.error_mask) ? 4U : 0U);
  }
  static status stop() noexcept
  {
    if constexpr (!available)
      return status::unsupported;
    else
    {
      const auto s = state::endpoint.stop() ? status::ok : status::timeout;
      (void)flags();
      return s;
    }
  }
  static status setup(std::uintptr_t source, std::span<std::uint16_t> buffer, bool circular,
                      bool halves = false) noexcept
  {
    if constexpr (!available)
      return status::unsupported;
    else
    {
      auto d = board::dma_channel();
      state::endpoint = resource(d, board::dma_controller());
      if (!state::endpoint.stop())
        return status::busy;
      if constexpr (requires { d->CTR2; })
      {
        if constexpr (requires { board::dma_request; })
          d->CTR2 = board::dma_request;
        else
          return status::unsupported;
      }
      else if constexpr (requires { d->NDTR; })
      {
        if constexpr (requires { board::dma_request; })
          *state::endpoint.control = (*state::endpoint.control & ~(7U << 25)) | (board::dma_request << 25);
      }
      if constexpr (requires {
                      board::dmamux_channel();
                      board::dma_request;
                    })
        board::dmamux_channel()->CCR = board::dma_request;
#if defined(DMAMUX_CxCR_DMAREQ_ID)
      else if constexpr (!requires { d->CTR2; } && !requires { d->NDTR; })
      {
        if constexpr (!requires { d->NDTR; })
          return status::unsupported;
      }
#endif
      if constexpr (requires {
                      board::dma_request_register();
                      board::dma_request;
                    })
      {
        constexpr unsigned shift = (board::dma_index - 1) * 4;
        auto reg = board::dma_request_register();
        *reg = (*reg & ~(15U << shift)) | (board::dma_request << shift);
      }
      auto s = state::endpoint.start(source, reinterpret_cast<std::uintptr_t>(buffer.data()), buffer.size(), false,
                                     circular, true, 2);
      if (s == status::ok && halves)
      {
        const auto half_irq = state::endpoint.layout == detail::serial_dma_layout::gpdma    ? 1U << 9
                              : state::endpoint.layout == detail::serial_dma_layout::stream ? 1U << 3
                                                                                            : 1U << 2;
        *state::endpoint.control = *state::endpoint.control | half_irq;
      }
      return s;
    }
  }
};
} // namespace adc_detail
} // namespace microhal::backends::stm32
