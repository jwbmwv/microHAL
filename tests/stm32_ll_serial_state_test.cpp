// SPDX-License-Identifier: Apache-2.0
#include "stm32_ll_registers.hpp"

#include "microhal/backends/stm32/ll/serial_modes.hpp"
#include <array>
#include <cassert>

namespace
{
using namespace microhal;
using namespace microhal::backends::stm32::ll;
USART_TypeDef uart_registers{};
SPI_TypeDef spi_registers{};
I2C_TypeDef i2c_registers{};
struct dma_channel
{
  volatile std::uint32_t CCR{}, CNDTR{}, CPAR{}, CMAR{};
};
struct dma_controller
{
  volatile std::uint32_t ISR{}, IFCR{};
};
dma_channel dma_tx{}, dma_rx{};
dma_controller dma_registers{};
struct gpdma_channel
{
  volatile std::uint32_t CCR{}, CBR1{}, CSAR{}, CDAR{}, CSR{}, CFCR{}, CTR1{}, CTR2{}, CLBAR{}, CLLR{};
};
gpdma_channel gpdma_registers{};
serial_gpdma_node gpdma_node{};
std::uint32_t now{};
bool ticking{};
int tx_calls{}, rx_calls{}, bus_calls{}, range_calls{};
status tx_result{}, rx_result{}, bus_result{}, range_result{};
std::size_t range_size{};
std::uint8_t* range_begin{};
void tx_complete(status v) noexcept
{
  ++tx_calls;
  tx_result = v;
}
void rx_complete(status v) noexcept
{
  ++rx_calls;
  rx_result = v;
}
void bus_complete(status v) noexcept
{
  ++bus_calls;
  bus_result = v;
}
void range_complete(status v, std::span<std::uint8_t> data) noexcept
{
  ++range_calls;
  range_result = v;
  range_size = data.size();
  range_begin = data.data();
}
template <peripheral_kind Kind> struct board
{
  static auto source_clock_hz() noexcept
  {
    return 16000000U;
  }
  static auto now_ms() noexcept
  {
    if (ticking)
      ++now;
    return now;
  }
  static auto tx_dma() noexcept
  {
    return channel_dma(dma_tx, dma_registers, 1);
  }
  static auto rx_dma() noexcept
  {
    return channel_dma(dma_rx, dma_registers, 2);
  }
};
template <transfer_mode Mode = transfer_mode::interrupt> struct uart_config
{
  static constexpr auto instance_index = 1U, baud_rate = 115200U, timeout_ms = 10U;
  static constexpr auto parity = uart_parity::none;
  static constexpr auto stop_bits = uart_stop_bits::one;
  static constexpr auto transfer_mode_value = Mode;
  static auto* instance() noexcept
  {
    return &uart_registers;
  }
};
struct gpdma_uart_config : uart_config<transfer_mode::dma>
{
  static constexpr auto instance_index = 2U;
};
struct lpuart_config : uart_config<>
{
  static constexpr auto instance_index = 3U;
  static constexpr bool require_lpuart = true;
  static auto* instance() noexcept
  {
    return &ll_test_lpuart_registers;
  }
};
struct unsupported_lpuart_config : uart_config<>
{
  static constexpr bool require_lpuart = true;
};
struct unsupported_fifo_config : uart_config<>
{
  static constexpr bool require_fifo = true;
};
struct binding_only_uart_binding
{
  static constexpr std::uintptr_t base_address = 0x40004400U;
  static auto source_clock_hz() noexcept
  {
    return 16000000U;
  }
  static auto now_ms() noexcept
  {
    if (ticking)
      ++now;
    return now;
  }
};
struct binding_only_uart_config final
{
  using binding = binding_only_uart_binding;
  static constexpr auto baud_rate = 115200U, timeout_ms = 10U;
  static constexpr auto parity = uart_parity::none;
  static constexpr auto stop_bits = uart_stop_bits::one;
  static auto* instance() noexcept
  {
    return &uart_registers;
  }
};
template <transfer_mode Mode = transfer_mode::interrupt, bool Slave = false,
          spi_duplex_mode Direction = spi_duplex_mode::full>
struct spi_config
{
  static constexpr auto instance_index = 1U, clock_hz = 1000000U, timeout_ms = 10U;
  static constexpr auto transfer_mode_value = Mode;
  static constexpr auto duplex_mode = Direction;
  static constexpr auto clock_polarity = spi_clock_polarity::idle_high;
  static constexpr auto clock_phase = spi_clock_phase::second_edge;
  static constexpr auto bit_order = spi_bit_order::lsb_first;
  static constexpr bool is_slave = Slave;
  static auto* instance() noexcept
  {
    return &spi_registers;
  }
};
template <bool Transmit> struct one_way_spi_binding
{
  static auto source_clock_hz() noexcept
  {
    return 16000000U;
  }
  static auto now_ms() noexcept
  {
    return now;
  }
  static auto tx_dma() noexcept
    requires(Transmit)
  {
    return channel_dma(dma_tx, dma_registers, 1);
  }
  static auto rx_dma() noexcept
    requires(!Transmit)
  {
    return channel_dma(dma_rx, dma_registers, 2);
  }
};
template <bool Transmit>
struct one_way_spi_config
    : spi_config<transfer_mode::dma, false, Transmit ? spi_duplex_mode::simplex_tx : spi_duplex_mode::simplex_rx>
{
  using binding = one_way_spi_binding<Transmit>;
  static constexpr auto instance_index = Transmit ? 4U : 5U;
};
template <transfer_mode Mode = transfer_mode::interrupt, bool Slave = false> struct i2c_config
{
  static constexpr auto instance_index = 1U, clock_hz = 100000U, timeout_ms = 10U, timing = 0x00303D5BU;
  static constexpr auto transfer_mode_value = Mode;
  static constexpr auto address_mode = i2c_address_mode::seven_bit;
  static constexpr auto own_address = 0x42U;
  static constexpr bool is_slave = Slave;
  static auto* instance() noexcept
  {
    return &i2c_registers;
  }
};
#if defined(MICROHAL_TEST_I2C_LEGACY)
struct ten_bit_i2c_config : i2c_config<>
{
  static constexpr auto address_mode = i2c_address_mode::ten_bit;
};
#endif
} // namespace
namespace microhal::backends::stm32::ll
{
template <> struct serial_binding<peripheral_kind::uart, 1> : board<peripheral_kind::uart>
{
};
template <> struct serial_binding<peripheral_kind::spi, 1> : board<peripheral_kind::spi>
{
};
template <> struct serial_binding<peripheral_kind::i2c, 1> : board<peripheral_kind::i2c>
{
};
template <> struct serial_binding<peripheral_kind::uart, 2> : board<peripheral_kind::uart>
{
  static auto tx_dma() noexcept
  {
    return gpdma(gpdma_registers, &gpdma_node);
  }
  static auto rx_dma() noexcept
  {
    return tx_dma();
  }
};
template <> struct serial_binding<peripheral_kind::uart, 3> : board<peripheral_kind::uart>
{
};
} // namespace microhal::backends::stm32::ll

