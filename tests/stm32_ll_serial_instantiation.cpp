// SPDX-License-Identifier: Apache-2.0
// This translation unit deliberately never includes a CubeHAL header.
#if defined(MICROHAL_BACKENDS_EXPLICIT) && MICROHAL_BACKENDS_EXPLICIT && defined(MICROHAL_ENABLE_STM32_LL) &&          \
    MICROHAL_ENABLE_STM32_LL && (!defined(MICROHAL_ENABLE_STM32_HAL) || !MICROHAL_ENABLE_STM32_HAL)
#define MICROHAL_LL_PUBLIC_FACADES 1
#endif
#if (defined(STM32C0) || defined(STM32F0) || defined(STM32F1) || defined(STM32F2) || defined(STM32F3) ||               \
     defined(STM32F4) || defined(STM32F7) || defined(STM32G0) || defined(STM32G4) || defined(STM32H5) ||               \
     defined(STM32H7) || defined(STM32H7RS) || defined(STM32L0) || defined(STM32L1) || defined(STM32L4) ||             \
     defined(STM32L5) || defined(STM32N6) || defined(STM32U0) || defined(STM32U3) || defined(STM32U5) ||               \
     defined(STM32WB)) &&                                                                                              \
    (!defined(MICROHAL_ENABLE_STM32_LL) || MICROHAL_ENABLE_STM32_LL)
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

#include "microhal/backends/stm32/ll/serial_modes.hpp"
#if defined(MICROHAL_LL_PUBLIC_FACADES)
#include "microhal/i2c/i2c.hpp"
#include "microhal/spi/spi.hpp"
#include "microhal/uart/uart.hpp"
#endif

#if defined(HAL_UART_MODULE_ENABLED) || defined(HAL_SPI_MODULE_ENABLED) || defined(HAL_I2C_MODULE_ENABLED)
#error "Native LL serial must compile without CubeHAL declarations"
#endif

namespace
{
using namespace microhal;
void completion(status) noexcept {}
void range_completion(status, std::span<std::uint8_t>) noexcept {}
template <transfer_mode Mode, std::uintptr_t Base, bool Slave = false,
          spi_duplex_mode Direction = spi_duplex_mode::full>
struct config
{
  static constexpr auto instance_index = 1U;
  static constexpr auto transfer_mode_value = Mode;
  static constexpr auto base_address = Base;
  static constexpr auto source_clock_hz = 16000000U;
  static constexpr auto baud_rate = 115200U, clock_hz = 100000U, timeout_ms = 10U, timing = 0x00303D5BU;
  static constexpr auto parity = uart_parity::none;
  static constexpr auto stop_bits = uart_stop_bits::one;
  static constexpr auto duplex_mode = Direction;
  static constexpr auto clock_polarity = spi_clock_polarity::idle_low;
  static constexpr auto clock_phase = spi_clock_phase::first_edge;
  static constexpr auto bit_order = spi_bit_order::msb_first;
  static constexpr auto address_mode = i2c_address_mode::seven_bit;
  static constexpr auto own_address = 0x50U;
  static constexpr bool is_slave = Slave;
  static constexpr bool enable_dma = Mode == transfer_mode::dma;
  static constexpr bool enable_interrupts = Mode == transfer_mode::interrupt;
  static constexpr bool require_fifo = false, require_lpuart = false;
};
template <peripheral_kind Kind> struct clock_binding
{
  static auto* instance() noexcept
  {
    if constexpr (Kind == peripheral_kind::uart)
      return USART1;
    else if constexpr (Kind == peripheral_kind::spi)
      return SPI1;
    else
      return I2C1;
  }
  static auto timing() noexcept
  {
    return 0x00303D5BU;
  }
  static std::uint32_t now_ms() noexcept
  {
    return 0U;
  }
  static std::uint32_t source_clock_hz() noexcept
  {
    return 16000000U;
  }
  static auto tx_dma() noexcept
  {
    using namespace backends::stm32::ll;
#if defined(DMA_CTR1_SDW_LOG2)
    static serial_gpdma_node node{};
    return gpdma(*GPDMA1_Channel0, &node);
#elif defined(DMA1_Stream0)
    return stream_dma(*DMA1_Stream0, DMA1->LISR, DMA1->LIFCR, 0U);
#else
    return channel_dma(*DMA1_Channel1, *DMA1, 1U);
#endif
  }
  static auto rx_dma() noexcept
  {
    using namespace backends::stm32::ll;
#if defined(DMA_CTR1_SDW_LOG2)
    static serial_gpdma_node node{};
    return gpdma(*GPDMA1_Channel1, &node);
#elif defined(DMA1_Stream0)
    return stream_dma(*DMA1_Stream1, DMA1->LISR, DMA1->LIFCR, 6U);
#else
    return channel_dma(*DMA1_Channel2, *DMA1, 2U);
#endif
  }
};
} // namespace
namespace microhal::backends::stm32::ll
{
template <> struct serial_binding<peripheral_kind::uart, 1> : clock_binding<peripheral_kind::uart>
{
};
template <> struct serial_binding<peripheral_kind::spi, 1> : clock_binding<peripheral_kind::spi>
{
};
template <> struct serial_binding<peripheral_kind::i2c, 1> : clock_binding<peripheral_kind::i2c>
{
};
} // namespace microhal::backends::stm32::ll

