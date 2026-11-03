// SPDX-License-Identifier: Apache-2.0
// Tests the production STM32 ownership/callback state machine without emulating registers.
#include "microhal/backends/mock/traits.hpp"
#include "microhal/backends/stm32/hal/traits.hpp"
#include "microhal/core/traits.hpp"
#include "microhal/i2c/i2c.hpp"
#include "microhal/i2s/i2s.hpp"
#include "microhal/spi/spi.hpp"
#include "microhal/uart/uart.hpp"

#include <array>
#include <cstdio>

enum HAL_StatusTypeDef
{
  HAL_OK,
  HAL_ERROR,
  HAL_BUSY,
  HAL_TIMEOUT
};
static std::uint32_t ticks{};
static void (*tick_hook)(){};
std::uint32_t HAL_GetTick()
{
  if (tick_hook != nullptr)
    tick_hook();
  return ticks++;
}

#define MICROHAL_STM32_HAL_HAS_I2C 0
#define MICROHAL_STM32_HAL_HAS_SPI 0
#define MICROHAL_STM32_HAL_HAS_UART 0
// namespace microhal::backends::stm32::hal
#include "microhal/backends/stm32/hal/serial_modes.hpp"

namespace fixture
{
using microhal::status;
struct backend
{
};
struct legacy_i2s_api
{
  template <typename Config> static bool initialize() noexcept
  {
    return true;
  }
  template <typename Config, auto Handler> static bool start_dma_tx(std::span<const std::uint16_t>) noexcept
  {
    return true;
  }
};
template <typename Capability> consteval auto with_modes(Capability capability)
{
  capability.transfer_modes = {true, true, true};
  capability.isr = true;
  capability.dma = true;
  return capability;
}
struct handle
{
  using callback = void (*)(handle*);
  callback tx{}, rx{}, duplex{}, error{};
  void* hdmatx{this};
  void* hdmarx{this};
};
template <microhal::peripheral_kind Kind> struct driver
{
  using handle_type = fixture::handle;
  static constexpr bool callbacks = true;
  static inline fixture::handle h{};
  static inline status submission = status::ok;
  static inline status error_value = status::io_error;
  static inline status stop_result = status::ok;
  static inline bool is_ready = true;
  static inline bool complete_inline = false;
  static inline int starts{}, stops{};
  static inline microhal::transfer_mode last_mode{};
  static inline bool last_first{}, last_last{}, last_receive{}, last_slave{};
  static inline std::uint16_t last_address{};
  template <typename Config> static auto& handle()
  {
    return h;
  }
  template <typename Config> static status initialize()
  {
    return status::ok;
  }
  static bool ready(handle_type&)
  {
    return is_ready;
  }
  static status error(handle_type&)
  {
    return error_value;
  }
  static auto& tx_callback(handle_type& value)
  {
    return value.tx;
  }
  static auto& rx_callback(handle_type& value)
  {
    return value.rx;
  }
  static auto& duplex_callback(handle_type& value)
  {
    return value.duplex;
  }
  static auto& error_callback(handle_type& value)
  {
    return value.error;
  }
  static status stop(handle_type& value)
  {
    ++stops;
    if (value.error != nullptr)
      value.error(&value);
    return stop_result;
  }
  template <typename Config, typename Sample>
  static status start(handle_type& value, std::span<const Sample>, std::span<Sample> rx, std::uint16_t address,
                      bool first, bool last)
  {
    ++starts;
    last_mode = microhal::configured_transfer_mode<Config>();
    last_first = first;
    last_last = last;
    last_receive = !rx.empty();
    last_slave = microhal::serial_slave_enabled<Config>();
    last_address = address;
    if (complete_inline && value.tx != nullptr)
      value.tx(&value);
    return submission;
  }
};
} // namespace fixture

namespace microhal::backends::stm32::hal
{
template <> struct serial_driver<peripheral_kind::spi> : fixture::driver<peripheral_kind::spi>
{
};
template <> struct serial_driver<peripheral_kind::i2c> : fixture::driver<peripheral_kind::i2c>
{
};
template <> struct serial_driver<peripheral_kind::uart> : fixture::driver<peripheral_kind::uart>
{
};
template <> struct serial_driver<peripheral_kind::i2s> : fixture::driver<peripheral_kind::i2s>
{
};
} // namespace microhal::backends::stm32::hal