template <microhal::spi_duplex_mode Direction, microhal::transfer_mode Mode, bool Slave> void exercise_spi_direction()
{
  using spi = serial_mode_api<stm32_ll_tag, peripheral_kind::spi>;
  using config = spi_config<Mode, Slave, Direction>;
  std::uint8_t output[1]{0xA7}, input[1]{};
  spi_registers.SR = 0U;
  dma_tx.CCR = 0U;
  dma_rx.CCR = 0U;
  dma_registers.ISR = 0U;
  assert(spi::initialize<config>() == status::ok);
#if defined(MICROHAL_TEST_SPI_MODERN)
  const auto communication = Direction == spi_duplex_mode::half         ? SPI_CFG2_COMM
                             : Direction == spi_duplex_mode::simplex_tx ? SPI_CFG2_COMM_0
                                                                        : SPI_CFG2_COMM_1;
  assert((spi_registers.CFG2 & SPI_CFG2_COMM) == communication);
#else
  const auto communication = Direction == spi_duplex_mode::simplex_rx ? SPI_CR1_RXONLY : SPI_CR1_BIDIMODE;
  assert((spi_registers.CR1 & (SPI_CR1_RXONLY | SPI_CR1_BIDIMODE)) == communication);
#endif
  assert((spi::transfer<config, bus_complete>(output, input) == status::invalid_argument));
  if constexpr (Direction == spi_duplex_mode::simplex_tx)
    assert((spi::read<config, bus_complete>(input) == status::invalid_argument));
  if constexpr (Direction == spi_duplex_mode::simplex_rx)
    assert((spi::write<config, bus_complete>(output) == status::invalid_argument));
  if constexpr (Direction != spi_duplex_mode::simplex_rx)
  {
    const auto calls = bus_calls;
    assert((spi::write<config, bus_complete>(output) == status::ok));
    assert(!(dma_rx.CCR & 1U));
#if defined(MICROHAL_TEST_SPI_MODERN)
    if constexpr (Direction == spi_duplex_mode::half)
      assert(spi_registers.CR1 & SPI_CR1_HDDIR);
    spi_registers.SR = SPI_SR_TXP;
#else
    assert(spi_registers.CR1 & SPI_CR1_BIDIOE);
    spi_registers.SR = SPI_SR_TXE | SPI_SR_BSY;
#endif
    if constexpr (Mode == transfer_mode::dma)
    {
      dma_tx.CNDTR = 0U;
      dma_registers.ISR = 2U;
      (void)spi::handle_dma_interrupt<config>();
    }
    else
    {
      (void)spi::handle_interrupt<config>();
#if defined(MICROHAL_TEST_SPI_MODERN)
      assert(spi_registers.TXDR == output[0]);
#else
      assert(spi_registers.DR == output[0]);
#endif
    }
    assert(bus_calls == calls && spi::transfer_status<config>() == status::busy);
#if defined(MICROHAL_TEST_SPI_MODERN)
    spi_registers.SR = SPI_SR_TXP | SPI_SR_EOT;
#else
    spi_registers.SR = SPI_SR_TXE;
#endif
    (void)spi::handle_interrupt<config>();
    assert(bus_calls == calls + 1 && spi::transfer_status<config>() == status::ok);
  }
  if constexpr (Direction != spi_duplex_mode::simplex_tx)
  {
    const auto calls = bus_calls;
    spi_registers.SR = 0U;
    dma_registers.ISR = 0U;
    assert((spi::read<config, bus_complete>(input) == status::ok));
    assert(!(dma_tx.CCR & 1U));
#if defined(MICROHAL_TEST_SPI_MODERN)
    if constexpr (Direction == spi_duplex_mode::half)
      assert(!(spi_registers.CR1 & SPI_CR1_HDDIR));
    spi_registers.RXDR = 0x31U;
    spi_registers.SR = SPI_SR_RXP;
#else
    assert(!(spi_registers.CR1 & SPI_CR1_BIDIOE));
    spi_registers.DR = 0x31U;
    spi_registers.SR = SPI_SR_RXNE | SPI_SR_BSY;
#endif
    if constexpr (Mode == transfer_mode::dma)
    {
      input[0] = 0x31U;
      dma_rx.CNDTR = 0U;
      dma_registers.ISR = 2U << 4;
      (void)spi::handle_dma_interrupt<config>();
    }
    else
      (void)spi::handle_interrupt<config>();
    assert(bus_calls == calls && spi::transfer_status<config>() == status::busy);
#if defined(MICROHAL_TEST_SPI_MODERN)
    spi_registers.SR = SPI_SR_EOT;
#else
    if constexpr (!Slave)
      assert(!(spi_registers.CR1 & SPI_CR1_SPE));
    spi_registers.SR = SPI_SR_TXE;
#endif
    (void)spi::handle_interrupt<config>();
    assert(bus_calls == calls + 1 && input[0] == 0x31U && spi::transfer_status<config>() == status::ok);
    spi_registers.SR = 0U;
    dma_registers.ISR = 0U;
    assert((spi::read<config, bus_complete>(input) == status::ok));
#if defined(MICROHAL_TEST_SPI_MODERN)
    if constexpr (!Slave)
    {
      assert(spi::abort<config>() == status::io_error);
      assert((spi::read<config, bus_complete>(input) == status::busy));
      spi_registers.SR = SPI_SR_SUSP;
    }
#else
    spi_registers.SR = SPI_SR_BSY | (!Slave && Direction == spi_duplex_mode::simplex_rx ? SPI_SR_RXNE : 0U);
    assert(spi::abort<config>() == status::io_error);
    assert((spi::read<config, bus_complete>(input) == status::busy));
    spi_registers.SR = 0U;
#endif
    assert(spi::abort<config>() == status::ok);
    assert(!(spi_registers.CR1 & SPI_CR1_SPE) && !(dma_rx.CCR & 1U));
  }
}

