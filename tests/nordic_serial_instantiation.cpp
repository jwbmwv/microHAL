// SPDX-License-Identifier: Apache-2.0
#include "microhal/backends/nordic/nrfx/traits.hpp"
#include "microhal/i2c/i2c.hpp"
#include "microhal/spi/spi.hpp"
#include "microhal/uart/uart.hpp"

#if MICROHAL_NORDIC_NATIVE_TARGET
namespace
{
using namespace microhal;
struct pins
{
  static constexpr std::uint32_t tx_pin = 1U;
  static constexpr std::uint32_t rx_pin = 2U;
  static constexpr std::uint32_t rts_pin = 3U;
  static constexpr std::uint32_t cts_pin = 4U;
  static constexpr std::uint32_t sck_pin = 5U;
  static constexpr std::uint32_t mosi_pin = 6U;
  static constexpr std::uint32_t miso_pin = 7U;
  static constexpr std::uint32_t ss_pin = 8U;
  static constexpr std::uint32_t scl_pin = 9U;
  static constexpr std::uint32_t sda_pin = 10U;
#if MICROHAL_NORDIC_HAS_SERIAL_TIMEOUT
  static inline nrfx_timer_t timer{
#if defined(NRF54L15_XXAA)
      NRF_TIMER00,
#elif defined(NRF54H20_XXAA)
      NRF_TIMER130,
#else
      NRF_TIMER0,
#endif
      {}};
  static constexpr nrfx_timer_t* rx_timeout_timer = &timer;
#if defined(NRF54H20_XXAA)
  static inline NRF_DPPIC_Type* rx_timeout_dppic = NRF_DPPIC133;
  static constexpr std::uint8_t rx_timeout_channel = 0U;
#else
  static inline nrfx_gppi_t gppi{};
  static constexpr nrfx_gppi_t* rx_timeout_gppi = &gppi;
#endif
  static constexpr std::uint32_t rx_timeout_us = 100U;
#endif
};
struct config
{
  using binding = pins;
  static constexpr std::uint8_t instance_index = 0U;
  static constexpr std::uint32_t baud_rate = 115200U;
  static constexpr auto parity = uart_parity::none;
  static constexpr auto stop_bits = uart_stop_bits::one;
  static constexpr std::uint32_t clock_hz = 100000U;
  static constexpr auto duplex_mode = spi_duplex_mode::full;
  static constexpr auto clock_polarity = spi_clock_polarity::idle_low;
  static constexpr auto clock_phase = spi_clock_phase::first_edge;
  static constexpr auto bit_order = spi_bit_order::msb_first;
  static constexpr auto address_mode = i2c_address_mode::seven_bit;
  static constexpr auto timeout_ms = 1U;
  static constexpr bool enable_dma = true;
  static constexpr bool enable_interrupts = false;
  static constexpr bool require_fifo = false;
  static constexpr bool require_lpuart = false;
};
template <transfer_mode Mode> struct uart_config : config
{
  static constexpr auto transfer_mode_value = Mode;
};
template <transfer_mode Mode> using spi_config = uart_config<Mode>;
template <transfer_mode Mode> using i2c_config = uart_config<Mode>;
struct spi_slave_config : config
{
  static constexpr bool is_slave = true;
  static constexpr auto transfer_mode_value = transfer_mode::dma;
};
struct i2c_slave_config : spi_slave_config
{
  static constexpr std::uint16_t own_address = 0x42U;
};
struct half_pins
{
  static constexpr auto sck_pin = 5U;
  static constexpr auto mosi_pin = 6U;
};
struct half_config : spi_config<transfer_mode::dma>
{
  using binding = half_pins;
  static constexpr auto duplex_mode = spi_duplex_mode::half;
};
struct simplex_config : spi_config<transfer_mode::dma>
{
  struct binding
  {
    static constexpr auto sck_pin = 5U;
    static constexpr auto miso_pin = 7U;
  };
  static constexpr auto duplex_mode = spi_duplex_mode::simplex_rx;
};
struct simplex_slave_config : spi_slave_config
{
  struct binding
  {
    static constexpr auto sck_pin = 5U;
    static constexpr auto mosi_pin = 6U;
    static constexpr auto ss_pin = 8U;
  };
  static constexpr auto duplex_mode = spi_duplex_mode::simplex_rx;
};
struct fast_uart_config : uart_config<transfer_mode::dma>
{
#if defined(NRF54H20_XXAA)
  static constexpr std::uint8_t instance_index = 8U;
  static constexpr auto baud_rate = 20'000'000U;
#else
  static constexpr auto baud_rate = 8'000'000U;
#endif
  static constexpr auto parity = uart_parity::odd;
  static constexpr auto stop_bits = uart_stop_bits::two;
};
struct flow_control_config : uart_config<transfer_mode::dma>
{
  struct binding : pins
  {
    static constexpr bool hardware_flow_control = true;
  };
};
struct disabled_flow_config : uart_config<transfer_mode::dma>
{
  struct binding : pins
  {
    static constexpr bool hardware_flow_control = false;
  };
};
struct missing_cts_config : uart_config<transfer_mode::dma>
{
  struct binding
  {
    static constexpr bool hardware_flow_control = true;
    static constexpr auto tx_pin = 1U, rx_pin = 2U, rts_pin = 3U;
  };
};
struct missing_rts_config : uart_config<transfer_mode::dma>
{
  struct binding
  {
    static constexpr bool hardware_flow_control = true;
    static constexpr auto tx_pin = 1U, rx_pin = 2U, cts_pin = 4U;
  };
};
struct invalid_flow_config : flow_control_config
{
  struct binding : flow_control_config::binding
  {
    static constexpr auto cts_pin = UINT32_MAX;
  };
};
struct malformed_flow_config : flow_control_config
{
  struct binding : flow_control_config::binding
  {
    static constexpr auto hardware_flow_control = 1U;
  };
};
struct negative_flow_config : flow_control_config
{
  struct binding : flow_control_config::binding
  {
    static constexpr auto rts_pin = -1;
  };
};
struct nonintegral_flow_config : flow_control_config
{
  struct binding : flow_control_config::binding
  {
    static constexpr auto cts_pin = 4.5;
  };
};
void completed(status) noexcept {}
void received(status, std::span<std::uint8_t>) noexcept {}
using uart_api = serial_mode_api<nordic_nrf_tag, peripheral_kind::uart>;
using spi_api = serial_mode_api<nordic_nrf_tag, peripheral_kind::spi>;
using i2c_api = serial_mode_api<nordic_nrf_tag, peripheral_kind::i2c>;
} // namespace