namespace microhal
{
template <> struct backend_traits<fixture::backend>
{
  using backend_tag = fixture::backend;
  using i2s_api = fixture::legacy_i2s_api;
  static constexpr auto vendor = vendor_kind::mock;
  static constexpr auto kind = backend_kind::mock_recording;
  static constexpr std::string_view name = "serial-state-fixture";
  static consteval auto i2c_capability()
  {
    return fixture::with_modes(backend_traits<mock_tag>::i2c_capability());
  }
  static consteval auto spi_capability()
  {
    return fixture::with_modes(backend_traits<mock_tag>::spi_capability());
  }
  static consteval auto uart_capability()
  {
    return fixture::with_modes(backend_traits<mock_tag>::uart_capability());
  }
  static consteval auto i2s_capability()
  {
    auto capability = fixture::with_modes(backend_traits<mock_tag>::i2s_capability());
    capability.receive_dma = true;
    return capability;
  }
};
template <peripheral_kind Kind>
struct serial_mode_api<fixture::backend, Kind> : backends::stm32::hal::serial_engine<Kind>
{
};
} // namespace microhal

namespace
{
using microhal::peripheral_kind;
using microhal::status;
using microhal::transfer_mode;
template <transfer_mode Mode> struct config
{
  static constexpr auto transfer_mode_value = Mode;
  static constexpr auto timeout_ms = 3U;
};
using spi_engine = microhal::backends::stm32::hal::serial_engine<peripheral_kind::spi>;
using i2c_engine = microhal::backends::stm32::hal::serial_engine<peripheral_kind::i2c>;
using spi_driver = microhal::backends::stm32::hal::serial_driver<peripheral_kind::spi>;
using i2c_driver = microhal::backends::stm32::hal::serial_driver<peripheral_kind::i2c>;
using irq = config<transfer_mode::interrupt>;
using dma = config<transfer_mode::dma>;
using polled = config<transfer_mode::polled>;
struct dma_binding
{
  static inline status configured_result = status::ok;
  static inline int calls{};
  template <typename Handle> static status configure_dma(Handle&) noexcept
  {
    ++calls;
    return configured_result;
  }
};
struct bound_dma : dma
{
  using binding = dma_binding;
};
static_assert(microhal::serial_dma_enabled<dma>() && !microhal::serial_interrupt_enabled<dma>());
static_assert(!microhal::serial_dma_enabled<polled>() && !microhal::serial_interrupt_enabled<polled>());
static_assert(!microhal::serial_dma_enabled<irq>() && microhal::serial_interrupt_enabled<irq>());
int failures{}, callbacks{}, second_callbacks{}, original_callbacks{};
int facade_callbacks{};
status completion_status{};
void check(bool value, const char* text)
{
  if (!value)
  {
    ++failures;
    std::printf("FAIL: %s\n", text);
  }
}
void completed(status result) noexcept
{
  ++callbacks;
  completion_status = result;
}
void second(status) noexcept
{
  ++second_callbacks;
}
void original(fixture::handle*)
{
  ++original_callbacks;
}
void facade_completed(status) noexcept
{
  ++facade_callbacks;
}
void finish_from_tick()
{
  tick_hook = nullptr;
  spi_driver::error_value = status::ok;
  spi_driver::h.tx(&spi_driver::h);
}
} // namespace

