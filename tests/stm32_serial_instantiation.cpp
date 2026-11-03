// SPDX-License-Identifier: Apache-2.0
#if defined(STM32C0) || defined(STM32F0) || defined(STM32F1) || defined(STM32F2) || defined(STM32F3) ||                \
    defined(STM32F4) || defined(STM32F7) || defined(STM32G0) || defined(STM32G4) || defined(STM32H5) ||                \
    defined(STM32H7) || defined(STM32H7RS) || defined(STM32L0) || defined(STM32L1) || defined(STM32L4) ||              \
    defined(STM32L5) || defined(STM32N6) || defined(STM32U0) || defined(STM32U3) || defined(STM32U5) ||                \
    defined(STM32WB)
#include "microhal/core/backend_selector.hpp"
#include "microhal/i2c/i2c.hpp"
#include "microhal/i2s/i2s.hpp"
#include "microhal/spi/spi.hpp"
#include "microhal/uart/uart.hpp"

namespace
{
void completion(microhal::status) noexcept {}
template <typename Config, std::uintptr_t Base> struct bound_config
{
  static constexpr auto instance_index = Config::instance_index;
  static constexpr auto transfer_mode_value = Config::transfer_mode_value;
  static constexpr bool enable_dma = Config::enable_dma;
  static constexpr std::uintptr_t base_address = Base;
  static constexpr std::uint32_t timing = 0x00303D5BU;
  static constexpr std::uint32_t timeout_ms = 10U;
  static constexpr std::uint32_t clock_hz = 100000U;
  static constexpr auto address_mode = microhal::i2c_address_mode::seven_bit;
  static constexpr bool enable_interrupts = Config::transfer_mode_value == microhal::transfer_mode::interrupt;
  static constexpr auto duplex_mode = microhal::spi_duplex_mode::full;
  static constexpr auto clock_polarity = microhal::spi_clock_polarity::idle_low;
  static constexpr auto clock_phase = microhal::spi_clock_phase::first_edge;
  static constexpr auto bit_order = microhal::spi_bit_order::msb_first;
  static constexpr auto baud_rate = 115200U;
  static constexpr auto parity = microhal::uart_parity::none;
  static constexpr auto stop_bits = microhal::uart_stop_bits::one;
  static constexpr bool require_fifo = false, require_lpuart = false;
  static constexpr auto mode = microhal::i2s_mode::master_tx;
  static constexpr auto standard = microhal::i2s_standard::philips;
  static constexpr auto sample_rate = 48000U;
  static constexpr bool enable_master_clock = false;
};
template <typename Config> struct receive_config : Config
{
  static constexpr auto mode = microhal::i2s_mode::master_rx;
};

template <typename Backend, microhal::transfer_mode Mode> void exercise()
{
  using namespace microhal;
  std::uint8_t tx[4]{}, rx[4]{};
  if constexpr (backend_traits<Backend>::i2c_capability().transfer_modes.supports(Mode))
  {
    using bus = i2c::master<
        Backend,
        bound_config<i2c::master_config<1, 100000, i2c_address_mode::seven_bit, false, false, 10, Mode>, I2C1_BASE>>;
    (void)bus::initialize();
    (void)bus::write_blocking(0x50, tx);
    (void)bus::read_blocking(0x50, rx);
    (void)bus::write_read_blocking(0x50, tx, rx);
    (void)bus::template write_async<completion>(0x50, tx);
    (void)bus::template read_async<completion>(0x50, rx);
    (void)bus::template write_read_async<completion>(0x50, tx, rx);
    (void)bus::abort();
    bus::mode_api::template handle_interrupt<typename bus::config_type>();
  }
  if constexpr (backend_traits<Backend>::spi_capability().transfer_modes.supports(Mode))
  {
    using spi_bus = spi::master<
        Backend,
        bound_config<spi::master_config<1, 1000000, spi_duplex_mode::full, spi_clock_polarity::idle_low,
                                        spi_clock_phase::first_edge, spi_bit_order::msb_first, false, false, 10, Mode>,
                     SPI1_BASE>>;
    (void)spi_bus::initialize();
    (void)spi_bus::transfer(tx, rx);
    (void)spi_bus::write_blocking(tx);
    (void)spi_bus::read_blocking(rx);
    (void)spi_bus::template transfer_async<completion>(tx, rx);
    (void)spi_bus::template write_async<completion>(tx);
    (void)spi_bus::template read_async<completion>(rx);
    (void)spi_bus::abort();
    spi_bus::mode_api::template handle_interrupt<typename spi_bus::config_type>();
  }
  if constexpr (backend_traits<Backend>::uart_capability().transfer_modes.supports(Mode))
  {
    using port = uart::port<Backend, bound_config<uart::port_config<1, 115200, uart_parity::none, uart_stop_bits::one,
                                                                    false, false, false, false, 10, Mode>,
                                                  USART1_BASE>>;
    (void)port::initialize();
    (void)port::write_blocking(tx);
    (void)port::read_blocking(rx);
    (void)port::template start_async_tx<completion>(tx);
    (void)port::template start_async_rx<completion>(rx);
    (void)port::abort();
    port::mode_api::template handle_interrupt<typename port::config_type>();
  }
#if defined(SPI_I2SCFGR_I2SMOD) && defined(HAL_I2S_MODULE_ENABLED)
  if constexpr (backend_traits<Backend>::i2s_capability().transfer_modes.supports(Mode))
  {
    using audio = i2s::interface<
        Backend, bound_config<i2s::i2s_config<1, i2s_mode::master_tx, i2s_standard::philips, 48000, false, false, Mode>,
                              SPI1_BASE>>;
    std::uint16_t samples[4]{};
    (void)audio::initialize();
    (void)audio::transmit_blocking(samples);
    (void)audio::receive_blocking(samples);
    (void)audio::template transmit_async<completion>(samples);
    (void)audio::template receive_async<completion>(samples);
    (void)audio::abort();
    audio::mode_api::template handle_interrupt<typename audio::config_type>();
    using receiver = i2s::interface<Backend, receive_config<typename audio::config_type>>;
    (void)receiver::initialize();
    (void)receiver::receive_blocking(samples);
    (void)receiver::template receive_async<completion>(samples);
    (void)receiver::abort();
  }
#endif
}
} // namespace

void instantiate_stm32_serial_modes()
{
  exercise<microhal::stm32_hal_tag, microhal::transfer_mode::polled>();
  exercise<microhal::stm32_hal_tag, microhal::transfer_mode::interrupt>();
  exercise<microhal::stm32_hal_tag, microhal::transfer_mode::dma>();
  exercise<microhal::stm32_ll_tag, microhal::transfer_mode::polled>();
  exercise<microhal::stm32_ll_tag, microhal::transfer_mode::interrupt>();
  exercise<microhal::stm32_ll_tag, microhal::transfer_mode::dma>();
}
#endif