template <microhal::transfer_mode Mode, bool Slave> void exercise_spi_directions()
{
  exercise_spi_direction<spi_duplex_mode::half, Mode, Slave>();
  exercise_spi_direction<spi_duplex_mode::simplex_rx, Mode, Slave>();
  exercise_spi_direction<spi_duplex_mode::simplex_tx, Mode, Slave>();
}
template <bool Transmit> void exercise_one_way_spi_binding()
{
  using spi = serial_mode_api<stm32_ll_tag, peripheral_kind::spi>;
  using config = one_way_spi_config<Transmit>;
  std::uint8_t data[1]{};
  spi_registers.SR = 0U;
  dma_tx.CCR = 0U;
  dma_rx.CCR = 0U;
  dma_registers.ISR = 0U;
  assert(spi::initialize<config>() == status::ok);
  if constexpr (Transmit)
    assert((spi::write<config, bus_complete>(data) == status::ok));
  else
    assert((spi::read<config, bus_complete>(data) == status::ok));
  assert(((Transmit ? dma_tx.CCR : dma_rx.CCR) & 1U) != 0U);
  assert(((Transmit ? dma_rx.CCR : dma_tx.CCR) & 1U) == 0U);
  dma_tx.CNDTR = 0U;
  dma_rx.CNDTR = 0U;
  dma_registers.ISR = Transmit ? 2U : 2U << 4;
#if defined(MICROHAL_TEST_SPI_MODERN)
  spi_registers.SR = SPI_SR_EOT;
#else
  spi_registers.SR = SPI_SR_TXE;
#endif
  (void)spi::handle_dma_interrupt<config>();
  assert(spi::transfer_status<config>() == status::ok);
}

