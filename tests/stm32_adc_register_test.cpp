// SPDX-License-Identifier: Apache-2.0
// Register-model coverage deliberately exercises native ADC/DMA programming without Cube.
#include <cstdint>
#if defined(MICROHAL_TEST_ADC_MODERN)
#define STM32G4
#define ADC_CFGR_RES (3U << 3)
#define ADC_CFGR_RES_Pos 3
#define ADC_CFGR_CONT (1U << 13)
#define ADC_CFGR_EXTSEL (15U << 6)
#define ADC_CFGR_EXTSEL_Pos 6
#define ADC_CFGR_EXTEN (3U << 10)
#define ADC_CFGR_EXTEN_Pos 10
#define ADC_CFGR_DMAEN 1U
#define ADC_CFGR_DMACFG 2U
#define ADC_SQR1_SQ1_Pos 6
#define ADC_SMPR1_SMP0 7U
#define ADC_IER_EOCIE 4U
#define ADC_IER_OVRIE 16U
#define ADC_ISR_EOC 4U
#define ADC_ISR_EOS 8U
#define ADC_ISR_OVR 16U
#else
#define STM32F4
#define ADC_CR1_RES (3U << 24)
#define ADC_CR1_RES_Pos 24
#define ADC_CR1_SCAN (1U << 8)
#define ADC_CR1_EOCIE (1U << 5)
#define ADC_CR1_OVRIE (1U << 26)
#define ADC_CR2_CONT 2U
#define ADC_CR2_ADON 1U
#define ADC_CR2_DMA (1U << 8)
#define ADC_CR2_DDS (1U << 9)
#define ADC_CR2_EXTSEL (15U << 24)
#define ADC_CR2_EXTSEL_Pos 24
#define ADC_CR2_EXTEN (3U << 28)
#define ADC_CR2_EXTEN_Pos 28
#define ADC_CR2_SWSTART (1U << 30)
#define ADC_SR_OVR (1U << 5)
#endif
#include "microhal/backends/stm32/adc_registers.hpp"
#include <cassert>

