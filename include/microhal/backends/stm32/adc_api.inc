// SPDX-License-Identifier: Apache-2.0
// Included inside a backend namespace, after vendor peripheral declarations.
struct adc_instance
{
#if defined(ADC1)
  template <typename C> static ADC_TypeDef* get() noexcept
  {
    using board = microhal::backends::stm32::adc_detail::binding_t<C>;
    if constexpr (requires { C::base_address; })
      return reinterpret_cast<ADC_TypeDef*>(C::base_address);
    else if constexpr (requires { board::base_address; })
      return reinterpret_cast<ADC_TypeDef*>(board::base_address);
    else if constexpr (C::instance_index == 1)
      return ADC1;
#if defined(ADC2)
    else if constexpr (C::instance_index == 2)
      return ADC2;
#endif
#if defined(ADC3)
    else if constexpr (C::instance_index == 3)
      return ADC3;
#endif
#if defined(ADC4)
    else if constexpr (C::instance_index == 4)
      return ADC4;
#endif
#if defined(ADC5)
    else if constexpr (C::instance_index == 5)
      return ADC5;
#endif
    else
    {
      static_assert(dependent_false_v<C>, "microHAL: ADC instance is unavailable.");
    }
  }
#endif
};
#if defined(ADC1)
struct adc_api final : microhal::backends::stm32::adc_detail::engine<
                           microhal::backends::stm32::adc_detail::register_hardware<ADC_TypeDef, adc_instance>>
{
  template <typename C, auto H> static status start_interrupt(std::uint8_t channel) noexcept
  {
    return start_async<C, H>(channel);
  }
  template <typename C, auto H> static status start_dma(std::uint8_t channel, std::span<std::uint16_t> buffer) noexcept
  {
    return start_dma_scan<C, H>({&channel, 1}, buffer);
  }
};
#else
struct adc_api final
{
  template <typename C> static status initialize() noexcept
  {
    return status::unsupported;
  }
  template <typename C> static result<std::uint16_t> read_blocking(std::uint8_t) noexcept
  {
    return status::unsupported;
  }
  template <typename C> static status start_continuous(std::span<const std::uint8_t>) noexcept
  {
    return status::unsupported;
  }
  template <typename C, auto H> static status start_async(std::uint8_t) noexcept
  {
    return status::unsupported;
  }
  template <typename C, auto H>
  static status start_dma_scan(std::span<const std::uint8_t>, std::span<std::uint16_t>) noexcept
  {
    return status::unsupported;
  }
  template <typename C, auto H>
  static status start_dma_stream(std::span<const std::uint8_t>, std::span<std::uint16_t>, bool = true) noexcept
  {
    return status::unsupported;
  }
  template <typename C> static status stop() noexcept
  {
    return status::unsupported;
  }
  template <typename C> static status abort() noexcept
  {
    return status::unsupported;
  }
  template <typename C> static result<dma_transfer_state> transfer_status() noexcept
  {
    return status::unsupported;
  }
  template <typename C> static void handle_interrupt() noexcept {}
  template <typename C> static void handle_dma_interrupt() noexcept {}
};
#endif