int main()
{
  const std::array<std::uint8_t, 4> tx{};
  std::array<std::uint8_t, 4> rx{};
  check(spi_engine::write<irq, completed>(tx) == status::unsupported, "initialize required");
  check(spi_engine::initialize<irq>() == status::ok, "IRQ initialize");
  spi_driver::h.tx = original;
  spi_driver::h.rx = original;
  spi_driver::h.duplex = original;
  spi_driver::h.error = original;
  check(spi_engine::write<irq, completed>(tx) == status::ok, "IRQ submit");
  check(spi_driver::last_mode == transfer_mode::interrupt, "selected IRQ mode passed to driver");
  check(spi_engine::read<irq, second>(rx) == status::busy, "ownership spans handlers and directions");
  check(spi_engine::initialize<irq>() == status::busy, "reinitialize while active rejected");
  spi_driver::error_value = status::ok;
  spi_driver::h.tx(&spi_driver::h);
  check(callbacks == 1 && second_callbacks == 0 && completion_status == status::ok, "exact completion handler");
  check(spi_driver::h.tx == original && spi_driver::h.rx == original && spi_driver::h.duplex == original &&
            spi_driver::h.error == original,
        "all callbacks restored");
  check(original_callbacks == 0, "saved callbacks not spuriously invoked");

  check(spi_engine::initialize<polled>() == status::ok, "polled initialize");
  const auto starts = spi_driver::starts;
  check(spi_engine::write<polled, completed>(tx) == status::unsupported, "polled is not fake async");
  check(spi_driver::starts == starts, "unsupported async does not start hardware");
  check(spi_engine::write<polled, nullptr>(tx) == status::ok && spi_driver::last_mode == transfer_mode::polled,
        "blocking poll uses polling driver");
  check(spi_engine::initialize<dma>() == status::ok, "DMA initialize");
  check(spi_engine::read<dma, completed>(rx) == status::ok && spi_driver::last_mode == transfer_mode::dma &&
            spi_driver::last_receive,
        "DMA RX routing");
  check(spi_engine::abort<dma>() == status::ok && callbacks == 2 && completion_status != status::ok &&
            spi_driver::stops == 1,
        "abort quiesces before callback exactly once");
  check(spi_engine::read<dma, completed>(rx) == status::unsupported, "abort requires reinitialize");
  check(spi_engine::initialize<dma>() == status::ok, "reinitialize after abort");
  spi_driver::submission = status::io_error;
  check(spi_engine::write<dma, completed>(tx) == status::io_error && callbacks == 2 && spi_driver::stops == 2,
        "submission failure cleans up without callback");
  spi_driver::submission = status::ok;
  check(spi_engine::initialize<dma>() == status::ok, "reinitialize after failed submission");
  ticks = 0;
  check(spi_engine::write<dma, nullptr>(tx) == status::timeout && spi_driver::stops == 3 && callbacks == 2 &&
            spi_engine::transfer_status<dma>() == status::timeout,
        "bounded blocking DMA aborts before timeout return");
  check(spi_engine::initialize<irq>() == status::ok, "reinitialize after timeout");
  tick_hook = finish_from_tick;
  check(spi_engine::write<irq, nullptr>(tx) == status::ok, "blocking IRQ waits for completion");
  check(spi_engine::write<irq, completed>({}) == status::invalid_argument, "empty rejected");
  check(spi_engine::transfer<irq, completed>(tx, std::span{rx}.first(2)) == status::invalid_argument,
        "duplex size mismatch rejected");
  check(spi_engine::write<irq, completed>(tx) == status::ok, "error-path submit");
  spi_driver::error_value = status::io_error;
  spi_driver::h.error(&spi_driver::h);
  check(callbacks == 3 && completion_status == status::io_error && spi_driver::stops == 4,
        "error callback quiesces and reports failure");
  check(spi_engine::initialize<dma>() == status::ok, "initialize for failed-abort recovery");
  check(spi_engine::write<dma, completed>(tx) == status::ok && spi_engine::transfer_status<dma>() == status::busy,
        "active status exposed");
  spi_driver::stop_result = status::io_error;
  check(spi_engine::abort<dma>() == status::io_error && spi_engine::transfer_status<dma>() == status::io_error &&
            callbacks == 3,
        "failed abort reports error without releasing buffer or notifying completion");
  check(spi_engine::initialize<dma>() == status::busy, "failed abort retains ownership");
  spi_driver::stop_result = status::ok;
  check(spi_engine::abort<dma>() == status::ok && callbacks == 4,
        "retrying abort releases ownership only after driver stops");

  check(i2c_engine::initialize<irq>() == status::ok, "I2C initialize");
  check(i2c_engine::write_read<irq, completed>(tx, rx, 0x50) == status::ok && i2c_driver::last_first &&
            !i2c_driver::last_last,
        "combined first frame retains bus");
  i2c_driver::h.tx(&i2c_driver::h);
  check(callbacks == 4 && i2c_driver::starts == 2 && !i2c_driver::last_first && i2c_driver::last_last &&
            i2c_driver::last_receive && i2c_driver::last_address == 0x50,
        "combined receive starts only after transmit, no early callback");
  i2c_driver::error_value = status::ok;
  i2c_driver::h.rx(&i2c_driver::h);
  check(callbacks == 5 && completion_status == status::ok, "combined callback after receive");
  check(i2c_engine::write_read<irq, completed>(tx, rx, 0x50) == status::ok, "combined retry");
  i2c_driver::submission = status::io_error;
  i2c_driver::h.tx(&i2c_driver::h);
  check(callbacks == 6 && completion_status == status::io_error && i2c_driver::stops == 1,
        "second phase submission failure releases bus before callback");
  check(i2c_engine::initialize<dma>() == status::ok, "I2C DMA initialize");
  i2c_driver::h.hdmarx = nullptr;
  const auto i2c_starts = i2c_driver::starts;
  check(i2c_engine::write_read<dma, completed>(tx, rx, 0x50) == status::unsupported && i2c_driver::starts == i2c_starts,
        "combined DMA validates both resources before starting");
  check(microhal::transfer_mode_capability{true, false, true}.supports(transfer_mode::dma) &&
            !microhal::transfer_mode_capability{true, false, true}.supports(transfer_mode::interrupt),
        "capabilities distinguish DMA from interrupt and polling");
  std::array<std::uint8_t, 65536> large{};
  check(spi_engine::write<polled, nullptr>(large) == status::invalid_argument,
        "16-bit transfer counter rejects 65536 elements");
  check(spi_engine::write<polled, nullptr>(std::span{large}.first(65535)) == status::ok,
        "16-bit transfer counter accepts 65535 elements");

  using bus = microhal::i2c::master<fixture::backend,
                                    microhal::i2c::master_config<0, 100000, microhal::i2c_address_mode::seven_bit,
                                                                 false, false, 10, transfer_mode::polled>>;
  i2c_driver::submission = status::ok;
  check(bus::initialize() == status::ok && bus::read_blocking(0x50, rx) == status::ok &&
            i2c_driver::last_mode == transfer_mode::polled,
        "I2C facade selects polled engine");
  check(bus::template write_async<facade_completed>(0x50, tx) == status::unsupported,
        "I2C facade does not fake polled async");
  check(bus::write_blocking(0x80, tx) == status::invalid_argument, "I2C facade retains address validation");
  using port =
      microhal::uart::port<fixture::backend, microhal::uart::port_config<0, 115200, microhal::uart_parity::none,
                                                                         microhal::uart_stop_bits::one, false, false,
                                                                         false, false, 10, transfer_mode::dma>>;
  using uart_driver = microhal::backends::stm32::hal::serial_driver<peripheral_kind::uart>;
  uart_driver::error_value = status::ok;
  check(port::initialize() == status::ok && port::template start_async_tx<facade_completed>(tx) == status::ok &&
            uart_driver::last_mode == transfer_mode::dma,
        "UART facade selects DMA engine");
  uart_driver::h.tx(&uart_driver::h);
  check(facade_callbacks == 1, "UART facade callback delivered");
  using audio = microhal::i2s::interface<
      fixture::backend, microhal::i2s::i2s_config<0, microhal::i2s_mode::master_rx, microhal::i2s_standard::philips,
                                                  48000, false, false, transfer_mode::interrupt>>;
  using audio_driver = microhal::backends::stm32::hal::serial_driver<peripheral_kind::i2s>;
  audio_driver::error_value = status::ok;
  std::array<std::uint16_t, 4> samples{};
  check(audio::initialize() == status::ok && audio::template receive_async<facade_completed>(samples) == status::ok &&
            audio_driver::last_mode == transfer_mode::interrupt && audio_driver::last_receive,
        "I2S facade selects interrupt receive");
  audio_driver::h.rx(&audio_driver::h);
  check(facade_callbacks == 2, "I2S facade callback delivered");
  using automatic_audio =
      microhal::i2s::interface<fixture::backend, microhal::i2s::i2s_config<0, microhal::i2s_mode::master_tx,
                                                                           microhal::i2s_standard::philips, 48000>>;
  check(automatic_audio::template start_dma_tx<facade_completed>(samples) == status::ok &&
            automatic_audio::stop() == status::unsupported && automatic_audio::abort() == status::unsupported,
        "automatic I2S cancellation remains unsupported without legacy transfer state");
  using spi_bus = microhal::spi::master<
      fixture::backend,
      microhal::spi::master_config<0, 1000000, microhal::spi_duplex_mode::full, microhal::spi_clock_polarity::idle_low,
                                   microhal::spi_clock_phase::first_edge, microhal::spi_bit_order::msb_first, false,
                                   false, 10, transfer_mode::dma>>;
  spi_driver::error_value = status::ok;
  check(spi_bus::initialize() == status::ok &&
            spi_bus::template transfer_async<facade_completed>(tx, rx) == status::ok &&
            spi_driver::last_mode == transfer_mode::dma,
        "SPI facade selects DMA full duplex");
  spi_driver::h.duplex(&spi_driver::h);
  check(facade_callbacks == 3, "SPI facade duplex callback delivered");
  check(spi_bus::template transfer_async<facade_completed>({}, {}) == status::invalid_argument,
        "explicit SPI async rejects empty buffers");
  using legacy_spi = microhal::spi::master<microhal::mock_tag, microhal::spi::master_config<0, 1000000>>;
  check(legacy_spi::initialize() == status::ok && legacy_spi::transfer(tx, rx) == status::ok,
        "automatic mode preserves legacy backend dispatch");
  check(legacy_spi::abort() == status::unsupported, "automatic cancellation does not falsely claim support");
  spi_driver::complete_inline = true;
  check(spi_bus::template write_async<facade_completed>(tx) == status::ok && facade_callbacks == 4 &&
            spi_bus::transfer_status() == status::ok,
        "inline completion does not leave stale ownership");
  spi_driver::complete_inline = false;
  check(spi_engine::initialize<bound_dma>() == status::ok && dma_binding::calls == 1,
        "board DMA configuration hook invoked");
  dma_binding::configured_result = status::io_error;
  check(spi_engine::initialize<bound_dma>() == status::io_error &&
            spi_engine::write<bound_dma, facade_completed>(tx) == status::unsupported,
        "failed board reconfiguration invalidates prior initialization");
  using spi_slave = microhal::spi::slave<fixture::backend, microhal::spi::slave_config<0>>;
  check(spi_slave::initialize() == status::ok &&
            spi_slave::template transfer_async<facade_completed>(tx, rx) == status::ok && spi_driver::last_slave,
        "SPI slave facade selects slave configuration");
  check(spi_slave::template read_async<facade_completed>(rx) == status::busy,
        "SPI slave rejects overlapping operation");
  spi_driver::h.duplex(&spi_driver::h);
  check(spi_slave::transfer_status() == status::ok, "SPI slave callback completes operation");
  using i2c_slave =
      microhal::i2c::slave<fixture::backend, microhal::i2c::slave_config<0, 0x52, microhal::i2c_address_mode::seven_bit,
                                                                         100000, false, false, 10, transfer_mode::dma>>;
  static_assert(i2c_slave::config_type::own_address == 0x52);
  i2c_driver::error_value = status::ok;
  check(i2c_slave::initialize() == status::ok &&
            i2c_slave::template receive_async<facade_completed>(rx) == status::ok && i2c_driver::last_slave &&
            i2c_driver::last_receive && i2c_driver::last_address == 0,
        "I2C slave receives without a remote address");
  check(i2c_slave::template transmit_async<facade_completed>(tx) == status::busy, "I2C slave retains buffer ownership");
  i2c_driver::h.rx(&i2c_driver::h);
  check(i2c_slave::template transmit_async<facade_completed>(tx) == status::ok && !i2c_driver::last_receive,
        "I2C slave transmits without a remote address");
  i2c_driver::stop_result = status::io_error;
  check(i2c_slave::abort() == status::io_error &&
            i2c_slave::template receive_async<facade_completed>(rx) == status::busy,
        "I2C slave failed cancellation retains ownership");
  i2c_driver::stop_result = status::ok;
  check(i2c_slave::abort() == status::ok, "I2C slave abort recovers");
  return failures == 0 ? 0 : 1;
}
