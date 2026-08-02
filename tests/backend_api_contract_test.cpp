// SPDX-License-Identifier: Apache-2.0
// Verifies that every backend-advertised peripheral API is callable through the canonical
// `*_api_t<Backend>` contract used by the public facades.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "microhal/core/backend_selector.hpp"
#include "microhal/dma/dma.hpp"
#include "microhal/rcc/rcc.hpp"
#include "microhal/rtc/rtc.hpp"

namespace
{

constexpr auto null_handler = nullptr;

struct gpio_cfg final
{
  [[maybe_unused]] static constexpr std::uint8_t port_index{0U};
  [[maybe_unused]] static constexpr std::uint8_t pin_index{0U};
  [[maybe_unused]] static constexpr microhal::gpio_mode mode{microhal::gpio_mode::output};
  [[maybe_unused]] static constexpr microhal::gpio_pull pull{microhal::gpio_pull::none};
  [[maybe_unused]] static constexpr microhal::gpio_speed speed{microhal::gpio_speed::low};
  [[maybe_unused]] static constexpr std::uint8_t alternate_function{microhal::gpio_no_alternate};
  [[maybe_unused]] static constexpr microhal::gpio_interrupt_trigger interrupt_trigger{
      microhal::gpio_interrupt_trigger::none};
};

struct i2c_cfg final
{
  [[maybe_unused]] static constexpr std::uint8_t instance_index{0U};
  [[maybe_unused]] static constexpr std::uint32_t clock_hz{100000U};
  [[maybe_unused]] static constexpr microhal::i2c_address_mode address_mode{microhal::i2c_address_mode::seven_bit};
  [[maybe_unused]] static constexpr bool enable_dma{false};
  [[maybe_unused]] static constexpr bool enable_interrupts{false};
};

struct spi_cfg final
{
  [[maybe_unused]] static constexpr std::uint8_t instance_index{0U};
  [[maybe_unused]] static constexpr std::uint32_t clock_hz{1000000U};
  [[maybe_unused]] static constexpr microhal::spi_duplex_mode duplex_mode{microhal::spi_duplex_mode::full};
  [[maybe_unused]] static constexpr microhal::spi_clock_polarity clock_polarity{microhal::spi_clock_polarity::idle_low};
  [[maybe_unused]] static constexpr microhal::spi_clock_phase clock_phase{microhal::spi_clock_phase::first_edge};
  [[maybe_unused]] static constexpr microhal::spi_bit_order bit_order{microhal::spi_bit_order::msb_first};
  [[maybe_unused]] static constexpr bool enable_dma{false};
  [[maybe_unused]] static constexpr bool enable_interrupts{false};
};

struct qspi_cfg final
{
  [[maybe_unused]] static constexpr std::uint32_t clock_hz{1000000U};
  [[maybe_unused]] static constexpr bool enable_dma{false};
  [[maybe_unused]] static constexpr bool enable_interrupts{false};
};

struct uart_cfg final
{
  [[maybe_unused]] static constexpr std::uint8_t instance_index{0U};
  [[maybe_unused]] static constexpr std::uint32_t baud_rate{115200U};
  [[maybe_unused]] static constexpr microhal::uart_parity parity{microhal::uart_parity::none};
  [[maybe_unused]] static constexpr microhal::uart_stop_bits stop_bits{microhal::uart_stop_bits::one};
  [[maybe_unused]] static constexpr bool enable_dma{false};
  [[maybe_unused]] static constexpr bool enable_interrupts{false};
  [[maybe_unused]] static constexpr bool require_lpuart{false};
};

struct timer_cfg final
{
  [[maybe_unused]] static constexpr std::uint8_t instance_index{0U};
  [[maybe_unused]] static constexpr std::uint32_t clock_hz{1000000U};
  [[maybe_unused]] static constexpr microhal::timer_counter_mode counter_mode{microhal::timer_counter_mode::up};
  [[maybe_unused]] static constexpr microhal::timer_clock_source clock_source{microhal::timer_clock_source::internal};
  [[maybe_unused]] static constexpr bool enable_dma{false};
  [[maybe_unused]] static constexpr bool enable_interrupts{false};