template <microhal::transfer_mode Mode, microhal::spi_duplex_mode Direction, bool Slave>
void instantiate_spi_direction()
{
  using namespace microhal;
  using spi = serial_mode_api<stm32_ll_tag, peripheral_kind::spi>;
  using c = config<Mode, SPI1_BASE, Slave, Direction>;
  std::uint8_t tx[2]{}, rx[2]{};
  (void)spi::initialize<c>();
  (void)spi::write<c, completion>(tx);
  (void)spi::read<c, completion>(rx);
  (void)spi::transfer<c, completion>(tx, rx);
  (void)spi::handle_interrupt<c>();
  (void)spi::handle_dma_interrupt<c>();
  (void)spi::abort<c>();
#if defined(MICROHAL_LL_PUBLIC_FACADES)
  using public_master = microhal::spi::master<
      stm32_ll_tag,
      microhal::spi::master_config<1, 1000000, Direction, spi_clock_polarity::idle_low, spi_clock_phase::first_edge,
                                   spi_bit_order::msb_first, false, false, 10, Mode>>;
  using public_slave = microhal::spi::slave<
      stm32_ll_tag, microhal::spi::slave_config<1, Direction, spi_clock_polarity::idle_low, spi_clock_phase::first_edge,
                                                spi_bit_order::msb_first, false, false, 10, Mode>>;
  using public_bus = std::conditional_t<Slave, public_slave, public_master>;
  (void)public_bus::initialize();
  (void)public_bus::template write_async<completion>(tx);
  (void)public_bus::template read_async<completion>(rx);
  (void)public_bus::abort();
#endif
}