void nordic_serial_instantiate()
{
  std::array<std::uint8_t, 16> bytes{};
  using U = uart_config<transfer_mode::dma>;
  using S = spi_config<transfer_mode::dma>;
  using I = i2c_config<transfer_mode::dma>;
  (void)uart_api::initialize<U>();
  (void)uart_api::write<U, completed>(bytes);
  (void)uart_api::read<U, completed>(bytes);
  (void)uart_api::start_receive_to_idle<U, received>(bytes);
  (void)uart_api::start_circular_rx<U, received>(bytes);
  (void)uart_api::tx_status<U>();
  (void)uart_api::rx_status<U>();
  (void)uart_api::abort_tx<U>();
  (void)uart_api::abort_rx<U>();
  (void)uart_api::abort<U>();
  (void)uart_api::transfer_status<U>();
  (void)uart_api::handle_interrupt<U>();
  (void)uart_api::handle_dma_interrupt<U>();
  (void)uart_api::handle_rx_timeout_interrupt<U>();
  (void)uart_api::write<U, nullptr>(bytes);
  (void)uart_api::read<U, nullptr>(bytes);
  (void)uart_api::initialize<uart_config<transfer_mode::polled>>();
  (void)uart_api::initialize<uart_config<transfer_mode::interrupt>>();
  (void)uart_api::initialize<fast_uart_config>();
  (void)uart_api::read<uart_config<transfer_mode::polled>, nullptr>(bytes);
  (void)spi_api::initialize<S>();
  (void)spi_api::transfer<S, completed>(bytes, bytes);
  (void)spi_api::write<S, completed>(bytes);
  (void)spi_api::read<S, completed>(bytes);
  (void)spi_api::transfer<S, nullptr>(bytes, bytes);
  (void)spi_api::abort<S>();
  (void)spi_api::transfer_status<S>();
  (void)spi_api::handle_interrupt<S>();
  (void)spi_api::handle_dma_interrupt<S>();
  (void)spi_api::initialize<spi_config<transfer_mode::polled>>();
  (void)spi_api::transfer<spi_config<transfer_mode::polled>, nullptr>(bytes, bytes);
  (void)spi_api::initialize<half_config>();
  (void)spi_api::write<half_config, completed>(bytes);
  (void)spi_api::read<half_config, completed>(bytes);
  (void)spi_api::initialize<simplex_config>();
  (void)spi_api::read<simplex_config, completed>(bytes);
  (void)spi_api::initialize<spi_slave_config>();
  (void)spi_api::transfer<spi_slave_config, completed>(bytes, bytes);
  (void)spi_api::transfer<spi_slave_config, nullptr>(bytes, bytes);
  (void)spi_api::abort<spi_slave_config>();
  (void)spi_api::transfer_status<spi_slave_config>();
  (void)spi_api::handle_interrupt<spi_slave_config>();
  (void)spi_api::initialize<simplex_slave_config>();
  (void)spi_api::read<simplex_slave_config, completed>(bytes);
  (void)i2c_api::initialize<I>();
  (void)i2c_api::write<I, completed>(bytes, 0x42U);
  (void)i2c_api::read<I, completed>(bytes, 0x42U);
  (void)i2c_api::write_read<I, completed>(bytes, bytes, 0x42U);
  (void)i2c_api::write_read<I, nullptr>(bytes, bytes, 0x42U);
  (void)i2c_api::probe<I>(0x42U);
  (void)i2c_api::abort<I>();
  (void)i2c_api::transfer_status<I>();
  (void)i2c_api::handle_interrupt<I>();
  (void)i2c_api::handle_dma_interrupt<I>();
  (void)i2c_api::initialize<i2c_config<transfer_mode::polled>>();
  (void)i2c_api::write_read<i2c_config<transfer_mode::polled>, nullptr>(bytes, bytes, 0x42U);
  (void)i2c_api::initialize<i2c_slave_config>();
  (void)i2c_api::write<i2c_slave_config, completed>(bytes);
  (void)i2c_api::read<i2c_slave_config, completed>(bytes);
  (void)i2c_api::abort<i2c_slave_config>();
  (void)i2c_api::transfer_status<i2c_slave_config>();
  (void)i2c_api::handle_interrupt<i2c_slave_config>();

  using public_uart = uart::port<nordic_nrf_tag, U>;
  (void)public_uart::initialize();
  (void)public_uart::start_async_tx<completed>(bytes);
  (void)public_uart::start_async_rx<completed>(bytes);
  (void)public_uart::start_receive_to_idle<received>(bytes);
  (void)public_uart::start_circular_rx<received>(bytes);
  (void)public_uart::tx_status();
  (void)public_uart::rx_status();
  (void)public_uart::abort_tx();
  (void)public_uart::abort_rx();
  (void)public_uart::transfer_status();
  (void)public_uart::abort();
  (void)public_uart::handle_interrupt();
  (void)public_uart::handle_dma_interrupt();
  (void)public_uart::handle_rx_timeout_interrupt();
  (void)uart::port<nordic_nrf_tag, flow_control_config>::initialize();
  (void)uart::port<nordic_nrf_tag, disabled_flow_config>::initialize();
  (void)uart::port<nordic_nrf_tag, missing_cts_config>::initialize();
  (void)uart::port<nordic_nrf_tag, missing_rts_config>::initialize();
  (void)uart::port<nordic_nrf_tag, invalid_flow_config>::initialize();
  (void)uart::port<nordic_nrf_tag, malformed_flow_config>::initialize();
  (void)uart::port<nordic_nrf_tag, negative_flow_config>::initialize();
  (void)uart::port<nordic_nrf_tag, nonintegral_flow_config>::initialize();

  using public_spi_slave = spi::slave<nordic_nrf_tag, spi_slave_config>;
  (void)public_spi_slave::initialize();
  (void)public_spi_slave::transmit_async<completed>(bytes);
  (void)public_spi_slave::receive_async<completed>(bytes);
  (void)public_spi_slave::transfer_async<completed>(bytes, bytes);
  (void)public_spi_slave::transfer(bytes, bytes);
  (void)public_spi_slave::write_blocking(bytes);
  (void)public_spi_slave::read_blocking(bytes);
  (void)public_spi_slave::transfer_status();
  (void)public_spi_slave::abort();
  (void)public_spi_slave::handle_interrupt();
  (void)public_spi_slave::handle_dma_interrupt();

  using public_i2c_slave = i2c::slave<nordic_nrf_tag, i2c_slave_config>;
  (void)public_i2c_slave::initialize();
  (void)public_i2c_slave::transmit_async<completed>(bytes);
  (void)public_i2c_slave::receive_async<completed>(bytes);
  (void)public_i2c_slave::transfer_status();
  (void)public_i2c_slave::abort();
  (void)public_i2c_slave::handle_interrupt();
  (void)public_i2c_slave::handle_dma_interrupt();
}
#endif