  // Legacy fields still read by some backend implementations.
  [[maybe_unused]] static constexpr std::uint16_t prescaler{0U};
  [[maybe_unused]] static constexpr std::uint32_t period{1000U};
};

struct rtc_cfg final
{
  [[maybe_unused]] static constexpr bool enable_periodic_irq{false};
  [[maybe_unused]] static constexpr bool enable_alarm_irq{false};
};

struct flash_cfg final
{
  [[maybe_unused]] static constexpr std::uint32_t program_alignment_bytes{8U};
  [[maybe_unused]] static constexpr std::uint32_t erase_size_bytes{4096U};
  [[maybe_unused]] static constexpr bool enable_dma{false};
  [[maybe_unused]] static constexpr bool enable_interrupts{false};
};

struct adc_cfg final
{
  [[maybe_unused]] static constexpr std::uint8_t instance_index{0U};
  [[maybe_unused]] static constexpr microhal::adc_resolution resolution{microhal::adc_resolution::bits_12};
  [[maybe_unused]] static constexpr microhal::adc_sample_time sample_time{microhal::adc_sample_time::cycles_84};
  [[maybe_unused]] static constexpr bool enable_dma{false};
  [[maybe_unused]] static constexpr bool enable_interrupts{false};
  [[maybe_unused]] static constexpr bool differential_mode{false};
};

struct dac_cfg final
{
  [[maybe_unused]] static constexpr std::uint8_t instance_index{0U};
  [[maybe_unused]] static constexpr std::uint8_t channel{1U};
  [[maybe_unused]] static constexpr microhal::dac_resolution resolution{microhal::dac_resolution::bits_12};
  [[maybe_unused]] static constexpr bool enable_dma{false};
  [[maybe_unused]] static constexpr bool enable_waveform{false};
};

struct can_cfg final
{
  [[maybe_unused]] static constexpr std::uint8_t instance_index{0U};
  [[maybe_unused]] static constexpr std::uint32_t baudrate{500000U};
  [[maybe_unused]] static constexpr microhal::can_mode mode{microhal::can_mode::normal};
  [[maybe_unused]] static constexpr bool enable_can_fd{false};
  [[maybe_unused]] static constexpr bool enable_interrupts{false};
};

struct usb_cfg final
{
  [[maybe_unused]] static constexpr std::uint8_t instance_index{0U};
  [[maybe_unused]] static constexpr microhal::usb_speed speed{microhal::usb_speed::full};
  [[maybe_unused]] static constexpr bool device_mode{true};
  [[maybe_unused]] static constexpr bool host_mode{false};
  [[maybe_unused]] static constexpr bool otg_mode{false};
};

struct ethernet_cfg final
{
  [[maybe_unused]] static constexpr std::uint8_t instance_index{0U};
  [[maybe_unused]] static constexpr microhal::ethernet_link_speed link_speed{microhal::ethernet_link_speed::mbps100};
  [[maybe_unused]] static constexpr microhal::ethernet_duplex duplex{microhal::ethernet_duplex::full};
  [[maybe_unused]] static constexpr bool enable_checksum_offload{false};
  [[maybe_unused]] static constexpr bool enable_dma{false};
  [[maybe_unused]] static constexpr bool enable_interrupts{false};
};

struct sd_cfg final
{
  [[maybe_unused]] static constexpr std::uint8_t instance_index{0U};
  [[maybe_unused]] static constexpr std::uint32_t clock_hz{25000000U};
  [[maybe_unused]] static constexpr microhal::sd_bus_width bus_width{microhal::sd_bus_width::one};
  [[maybe_unused]] static constexpr microhal::sd_timing_mode timing_mode{microhal::sd_timing_mode::default_speed};
  [[maybe_unused]] static constexpr bool enable_dma{false};
  [[maybe_unused]] static constexpr bool enable_interrupts{false};
};

struct dma_cfg final
{
  [[maybe_unused]] static constexpr std::uint8_t instance_index{0U};
  [[maybe_unused]] static constexpr std::uint8_t stream{0U};
  [[maybe_unused]] static constexpr std::uint8_t channel{0U};
  [[maybe_unused]] static constexpr microhal::dma::dma_direction direction{
      microhal::dma::dma_direction::periph_to_memory};
  [[maybe_unused]] static constexpr microhal::dma::dma_priority priority{microhal::dma::dma_priority::medium};
  [[maybe_unused]] static constexpr bool circular_mode{false};
  [[maybe_unused]] static constexpr bool double_buffer{false};
};

struct pwm_cfg final
{
  [[maybe_unused]] static constexpr std::uint8_t timer_index{0U};
  [[maybe_unused]] static constexpr std::uint8_t channel{1U};
  [[maybe_unused]] static constexpr std::uint32_t frequency_hz{1000U};
  [[maybe_unused]] static constexpr microhal::pwm_polarity polarity{microhal::pwm_polarity::active_high};
  [[maybe_unused]] static constexpr bool enable_dead_time{false};