int main()
{
  {
    gpdma_channel channel{};
    serial_gpdma_node node{};
    for (unsigned missing = 0U; missing < 5U; ++missing)
    {
      auto endpoint = gpdma(channel, &node);
      switch (missing)
      {
      case 0U:
        endpoint.request_control = nullptr;
        break;
      case 1U:
        endpoint.transfer_control = nullptr;
        break;
      case 2U:
        endpoint.circular_node = nullptr;
        break;
      case 3U:
        endpoint.link_base = nullptr;
        break;
      default:
        endpoint.link_address = nullptr;
        break;
      }
      assert(endpoint.start(0x40000000U, 0x20000000U, 8U, false, true) == status::unsupported);
      assert(channel.CCR == 0U && channel.CBR1 == 0U && channel.CFCR == 0U);
      assert(channel.CTR1 == 0U && channel.CTR2 == 0U && channel.CLLR == 0U);
    }
    auto endpoint = gpdma(channel);
    endpoint.request_control = nullptr;
    assert(endpoint.start(0x40000000U, 0x20000000U, 8U, false) == status::ok);
    assert(channel.CBR1 == 8U && (channel.CCR & 1U));
  }
  using uart = serial_mode_api<stm32_ll_tag, peripheral_kind::uart>;
#if !defined(MICROHAL_TEST_SPI_MODERN)
  using spi = serial_mode_api<stm32_ll_tag, peripheral_kind::spi>;
#endif
  using i2c = serial_mode_api<stm32_ll_tag, peripheral_kind::i2c>;
  using u = uart_config<>;
  using ud = uart_config<transfer_mode::dma>;
#if !defined(MICROHAL_TEST_SPI_MODERN)
  using s = spi_config<>;
  using sd = spi_config<transfer_mode::dma>;
#endif
  using i = i2c_config<>;
  using id = i2c_config<transfer_mode::dma>;
  std::uint8_t output[2]{0xA5, 0x5A}, input[4]{};
  assert(uart::initialize<u>() == status::ok);
  assert(uart_registers.BRR == 139U);
  assert(uart::initialize<lpuart_config>() == status::ok);
  assert(ll_test_lpuart_registers.BRR == 35556U && ll_test_lpuart_registers.PRESC == 0U);
  assert(uart::initialize<unsupported_lpuart_config>() == status::unsupported);
  assert(uart::initialize<unsupported_fifo_config>() == status::unsupported);
  assert(uart::initialize<binding_only_uart_config>() == status::ok);
  assert((uart::write<u, tx_complete>(output) == status::ok));
  assert((uart::read<u, rx_complete>(std::span{input, 2}) == status::ok));
  assert((uart::write<u, tx_complete>(output) == status::busy));
  assert(uart::initialize<u>() == status::busy);
  uart_registers.ISR = (1U << 7) | (1U << 5);
  uart_registers.RDR = 0x31;
  (void)uart::handle_interrupt<u>();
  assert(uart_registers.TDR == 0xA5U && input[0] == 0x31U);
  uart_registers.RDR = 0x32;
  (void)uart::handle_interrupt<u>();
  assert(rx_calls == 1 && rx_result == status::ok && tx_calls == 0);
  uart_registers.ISR = 1U << 6;
  (void)uart::handle_interrupt<u>();
  assert(tx_calls == 1 && tx_result == status::ok);
  assert((uart::write<u, tx_complete>(output) == status::ok));
  assert((uart::read<u, rx_complete>(input) == status::ok));
  uart_registers.ISR = 1U << 3; // ORE affects RX only.
  (void)uart::handle_interrupt<u>();
  assert(rx_calls == 2 && rx_result == status::io_error);
  assert(uart::tx_status<u>() == status::busy);
  assert(uart::abort_tx<u>() == status::ok && tx_calls == 2);
  assert((uart::write<ud, tx_complete>(output) == status::ok));
  dma_tx.CNDTR = 0U;
  dma_registers.ISR = 2U;
  uart_registers.ISR = 0U;
  (void)uart::handle_dma_interrupt<ud>();
  assert(uart::tx_status<ud>() == status::busy && tx_calls == 2);
  assert((uart_registers.CR1 & USART_CR1_TCIE) && !(uart_registers.CR3 & USART_CR3_DMAT));
  uart_registers.ISR = 1U << 6;
  (void)uart::handle_interrupt<ud>();
  assert(uart::tx_status<ud>() == status::ok && tx_calls == 3);
  dma_registers.ISR = 0U;
  assert((uart::start_receive_to_idle<u, range_complete>(input) == status::ok));
  uart_registers.ISR = 1U << 5;
  uart_registers.RDR = 9;
  (void)uart::handle_interrupt<u>();
  uart_registers.ISR = 1U << 4;
  (void)uart::handle_interrupt<u>();
  assert(range_calls == 1 && range_size == 1 && range_begin == input);
  assert((uart::start_receive_to_idle<ud, range_complete>(input) == status::ok));
  assert((dma_rx.CCR & 1U) != 0U && dma_rx.CNDTR == 4U);
  dma_rx.CNDTR = 2U;
  (void)uart::handle_interrupt<ud>();
  assert(range_calls == 2 && range_size == 2);
  assert((uart::start_circular_rx<ud, range_complete>(input) == status::ok));
  dma_registers.ISR = 4U << 4;
  (void)uart::handle_dma_interrupt<ud>();
  assert(range_size == 2 && range_begin == input);
  dma_registers.ISR = 2U << 4;
  (void)uart::handle_dma_interrupt<ud>();
  assert(range_size == 2 && range_begin == input + 2 && uart::rx_status<ud>() == status::busy);
  dma_registers.ISR = 8U << 4;
  (void)uart::handle_dma_interrupt<ud>();
  assert(range_result == status::io_error && range_size == 0 && uart::rx_status<ud>() == status::io_error);
  dma_registers.ISR = 0U;
  assert((uart::read<u, rx_complete>(input) == status::ok));
  now += 10;
  assert(uart::rx_status<u>() == status::timeout);
  assert((uart::read<gpdma_uart_config, rx_complete>(input) == status::ok));
  assert(uart::abort_rx<gpdma_uart_config>() == status::io_error);
  assert((uart::read<gpdma_uart_config, rx_complete>(input) == status::busy));
  // Hardware completes suspension/reset later; retry releases the original buffer once.
  gpdma_registers.CCR = 0U;
  assert(uart::abort_rx<gpdma_uart_config>() == status::ok);
  assert((uart::start_circular_rx<gpdma_uart_config, range_complete>(input) == status::ok));
  assert(gpdma_node.count == sizeof(input) && (gpdma_node.link & 0xF8010000U) == 0xF8010000U);
  assert(gpdma_registers.CLBAR ==
         (static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&gpdma_node)) & 0xFFFF0000U));
  assert(uart::abort_rx<gpdma_uart_config>() == status::io_error);
  gpdma_registers.CCR = 0U;
  assert(uart::abort_rx<gpdma_uart_config>() == status::ok);
  // Wrapping time is measured by unsigned subtraction.
  now = 0xFFFFFFFAU;
  assert((uart::read<u, rx_complete>(input) == status::ok));
  now = 5U;
  assert(uart::rx_status<u>() == status::timeout);
  ticking = true;
  assert((uart::read<uart_config<transfer_mode::polled>, rx_complete>(input) == status::timeout));
  assert((uart::read<u, nullptr>(input) == status::timeout));
  assert((uart::read<binding_only_uart_config, nullptr>(input) == status::timeout));
  assert((uart::read<gpdma_uart_config, nullptr>(input) == status::io_error));
  assert((uart::read<gpdma_uart_config, rx_complete>(input) == status::busy));
  gpdma_registers.CCR = 0U;
  assert(uart::abort_rx<gpdma_uart_config>() == status::ok);
  ticking = false;