template <microhal::transfer_mode Mode> void instantiate_native_ll()
{
  using namespace microhal;
  using uart = serial_mode_api<stm32_ll_tag, peripheral_kind::uart>;
  using spi = serial_mode_api<stm32_ll_tag, peripheral_kind::spi>;
  using i2c = serial_mode_api<stm32_ll_tag, peripheral_kind::i2c>;
  using u = config<Mode, USART1_BASE>;
  using s = config<Mode, SPI1_BASE>;
  using i = config<Mode, I2C1_BASE>;
  using ss = config<Mode, SPI1_BASE, true>;
  using is = config<Mode, I2C1_BASE, true>;
  std::uint8_t tx[4]{}, rx[4]{};
  instantiate_spi_direction<Mode, spi_duplex_mode::half, false>();
  instantiate_spi_direction<Mode, spi_duplex_mode::half, true>();
  instantiate_spi_direction<Mode, spi_duplex_mode::simplex_rx, false>();
  instantiate_spi_direction<Mode, spi_duplex_mode::simplex_rx, true>();
  instantiate_spi_direction<Mode, spi_duplex_mode::simplex_tx, false>();
  instantiate_spi_direction<Mode, spi_duplex_mode::simplex_tx, true>();
  (void)uart::initialize<u>();
  (void)uart::write<u, completion>(tx);
  (void)uart::read<u, completion>(rx);
  (void)uart::start_receive_to_idle<u, range_completion>(rx);
  (void)uart::start_circular_rx<u, range_completion>(rx);
  (void)uart::abort_tx<u>();
  (void)uart::abort_rx<u>();
  (void)uart::abort<u>();
  (void)uart::tx_status<u>();
  (void)uart::rx_status<u>();
  (void)uart::handle_interrupt<u>();
  (void)uart::handle_dma_interrupt<u>();
  (void)spi::initialize<s>();
  (void)spi::initialize<ss>();
  (void)spi::transfer<s, completion>(tx, rx);
  (void)spi::transfer<ss, completion>(tx, rx);
  (void)spi::read<s, completion>(rx);
  (void)spi::write<s, completion>(tx);
  (void)spi::abort<s>();
  (void)spi::handle_interrupt<s>();
  (void)spi::handle_dma_interrupt<s>();
  (void)i2c::initialize<i>();
  (void)i2c::initialize<is>();
  (void)i2c::read<i, completion>(rx, 0x50);
  (void)i2c::write<i, completion>(tx, 0x50);
  (void)i2c::write_read<i, completion>(tx, rx, 0x50);
  (void)i2c::read<is, completion>(rx);
  (void)i2c::write<is, completion>(tx);
  (void)i2c::abort<i>();
  (void)i2c::probe<i>(0x50);
  (void)i2c::handle_interrupt<i>();
  (void)i2c::handle_dma_interrupt<i>();
#if defined(MICROHAL_LL_PUBLIC_FACADES)
  using port = microhal::uart::port<stm32_ll_tag, u>;
  using spi_master = microhal::spi::master<stm32_ll_tag, s>;
  using i2c_master = microhal::i2c::master<stm32_ll_tag, i>;
  using spi_slave = microhal::spi::slave<stm32_ll_tag, ss>;
  using i2c_slave = microhal::i2c::slave<stm32_ll_tag, is>;
  (void)port::initialize();
  (void)port::write_blocking(tx);
  (void)port::read_blocking(rx);
  (void)port::template start_async_tx<completion>(tx);
  (void)port::template start_async_rx<completion>(rx);
  (void)port::template start_receive_to_idle<range_completion>(rx);
  (void)port::template start_circular_rx<range_completion>(rx);
  (void)port::handle_dma_interrupt();
  (void)spi_master::initialize();
  (void)spi_master::transfer(tx, rx);
  (void)spi_master::template transfer_async<completion>(tx, rx);
  (void)spi_slave::template transfer_async<completion>(tx, rx);
  (void)i2c_master::initialize();
  (void)i2c_master::write_read_blocking(0x50, tx, rx);
  (void)i2c_master::template write_read_async<completion>(0x50, tx, rx);
  (void)i2c_slave::template transmit_async<completion>(tx);
  (void)i2c_slave::template receive_async<completion>(rx);
  using final_port =
      microhal::uart::port<stm32_ll_tag, microhal::uart::port_config<1, 115200, uart_parity::none, uart_stop_bits::one,
                                                                     false, false, false, false, 10, Mode>>;
  using final_spi = microhal::spi::master<
      stm32_ll_tag,
      microhal::spi::master_config<1, 1000000, spi_duplex_mode::full, spi_clock_polarity::idle_low,
                                   spi_clock_phase::first_edge, spi_bit_order::msb_first, false, false, 10, Mode>>;
  using final_i2c = microhal::i2c::master<
      stm32_ll_tag, microhal::i2c::master_config<1, 100000, i2c_address_mode::seven_bit, false, false, 10, Mode>>;
  using final_spi_slave = microhal::spi::slave<
      stm32_ll_tag,
      microhal::spi::slave_config<1, spi_duplex_mode::full, spi_clock_polarity::idle_low, spi_clock_phase::first_edge,
                                  spi_bit_order::msb_first, false, false, 10, Mode>>;
  using final_i2c_slave = microhal::i2c::slave<
      stm32_ll_tag, microhal::i2c::slave_config<1, 0x50, i2c_address_mode::seven_bit, 100000, false, false, 10, Mode>>;
  (void)final_port::initialize();
  (void)final_port::write_blocking(tx);
  (void)final_port::read_blocking(rx);
  (void)final_spi::initialize();
  (void)final_spi::transfer(tx, rx);
  (void)final_spi::write_blocking(tx);
  (void)final_spi::read_blocking(rx);
  (void)final_i2c::initialize();
  (void)final_i2c::write_read_blocking(0x50, tx, rx);
  (void)final_spi_slave::initialize();
  (void)final_spi_slave::template transfer_async<completion>(tx, rx);
  (void)final_i2c_slave::initialize();
  (void)final_i2c_slave::template receive_async<completion>(rx);
#endif
}
template void instantiate_native_ll<microhal::transfer_mode::polled>();
template void instantiate_native_ll<microhal::transfer_mode::interrupt>();
template void instantiate_native_ll<microhal::transfer_mode::dma>();
#if defined(MICROHAL_LL_PUBLIC_FACADES)
template void instantiate_native_ll<microhal::transfer_mode::automatic>();
#endif
#endif