  // Legacy fields still read by some backend implementations.
  [[maybe_unused]] static constexpr std::uint16_t prescaler{0U};
  [[maybe_unused]] static constexpr std::uint32_t period{1000U};
};

struct watchdog_cfg final
{
  [[maybe_unused]] static constexpr std::uint8_t instance_index{0U};
  [[maybe_unused]] static constexpr std::uint32_t timeout_ms{1000U};
  [[maybe_unused]] static constexpr bool independent_mode{true};
  [[maybe_unused]] static constexpr bool window_mode{false};
};

struct i2s_cfg final
{
  [[maybe_unused]] static constexpr std::uint8_t instance_index{0U};
  [[maybe_unused]] static constexpr microhal::i2s_mode mode{microhal::i2s_mode::master_tx};
  [[maybe_unused]] static constexpr microhal::i2s_standard standard{microhal::i2s_standard::philips};
  [[maybe_unused]] static constexpr std::uint32_t sample_rate{48000U};
  [[maybe_unused]] static constexpr bool enable_master_clock{false};
  [[maybe_unused]] static constexpr bool enable_dma{false};
};

struct rcc_cfg final
{
  [[maybe_unused]] static constexpr microhal::rcc::clock_source source{microhal::rcc::clock_source::hsi};
  [[maybe_unused]] static constexpr std::uint32_t system_clock_hz{16000000U};
  [[maybe_unused]] static constexpr bool enable_pll{false};
};

struct crc_cfg final
{
  [[maybe_unused]] static constexpr std::uint8_t instance_index{0U};
  [[maybe_unused]] static constexpr bool enable_rng{false};
  [[maybe_unused]] static constexpr std::uint32_t polynomial{0x04C11DB7U};
};

struct autobaud_cfg final
{
  using timer_config = timer_cfg;
  using gpio_pin = gpio_cfg;
  [[maybe_unused]] static constexpr std::uint32_t min_baud{9600U};
  [[maybe_unused]] static constexpr std::uint32_t max_baud{115200U};
};

template <typename Backend, microhal::peripheral_kind Peripheral> consteval bool peripheral_is_supported() noexcept
{
  return microhal::capability_for<Backend, Peripheral>().supported;
}

template <typename Backend> consteval bool backend_contract_holds() noexcept
{
  using namespace microhal;

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::gpio>())
  {
    if constexpr (!(requires {
                    gpio_api_t<Backend>::template configure<gpio_cfg>();
                    gpio_api_t<Backend>::template write<gpio_cfg>(true);
                    gpio_api_t<Backend>::template read<gpio_cfg>();
                    gpio_api_t<Backend>::template enable_interrupt<gpio_cfg, null_handler>();
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::i2c>())
  {
    if constexpr (!(requires {
                    i2c_api_t<Backend>::template initialize<i2c_cfg>();
                    i2c_api_t<Backend>::template write_blocking<i2c_cfg>(std::span<const std::uint8_t>{}, 0U);
                    i2c_api_t<Backend>::template read_blocking<i2c_cfg>(std::span<std::uint8_t>{}, 0U);
                  }))
    {
      return false;
    }
    if constexpr (capability_for<Backend, peripheral_kind::i2c>().isr && !(requires {
                    i2c_api_t<Backend>::template start_transfer<i2c_cfg, null_handler>(std::span<std::uint8_t>{}, 0U);
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::spi>())
  {
    if constexpr (!(requires {
                    spi_api_t<Backend>::template initialize<spi_cfg>();
                    spi_api_t<Backend>::template transfer<spi_cfg>(std::span<const std::uint8_t>{},
                                                                   std::span<std::uint8_t>{});
                  }))
    {
      return false;
    }
    if constexpr (capability_for<Backend, peripheral_kind::spi>().isr && !(requires {
                    spi_api_t<Backend>::template transfer_async<spi_cfg, null_handler>(std::span<const std::uint8_t>{},
                                                                                       std::span<std::uint8_t>{});
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::qspi>() &&
                !(requires { qspi_api_t<Backend>::template initialize<qspi_cfg>(); }))
  {
    return false;
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::uart>())
  {
    if constexpr (!(requires {
                    uart_api_t<Backend>::template initialize<uart_cfg>();
                    uart_api_t<Backend>::template write_blocking<uart_cfg>(std::span<const std::uint8_t>{});
                    uart_api_t<Backend>::template read_blocking<uart_cfg>(std::span<std::uint8_t>{});
                  }))
    {
      return false;
    }
    if constexpr (capability_for<Backend, peripheral_kind::uart>().isr)
    {
      if constexpr (!(requires {
                      uart_api_t<Backend>::template start_async_rx<uart_cfg, null_handler>(std::span<std::uint8_t>{});
                      uart_api_t<Backend>::template start_async_tx<uart_cfg, null_handler>(
                          std::span<const std::uint8_t>{});
                    }))
      {
        return false;
      }
    }
    if constexpr (backend_traits<Backend>::uart_capability().autobaud)
    {
      if constexpr (!(requires {
                      uart_api_t<Backend>::template arm_autobaud<uart_cfg, autobaud_cfg>();
                      uart_api_t<Backend>::template apply_autobaud<uart_cfg>(9600U);
                    }))
      {
        return false;
      }
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::rtc>())
  {
    if constexpr (!(requires {
                    rtc_api_t<Backend>::template initialize<rtc_cfg>();
                    rtc_api_t<Backend>::template set_time<rtc_cfg>(0U, 0U, 0U);
                    rtc_api_t<Backend>::template get_time<rtc_cfg>(
                        std::declval<std::uint8_t&>(), std::declval<std::uint8_t&>(), std::declval<std::uint8_t&>());
                  }))
    {
      return false;
    }
    if constexpr (capability_for<Backend, peripheral_kind::rtc>().isr && !(requires {
                    rtc_api_t<Backend>::template enable_alarm<rtc_cfg, null_handler>(
                        microhal::rtc::date_time{.hour = 0U, .minute = 0U, .second = 0U});
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::flash>() && !(requires {
                  flash_api_t<Backend>::template erase<flash_cfg>(0U, 0U);
                  flash_api_t<Backend>::template program<flash_cfg>(0U, std::span<const std::uint8_t>{});
                  flash_api_t<Backend>::template read<flash_cfg>(0U, std::span<std::uint8_t>{});
                }))
  {
    return false;
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::timer>())
  {
    if constexpr (!(requires {
                    timer_api_t<Backend>::template initialize<timer_cfg>();
                    timer_api_t<Backend>::template counter<timer_cfg>();
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::adc>())
  {
    if constexpr (!(requires {
                    adc_api_t<Backend>::template initialize<adc_cfg>();
                    adc_api_t<Backend>::template read_blocking<adc_cfg>(0U);
                    adc_api_t<Backend>::template start_continuous<adc_cfg>(std::span<const std::uint8_t>{});
                  }))
    {
      return false;
    }
    if constexpr (capability_for<Backend, peripheral_kind::adc>().isr &&
                  !(requires { adc_api_t<Backend>::template start_async<adc_cfg, null_handler>(0U); }))
    {
      return false;
    }
    if constexpr (capability_for<Backend, peripheral_kind::adc>().dma && !(requires {
                    adc_api_t<Backend>::template start_dma_scan<adc_cfg, null_handler>(std::span<const std::uint8_t>{},
                                                                                       std::span<std::uint16_t>{});
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::dac>())
  {
    if constexpr (!(requires {
                    dac_api_t<Backend>::template initialize<dac_cfg>();
                    dac_api_t<Backend>::template write_blocking<dac_cfg>(0U);
                  }))
    {
      return false;
    }
    if constexpr (backend_traits<Backend>::dac_capability().dual_mode &&
                  !(requires { dac_api_t<Backend>::template write_dual<dac_cfg>(0U, 0U); }))
    {
      return false;
    }
    if constexpr (capability_for<Backend, peripheral_kind::dac>().dma && !(requires {
                    dac_api_t<Backend>::template start_dma_waveform<dac_cfg, null_handler>(
                        std::span<const std::uint16_t>{});
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::can>())
  {
    if constexpr (!(requires { can_api_t<Backend>::template initialize<can_cfg>(); }))
    {
      return false;
    }
    if constexpr (!(requires {
                    can_api_t<Backend>::template transmit<can_cfg>(std::declval<const microhal::can::message&>());
                  }) && !(requires {
                    can_api_t<Backend>::template transmit<can_cfg>(std::declval<const std::uint8_t*>(),
                                                                   std::declval<std::size_t>());
                  }))
    {
      return false;
    }
    if constexpr (!(requires {
                    can_api_t<Backend>::template receive<can_cfg>(std::declval<microhal::can::message&>());
                  }) && !(requires {
                    can_api_t<Backend>::template receive<can_cfg>(std::declval<std::uint8_t*>(),
                                                                  std::declval<std::size_t&>());
                  }))
    {
      return false;
    }
    if constexpr (!(requires { can_api_t<Backend>::template configure_filter<can_cfg>(0U, 0U, 0U); }))
    {
      return false;
    }
    if constexpr (capability_for<Backend, peripheral_kind::can>().isr &&
                  !(requires { can_api_t<Backend>::template enable_rx_interrupt<can_cfg, null_handler>(); }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::usb>() && !(requires {
                  usb_api_t<Backend>::template initialize<usb_cfg>();
                  usb_api_t<Backend>::template connect<usb_cfg>();
                  usb_api_t<Backend>::template disconnect<usb_cfg>();
                  usb_api_t<Backend>::template endpoint_transmit<usb_cfg>(0U, std::span<const std::uint8_t>{});
                  usb_api_t<Backend>::template endpoint_receive<usb_cfg>(0U, std::span<std::uint8_t>{});
                }))
  {
    return false;
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::ethernet>() && !(requires {
                  ethernet_api_t<Backend>::template initialize<ethernet_cfg>();
                  ethernet_api_t<Backend>::template start<ethernet_cfg>();
                  ethernet_api_t<Backend>::template stop<ethernet_cfg>();
                  ethernet_api_t<Backend>::template transmit_frame<ethernet_cfg>(std::span<const std::uint8_t>{});
                  ethernet_api_t<Backend>::template receive_frame<ethernet_cfg>(std::span<std::uint8_t>{});
                  ethernet_api_t<Backend>::template set_mac_address<ethernet_cfg>(std::array<std::uint8_t, 6U>{});
                }))
  {
    return false;
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::sd>() && !(requires {
                  sd_api_t<Backend>::template initialize<sd_cfg>();
                  sd_api_t<Backend>::template read_blocks<sd_cfg>(0U, std::span<std::uint8_t>{});
                  sd_api_t<Backend>::template write_blocks<sd_cfg>(0U, std::span<const std::uint8_t>{});
                  sd_api_t<Backend>::template erase_blocks<sd_cfg>(0U, 0U);
                  sd_api_t<Backend>::template capacity_blocks<sd_cfg>();
                }))
  {
    return false;
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::dma>() && !(requires {
                  dma_api_t<Backend>::template initialize<dma_cfg>();
                  dma_api_t<Backend>::template start_transfer<dma_cfg, null_handler>(0U, 0U, 0U);
                  dma_api_t<Backend>::template abort<dma_cfg>();
                }))
  {
    return false;
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::pwm>() && !(requires {
                  pwm_api_t<Backend>::template initialize<pwm_cfg>();
                  pwm_api_t<Backend>::template set_duty_cycle<pwm_cfg>(0U);
                  pwm_api_t<Backend>::template start<pwm_cfg>();
                  pwm_api_t<Backend>::template stop<pwm_cfg>();
                }))
  {
    return false;
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::watchdog>() && !(requires {
                  watchdog_api_t<Backend>::template initialize<watchdog_cfg>();
                  watchdog_api_t<Backend>::template start<watchdog_cfg>();
                  watchdog_api_t<Backend>::template refresh<watchdog_cfg>();
                }))
  {
    return false;
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::i2s>() && !(requires {
                  i2s_api_t<Backend>::template initialize<i2s_cfg>();
                  i2s_api_t<Backend>::template transmit_blocking<i2s_cfg>(std::span<const std::uint16_t>{});
                  i2s_api_t<Backend>::template receive_blocking<i2s_cfg>(std::span<std::uint16_t>{});
                  i2s_api_t<Backend>::template start_dma_tx<i2s_cfg, null_handler>(std::span<const std::uint16_t>{});
                }))
  {
    return false;
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::rcc>() && !(requires {
                  rcc_api_t<Backend>::template configure<rcc_cfg>();
                  rcc_api_t<Backend>::template get_system_clock_hz<rcc_cfg>();
                  rcc_api_t<Backend>::template enable_peripheral_clock<rcc_cfg>(microhal::peripheral_kind::uart, 0U);
                  rcc_api_t<Backend>::template disable_peripheral_clock<rcc_cfg>(microhal::peripheral_kind::uart, 0U);
                }))
  {
    return false;
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::crc>() && !(requires {
                  crc_api_t<Backend>::template initialize<crc_cfg>();
                  crc_api_t<Backend>::template calculate<crc_cfg>(std::span<const std::uint8_t>{});
                  crc_api_t<Backend>::template reset<crc_cfg>();
                  crc_api_t<Backend>::template get_random<crc_cfg>();
                }))
  {
    return false;
  }

  return true;
}

template <typename Backend> consteval bool backend_raw_contract_holds() noexcept
{
  using namespace microhal;

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::gpio>())
  {
    using raw_gpio = typename backend_traits<Backend>::gpio_api;
    if constexpr (!(requires {
                    raw_gpio::template configure<gpio_cfg>();
                    raw_gpio::template write<gpio_cfg>(true);
                    raw_gpio::template read<gpio_cfg>();
                    raw_gpio::template enable_interrupt<gpio_cfg, null_handler>();
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::i2c>())
  {
    using raw_i2c = typename backend_traits<Backend>::i2c_api;
    if constexpr (!(requires {
                    raw_i2c::template initialize<i2c_cfg>();
                    raw_i2c::template write_blocking<i2c_cfg>(std::span<const std::uint8_t>{}, 0U);
                    raw_i2c::template read_blocking<i2c_cfg>(std::span<std::uint8_t>{}, 0U);
                    raw_i2c::template start_transfer<i2c_cfg, null_handler>(std::span<std::uint8_t>{}, 0U);
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::spi>())
  {
    using raw_spi = typename backend_traits<Backend>::spi_api;
    if constexpr (!(requires {
                    raw_spi::template initialize<spi_cfg>();
                    raw_spi::template transfer<spi_cfg>(std::span<const std::uint8_t>{}, std::span<std::uint8_t>{});
                    raw_spi::template transfer_async<spi_cfg, null_handler>(std::span<const std::uint8_t>{},
                                                                            std::span<std::uint8_t>{});
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::qspi>())
  {
    using raw_qspi = typename backend_traits<Backend>::qspi_api;
    if constexpr (!(requires { raw_qspi::template initialize<qspi_cfg>(); }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::uart>())
  {
    using raw_uart = typename backend_traits<Backend>::uart_api;
    if constexpr (!(requires {
                    raw_uart::template initialize<uart_cfg>();
                    raw_uart::template write_blocking<uart_cfg>(std::span<const std::uint8_t>{});
                    raw_uart::template read_blocking<uart_cfg>(std::span<std::uint8_t>{});
                    raw_uart::template start_async_rx<uart_cfg, null_handler>(std::span<std::uint8_t>{});
                    raw_uart::template start_async_tx<uart_cfg, null_handler>(std::span<const std::uint8_t>{});
                    raw_uart::template arm_autobaud<uart_cfg, autobaud_cfg>();
                    raw_uart::template apply_autobaud<uart_cfg>(9600U);
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::rtc>())
  {
    using raw_rtc = typename backend_traits<Backend>::rtc_api;
    if constexpr (!(requires {
                    raw_rtc::template initialize<rtc_cfg>();
                    raw_rtc::template set_time<rtc_cfg>(0U, 0U, 0U);
                    raw_rtc::template get_time<rtc_cfg>(std::declval<std::uint8_t&>(), std::declval<std::uint8_t&>(),
                                                        std::declval<std::uint8_t&>());
                    raw_rtc::template enable_alarm<rtc_cfg, null_handler>(
                        microhal::rtc::date_time{.hour = 0U, .minute = 0U, .second = 0U});
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::flash>())
  {
    using raw_flash = typename backend_traits<Backend>::flash_api;
    if constexpr (!(requires {
                    raw_flash::template erase<flash_cfg>(0U, 0U);
                    raw_flash::template program<flash_cfg>(0U, std::span<const std::uint8_t>{});
                    raw_flash::template read<flash_cfg>(0U, std::span<std::uint8_t>{});
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::timer>())
  {
    using raw_timer = typename backend_traits<Backend>::timer_api;
    if constexpr (!(requires {
                    raw_timer::template initialize<timer_cfg>();
                    raw_timer::template counter<timer_cfg>();
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::adc>())
  {
    using raw_adc = typename backend_traits<Backend>::adc_api;
    if constexpr (!(requires {
                    raw_adc::template initialize<adc_cfg>();
                    raw_adc::template read_blocking<adc_cfg>(0U);
                    raw_adc::template start_continuous<adc_cfg>(std::span<const std::uint8_t>{});
                    raw_adc::template start_async<adc_cfg, null_handler>(0U);
                    raw_adc::template start_dma_scan<adc_cfg, null_handler>(std::span<const std::uint8_t>{},
                                                                            std::span<std::uint16_t>{});
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::dac>())
  {
    using raw_dac = typename backend_traits<Backend>::dac_api;
    if constexpr (!(requires {
                    raw_dac::template initialize<dac_cfg>();
                    raw_dac::template write_blocking<dac_cfg>(0U);
                    raw_dac::template write_dual<dac_cfg>(0U, 0U);
                    raw_dac::template start_dma_waveform<dac_cfg, null_handler>(std::span<const std::uint16_t>{});
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::can>())
  {
    using raw_can = typename backend_traits<Backend>::can_api;
    if constexpr (!(requires { raw_can::template initialize<can_cfg>(); }))
    {
      return false;
    }
    if constexpr (!(requires { raw_can::template transmit<can_cfg>(std::declval<const microhal::can::message&>()); }) &&
                  !(requires {
                    raw_can::template transmit<can_cfg>(std::declval<const std::uint8_t*>(),
                                                        std::declval<std::size_t>());
                  }))
    {
      return false;
    }
    if constexpr (!(requires { raw_can::template receive<can_cfg>(std::declval<microhal::can::message&>()); }) &&
                  !(requires {
                    raw_can::template receive<can_cfg>(std::declval<std::uint8_t*>(), std::declval<std::size_t&>());
                  }))
    {
      return false;
    }
    if constexpr (!(requires {
                    raw_can::template configure_filter<can_cfg>(0U, 0U, 0U);
                    raw_can::template enable_rx_interrupt<can_cfg, null_handler>();
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::usb>())
  {
    using raw_usb = typename backend_traits<Backend>::usb_api;
    if constexpr (!(requires {
                    raw_usb::template initialize<usb_cfg>();
                    raw_usb::template connect<usb_cfg>();
                    raw_usb::template disconnect<usb_cfg>();
                    raw_usb::template endpoint_transmit<usb_cfg>(0U, std::span<const std::uint8_t>{});
                    raw_usb::template endpoint_receive<usb_cfg>(0U, std::span<std::uint8_t>{});
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::ethernet>())
  {
    using raw_ethernet = typename backend_traits<Backend>::ethernet_api;
    if constexpr (!(requires {
                    raw_ethernet::template initialize<ethernet_cfg>();
                    raw_ethernet::template start<ethernet_cfg>();
                    raw_ethernet::template stop<ethernet_cfg>();
                    raw_ethernet::template transmit_frame<ethernet_cfg>(std::span<const std::uint8_t>{});
                    raw_ethernet::template receive_frame<ethernet_cfg>(std::span<std::uint8_t>{});
                    raw_ethernet::template set_mac_address<ethernet_cfg>(std::array<std::uint8_t, 6U>{});
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::sd>())
  {
    using raw_sd = typename backend_traits<Backend>::sd_api;
    if constexpr (!(requires {
                    raw_sd::template initialize<sd_cfg>();
                    raw_sd::template read_blocks<sd_cfg>(0U, std::span<std::uint8_t>{});
                    raw_sd::template write_blocks<sd_cfg>(0U, std::span<const std::uint8_t>{});
                    raw_sd::template erase_blocks<sd_cfg>(0U, 0U);
                    raw_sd::template capacity_blocks<sd_cfg>();
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::dma>())
  {
    using raw_dma = typename backend_traits<Backend>::dma_api;
    if constexpr (!(requires {
                    raw_dma::template initialize<dma_cfg>();
                    raw_dma::template start_transfer<dma_cfg, null_handler>(0U, 0U, 0U);
                    raw_dma::template abort<dma_cfg>();
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::pwm>())
  {
    using raw_pwm = typename backend_traits<Backend>::pwm_api;
    if constexpr (!(requires {
                    raw_pwm::template initialize<pwm_cfg>();
                    raw_pwm::template set_duty_cycle<pwm_cfg>(0U);
                    raw_pwm::template start<pwm_cfg>();
                    raw_pwm::template stop<pwm_cfg>();
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::watchdog>())
  {
    using raw_watchdog = typename backend_traits<Backend>::watchdog_api;
    if constexpr (!(requires {
                    raw_watchdog::template initialize<watchdog_cfg>();
                    raw_watchdog::template start<watchdog_cfg>();
                    raw_watchdog::template refresh<watchdog_cfg>();
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::i2s>())
  {
    using raw_i2s = typename backend_traits<Backend>::i2s_api;
    if constexpr (!(requires {
                    raw_i2s::template initialize<i2s_cfg>();
                    raw_i2s::template transmit_blocking<i2s_cfg>(std::span<const std::uint16_t>{});
                    raw_i2s::template receive_blocking<i2s_cfg>(std::span<std::uint16_t>{});
                    raw_i2s::template start_dma_tx<i2s_cfg, null_handler>(std::span<const std::uint16_t>{});
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::rcc>())
  {
    using raw_rcc = typename backend_traits<Backend>::rcc_api;
    if constexpr (!(requires {
                    raw_rcc::template configure<rcc_cfg>();
                    raw_rcc::template get_system_clock_hz<rcc_cfg>();
                    raw_rcc::template enable_peripheral_clock<rcc_cfg>(microhal::peripheral_kind::uart, 0U);
                    raw_rcc::template disable_peripheral_clock<rcc_cfg>(microhal::peripheral_kind::uart, 0U);
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::crc>())
  {
    using raw_crc = typename backend_traits<Backend>::crc_api;
    if constexpr (!(requires {
                    raw_crc::template initialize<crc_cfg>();
                    raw_crc::template calculate<crc_cfg>(std::span<const std::uint8_t>{});
                    raw_crc::template reset<crc_cfg>();
                    raw_crc::template get_random<crc_cfg>();
                  }))
    {
      return false;
    }
  }

  return true;
}

template <typename Capability> consteval bool capability_flags_are_sane(const Capability& capability) noexcept
{
  if (!capability.supported)
  {
    return !capability.blocking && !capability.non_blocking && !capability.isr && !capability.dma;
  }
  return capability.blocking || capability.non_blocking;
}

template <typename Backend> consteval bool backend_capability_claims_hold() noexcept
{
  using namespace microhal;

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::gpio>())
  {
    using raw_gpio = typename backend_traits<Backend>::gpio_api;
    constexpr auto capability = capability_for<Backend, peripheral_kind::gpio>();
    if constexpr (!capability_flags_are_sane(capability))
    {
      return false;
    }
    if constexpr (capability.blocking && !(requires {
                    raw_gpio::template write<gpio_cfg>(true);
                    raw_gpio::template read<gpio_cfg>();
                  }))
    {
      return false;
    }
    if constexpr ((capability.non_blocking || capability.isr) &&
                  !(requires { raw_gpio::template enable_interrupt<gpio_cfg, null_handler>(); }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::i2c>())
  {
    using raw_i2c = typename backend_traits<Backend>::i2c_api;
    constexpr auto capability = capability_for<Backend, peripheral_kind::i2c>();
    if constexpr (!capability_flags_are_sane(capability))
    {
      return false;
    }
    if constexpr (capability.blocking && !(requires {
                    raw_i2c::template write_blocking<i2c_cfg>(std::span<const std::uint8_t>{}, 0U);
                    raw_i2c::template read_blocking<i2c_cfg>(std::span<std::uint8_t>{}, 0U);
                  }))
    {
      return false;
    }
    if constexpr ((capability.non_blocking || capability.isr || capability.dma) && !(requires {
                    raw_i2c::template start_transfer<i2c_cfg, null_handler>(std::span<std::uint8_t>{}, 0U);
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::spi>())
  {
    using raw_spi = typename backend_traits<Backend>::spi_api;
    constexpr auto capability = capability_for<Backend, peripheral_kind::spi>();
    if constexpr (!capability_flags_are_sane(capability))
    {
      return false;
    }
    if constexpr (capability.blocking && !(requires {
                    raw_spi::template transfer<spi_cfg>(std::span<const std::uint8_t>{}, std::span<std::uint8_t>{});
                  }))
    {
      return false;
    }
    if constexpr ((capability.non_blocking || capability.isr || capability.dma) && !(requires {
                    raw_spi::template transfer_async<spi_cfg, null_handler>(std::span<const std::uint8_t>{},
                                                                            std::span<std::uint8_t>{});
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::uart>())
  {
    using raw_uart = typename backend_traits<Backend>::uart_api;
    constexpr auto capability = capability_for<Backend, peripheral_kind::uart>();
    if constexpr (!capability_flags_are_sane(capability))
    {
      return false;
    }
    if constexpr (capability.blocking && !(requires {
                    raw_uart::template write_blocking<uart_cfg>(std::span<const std::uint8_t>{});
                    raw_uart::template read_blocking<uart_cfg>(std::span<std::uint8_t>{});
                  }))
    {
      return false;
    }
    if constexpr ((capability.non_blocking || capability.isr) && !(requires {
                    raw_uart::template start_async_rx<uart_cfg, null_handler>(std::span<std::uint8_t>{});
                    raw_uart::template start_async_tx<uart_cfg, null_handler>(std::span<const std::uint8_t>{});
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::adc>())
  {
    using raw_adc = typename backend_traits<Backend>::adc_api;
    constexpr auto capability = capability_for<Backend, peripheral_kind::adc>();
    if constexpr (!capability_flags_are_sane(capability))
    {
      return false;
    }
    if constexpr (capability.blocking && !(requires { raw_adc::template read_blocking<adc_cfg>(0U); }))
    {
      return false;
    }
    if constexpr ((capability.non_blocking || capability.isr) &&
                  !(requires { raw_adc::template start_async<adc_cfg, null_handler>(0U); }))
    {
      return false;
    }
    if constexpr (capability.dma && !(requires {
                    raw_adc::template start_dma_scan<adc_cfg, null_handler>(std::span<const std::uint8_t>{},
                                                                            std::span<std::uint16_t>{});
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::dac>())
  {
    using raw_dac = typename backend_traits<Backend>::dac_api;
    constexpr auto capability = capability_for<Backend, peripheral_kind::dac>();
    if constexpr (!capability_flags_are_sane(capability))
    {
      return false;
    }
    if constexpr (capability.blocking && !(requires { raw_dac::template write_blocking<dac_cfg>(0U); }))
    {
      return false;
    }
    if constexpr ((capability.non_blocking || capability.isr || capability.dma) && !(requires {
                    raw_dac::template start_dma_waveform<dac_cfg, null_handler>(std::span<const std::uint16_t>{});
                  }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::can>())
  {
    using raw_can = typename backend_traits<Backend>::can_api;
    constexpr auto capability = capability_for<Backend, peripheral_kind::can>();
    if constexpr (!capability_flags_are_sane(capability))
    {
      return false;
    }
    if constexpr (capability.blocking &&
                  !(
                      requires {
                        raw_can::template transmit<can_cfg>(std::declval<const microhal::can::message&>());
                      } ||
                      requires {
                        raw_can::template transmit<can_cfg>(std::declval<const std::uint8_t*>(),
                                                            std::declval<std::size_t>());
                      }))
    {
      return false;
    }
    if constexpr ((capability.non_blocking || capability.isr) &&
                  !(requires { raw_can::template enable_rx_interrupt<can_cfg, null_handler>(); }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::dma>())
  {
    using raw_dma = typename backend_traits<Backend>::dma_api;
    constexpr auto capability = capability_for<Backend, peripheral_kind::dma>();
    if constexpr (!capability_flags_are_sane(capability))
    {
      return false;
    }
    if constexpr ((capability.non_blocking || capability.isr || capability.dma) &&
                  !(requires { raw_dma::template start_transfer<dma_cfg, null_handler>(0U, 0U, 0U); }))
    {
      return false;
    }
  }

  if constexpr (peripheral_is_supported<Backend, peripheral_kind::i2s>())
  {
    using raw_i2s = typename backend_traits<Backend>::i2s_api;
    constexpr auto capability = capability_for<Backend, peripheral_kind::i2s>();
    if constexpr (!capability_flags_are_sane(capability))
    {
      return false;
    }
    if constexpr (capability.blocking && !(requires {
                    raw_i2s::template transmit_blocking<i2s_cfg>(std::span<const std::uint16_t>{});
                    raw_i2s::template receive_blocking<i2s_cfg>(std::span<std::uint16_t>{});
                  }))
    {
      return false;
    }
    if constexpr ((capability.non_blocking || capability.isr || capability.dma) && !(requires {
                    raw_i2s::template start_dma_tx<i2s_cfg, null_handler>(std::span<const std::uint16_t>{});
                  }))
    {
      return false;
    }
  }

  return true;
}

#if MICROHAL_ENABLE_STM32_HAL
static_assert(backend_contract_holds<microhal::stm32_hal_tag>());
static_assert(backend_raw_contract_holds<microhal::stm32_hal_tag>());
static_assert(backend_capability_claims_hold<microhal::stm32_hal_tag>());
#endif
#if MICROHAL_ENABLE_STM32_LL
static_assert(backend_contract_holds<microhal::stm32_ll_tag>());
static_assert(backend_raw_contract_holds<microhal::stm32_ll_tag>());
static_assert(backend_capability_claims_hold<microhal::stm32_ll_tag>());
#endif
#if MICROHAL_ENABLE_KINETIS_ESPRESSO
static_assert(backend_contract_holds<microhal::kinetis_espresso_tag>());
static_assert(backend_raw_contract_holds<microhal::kinetis_espresso_tag>());
static_assert(backend_capability_claims_hold<microhal::kinetis_espresso_tag>());
#endif
#if MICROHAL_ENABLE_ZEPHYR
static_assert(backend_contract_holds<microhal::zephyr_tag>());
static_assert(backend_raw_contract_holds<microhal::zephyr_tag>());
static_assert(backend_capability_claims_hold<microhal::zephyr_tag>());
#endif
#if MICROHAL_ENABLE_NUTTX
static_assert(backend_contract_holds<microhal::nuttx_tag>());
static_assert(backend_raw_contract_holds<microhal::nuttx_tag>());
static_assert(backend_capability_claims_hold<microhal::nuttx_tag>());
#endif
#if MICROHAL_ENABLE_NORDIC_NRFX
static_assert(backend_contract_holds<microhal::nordic_nrf_tag>());
static_assert(backend_raw_contract_holds<microhal::nordic_nrf_tag>());
static_assert(backend_capability_claims_hold<microhal::nordic_nrf_tag>());
#endif
#if MICROHAL_ENABLE_MOCK
static_assert(backend_contract_holds<microhal::mock_tag>());
static_assert(backend_raw_contract_holds<microhal::mock_tag>());
static_assert(backend_capability_claims_hold<microhal::mock_tag>());
#endif

} // namespace

int main()
{
  return 0;
}