#if !defined(MICROHAL_TEST_SPI_MODERN)
  assert(spi::initialize<s>() == status::ok);
  assert((spi_registers.CR1 & (SPI_CR1_CPOL | SPI_CR1_CPHA | SPI_CR1_LSBFIRST)) != 0U);
  assert((spi::transfer<s, bus_complete>(output, std::span{input, 1}) == status::invalid_argument));
  assert((spi::transfer<s, bus_complete>(output, std::span{input, 2}) == status::ok));
  spi_registers.SR = SPI_SR_TXE;
  (void)spi::handle_interrupt<s>();
  assert(spi_registers.DR == output[0]);
  spi_registers.DR = 0x11;
  spi_registers.SR = SPI_SR_RXNE | SPI_SR_TXE | SPI_SR_BSY;
  (void)spi::handle_interrupt<s>();
  spi_registers.DR = 0x22;
  spi_registers.SR = SPI_SR_RXNE;
  (void)spi::handle_interrupt<s>();
  assert(input[0] == 0x11 && input[1] == 0x22 && bus_calls == 1 && bus_result == status::ok);
  assert((spi::read<sd, bus_complete>(input) == status::ok));
  assert((dma_tx.CCR & 0x80U) == 0U); // Dummy TX must not walk memory.
  dma_tx.CNDTR = 0U;
  dma_rx.CNDTR = 0U;
  dma_registers.ISR = 2U | (2U << 4);
  spi_registers.SR = SPI_SR_BSY;
  (void)spi::handle_dma_interrupt<sd>();
  assert(spi::transfer_status<sd>() == status::busy && bus_calls == 1);
  spi_registers.SR = SPI_SR_TXE;
  (void)spi::handle_interrupt<sd>();
  assert(bus_calls == 2 && bus_result == status::ok);
  assert((spi::write<spi_config<transfer_mode::interrupt, true>, bus_complete>(output) == status::ok));
  assert(spi::abort<s>() == status::ok);
  assert((spi::write<s, bus_complete>(output) == status::ok));
  spi_registers.SR = SPI_SR_OVR;
  (void)spi::handle_interrupt<s>();
  assert(spi::transfer_status<s>() == status::io_error);
  using spi_slave = spi_config<transfer_mode::interrupt, true>;
  assert(spi::initialize<spi_slave>() == status::ok);
  assert(!(spi_registers.CR1 & SPI_CR1_MSTR));
  assert((spi::transfer<spi_slave, bus_complete>(std::span{output, 1}, std::span{input, 1}) == status::ok));
  spi_registers.SR = SPI_SR_TXE;
  (void)spi::handle_interrupt<spi_slave>();
  spi_registers.DR = 0x66;
  spi_registers.SR = SPI_SR_RXNE;
  (void)spi::handle_interrupt<spi_slave>();
  assert(spi::transfer_status<spi_slave>() == status::ok && input[0] == 0x66);
