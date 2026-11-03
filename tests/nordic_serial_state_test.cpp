// SPDX-License-Identifier: Apache-2.0
#include "nordic_serial_driver_fake.hpp"
#include <cassert>

namespace
{
using namespace microhal;
using namespace microhal::backends::nordic::nrfx;
namespace detail = microhal::backends::nordic::nrfx::detail;
inline nrfx_timer_t timeout_timer{};
inline nrfx_gppi_t gppi{};
struct pins
{
  static constexpr auto sck_pin = 1U, mosi_pin = 2U, miso_pin = 3U, ss_pin = 4U, scl_pin = 5U, sda_pin = 6U;
  static constexpr auto rx_timeout_timer = &timeout_timer;
  static constexpr auto rx_timeout_gppi = &gppi;
  static constexpr auto rx_timeout_us = 100U;
};
struct config
{
  using binding = pins;
  static constexpr auto instance_index = 0U;
  static constexpr auto duplex_mode = spi_duplex_mode::full;
  static constexpr auto clock_polarity = spi_clock_polarity::idle_low;
  static constexpr auto clock_phase = spi_clock_phase::first_edge;
  static constexpr auto bit_order = spi_bit_order::msb_first;
  static constexpr auto address_mode = i2c_address_mode::seven_bit;
  static constexpr auto own_address = 0x42U;
};
struct slave_config : config
{
  static constexpr bool is_slave = true;
};
unsigned completions{}, receives{};
status last_result{};
std::span<std::uint8_t> last_bytes{};
std::array<std::uint8_t, 16> rearm_buffer{};
unsigned tx_rearms{}, rx_rearms{};
status rx_rearm_result{};
void completed(status value) noexcept
{
  ++completions;
  last_result = value;
}
void received(status value, std::span<std::uint8_t> bytes) noexcept
{
  ++receives;
  last_result = value;
  last_bytes = bytes;
}
void rearm_tx(status value) noexcept
{
  if (value == status::error)
  {
    ++tx_rearms;
    auto& state = detail::uarte_states[0];
    assert(state.tx.begin(completed));
    state.tx_size = 4U;
    state.tx_cancelled = false;
  }
}
void rearm_rx(status value, std::span<std::uint8_t>) noexcept
{
  if (value == status::error)
  {
    ++rx_rearms;
    rx_rearm_result = uart_api::start_receive_to_idle<config, received>(rearm_buffer);
  }
}
void trigger_tx_done()
{
  nrfx_uarte_event_t event{NRFX_UARTE_EVT_TX_DONE, {}};
  event.data.tx.flags = NRFX_UARTE_TX_DONE_ABORTED;
  detail::uarte_handler(&event, &detail::uarte_states[0]);
}
void trigger_rx_disabled()
{
  nrfx_uarte_event_t event{NRFX_UARTE_EVT_RX_DISABLED, {}};
  detail::uarte_handler(&event, &detail::uarte_states[0]);
}

struct flow_control_pins
{
  static constexpr bool hardware_flow_control = true;
  static constexpr auto rts_pin = 3U, cts_pin = 4U;
};
struct default_flow_pins
{
  static constexpr auto rts_pin = 3U, cts_pin = 4U;
};
struct disabled_flow_pins
{
  static constexpr bool hardware_flow_control = false;
  static constexpr auto rts_pin = UINT32_MAX;
};
struct missing_cts_pins
{
  static constexpr bool hardware_flow_control = true;
  static constexpr auto rts_pin = 3U;
};
struct missing_rts_pins
{
  static constexpr bool hardware_flow_control = true;
  static constexpr auto cts_pin = 4U;
};
struct invalid_flow_pin : flow_control_pins
{
  static constexpr auto cts_pin = 48U;
};
struct disconnected_flow_pin : flow_control_pins
{
  static constexpr auto cts_pin = UINT32_MAX;
};
struct duplicate_flow_pins : flow_control_pins
{
  static constexpr auto cts_pin = rts_pin;
};
struct overlapping_flow_pin : flow_control_pins
{
  static constexpr auto rts_pin = 1U;
};
struct negative_flow_pin : flow_control_pins
{
  static constexpr auto rts_pin = -1;
};
struct oversized_flow_pin : flow_control_pins
{
  static constexpr auto rts_pin = std::uint64_t{UINT32_MAX} + 1U;
};
struct nonintegral_flow_pin : flow_control_pins
{
  static constexpr auto rts_pin = 3.5;
};
struct boolean_flow_pin : flow_control_pins
{
  static constexpr auto cts_pin = true;
};
struct overlapping_receive_pin : flow_control_pins
{
  static constexpr auto cts_pin = 2U;
};
struct untyped_flow_flag : flow_control_pins
{
  static constexpr auto hardware_flow_control = 1U;
};
template <typename Pins> struct flow_config
{
  using binding = Pins;
};

template <typename Pins> void check_flow_configuration(status expected, bool enabled = false)
{
  register_block block{};
  nrfx_uarte_t instance{&block, {}};
  nrfx_uarte_config_t native{};
  native.txd_pin = 1U;
  native.rxd_pin = 2U;
  native.config.hwfc = NRF_UARTE_HWFC_ENABLED;
  const auto before = fake::uart_initializations;
  assert(detail::uarte_initialize_driver<flow_config<Pins>>(instance, native, detail::uarte_handler) == expected);
  if (expected == status::ok)
  {
    assert(fake::uart_initializations == before + 1U && instance.cb.initialized);
    assert(fake::uart_configuration.config.hwfc == (enabled ? NRF_UARTE_HWFC_ENABLED : NRF_UARTE_HWFC_DISABLED));
    assert(fake::uart_configuration.rts_pin == (enabled ? 3U : NRF_UARTE_PSEL_DISCONNECTED));
    assert(fake::uart_configuration.cts_pin == (enabled ? 4U : NRF_UARTE_PSEL_DISCONNECTED));
    detail::serial_release(&block, 3U);
  }
  else
  {
    assert(fake::uart_initializations == before && !instance.cb.initialized);
    assert(detail::serial_claim(&block, 2U) == status::ok);
    detail::serial_release(&block, 2U);
  }
}

void uart_flow_control()
{
  check_flow_configuration<flow_control_pins>(status::ok, true);
  check_flow_configuration<default_flow_pins>(status::ok);
  check_flow_configuration<disabled_flow_pins>(status::ok);
  check_flow_configuration<pins>(status::ok);
  check_flow_configuration<missing_cts_pins>(status::invalid_argument);
  check_flow_configuration<missing_rts_pins>(status::invalid_argument);
  check_flow_configuration<invalid_flow_pin>(status::invalid_argument);
  check_flow_configuration<disconnected_flow_pin>(status::invalid_argument);
  check_flow_configuration<duplicate_flow_pins>(status::invalid_argument);
  check_flow_configuration<overlapping_flow_pin>(status::invalid_argument);
  check_flow_configuration<negative_flow_pin>(status::invalid_argument);
  check_flow_configuration<oversized_flow_pin>(status::invalid_argument);
  check_flow_configuration<nonintegral_flow_pin>(status::invalid_argument);
  check_flow_configuration<boolean_flow_pin>(status::invalid_argument);
  check_flow_configuration<overlapping_receive_pin>(status::invalid_argument);
  check_flow_configuration<untyped_flow_flag>(status::invalid_argument);

  register_block block{};
  nrfx_uarte_t instance{&block, {}};
  nrfx_uarte_config_t native{};
  native.txd_pin = 1U;
  native.rxd_pin = 2U;
  fake::uart_init_error = -EBUSY;
  assert((detail::uarte_initialize_driver<flow_config<flow_control_pins>>(instance, native, detail::uarte_handler) ==
          status::busy));
  assert(detail::serial_claim(&block, 2U) == status::ok);
  detail::serial_release(&block, 2U);
  fake::uart_init_error = 0;
}

void uart_lifecycle()
{
  std::array<std::uint8_t, 16> bytes{};
  auto& state = detail::uarte_states[0];
  state.instance = &detail::uarte_instances[0];
  assert(state.tx.begin(completed));
  state.tx_size = 4U;
  assert(state.rx.begin(completed));
  state.rx_size = 8U;
  fake::tx_abort_error = -EBUSY;
  assert(uart_api::abort_tx<config>() == status::busy);
  assert(state.tx.active && state.rx.active);
  fake::tx_abort_error = 0;
  assert(uart_api::abort_tx<config>() == status::ok);
  assert(!state.tx.active && state.rx.active);
  assert(completions == 1U && last_result == status::error);
  nrfx_uarte_event_t event{NRFX_UARTE_EVT_RX_DONE, {}};
  event.data.rx = {bytes.data(), 3U};
  detail::uarte_handler(&event, &state);
  assert(state.rx.active && completions == 1U);
  event.type = NRFX_UARTE_EVT_RX_DISABLED;
  detail::uarte_handler(&event, &state);
  assert(!state.rx.active && completions == 2U && last_result == status::io_error);
  detail::uarte_handler(&event, &state);
  assert(completions == 2U);

  assert((uart_api::start_receive_to_idle<config, received>(bytes) == status::ok));
  assert(timeout_timer.cb.initialized);
  assert((uart_api::start_receive_to_idle<config, received>(bytes) == status::busy));
  fake::timer_handler(NRF_TIMER_EVENT_COMPARE0, fake::timer_context);
  assert(fake::disable_all && !fake::sync_abort);
  event.type = NRFX_UARTE_EVT_RX_DONE;
  event.data.rx = {bytes.data(), 5U};
  detail::uarte_handler(&event, &state);
  assert(receives == 0U);
  event.type = NRFX_UARTE_EVT_RX_DISABLED;
  detail::uarte_handler(&event, &state);
  assert(receives == 1U && last_result == status::ok && last_bytes.size() == 5U);
  assert(!timeout_timer.cb.initialized && fake::release_calls == 1);
  assert(uart_api::rx_status<config>() == status::ok);

  assert((uart_api::start_circular_rx<config, received>(bytes) == status::ok));
  event.type = NRFX_UARTE_EVT_RX_BUF_REQUEST;
  detail::uarte_handler(&event, &state);
  assert(fake::queued == bytes.data() + 8U && fake::queued_size == 8U);
  event.type = NRFX_UARTE_EVT_RX_DONE;
  event.data.rx = {bytes.data(), 8U};
  detail::uarte_handler(&event, &state);
  assert(receives == 2U && last_bytes.size() == 8U && state.rx.active);
  fake::timer_handler(NRF_TIMER_EVENT_COMPARE0, fake::timer_context);
  assert(!fake::disable_all);
  event.type = NRFX_UARTE_EVT_RX_BUF_REQUEST;
  detail::uarte_handler(&event, &state);
  assert(fake::queued == bytes.data());
  fake::rx_abort_error = -EBUSY;
  assert(uart_api::abort_rx<config>() == status::busy && state.rx.active);
  fake::rx_abort_error = 0;
  assert(uart_api::abort_rx<config>() == status::ok);
  assert(receives == 3U && last_result == status::error && !state.rx.active);

  fake::attach_error = -EINVAL;
  assert((uart_api::start_receive_to_idle<config, received>(bytes) == status::invalid_argument));
  assert(!state.rx.active && !timeout_timer.cb.initialized && receives == 3U);
  fake::attach_error = 0;
  assert((uart_api::start_receive_to_idle<config, received>(bytes) == status::ok));
  event.type = NRFX_UARTE_EVT_ERROR;
  detail::uarte_handler(&event, &state);
  assert(state.rx.active && state.rx_error);
  event.type = NRFX_UARTE_EVT_RX_DISABLED;
  detail::uarte_handler(&event, &state);
  assert(receives == 4U && last_result == status::io_error && !timeout_timer.cb.initialized);
  assert(state.tx.begin(completed));
  state.tx_size = 8U;
  state.tx_cancelled = false;
  event.type = NRFX_UARTE_EVT_TX_DONE;
  event.data.tx.length = 8U;
  event.data.tx.flags = NRFX_UARTE_TX_DONE_ABORTED;
  detail::uarte_handler(&event, &state);
  assert(last_result == status::error && !state.tx.active);

  assert(state.tx.begin(rearm_tx));
  state.tx_size = 8U;
  state.tx_cancelled = false;
  fake::tx_abort_hook = trigger_tx_done;
  assert(uart_api::abort_tx<config>() == status::ok);
  assert(tx_rearms == 1U && state.tx.active);

  assert((uart_api::start_receive_to_idle<config, rearm_rx>(bytes) == status::ok));
  const auto releases_before_abort = fake::release_calls;
  fake::rx_abort_hook = trigger_rx_disabled;
  assert(uart_api::abort_rx<config>() == status::ok);
  assert(rx_rearms == 1U && rx_rearm_result == status::ok);
  assert(state.rx.active && timeout_timer.cb.initialized);
  assert(fake::release_calls == releases_before_abort + 1U);
  trigger_rx_disabled();
  assert(!state.rx.active && !timeout_timer.cb.initialized);
  assert(fake::release_calls == releases_before_abort + 2U);
}

void slaves()
{
  std::array<std::uint8_t, 8> bytes{};
  assert(spi_api::initialize_slave<slave_config>() == status::ok);
  assert((spi_api::slave_transfer<slave_config, completed>(bytes, bytes) == status::ok));
  assert((spi_api::slave_transfer<slave_config, completed>(bytes, bytes) == status::busy));
  nrfx_spis_event_t event{NRFX_SPIS_BUFFERS_SET_DONE, 0U, 0U};
  detail::spis_handler(&event, &detail::spis_states[0]);
  assert(spi_api::transfer_status<slave_config>() == status::busy);
  event = {NRFX_SPIS_XFER_DONE, 8U, 7U};
  detail::spis_handler(&event, &detail::spis_states[0]);
  assert(last_result == status::io_error);
  assert((spi_api::slave_transfer<slave_config, completed>(bytes, {}) == status::ok));
  assert(spi_api::abort<slave_config>() == status::ok);
  assert(last_result == status::error && !detail::spis_instances[0].cb.initialized);
  assert(spi_api::abort<slave_config>() == status::ok);
  assert(spi_api::initialize_slave<slave_config>() == status::ok);
  assert((spi_api::slave_transfer<slave_config, completed>(bytes, bytes) == status::ok));
  spis_errors = NRF_SPIS_STATUS_OVERFLOW;
  event = {NRFX_SPIS_XFER_DONE, 8U, 8U};
  detail::spis_handler(&event, &detail::spis_states[0]);
  assert(last_result == status::io_error);
  spis_errors = 0U;

  assert(i2c_api::initialize_slave<slave_config>() == status::ok);
  assert((i2c_api::slave_transfer<slave_config, completed>({}, bytes) == status::ok));
  nrfx_twis_event_t twis{NRFX_TWIS_EVT_WRITE_REQ, {}};
  detail::twis_handler<0>(&twis);
  assert(fake::prepare_calls == 1 && fake::slave_enabled);
  twis.type = NRFX_TWIS_EVT_WRITE_DONE;
  twis.data.rx_amount = 8U;
  const auto before_done = completions;
  detail::twis_handler<0>(&twis);
  assert(completions == before_done);
  detail::twis_complete(0U);
  assert(last_result == status::ok && !fake::slave_enabled);
  assert((i2c_api::slave_transfer<slave_config, completed>(bytes, {}) == status::ok));
  twis.type = NRFX_TWIS_EVT_WRITE_REQ;
  detail::twis_handler<0>(&twis);
  detail::twis_complete(0U);
  assert(last_result == status::invalid_argument && !fake::slave_enabled);
  assert((i2c_api::slave_transfer<slave_config, completed>({}, bytes) == status::ok));
  assert(i2c_api::abort<slave_config>() == status::ok);
  assert(last_result == status::error);
}
void resources()
{
  register_block block{};
  assert(detail::serial_claim(&block, 1U) == status::ok);
  assert(detail::serial_claim(&block, 2U) == status::busy);
  detail::serial_release(&block, 2U);
  assert(detail::serial_claim(&block, 2U) == status::busy);
  detail::serial_release(&block, 1U);
  assert(detail::serial_claim(&block, 2U) == status::ok);
  detail::serial_release(&block, 2U);
}
void i2c_master()
{
  std::array<std::uint8_t, 8> bytes{};
  assert((i2c_api::master_transfer<config, completed>(bytes, {}, 0x80U) == status::invalid_argument));
  assert((i2c_api::master_transfer<config, completed>(bytes, bytes, 0x42U) == status::ok));
  assert(fake::master_descriptor.type == NRFX_TWIM_XFER_TXRX);
  assert(fake::master_descriptor.primary_length == 8U && fake::master_descriptor.secondary_length == 8U);
  assert((i2c_api::master_transfer<config, completed>({}, bytes, 0x42U) == status::busy));
  const auto before_cancel = completions;
  assert(i2c_api::abort<config>() == status::ok);
  assert(completions == before_cancel + 1U && last_result == status::error && fake::master_enabled);
  assert(i2c_api::transfer_status<config>() == status::error);
  fake::buffer_error = -EINVAL;
  assert((i2c_api::master_transfer<config, completed>({}, bytes, 0x42U) == status::invalid_argument));
  assert(!detail::twim_states[0].active && completions == before_cancel + 1U);
  fake::buffer_error = 0;
  assert((i2c_api::master_transfer<config, completed>({}, bytes, 0x42U) == status::ok));
  assert(fake::master_descriptor.type == NRFX_TWIM_XFER_RX && fake::master_descriptor.p_primary_buf == bytes.data());
  detail::twim_states[0].finish(status::io_error);
  assert(last_result == status::io_error && i2c_api::transfer_status<config>() == status::io_error);
}
} // namespace
int main()
{
  uart_flow_control();
  uart_lifecycle();
  slaves();
  resources();
  i2c_master();
}
