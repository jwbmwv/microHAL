// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "microhal/backends/stm32/adc_dma.hpp"
#include "microhal/backends/stm32/adc_engine.hpp"

namespace microhal::backends::stm32::adc_detail
{
template <typename C> constexpr unsigned poll_limit() noexcept
{
  if constexpr (requires { C::poll_limit; })
    return C::poll_limit;
  else
    return 1000000U;
}
template <typename C> constexpr auto edge() noexcept
{
  if constexpr (requires { C::trigger_edge; })
    return C::trigger_edge;
  else
    return adc_trigger_edge::software;
}
template <typename C> constexpr std::uint32_t trigger() noexcept
{
  if constexpr (requires { C::external_trigger; })
    return C::external_trigger;
  else
    return 0;
}
template <typename R> static void modify(R& reg, std::uint32_t mask, std::uint32_t value) noexcept
{
  reg = (reg & ~mask) | value;
}
template <typename C, typename Predicate> status until(Predicate predicate) noexcept
{
  for (unsigned i = 0; i < poll_limit<C>(); ++i)
    if (predicate())
      return status::ok;
  return status::timeout;
}

template <typename Registers, typename Instance> struct register_hardware
{
  template <typename C> static constexpr bool fixed_sequence() noexcept
  {
#if defined(STM32U5)
    // U5 uses one CMSIS struct for two different ADC IPs; CHSELR is reserved on ADC1/2.
    return C::instance_index == 4;
#else
    return requires(Registers* a) { a->CHSELR; };
#endif
  }
  template <typename C> static Registers* adc() noexcept
  {
    return Instance::template get<C>();
  }
  template <typename C> static status initialize() noexcept
  {
    using board = binding_t<C>;
    if constexpr (requires { board::enable_clock(); })
      board::enable_clock();
    if constexpr (requires { board::configure_pins(); })
      board::configure_pins();
    auto a = adc<C>();
    auto s = stop_adc<C>(a);
    if (s != status::ok)
      return s;
#if defined(ADC_CR_ADEN)
    if constexpr (requires { a->CR; })
    {
      if (a->CR & ADC_CR_ADEN)
      {
        a->CR = a->CR | ADC_CR_ADDIS;
        s = until<C>([=] { return !(a->CR & ADC_CR_ADEN); });
        if (s != status::ok)
          return s;
      }
#if defined(ADC_CR_DEEPPWD)
      modify(a->CR, ADC_CR_DEEPPWD, 0);
#endif
#if defined(ADC_CR_ADVREGEN)
      modify(a->CR, ADC_CR_ADVREGEN, ADC_CR_ADVREGEN & (~ADC_CR_ADVREGEN + 1U));
      // At least the regulator's startup interval, independent of an RTOS tick.
      for (unsigned i = 0; i < 10000U; ++i)
        std::atomic_signal_fence(std::memory_order_seq_cst);
#endif
#if defined(ADC_CR_ADCAL)
#if defined(ADC_CR_ADCALDIF)
      modify(a->CR, ADC_CR_ADCALDIF, C::differential_mode ? ADC_CR_ADCALDIF : 0U);
#endif
      a->CR = a->CR | ADC_CR_ADCAL;
      s = until<C>([=] { return !(a->CR & ADC_CR_ADCAL); });
      if (s != status::ok)
        return s;
      for (unsigned i = 0; i < 1000U; ++i)
        std::atomic_signal_fence(std::memory_order_seq_cst);
#endif
    }
#endif
#if defined(ADC_CR2_ADON)
    if constexpr (requires { a->CR2; })
    {
      a->CR2 = a->CR2 | ADC_CR2_ADON;
      for (unsigned i = 0; i < 1000U; ++i)
        std::atomic_signal_fence(std::memory_order_seq_cst);
#if defined(ADC_CR2_RSTCAL)
      a->CR2 = a->CR2 | ADC_CR2_RSTCAL;
      s = until<C>([=] { return !(a->CR2 & ADC_CR2_RSTCAL); });
      if (s != status::ok)
        return s;
      a->CR2 = a->CR2 | ADC_CR2_CAL;
      s = until<C>([=] { return !(a->CR2 & ADC_CR2_CAL); });
      if (s != status::ok)
        return s;
#endif
    }
#endif
    return status::ok;
  }
  template <typename C> static constexpr unsigned sampling() noexcept
  {
    static_assert(static_cast<unsigned>(C::sample_time) < 8, "microHAL: invalid ADC sampling time.");
    // Half-cycle units retain the fractional sampling times on modern ADCs.
    constexpr unsigned requested[]{6, 30, 56, 112, 168, 224, 288, 960};
#if defined(STM32F2) || defined(STM32F4) || defined(STM32F7)
    constexpr unsigned times[]{6, 30, 56, 112, 168, 224, 288, 960};
#elif defined(STM32F0) || defined(STM32F1)
    constexpr unsigned times[]{3, 15, 27, 57, 83, 111, 143, 479};
#elif defined(STM32L1)
    constexpr unsigned times[]{8, 18, 32, 48, 96, 192, 384, 768};
#elif defined(STM32C0) || defined(STM32G0) || defined(STM32L0) || defined(STM32U0)
    constexpr unsigned times[]{3, 7, 15, 25, 39, 79, 159, 321};
#elif defined(STM32H7)
#if defined(ADC3_CFGR_RES)
    if constexpr (C::instance_index == 3)
    {
      constexpr unsigned times[]{5, 13, 25, 49, 95, 185, 495, 1281};
      for (unsigned i = 0; i < 8; ++i)
        if (times[i] >= requested[static_cast<unsigned>(C::sample_time)])
          return i;
      return 8;
    }
#endif
    constexpr unsigned times[]{3, 5, 17, 33, 65, 129, 775, 1621};
#elif defined(STM32N6) || defined(STM32U3)
    constexpr unsigned times[]{3, 5, 13, 23, 47, 93, 493, 2999};
#elif defined(STM32U5)
    if constexpr (C::instance_index == 4)
    {
      constexpr unsigned times[]{3, 7, 15, 25, 39, 79, 159, 1629};
      for (unsigned i = 0; i < 8; ++i)
        if (times[i] >= requested[static_cast<unsigned>(C::sample_time)])
          return i;
      return 8;
    }
    constexpr unsigned times[]{10, 12, 24, 40, 72, 136, 782, 1628};
#elif defined(STM32F3)
    constexpr unsigned times[]{3, 5, 9, 15, 39, 123, 363, 1203};
#else
    constexpr unsigned times[]{5, 13, 25, 49, 95, 185, 495, 1281};
#endif
    for (unsigned i = 0; i < 8; ++i)
      if (times[i] >= requested[static_cast<unsigned>(C::sample_time)])
        return i;
    return 8;
  }
  template <typename C>
  static status configure(std::span<const std::uint8_t> channels, [[maybe_unused]] bool continuous,
                          [[maybe_unused]] bool dma, [[maybe_unused]] bool irq) noexcept
  {
    auto a = adc<C>();
    if constexpr (static_cast<unsigned>(edge<C>()) > 3)
      return status::invalid_argument;
    if (channels.empty())
      return status::invalid_argument;
    if constexpr (fixed_sequence<C>())
    {
      if (channels.size() > 24)
        return status::invalid_argument;
    }
    else if constexpr (requires { a->SQR5; })
    {
      if (channels.size() > 28)
        return status::invalid_argument;
    }
    else if (channels.size() > 16)
      return status::invalid_argument;
    for (auto channel : channels)
    {
#if defined(STM32F1)
      if (channel > 17)
        return status::invalid_argument;
#elif defined(STM32L1)
      if (channel > 31)
        return status::invalid_argument;
#elif defined(STM32F2) || defined(STM32F4) || defined(STM32F7) || defined(STM32F3)
      if (channel > 18)
        return status::invalid_argument;
#elif defined(STM32U5)
      if (channel > (C::instance_index == 4 ? 23 : 19))
        return status::invalid_argument;
#else
      if (channel > 19)
        return status::invalid_argument;
#endif
    }
    if constexpr (fixed_sequence<C>())
    {
      // Fixed sequencers convert in ascending channel order.
      for (std::size_t i = 1; i < channels.size(); ++i)
        if (channels[i] <= channels[i - 1])
          return status::unsupported;
    }
    if constexpr (C::differential_mode)
    {
      if constexpr (fixed_sequence<C>() || !requires { a->DIFSEL; })
        return status::unsupported;
    }
    auto s = stop_adc<C>(a);
    if (s != status::ok)
      return s;
#if defined(ADC_CR_ADEN)
    if constexpr (requires { a->CR; })
    {
      if (a->CR & ADC_CR_ADEN)
      {
        a->CR = a->CR | ADC_CR_ADDIS;
        s = until<C>([=] { return !(a->CR & ADC_CR_ADEN); });
        if (s != status::ok)
          return s;
      }
    }
#endif
    constexpr unsigned bits = static_cast<unsigned>(C::resolution);
#if defined(STM32H7)
#if defined(ADC3_CFGR_RES)
    if constexpr (C::instance_index == 3)
    {
      if constexpr (bits != 12 && bits != 10 && bits != 8 && bits != 6)
        return status::unsupported;
    }
    else
#endif
        if constexpr (bits != 16 && bits != 14 && bits != 12 && bits != 10 && bits != 8)
      return status::unsupported;
    constexpr unsigned resolution = (16 - bits) / 2;
#elif defined(STM32U5)
    if constexpr (C::instance_index == 4)
    {
      if constexpr (bits != 12 && bits != 10 && bits != 8 && bits != 6)
        return status::unsupported;
    }
    else if constexpr (bits != 14 && bits != 12 && bits != 10 && bits != 8)
      return status::unsupported;
    constexpr unsigned resolution = ((C::instance_index == 4 ? 12 : 14) - bits) / 2;
#else
    if constexpr (bits != 12 && bits != 10 && bits != 8 && bits != 6)
      return status::unsupported;
    [[maybe_unused]] constexpr unsigned resolution = (12 - bits) / 2;
#endif
#if defined(ADC_CFGR_RES)
    if constexpr (requires { a->CFGR; })
    {
#if defined(STM32H7) && defined(ADC3_CFGR_RES)
      if constexpr (C::instance_index == 3)
        modify(a->CFGR, ADC3_CFGR_RES, ((12 - bits) / 2) << ADC3_CFGR_RES_Pos);
      else
#endif
        modify(a->CFGR, ADC_CFGR_RES, resolution << ADC_CFGR_RES_Pos);
    }
#elif defined(ADC_CFGR1_RES)
    if constexpr (requires { a->CFGR1; })
      modify(a->CFGR1, ADC_CFGR1_RES, resolution << ADC_CFGR1_RES_Pos);
#elif defined(ADC_CR1_RES)
    if constexpr (requires { a->CR1; })
      modify(a->CR1, ADC_CR1_RES, resolution << ADC_CR1_RES_Pos);
#else
    if constexpr (bits != 12)
      return status::unsupported;
#endif
    std::uint32_t selected = 0;
    for (auto channel : channels)
      selected |= 1U << channel;
    if constexpr (!fixed_sequence<C>() && requires { a->DIFSEL; })
      a->DIFSEL = C::differential_mode ? selected : 0;
    if constexpr (!fixed_sequence<C>() && requires { a->PCSEL; }
#if defined(STM32H7) && defined(ADC3_CFGR_RES)
                  && C::instance_index != 3
#endif
    )
      a->PCSEL = selected;
    if constexpr (fixed_sequence<C>())
    {
#if defined(ADC_CFGR1_CHSELRMOD)
      modify(a->CFGR1, ADC_CFGR1_CHSELRMOD, 0);
#endif
#if defined(ADC_CFGR1_SCANDIR)
      modify(a->CFGR1, ADC_CFGR1_SCANDIR, 0);
#endif
#if defined(STM32U5)
      modify(a->CFGR1, ADC4_CFGR1_SCANDIR, 0);
#endif
      a->CHSELR = selected;
    }
    else if constexpr (requires { a->SQR5; })
    {
      a->SQR1 = static_cast<std::uint32_t>(channels.size() - 1) << 20;
      a->SQR2 = 0;
      a->SQR3 = 0;
      a->SQR4 = 0;
      a->SQR5 = 0;
      for (unsigned i = 0; i < channels.size(); ++i)
      {
        if (i < 6)
          a->SQR5 = a->SQR5 | (channels[i] << (5U * i));
        else if (i < 12)
          a->SQR4 = a->SQR4 | (channels[i] << (5U * (i - 6)));
        else if (i < 18)
          a->SQR3 = a->SQR3 | (channels[i] << (5U * (i - 12)));
        else if (i < 24)
          a->SQR2 = a->SQR2 | (channels[i] << (5U * (i - 18)));
        else
          a->SQR1 = a->SQR1 | (channels[i] << (5U * (i - 24)));
      }
    }
    else if constexpr (requires {
                         a->SQR1;
                         a->SQR2;
                         a->SQR3;
                       })
    {
#if defined(ADC_SQR1_SQ1_Pos)
      a->SQR1 = static_cast<std::uint32_t>(channels.size() - 1);
      a->SQR2 = 0;
      a->SQR3 = 0;
      if constexpr (requires { a->SQR4; })
        a->SQR4 = 0;
      for (unsigned i = 0; i < channels.size(); ++i)
      {
        if (i < 4)
          a->SQR1 = a->SQR1 | (channels[i] << (6U * (i + 1)));
        else if (i < 9)
          a->SQR2 = a->SQR2 | (channels[i] << (6U * (i - 4)));
        else if (i < 14)
          a->SQR3 = a->SQR3 | (channels[i] << (6U * (i - 9)));
        else if constexpr (requires { a->SQR4; })
          a->SQR4 = a->SQR4 | (channels[i] << (6U * (i - 14)));
      }
#else
      a->SQR1 = static_cast<std::uint32_t>(channels.size() - 1) << 20;
      a->SQR2 = 0;
      a->SQR3 = 0;
      for (unsigned i = 0; i < channels.size(); ++i)
      {
        if (i < 6)
          a->SQR3 = a->SQR3 | (channels[i] << (5U * i));
        else if (i < 12)
          a->SQR2 = a->SQR2 | (channels[i] << (5U * (i - 6)));
        else
          a->SQR1 = a->SQR1 | (channels[i] << (5U * (i - 12)));
      }
#endif
    }
    else
      return status::unsupported;
    constexpr auto sample_time = sampling<C>();
    if constexpr (sample_time > 7)
      return status::unsupported;
#if defined(STM32U5)
    if constexpr (C::instance_index == 4)
    {
      a->SMPR1 = sample_time;
    }
    else
#endif
        if constexpr (requires { a->SMPR; })
    {
      modify(a->SMPR, 7U, sample_time);
#if defined(ADC_SMPR_SMPSEL0)
      // Select common sampling time 1 for every channel.
      modify(a->SMPR, 0x7FFFFU << 8, 0);
#endif
    }
    else if constexpr (requires { a->SMPR3; })
    {
      for (auto channel : channels)
      {
        if (channel < 10)
          modify(a->SMPR3, 7U << (3 * channel), sample_time << (3 * channel));
        else if (channel < 20)
          modify(a->SMPR2, 7U << (3 * (channel - 10)), sample_time << (3 * (channel - 10)));
        else if (channel < 30)
          modify(a->SMPR1, 7U << (3 * (channel - 20)), sample_time << (3 * (channel - 20)));
        else if constexpr (requires { a->SMPR0; })
          modify(a->SMPR0, 7U << (3 * (channel - 30)), sample_time << (3 * (channel - 30)));
        else
          return status::unsupported;
      }
    }
    else if constexpr (requires {
                         a->SMPR1;
                         a->SMPR2;
                       })
    {
      for (auto channel : channels)
      {
#if defined(ADC_SMPR1_SMP0)
        if (channel < 10)
          modify(a->SMPR1, 7U << (3 * channel), sample_time << (3 * channel));
        else
          modify(a->SMPR2, 7U << (3 * (channel - 10)), sample_time << (3 * (channel - 10)));
#else
        if (channel < 10)
          modify(a->SMPR2, 7U << (3 * channel), sample_time << (3 * channel));
        else
          modify(a->SMPR1, 7U << (3 * (channel - 10)), sample_time << (3 * (channel - 10)));
#endif
      }
    }
#if defined(ADC_CFGR_CONT)
    if constexpr (requires { a->CFGR; })
    {
#if defined(ADC_CFGR_ALIGN)
      modify(a->CFGR, ADC_CFGR_ALIGN, 0);
#endif
#if defined(ADC_CFGR_DISCEN)
      modify(a->CFGR, ADC_CFGR_DISCEN, 0);
#endif
      if (trigger<C>() > (ADC_CFGR_EXTSEL >> ADC_CFGR_EXTSEL_Pos))
        return status::invalid_argument;
      modify(a->CFGR, ADC_CFGR_CONT, continuous ? ADC_CFGR_CONT : 0);
      modify(a->CFGR, ADC_CFGR_EXTSEL | ADC_CFGR_EXTEN,
             (trigger<C>() << ADC_CFGR_EXTSEL_Pos) | (static_cast<unsigned>(edge<C>()) << ADC_CFGR_EXTEN_Pos));
#if defined(ADC_CFGR_DMNGT)
      modify(a->CFGR, ADC_CFGR_DMNGT, dma ? 3U : 0U);
#elif defined(ADC_CFGR_DMAEN)
      modify(a->CFGR, ADC_CFGR_DMAEN | ADC_CFGR_DMACFG, dma ? (ADC_CFGR_DMAEN | ADC_CFGR_DMACFG) : 0U);
#endif
    }
#elif defined(ADC_CFGR1_CONT)
    if constexpr (requires { a->CFGR1; })
    {
#if defined(ADC_CFGR1_ALIGN)
      modify(a->CFGR1, ADC_CFGR1_ALIGN, 0);
#endif
#if defined(ADC_CFGR1_DISCEN)
      modify(a->CFGR1, ADC_CFGR1_DISCEN, 0);
#endif
      constexpr auto trigger_mask =
#if defined(STM32U5)
          C::instance_index == 4 ? ADC4_CFGR1_EXTSEL :
#endif
                                 ADC_CFGR1_EXTSEL;
      constexpr auto trigger_shift =
#if defined(STM32U5)
          C::instance_index == 4 ? ADC4_CFGR1_EXTSEL_Pos :
#endif
                                 ADC_CFGR1_EXTSEL_Pos;
      if (trigger<C>() > (trigger_mask >> trigger_shift))
        return status::invalid_argument;
      modify(a->CFGR1, ADC_CFGR1_CONT, continuous ? ADC_CFGR1_CONT : 0);
      modify(a->CFGR1, trigger_mask | ADC_CFGR1_EXTEN,
             (trigger<C>() << trigger_shift) | (static_cast<unsigned>(edge<C>()) << ADC_CFGR1_EXTEN_Pos));
#if defined(ADC_CFGR1_DMNGT)
      modify(a->CFGR1, ADC_CFGR1_DMNGT, dma ? 3U : 0U);
#elif defined(ADC_CFGR1_DMAEN)
      modify(a->CFGR1, ADC_CFGR1_DMAEN | ADC_CFGR1_DMACFG, dma ? (ADC_CFGR1_DMAEN | ADC_CFGR1_DMACFG) : 0U);
#endif
#if defined(STM32U5)
      if constexpr (C::instance_index == 4)
        modify(a->CFGR1, ADC4_CFGR1_ALIGN, 0);
#endif
    }
#elif defined(ADC_CR2_CONT)
    if constexpr (requires { a->CR2; })
    {
#if defined(ADC_CR2_ALIGN)
      modify(a->CR2, ADC_CR2_ALIGN, 0);
#endif
#if defined(ADC_CR1_DISCEN)
      modify(a->CR1, ADC_CR1_DISCEN, 0);
#endif
      if (trigger<C>() > (ADC_CR2_EXTSEL >> ADC_CR2_EXTSEL_Pos))
        return status::invalid_argument;
      modify(a->CR2, ADC_CR2_CONT | ADC_CR2_DMA, (continuous ? ADC_CR2_CONT : 0U) | (dma ? ADC_CR2_DMA : 0U));
#if defined(ADC_CR2_DDS)
      modify(a->CR2, ADC_CR2_DDS, dma ? ADC_CR2_DDS : 0U);
#endif
#if defined(ADC_CR1_SCAN)
      modify(a->CR1, ADC_CR1_SCAN, channels.size() > 1 ? ADC_CR1_SCAN : 0U);
#endif
#if defined(ADC_CR2_EXTEN)
      modify(a->CR2, ADC_CR2_EXTSEL | ADC_CR2_EXTEN,
             (trigger<C>() << ADC_CR2_EXTSEL_Pos) | (static_cast<unsigned>(edge<C>()) << ADC_CR2_EXTEN_Pos));
#elif defined(ADC_CR2_EXTTRIG)
      if constexpr (edge<C>() == adc_trigger_edge::falling || edge<C>() == adc_trigger_edge::both)
        return status::unsupported;
      modify(a->CR2, ADC_CR2_EXTSEL | ADC_CR2_EXTTRIG,
             (edge<C>() == adc_trigger_edge::software ? ADC_CR2_EXTSEL : trigger<C>() << ADC_CR2_EXTSEL_Pos) |
                 ADC_CR2_EXTTRIG);
#endif
    }
#endif
#if defined(ADC_IER_EOCIE)
    if constexpr (requires { a->IER; })
    {
      a->IER = (irq ? ADC_IER_EOCIE : 0U) | ((irq || dma) ? ADC_IER_OVRIE : 0U);
      a->ISR = ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR;
    }
#elif defined(ADC_CR1_EOCIE)
    if constexpr (requires { a->CR1; })
    {
      modify(a->CR1, ADC_CR1_EOCIE, irq ? ADC_CR1_EOCIE : 0U);
#if defined(ADC_CR1_OVRIE)
      modify(a->CR1, ADC_CR1_OVRIE, irq || dma ? ADC_CR1_OVRIE : 0U);
#endif
      a->SR = 0;
    }
#endif
    return status::ok;
  }
  template <typename C> static status start() noexcept
  {
    [[maybe_unused]] auto a = adc<C>();
#if defined(ADC_CR_ADEN)
    if constexpr (requires { a->CR; })
    {
      a->ISR = ADC_ISR_ADRDY;
      a->CR = a->CR | ADC_CR_ADEN;
      auto s = until<C>([=] { return (a->ISR & ADC_ISR_ADRDY) != 0; });
      if (s != status::ok)
        return s;
      a->CR = a->CR | ADC_CR_ADSTART;
      return status::ok;
    }
#endif
#if defined(ADC_CR2_ADON)
    if constexpr (requires { a->CR2; })
    {
      a->CR2 = a->CR2 | ADC_CR2_ADON;
      for (unsigned i = 0; i < 1000U; ++i)
        std::atomic_signal_fence(std::memory_order_seq_cst);
#if defined(ADC_CR2_SWSTART)
      if constexpr (edge<C>() == adc_trigger_edge::software)
        a->CR2 = a->CR2 | ADC_CR2_SWSTART;
#else
      if constexpr (edge<C>() == adc_trigger_edge::software)
        a->CR2 = a->CR2 | ADC_CR2_ADON;
#endif
      return status::ok;
    }
#endif
    return status::unsupported;
  }
  template <typename C> static bool ready() noexcept
  {
    auto a = adc<C>();
    if constexpr (requires { a->ISR; })
      return (a->ISR & (1U << 2)) != 0;
    else
      return (a->SR & (1U << 1)) != 0;
  }
  template <typename C> static bool error() noexcept
  {
    [[maybe_unused]] auto a = adc<C>();
#if defined(ADC_ISR_OVR)
    if constexpr (requires { a->ISR; })
      return (a->ISR & ADC_ISR_OVR) != 0;
#endif
#if defined(ADC_SR_OVR)
    if constexpr (requires { a->SR; })
      return (a->SR & ADC_SR_OVR) != 0;
#endif
    return false;
  }
  template <typename C> static std::uint16_t sample() noexcept
  {
    return static_cast<std::uint16_t>(adc<C>()->DR);
  }
  template <typename C> static status wait() noexcept
  {
    const auto s = until<C>([] { return ready<C>() || error<C>(); });
    return s != status::ok ? s : (error<C>() ? status::io_error : status::ok);
  }
  template <typename C> static status stop_adc([[maybe_unused]] Registers* a) noexcept
  {
#if defined(ADC_CR_ADSTP)
    if constexpr (requires { a->CR; })
    {
      if (a->CR & ADC_CR_ADSTART)
      {
        a->CR = a->CR | ADC_CR_ADSTP;
        const auto s = until<C>([=] { return !(a->CR & ADC_CR_ADSTART); });
        if (s != status::ok)
          return s;
      }
    }
#endif
#if defined(ADC_CR2_ADON)
    if constexpr (requires { a->CR2; })
      modify(a->CR2, ADC_CR2_ADON, 0);
#endif
    return status::ok;
  }
  template <typename C> static status stop(bool dma) noexcept
  {
    auto a = adc<C>();
    auto s = stop_adc<C>(a);
    if (dma)
    {
      const auto ds = dma_registers<C>::stop();
      if (s == status::ok)
        s = ds;
    }
#if defined(ADC_IER_EOCIE)
    if constexpr (requires { a->IER; })
    {
      a->IER = 0;
      a->ISR = ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR;
    }
#elif defined(ADC_CR1_EOCIE)
    if constexpr (requires { a->CR1; })
    {
      modify(a->CR1, ADC_CR1_EOCIE, 0);
#if defined(ADC_CR1_OVRIE)
      modify(a->CR1, ADC_CR1_OVRIE, 0);
#endif
      a->SR = 0;
    }
#endif
    return s;
  }
  template <typename C> static status setup_dma(std::span<std::uint16_t> buffer, bool circular, bool halves) noexcept
  {
    return dma_registers<C>::setup(reinterpret_cast<std::uintptr_t>(&adc<C>()->DR), buffer, circular, halves);
  }
  template <typename C> static unsigned dma_flags() noexcept
  {
    return dma_registers<C>::flags();
  }
};
} // namespace microhal::backends::stm32::adc_detail