#endif
  exercise_spi_directions<transfer_mode::interrupt, false>();
  exercise_spi_directions<transfer_mode::interrupt, true>();
  exercise_spi_directions<transfer_mode::dma, false>();
  exercise_spi_directions<transfer_mode::dma, true>();
  exercise_one_way_spi_binding<true>();
  exercise_one_way_spi_binding<false>();
  assert(i2c::initialize<i>() == status::ok);
#if defined(MICROHAL_TEST_I2C_LEGACY)
  assert((i2c::write_read<i, bus_complete>(output, std::span{input, 2}, 0x52) == status::ok));
  i2c_registers.SR1 = I2C_SR1_SB;
  (void)i2c::handle_interrupt<i>();
  assert(i2c_registers.DR == 0xA4U);
  i2c_registers.SR1 = I2C_SR1_ADDR;
  (void)i2c::handle_interrupt<i>();
  i2c_registers.SR1 = I2C_SR1_TXE;
  (void)i2c::handle_interrupt<i>();
  (void)i2c::handle_interrupt<i>();
  i2c_registers.SR1 = I2C_SR1_BTF;
  (void)i2c::handle_interrupt<i>();
  assert(i2c_registers.CR1 & I2C_CR1_START);
  i2c_registers.SR1 = I2C_SR1_SB;
  (void)i2c::handle_interrupt<i>();
  assert(i2c_registers.DR == 0xA5U);
  i2c_registers.SR1 = I2C_SR1_ADDR;
  (void)i2c::handle_interrupt<i>();
  assert((i2c_registers.CR1 & I2C_CR1_ACK) == 0U && (i2c_registers.CR1 & I2C_CR1_POS));
  i2c_registers.SR1 = I2C_SR1_BTF;
  i2c_registers.DR = 0x77;
  (void)i2c::handle_interrupt<i>();
  assert(i2c::transfer_status<i>() == status::ok && input[0] == 0x77 && input[1] == 0x77);
  assert(i2c::initialize<i>() == status::ok);
  assert((i2c::read<i, bus_complete>(std::span{input, 1}, 0x52) == status::ok));
  i2c_registers.SR1 = I2C_SR1_ADDR;
  (void)i2c::handle_interrupt<i>();
  assert(!(i2c_registers.CR1 & (I2C_CR1_ACK | I2C_CR1_POS)));
  assert((i2c_registers.CR1 & I2C_CR1_STOP) && (i2c_registers.CR2 & I2C_CR2_ITBUFEN));
  i2c_registers.SR1 = I2C_SR1_RXNE;
  i2c_registers.DR = 0x11;
  (void)i2c::handle_interrupt<i>();
  assert(i2c::transfer_status<i>() == status::ok && input[0] == 0x11);
  for (const std::size_t length : {3U, 5U})
  {
    std::uint8_t buffer[5]{};
    assert(i2c::initialize<i>() == status::ok);
    assert((i2c::read<i, bus_complete>(std::span{buffer, length}, 0x52) == status::ok));
    i2c_registers.SR1 = I2C_SR1_ADDR;
    (void)i2c::handle_interrupt<i>();
    assert((i2c_registers.CR1 & I2C_CR1_ACK) && !(i2c_registers.CR1 & I2C_CR1_POS));
    for (std::size_t n = 0; n < length - 3U; ++n)
    {
      i2c_registers.SR1 = I2C_SR1_RXNE;
      i2c_registers.DR = 0x22;
      (void)i2c::handle_interrupt<i>();
    }
    assert(!(i2c_registers.CR2 & I2C_CR2_ITBUFEN));
    i2c_registers.SR1 = I2C_SR1_BTF;
    (void)i2c::handle_interrupt<i>();
    assert(!(i2c_registers.CR1 & I2C_CR1_ACK));
    (void)i2c::handle_interrupt<i>();
    assert(i2c::transfer_status<i>() == status::ok);
  }
  for (const std::size_t length : {1U, 2U, 4U})
  {
    assert(i2c::initialize<id>() == status::ok);
    assert((i2c::read<id, bus_complete>(std::span{input, length}, 0x52) == status::ok));
    assert((i2c_registers.CR2 & I2C_CR2_LAST) == (length > 1U ? I2C_CR2_LAST : 0U));
    i2c_registers.SR1 = I2C_SR1_ADDR;
    (void)i2c::handle_interrupt<id>();
    if (length == 1U)
      assert((i2c_registers.CR1 & I2C_CR1_STOP) && !(i2c_registers.CR1 & I2C_CR1_ACK));
    if (length == 2U)
      assert((i2c_registers.CR1 & I2C_CR1_POS) && !(i2c_registers.CR1 & I2C_CR1_ACK));
    dma_rx.CNDTR = 0U;
    dma_registers.ISR = 2U << 4;
    i2c_registers.SR1 = 0U;
    (void)i2c::handle_dma_interrupt<id>();
    assert(i2c::transfer_status<id>() == status::ok);
    assert(!(i2c_registers.CR2 & (I2C_CR2_LAST | I2C_CR2_DMAEN)));
  }
  assert(i2c::initialize<ten_bit_i2c_config>() == status::ok);
  assert((i2c::read<ten_bit_i2c_config, bus_complete>(std::span{input, 2}, 0x2A5) == status::ok));
  i2c_registers.SR1 = I2C_SR1_SB;
  (void)i2c::handle_interrupt<ten_bit_i2c_config>();
  assert(i2c_registers.DR == 0xF4U);
  i2c_registers.SR1 = I2C_SR1_ADD10;
  (void)i2c::handle_interrupt<ten_bit_i2c_config>();
  assert(i2c_registers.DR == 0xA5U);
  i2c_registers.SR1 = I2C_SR1_ADDR;
  (void)i2c::handle_interrupt<ten_bit_i2c_config>();
  assert(i2c_registers.CR1 & I2C_CR1_START);
  i2c_registers.SR1 = I2C_SR1_SB;
  (void)i2c::handle_interrupt<ten_bit_i2c_config>();
  assert(i2c_registers.DR == 0xF5U);
  i2c_registers.SR1 = I2C_SR1_ADDR;
  (void)i2c::handle_interrupt<ten_bit_i2c_config>();
  i2c_registers.SR1 = I2C_SR1_BTF;
  (void)i2c::handle_interrupt<ten_bit_i2c_config>();
  assert(i2c::transfer_status<ten_bit_i2c_config>() == status::ok);
  assert(i2c::initialize<i>() == status::ok);
  assert((i2c::write<i, bus_complete>(output, 0x52) == status::ok));
  i2c_registers.SR1 = I2C_SR1_AF;
  i2c_registers.SR2 = I2C_SR2_MSL;
  (void)i2c::handle_interrupt<i>();
  assert(i2c::transfer_status<i>() == status::io_error);
  assert(!(i2c_registers.CR2 & (I2C_CR2_ITEVTEN | I2C_CR2_ITERREN)));
  using slave = i2c_config<transfer_mode::interrupt, true>;
  assert(i2c::initialize<slave>() == status::ok);
  assert((i2c::read<slave, bus_complete>(input) == status::ok));
  i2c_registers.SR1 = I2C_SR1_STOPF;
  (void)i2c::handle_interrupt<slave>();
  assert(i2c::transfer_status<slave>() == status::io_error);