namespace
{
using namespace microhal;
struct registers
{
  volatile std::uint32_t SR{}, CR1{}, CR2{}, CFGR{}, ISR{}, IER{}, DR{};
  volatile std::uint32_t SMPR1{}, SMPR2{}, SQR1{}, SQR2{}, SQR3{}, SQR4{};
};
struct stream_regs
{
  volatile std::uint32_t CR{}, NDTR{}, PAR{}, M0AR{}, FCR{};
};
struct dma_regs
{
  volatile std::uint32_t LISR{}, HISR{}, LIFCR{}, HIFCR{};
};
registers adc{};
stream_regs stream{};
dma_regs dma{};
struct instance
{
  template <typename C> static registers* get()
  {
    return &adc;
  }
};
struct binding
{
  static auto dma_channel()
  {
    return &stream;
  }
  static auto dma_controller()
  {
    return &dma;
  }
  static constexpr unsigned dma_index = 0, dma_request = 2;
};
struct config
{
  using binding = ::binding;
  [[maybe_unused]] static constexpr unsigned instance_index = 1;
  static constexpr auto resolution = adc_resolution::bits_10;
  static constexpr auto sample_time = adc_sample_time::cycles_84;
  static constexpr bool differential_mode = false;
  static constexpr auto trigger_edge = adc_trigger_edge::rising;
  static constexpr unsigned external_trigger = 3;
};
struct alias_config : config
{
};
using hw = backends::stm32::adc_detail::register_hardware<registers, instance>;
using dm = backends::stm32::adc_detail::dma_registers<config>;
using alias_dm = backends::stm32::adc_detail::dma_registers<alias_config>;
struct channel_regs
{
  volatile std::uint32_t CCR{}, CNDTR{}, CPAR{}, CMAR{};
};
struct channel_controller
{
  volatile std::uint32_t ISR{}, IFCR{};
};
channel_regs channel{};
channel_controller controller{};
struct channel_binding
{
  static auto dma_channel()
  {
    return &channel;
  }
  static auto dma_controller()
  {
    return &controller;
  }
  static constexpr unsigned dma_index = 2;
};
struct channel_config : config
{
  using binding = channel_binding;
};
struct gpdma_regs
{
  volatile std::uint32_t CCR{}, CBR1{}, CSAR{}, CDAR{}, CSR{}, CFCR{}, CTR1{}, CTR2{}, CLBAR{}, CLLR{};
};
gpdma_regs gp{};
struct gpdma_binding
{
  static auto dma_channel()
  {
    return &gp;
  }
  static auto dma_controller()
  {
    return &controller;
  }
  static constexpr unsigned dma_index = 0, dma_request = 7;
};
struct gpdma_config : config
{
  using binding = gpdma_binding;
};
} // namespace
int main()
{
  const std::uint8_t channels[]{1, 4, 8};
  assert(hw::configure<config>(channels, false, true, true) == status::ok);
#if defined(MICROHAL_TEST_ADC_MODERN)
  assert(adc.SQR1 == 2U + (1U << 6) + (4U << 12) + (8U << 18));
  assert(((adc.SMPR1 >> 3) & 7U) == 5U);
  assert(((adc.CFGR >> 3) & 3U) == 1U);
  assert(((adc.CFGR >> 6) & 15U) == 3U && ((adc.CFGR >> 10) & 3U) == 1U);
  assert((adc.CFGR & 3U) == 3U && !(adc.CFGR & ADC_CFGR_CONT));
  assert(adc.IER == (ADC_IER_EOCIE | ADC_IER_OVRIE));
#else
  assert(adc.SQR1 == (2U << 20) && adc.SQR3 == 1U + (4U << 5) + (8U << 10));
  assert(((adc.SMPR2 >> 3) & 7U) == 4U);
  assert(((adc.CR1 >> 24) & 3U) == 1U && (adc.CR1 & ADC_CR1_SCAN));
  assert(((adc.CR2 >> 24) & 15U) == 3U && ((adc.CR2 >> 28) & 3U) == 1U);
  assert(adc.CR2 & ADC_CR2_DMA);
#endif
  assert(hw::configure<config>(channels, true, false, false) == status::ok);
#if defined(MICROHAL_TEST_ADC_MODERN)
  assert(adc.CFGR & ADC_CFGR_CONT);
#else
  assert(adc.CR2 & ADC_CR2_CONT);
#endif
  std::uint16_t buffer[8]{};
  assert(dm::setup(reinterpret_cast<std::uintptr_t>(&adc.DR), buffer, true) == status::ok);
  assert(stream.NDTR == 8);
  assert(stream.PAR == static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&adc.DR)));
  assert(stream.M0AR == static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(buffer)));
  assert((stream.CR & ((3U << 11) | (3U << 13))) == ((1U << 11) | (1U << 13)));
  assert((stream.CR & (1U << 8)) && (stream.CR & 1U));
  dma.LISR = 0x30U;
  assert(alias_dm::flags() == 3 && dma.LIFCR == 0x30U);
  dma.LISR = 8U;
  assert(alias_dm::flags() == 4);
  assert(alias_dm::stop() == status::ok && !(stream.CR & 1U));
  assert(dm::setup(reinterpret_cast<std::uintptr_t>(&adc.DR), buffer, false, true) == status::ok);
  assert((stream.CR & (1U << 3)) && !(stream.CR & (1U << 8)));
  assert(dm::stop() == status::ok);
  using channel_dma = backends::stm32::adc_detail::dma_registers<channel_config>;
  const auto source = reinterpret_cast<std::uintptr_t>(&adc.DR);
  assert(channel_dma::setup(source, buffer, true) == status::ok);
  assert(channel.CNDTR == 8 && (channel.CCR & (1U << 5)));
  assert((channel.CCR & ((3U << 8) | (3U << 10))) == ((1U << 8) | (1U << 10)));
  assert(channel.CMAR == static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(buffer)));
  controller.ISR = 6U << 4;
  assert(channel_dma::flags() == 3 && controller.IFCR == (6U << 4));
  assert(channel_dma::stop() == status::ok);
  using gp_dma = backends::stm32::adc_detail::dma_registers<gpdma_config>;
  assert(gp_dma::setup(source, buffer, true) == status::ok);
  assert(gp.CBR1 == 16 && (gp.CTR1 & 0x30003U) == 0x10001U && gp.CTR2 == 7U);
  assert(gp.CSAR == static_cast<std::uint32_t>(source));
  assert(gp.CDAR == static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(buffer)));
  assert(gp.CLLR != 0 && gp_dma::node.count == 16 && gp_dma::node.link == gp.CLLR);
  gp.CSR = (1U << 8) | (1U << 9);
  assert(gp_dma::flags() == 3 && gp.CFCR == gp.CSR);
  // Model the hardware's completed reset before checking idempotent cleanup.
  gp.CCR = 0;
  assert(gp_dma::stop() == status::ok);
}
