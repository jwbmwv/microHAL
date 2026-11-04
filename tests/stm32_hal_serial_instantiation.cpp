// SPDX-License-Identifier: Apache-2.0
// Compile-only Cube HAL coverage, deliberately independent of the native LL backend.
#include "microhal/backends/stm32/hal/traits.hpp"

#if MICROHAL_STM32_HAL_FAMILY_SELECTED
namespace
{
void complete(microhal::status) noexcept {}
void received(microhal::status, std::span<std::uint8_t>) noexcept {}
template <microhal::transfer_mode Mode, std::uintptr_t Base, bool Slave = false> struct config
{
  static constexpr auto transfer_mode_value = Mode;
  static constexpr auto base_address = Base;
  static constexpr auto instance_index = 1U;
  static constexpr auto is_slave = Slave;
  static constexpr auto address_mode = microhal::i2c_address_mode::seven_bit;
  static constexpr auto own_address = 0x52U;
  static constexpr auto timing = 0x00303D5BU;
  static constexpr auto clock_hz = 100000U;
  static constexpr auto timeout_ms = 10U;
  static constexpr auto duplex_mode = microhal::spi_duplex_mode::full;
  static constexpr auto clock_polarity = microhal::spi_clock_polarity::idle_high;
  static constexpr auto clock_phase = microhal::spi_clock_phase::second_edge;
  static constexpr auto bit_order = microhal::spi_bit_order::lsb_first;
  static constexpr auto baud_rate = 115200U;
  static constexpr auto parity = microhal::uart_parity::none;
  static constexpr auto stop_bits = microhal::uart_stop_bits::one;
  static constexpr bool require_fifo = false, require_lpuart = false;
};
template <microhal::transfer_mode Mode> void exercise()
{
  using namespace microhal;
  std::uint8_t tx[8]{}, rx[8]{};
#if MICROHAL_STM32_HAL_HAS_I2C && defined(I2C1_BASE)
  using i2c = serial_mode_api<stm32_hal_tag, peripheral_kind::i2c>;
  using master = config<Mode, I2C1_BASE>;
  using slave = config<Mode, I2C1_BASE, true>;
  (void)i2c::initialize<master>();
  (void)i2c::write<master, complete>(tx, 0x50);
  (void)i2c::read<master, complete>(rx, 0x50);
  (void)i2c::abort<master>();
  (void)i2c::initialize<slave>();
  (void)i2c::write<slave, complete>(tx);
  (void)i2c::read<slave, complete>(rx);
  (void)i2c::write<slave, nullptr>(tx);
  (void)i2c::read<slave, nullptr>(rx);
  (void)i2c::abort<slave>();
  (void)i2c::handle_interrupt<slave>();
#endif
#if MICROHAL_STM32_HAL_HAS_SPI && defined(SPI1_BASE)
  using spi = serial_mode_api<stm32_hal_tag, peripheral_kind::spi>;
  using spi_slave = config<Mode, SPI1_BASE, true>;
  (void)spi::initialize<spi_slave>();
  (void)spi::transfer<spi_slave, complete>(tx, rx);
  (void)spi::write<spi_slave, complete>(tx);
  (void)spi::read<spi_slave, complete>(rx);
  (void)spi::transfer<spi_slave, nullptr>(tx, rx);
  (void)spi::abort<spi_slave>();
  (void)spi::handle_interrupt<spi_slave>();
#endif
#if MICROHAL_STM32_HAL_HAS_UART && defined(USART1_BASE)
  using uart = serial_mode_api<stm32_hal_tag, peripheral_kind::uart>;
  using port = config<Mode, USART1_BASE>;
  (void)uart::initialize<port>();
  (void)uart::write<port, complete>(tx);
  (void)uart::read<port, complete>(rx);
  (void)uart::start_receive_to_idle<port, received>(rx);
  (void)uart::start_circular_rx<port, received>(rx);
  (void)uart::abort_tx<port>();
  (void)uart::abort_rx<port>();
  (void)uart::tx_status<port>();
  (void)uart::rx_status<port>();
  (void)uart::handle_interrupt<port>();
  (void)uart::handle_dma_interrupt<port>();
  (void)uart::abort<port>();
#endif
}
} // namespace
void instantiate_stm32_hal_serial_extensions()
{
  exercise<microhal::transfer_mode::polled>();
  exercise<microhal::transfer_mode::interrupt>();
  exercise<microhal::transfer_mode::dma>();
}
#endif