#else
  assert((i2c::write_read<i, bus_complete>(output, std::span{input, 2}, 0x52) == status::ok));
  assert((i2c_registers.CR2 & 0x3FFU) == 0xA4U && !(i2c_registers.CR2 & I2C_CR2_AUTOEND));
  i2c_registers.ISR = I2C_ISR_TXIS;
  (void)i2c::handle_interrupt<i>();
  (void)i2c::handle_interrupt<i>();
  assert(i2c_registers.TXDR == 0x5AU);
  i2c_registers.ISR = I2C_ISR_TC;
  (void)i2c::handle_interrupt<i>();
  assert((i2c_registers.CR2 & (I2C_CR2_START | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND)) ==
         (I2C_CR2_START | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND));
  i2c_registers.ISR = I2C_ISR_RXNE;
  i2c_registers.RXDR = 0x77;
  (void)i2c::handle_interrupt<i>();
  (void)i2c::handle_interrupt<i>();
  i2c_registers.ISR = I2C_ISR_STOPF;
  (void)i2c::handle_interrupt<i>();
  assert(i2c::transfer_status<i>() == status::ok && input[0] == 0x77);
  i2c_registers.ISR = 0;
  assert((i2c::read<i, bus_complete>(input, 0x52) == status::ok));
  i2c_registers.ISR = I2C_ISR_NACKF;
  (void)i2c::handle_interrupt<i>();
  assert(i2c::transfer_status<i>() == status::io_error);
  using slave = i2c_config<transfer_mode::interrupt, true>;
  assert(i2c::initialize<slave>() == status::ok);
  assert((i2c_registers.OAR1 & 0xFEU) == 0x84U);
  assert((i2c::read<slave, bus_complete>(std::span{input, 1}) == status::ok));
  i2c_registers.ISR = I2C_ISR_ADDR;
  (void)i2c::handle_interrupt<slave>();
  i2c_registers.ISR = I2C_ISR_RXNE;
  i2c_registers.RXDR = 0x88;
  (void)i2c::handle_interrupt<slave>();
  i2c_registers.ISR = I2C_ISR_STOPF;
  (void)i2c::handle_interrupt<slave>();
  assert(i2c::transfer_status<slave>() == status::ok && input[0] == 0x88);
  i2c_registers.ISR = 0U;
  assert((i2c::write<slave, bus_complete>(output) == status::ok));
  i2c_registers.ISR = I2C_ISR_ADDR | I2C_ISR_DIR;
  (void)i2c::handle_interrupt<slave>();
  i2c_registers.ISR = I2C_ISR_TXIS;
  (void)i2c::handle_interrupt<slave>();
  (void)i2c::handle_interrupt<slave>();
  i2c_registers.ISR = I2C_ISR_NACKF | I2C_ISR_STOPF;
  (void)i2c::handle_interrupt<slave>();
  assert(i2c::transfer_status<slave>() == status::ok);
  i2c_registers.ISR = 0U;
  assert((i2c::read<slave, bus_complete>(input) == status::ok));
  i2c_registers.ISR = I2C_ISR_STOPF;
  (void)i2c::handle_interrupt<slave>();
  assert(i2c::transfer_status<slave>() == status::io_error);
  i2c_registers.ISR = 0U;
  assert((i2c::write<id, bus_complete>(output, 0x52) == status::ok));
  dma_tx.CNDTR = 0;
  dma_registers.ISR = 2U;
  i2c_registers.ISR = 0;
  (void)i2c::handle_dma_interrupt<id>();
  assert(i2c::transfer_status<id>() == status::busy);
  i2c_registers.ISR = I2C_ISR_STOPF;
  (void)i2c::handle_interrupt<id>();
  assert(i2c::transfer_status<id>() == status::ok);
  i2c_registers.ISR = 0U;
  dma_registers.ISR = 0U;
  assert((i2c::write_read<id, bus_complete>(output, input, 0x52) == status::ok));
  dma_tx.CNDTR = 0U;
  dma_registers.ISR = 2U;
  (void)i2c::handle_dma_interrupt<id>();
  i2c_registers.ISR = I2C_ISR_TC;
  dma_registers.ISR = 0U;
  (void)i2c::handle_interrupt<id>();
  assert((i2c_registers.CR2 & I2C_CR2_RD_WRN) && dma_rx.CNDTR == sizeof(input));
  dma_rx.CNDTR = 0U;
  dma_registers.ISR = 2U << 4;
  i2c_registers.ISR = 0U;
  (void)i2c::handle_dma_interrupt<id>();
  i2c_registers.ISR = I2C_ISR_STOPF;
  (void)i2c::handle_interrupt<id>();
  assert(i2c::transfer_status<id>() == status::ok);
  // Reload does not generate an unwanted STOP or a second START.
  std::uint8_t long_output[260]{};
  i2c_registers.ISR = 0U;
  assert((i2c::write<i, bus_complete>(long_output, 0x52) == status::ok));
  assert((i2c_registers.CR2 & I2C_CR2_NBYTES) == 255U << 16);
  i2c_registers.ISR = I2C_ISR_TXIS;
  for (unsigned n = 0; n < 255U; ++n)
    (void)i2c::handle_interrupt<i>();
  i2c_registers.ISR = I2C_ISR_TCR;
  (void)i2c::handle_interrupt<i>();
  assert((i2c_registers.CR2 & I2C_CR2_NBYTES) == 5U << 16);
  assert(!(i2c_registers.CR2 & (I2C_CR2_START | I2C_CR2_RELOAD)));
  i2c_registers.ISR = I2C_ISR_TXIS;
  for (unsigned n = 0; n < 5U; ++n)
    (void)i2c::handle_interrupt<i>();
  i2c_registers.ISR = I2C_ISR_STOPF;
  (void)i2c::handle_interrupt<i>();
  assert(i2c::transfer_status<i>() == status::ok);
#endif
  return 0;
}
