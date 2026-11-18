// SPDX-License-Identifier: Apache-2.0
#if defined(STM32C0) || defined(STM32F0) || defined(STM32F1) || defined(STM32F2) || defined(STM32F3) ||                \
    defined(STM32F4) || defined(STM32F7) || defined(STM32G0) || defined(STM32G4) || defined(STM32H5) ||                \
    defined(STM32H7) || defined(STM32H7RS) || defined(STM32L0) || defined(STM32L1) || defined(STM32L4) ||              \
    defined(STM32L5) || defined(STM32N6) || defined(STM32U0) || defined(STM32U3) || defined(STM32U5) ||                \
    defined(STM32WB)
#if defined(MICROHAL_ADC_REGISTERS_ONLY)
#if defined(STM32C0)
#include "stm32c0xx.h"
#elif defined(STM32F0)
#include "stm32f0xx.h"
#elif defined(STM32F1)
#include "stm32f1xx.h"
#elif defined(STM32F2)
#include "stm32f2xx.h"
#elif defined(STM32F3)
#include "stm32f3xx.h"
#elif defined(STM32F4)
#include "stm32f4xx.h"
#elif defined(STM32F7)
#include "stm32f7xx.h"
#elif defined(STM32G0)
#include "stm32g0xx.h"
#elif defined(STM32G4)
#include "stm32g4xx.h"
#elif defined(STM32H5)
#include "stm32h5xx.h"
#elif defined(STM32H7)
#include "stm32h7xx.h"
#elif defined(STM32H7RS)
#include "stm32h7rsxx.h"
#elif defined(STM32L0)
#include "stm32l0xx.h"
#elif defined(STM32L1)
#include "stm32l1xx.h"
#elif defined(STM32L4)
#include "stm32l4xx.h"
#elif defined(STM32L5)
#include "stm32l5xx.h"
#elif defined(STM32N6)
#include "stm32n6xx.h"
#elif defined(STM32U0)
#include "stm32u0xx.h"
#elif defined(STM32U3)
#include "stm32u3xx.h"
#elif defined(STM32U5)
#include "stm32u5xx.h"
#elif defined(STM32WB)
#include "stm32wbxx.h"
#endif
#include "microhal/backends/stm32/adc_registers.hpp"
#else
#include "microhal/adc/adc.hpp"
#include "microhal/core/backend_selector.hpp"
#endif

#if defined(ADC1)
namespace
{
void sample_done(microhal::status, std::uint16_t) noexcept {}
void scan_done(microhal::status) noexcept {}
void block_done(microhal::status, std::span<std::uint16_t>) noexcept {}
struct resources
{
  static auto dma_channel() noexcept
  {
#if defined(GPDMA1_Channel0)
    return GPDMA1_Channel0;
#elif defined(DMA1_Stream0)
    return DMA1_Stream0;
#else
    return DMA1_Channel1;
#endif
  }
  static auto dma_controller() noexcept
  {
#if defined(GPDMA1)
    return GPDMA1;
#else
    return DMA1;
#endif
  }
  static constexpr unsigned dma_index =
#if defined(GPDMA1_Channel0) || defined(DMA1_Stream0)
      0;
#else
      1;
#endif
  static constexpr unsigned dma_request = 0;
#if defined(DMAMUX1_Channel0)
  static auto dmamux_channel() noexcept
  {
    return DMAMUX1_Channel0;
  }
#endif
};
struct config
{
  using binding = resources;
  static constexpr unsigned instance_index = 1;
  static constexpr auto resolution = microhal::adc_resolution::bits_12;
  static constexpr auto sample_time = microhal::adc_sample_time::cycles_84;
  static constexpr bool enable_dma = true, enable_interrupts = true, differential_mode = false;
  static constexpr unsigned external_trigger = 0;
  static constexpr auto trigger_edge = microhal::adc_trigger_edge::rising;
};
template <unsigned Index> struct instance_config : config
{
  static constexpr unsigned instance_index = Index;
};
template <typename Api, typename C = config> void exercise_raw()
{
  const std::uint8_t channels[]{1, 2};
  std::uint16_t samples[8]{};
  (void)Api::template initialize<C>();
  (void)Api::template read_blocking<C>(1);
  (void)Api::template start_continuous<C>(channels);
  (void)Api::template start_async<C, sample_done>(1);
  (void)Api::template start_dma_scan<C, scan_done>(channels, samples);
  (void)Api::template start_dma_stream<C, block_done>(channels, samples);
  (void)Api::template start_dma_stream<C, block_done>(channels, samples, false);
  Api::template handle_interrupt<C>();
  Api::template handle_dma_interrupt<C>();
  (void)Api::template transfer_status<C>();
  (void)Api::template stop<C>();
  (void)Api::template abort<C>();
}
#if defined(MICROHAL_ADC_REGISTERS_ONLY)
struct instance
{
  template <typename C> static ADC_TypeDef* get() noexcept
  {
    return ADC1;
  }
};
using hardware = microhal::backends::stm32::adc_detail::register_hardware<ADC_TypeDef, instance>;
using native = microhal::backends::stm32::adc_detail::engine<hardware>;
#if defined(STM32U5)
static_assert(!hardware::fixed_sequence<config>());
static_assert(hardware::fixed_sequence<instance_config<4>>());
static_assert(hardware::sampling<config>() == 6);
static_assert(hardware::sampling<instance_config<4>>() == 7);
#endif
#if defined(STM32H7) && defined(ADC3_CFGR_RES)
static_assert(hardware::sampling<instance_config<3>>() == 5);
#endif
#else
template <typename Backend, typename C = config> void exercise()
{
  using adc = microhal::adc::converter<Backend, C>;
  const std::uint8_t channels[]{1, 2};
  std::uint16_t samples[8]{};
  (void)adc::initialize();
  (void)adc::read_blocking(1);
  (void)adc::start_continuous(channels);
  (void)adc::template start_async<sample_done>(1);
  (void)adc::template start_dma_scan<scan_done>(channels, samples);
  (void)adc::template start_dma_stream<block_done>(channels, samples);
  (void)adc::template start_dma_stream<block_done>(channels, samples, false);
  adc::handle_interrupt();
  adc::handle_dma_interrupt();
  (void)adc::transfer_status();
  (void)adc::stop();
  (void)adc::abort();
}
#endif
} // namespace
void microhal_stm32_adc_instantiation()
{
#if defined(MICROHAL_ADC_REGISTERS_ONLY)
  exercise_raw<native>();
#if defined(ADC2)
  exercise_raw<native, instance_config<2>>();
#endif
#if defined(ADC3)
  exercise_raw<native, instance_config<3>>();
#endif
#if defined(ADC4)
  exercise_raw<native, instance_config<4>>();
#endif
#if defined(ADC5)
  exercise_raw<native, instance_config<5>>();
#endif
#else
#if MICROHAL_ENABLE_STM32_HAL
  exercise<microhal::stm32_hal_tag>();
#if defined(ADC4)
  exercise<microhal::stm32_hal_tag, instance_config<4>>();
#endif
#endif
#if MICROHAL_ENABLE_STM32_LL
  exercise<microhal::stm32_ll_tag>();
#if defined(ADC4)
  exercise<microhal::stm32_ll_tag, instance_config<4>>();
#endif
#endif
#endif
}
#endif
#endif
