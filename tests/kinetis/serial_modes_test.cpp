// SPDX-License-Identifier: Apache-2.0
// Build twice: with sdk/ only, and with sdk/ plus sdk_edma/. These fixtures
// exercise real backend bodies; they do not certify an MCUXpresso SDK/device.
#include "microhal/backends/kinetis/espresso/traits.hpp"
#include "microhal/i2c/i2c.hpp"
#include "microhal/i2s/i2s.hpp"
#include "microhal/spi/spi.hpp"
#include "microhal/uart/uart.hpp"
#include <cassert>

using namespace microhal;
namespace native = microhal::backends::kinetis::espresso::serial;
template <peripheral_kind Kind> using api = serial_mode_api<kinetis_espresso_tag, Kind>;

template <typename Base> struct binding
{
  using dma_resource = std::conditional_t<
      (MICROHAL_KINETIS_SERIAL_I2C_DMA && !MICROHAL_KINETIS_SERIAL_I2C_EDMA && std::is_same_v<Base, I2C_Type>) ||
          (MICROHAL_KINETIS_SERIAL_UART_DMA && !MICROHAL_KINETIS_SERIAL_UART_EDMA && std::is_same_v<Base, UART_Type>) ||
          (MICROHAL_KINETIS_SERIAL_LPUART_DMA && !MICROHAL_KINETIS_SERIAL_LPUART_EDMA &&
           std::is_same_v<Base, LPUART_Type>),
      dma_handle_t, edma_handle_t>;
  inline static Base peripheral{};
  inline static Base* const instance = &peripheral;
  static constexpr std::uint32_t source_clock_hz = 48'000'000U;
  inline static dma_resource tx{1U}, rx{2U}, intermediary{3U};
  inline static dma_resource* const tx_dma_handle = &tx;
  inline static dma_resource* const rx_dma_handle = &rx;
  inline static dma_resource* const intermediary_dma_handle = &intermediary;
  inline static dma_resource* const dma_handle = &tx;
  static constexpr std::uint32_t transfer_flags = 0x1234U;
  static std::uint32_t now_ms() noexcept
  {
    return fixture::ticks++;
  }
  static void idle() noexcept
  {
    if (fixture::auto_complete)
      fixture::complete();
  }
};
template <typename Base, transfer_mode Mode> struct config
{
  using binding = ::binding<Base>;
  static constexpr auto transfer_mode_value = Mode;
  static constexpr auto address_mode = i2c_address_mode::seven_bit;
  static constexpr std::uint32_t clock_hz = 400'000U, baud_rate = 115'200U, timeout_ms = 64U;
  static constexpr auto mode = i2s_mode::master_tx;
  static constexpr auto standard = i2s_standard::philips;
  static constexpr std::uint32_t sample_rate = 48'000U;
  static constexpr bool enable_master_clock = false;
  static constexpr bool enable_dma = Mode == transfer_mode::dma;
  static constexpr bool enable_interrupts = Mode == transfer_mode::interrupt;
  static constexpr auto duplex_mode = spi_duplex_mode::full;
  static constexpr auto clock_polarity = spi_clock_polarity::idle_low;
  static constexpr auto clock_phase = spi_clock_phase::first_edge;
  static constexpr auto bit_order = spi_bit_order::msb_first;
  static constexpr auto parity = uart_parity::none;
  static constexpr auto stop_bits = uart_stop_bits::one;
  static constexpr bool require_fifo = false, require_lpuart = false;
};
template <typename Base, transfer_mode Mode> struct slave_config : config<Base, Mode>
{
  static constexpr bool is_slave = true;
};
inline unsigned callbacks{};
inline status last_callback{};
void completed(status result) noexcept
{
  ++callbacks;
  last_callback = result;
}
void alternate(status result) noexcept
{
  completed(result);
}
inline std::uint8_t tx[]{1, 2, 3, 4};
inline std::uint8_t rx[4]{};
inline std::uint16_t samples[4]{};
template <typename Base> struct polled_binding : binding<Base>
{
  static constexpr std::uint32_t transfer_flags = 0U;
};
template <typename Base> struct polled_config : config<Base, transfer_mode::polled>
{
  using binding = polled_binding<Base>;
};
struct default_timeout_config
{
};
static_assert(native::timeout_ms<default_timeout_config>() == 1000U);

template <typename Base> void i2c_tests()
{
  using C = config<Base, transfer_mode::interrupt>;
  using A = api<peripheral_kind::i2c>;
  using Slave = slave_config<Base, transfer_mode::automatic>;
  using I2cMaster = i2c::master<kinetis_espresso_tag, Slave>;
  using I2cSlave = i2c::slave<kinetis_espresso_tag, Slave>;
  const auto slave_starts = fixture::starts, slave_creates = fixture::creates;
  assert(I2cMaster::initialize() == status::unsupported);
  assert(I2cMaster::write_blocking(0x50U, tx) == status::unsupported);
  assert(I2cSlave::initialize() == status::unsupported);
  assert(fixture::starts == slave_starts && fixture::creates == slave_creates);
  assert(A::template initialize<C>() == status::ok);
  using S = slave_config<Base, transfer_mode::interrupt>;
  const auto starts_before_slave = fixture::starts;
  assert(A::template initialize<S>() == status::unsupported);
  assert((A::template read<S, nullptr>(rx, 0x50) == status::unsupported));
  assert(fixture::starts == starts_before_slave);
  assert((A::template write<C, nullptr>(tx, 0x50) == status::ok));
  assert((A::template read<C, nullptr>(rx, 0x50) == status::ok));
  assert((A::template write_read<C, nullptr>(tx, rx, 0x50) == status::ok));
  auto* s = native::engine<native::driver_t<C>>::template owned<typename C::binding>();
  assert(s->transfer.subaddress == 0x01020304U && s->transfer.subaddressSize == 4U);
  assert((A::template read<C, completed>(rx, 0x80) == status::invalid_argument));
  assert((A::template write<C, completed>({}, 0x50) == status::invalid_argument));
  const auto callbacks_before = callbacks;
  assert((A::template write<C, completed>(tx, 0x50) == status::ok));
  assert(A::template transfer_status<C>() == status::busy);
  const auto creates_before = fixture::creates;
  assert((A::template read<C, alternate>(rx, 0x50) == status::busy));
  assert(A::template initialize<C>() == status::busy);
  assert(fixture::creates == creates_before);
  A::template handle_interrupt<C>();
  assert(callbacks == callbacks_before + 1U && last_callback == status::ok);
  assert(A::template transfer_status<C>() == status::ok);
  fixture::auto_complete = false;
  const auto aborts_before = fixture::aborts;
  assert((A::template read<C, nullptr>(rx, 0x50) == status::timeout));
  assert(fixture::aborts == aborts_before + 1U && fixture::pending == nullptr);
  assert(A::template transfer_status<C>() == status::timeout);
  fixture::auto_complete = true;
  assert(A::template initialize<C>() == status::ok);
  assert((A::template read<C, nullptr>(rx, 0x50) == status::ok));
  fixture::next_status = kStatus_InvalidArgument;
  assert((A::template write<C, completed>(tx, 0x50) == status::invalid_argument));
  fixture::next_status = 0;
  fixture::completion_status = -7;
  assert((A::template read<C, nullptr>(rx, 0x50) == status::io_error));
  assert(A::template initialize<C>() == status::ok);
  assert((A::template read<C, completed>(rx, 0x50) == status::ok));
  fixture::complete();
  assert(last_callback == status::io_error && fixture::pending == nullptr);
  assert(A::template transfer_status<C>() == status::io_error);
  if constexpr (std::is_same_v<Base, I2C_Type>)
  {
    assert(A::template initialize<C>() == status::ok);
    assert((A::template read<C, completed>(rx, 0x50) == status::ok));
    C::binding::peripheral.S.flags = I2C_S_BUSY_MASK;
    assert(A::template abort<C>() == status::timeout);
    assert(A::template transfer_status<C>() == status::timeout);
    assert(fixture::pending == nullptr);
    C::binding::peripheral.S.flags = 0U;
  }
  fixture::completion_status = 0;
  using P = polled_config<Base>;
  const auto polled_starts = fixture::starts, polled_handles = fixture::creates;
  assert(A::template initialize<P>() == status::ok);
  assert((A::template write<P, completed>(tx, 0x50) == status::unsupported));
  assert((A::template write<P, nullptr>(tx, 0x50) == status::ok));
  assert((A::template read<P, nullptr>(rx, 0x50) == status::ok));
  if constexpr (std::is_same_v<Base, LPI2C_Type>)
  {
    std::uint8_t too_many[257]{};
    assert((A::template read<P, nullptr>(too_many, 0x50) == status::unsupported));
  }
  assert(fixture::starts == polled_starts && fixture::creates == polled_handles);
  if constexpr (std::is_same_v<Base, I2C_Type>)
    P::binding::peripheral.S.flags = I2C_S_RXAK_MASK;
  else
    P::binding::peripheral.flags = 0x20U;
  assert((A::template write<P, nullptr>(tx, 0x50) == status::io_error));
  if constexpr (std::is_same_v<Base, I2C_Type>)
    P::binding::peripheral.S.flags = 0U;
  else
    P::binding::peripheral.flags = 0U;
  assert(A::template initialize<P>() == status::ok);
  assert((A::template write_read<P, nullptr>(tx, rx, 0x50) == status::unsupported));
  fixture::poll_progress = false;
  assert((A::template read<P, nullptr>(rx, 0x50) == status::timeout));
  fixture::poll_progress = true;
  assert(A::template initialize<P>() == status::ok);
  assert((A::template read<P, nullptr>(rx, 0x50) == status::ok));
  using D = config<Base, transfer_mode::dma>;
  if constexpr (native::driver_t<D>::dma)
  {
    assert(A::template initialize<D>() == status::ok);
    auto* dma_state = native::engine<native::driver_t<D>>::template owned<typename D::binding>();
    assert(dma_state->dma_handle.dma == D::binding::dma_handle);
    assert((A::template write<D, nullptr>(tx, 0x50) == status::ok));
    assert((A::template read<D, completed>(rx, 0x50) == status::ok));
    fixture::complete();
    assert((A::template write_read<D, nullptr>(tx, rx, 0x50) == status::ok));
    assert((A::template read<D, alternate>(rx, 0x50) == status::ok));
    assert(A::template abort<D>() == status::ok && fixture::pending == nullptr);
  }
  else
    assert(A::template initialize<D>() == status::unsupported);
}

template <typename Base> void spi_tests()
{
  using C = config<Base, transfer_mode::interrupt>;
  using A = api<peripheral_kind::spi>;
  using SpiSlave = spi::slave<kinetis_espresso_tag, slave_config<Base, transfer_mode::automatic>>;
  const auto slave_starts = fixture::starts, slave_creates = fixture::creates;
  assert(SpiSlave::initialize() == status::unsupported);
  assert((SpiSlave::transfer(tx, rx) == status::unsupported));
  assert(fixture::starts == slave_starts && fixture::creates == slave_creates);
  assert(A::template initialize<C>() == status::ok);
  using S = slave_config<Base, transfer_mode::interrupt>;
  const auto starts_before_slave = fixture::starts;
  assert(A::template initialize<S>() == status::unsupported);
  assert((A::template transfer<S, nullptr>(tx, rx) == status::unsupported));
  assert(fixture::starts == starts_before_slave);
  assert((A::template transfer<C, nullptr>(tx, rx) == status::ok));
  assert((A::template write<C, nullptr>(tx) == status::ok));
  assert((A::template read<C, nullptr>(rx) == status::ok));
  assert((A::template transfer<C, completed>(tx, {}) == status::ok));
  auto* s = native::engine<native::driver_t<C>>::template owned<typename C::binding>();
  assert(s->transfer.rxData == nullptr && s->transfer.dataSize == 4U);
  assert(s->transfer.configFlags == 0x1234U);
  assert((A::template transfer<C, alternate>({}, rx) == status::busy));
  A::template handle_interrupt<C>();
  assert((A::template transfer<C, completed>(tx, std::span{rx, 2U}) == status::invalid_argument));
  assert((A::template transfer<C, completed>({}, {}) == status::invalid_argument));
  using P = polled_config<Base>;
  const auto polled_starts = fixture::starts, polled_handles = fixture::creates;
  assert(A::template initialize<P>() == status::ok);
  assert((A::template transfer<P, nullptr>(tx, rx) == status::ok));
  assert(fixture::starts == polled_starts && fixture::creates == polled_handles);
  assert(rx[0] == tx[0]);
  assert((A::template write<P, nullptr>(tx) == status::ok));
  assert((A::template read<P, nullptr>(rx) == status::ok));
  assert((A::template transfer<P, completed>(tx, rx) == status::unsupported));
  fixture::poll_progress = false;
  assert((A::template transfer<P, nullptr>(tx, rx) == status::timeout));
  fixture::poll_progress = true;
  assert((A::template transfer<P, nullptr>(tx, rx) == status::ok));
  P::binding::peripheral.flags = 4U;
  assert((A::template transfer<P, nullptr>(tx, rx) == status::io_error));
  P::binding::peripheral.flags = 0U;
  using D = config<Base, transfer_mode::dma>;
  if constexpr (native::driver_t<D>::dma)
  {
    assert(A::template initialize<D>() == status::ok);
    assert((A::template transfer<D, nullptr>(tx, rx) == status::ok));
    assert((A::template write<D, completed>(tx) == status::ok));
    fixture::complete();
    assert((A::template read<D, completed>(rx) == status::ok));
    assert(A::template abort<D>() == status::ok);
  }
  else
    assert(A::template initialize<D>() == status::unsupported);
}

template <typename Base> void uart_tests()
{
  using C = config<Base, transfer_mode::interrupt>;
  using A = api<peripheral_kind::uart>;
  assert(A::template initialize<C>() == status::ok);
  assert((A::template write<C, nullptr>(tx) == status::ok));
  assert((A::template read<C, nullptr>(rx) == status::ok));
  assert((A::template read<C, completed>(rx) == status::ok));
  assert((A::template write<C, alternate>(tx) == status::busy));
  A::template handle_interrupt<C>();
  using P = config<Base, transfer_mode::polled>;
  assert(A::template initialize<P>() == status::ok);
  assert((A::template write<P, completed>(tx) == status::unsupported));
  assert((A::template write<P, nullptr>(tx) == status::ok));
  assert((A::template read<P, nullptr>(rx) == status::ok));
  assert(rx[0] == 0xA5U);
  P::binding::peripheral.flags = 0U;
  assert((A::template read<P, nullptr>(rx) == status::timeout));
  P::binding::peripheral.flags = 1U;
  assert((A::template read<P, nullptr>(rx) == status::io_error));
  P::binding::peripheral.flags = 112U;
  using D = config<Base, transfer_mode::dma>;
  if constexpr (native::driver_t<D>::dma)
  {
    assert(A::template initialize<D>() == status::ok);
    assert((A::template write<D, nullptr>(tx) == status::ok));
    assert((A::template read<D, nullptr>(rx) == status::ok));
    assert((A::template write<D, completed>(tx) == status::ok));
    fixture::complete();
    assert((A::template read<D, completed>(rx) == status::ok));
    assert(A::template abort<D>() == status::ok);
  }
  else
    assert(A::template initialize<D>() == status::unsupported);
}

template <transfer_mode Mode> struct receive_config : config<I2S_Type, Mode>
{
  static constexpr auto mode = i2s_mode::master_rx;
};
template <transfer_mode Mode> void sai_tests()
{
  using C = config<I2S_Type, Mode>;
  using R = receive_config<Mode>;
  using A = api<peripheral_kind::i2s>;
  if constexpr (Mode == transfer_mode::dma && !native::sai_driver::dma)
    assert(A::template initialize<C>() == status::unsupported);
  else
  {
    assert(A::template initialize<C>() == status::ok);
    assert((A::template transmit<C, nullptr>(samples) == status::ok));
    assert((A::template receive<C, nullptr>(samples) == status::unsupported));
    if constexpr (Mode == transfer_mode::polled)
    {
      assert((A::template transmit<C, completed>(samples) == status::unsupported));
      fixture::poll_progress = false;
      assert((A::template transmit<C, nullptr>(samples) == status::timeout));
      fixture::poll_progress = true;
      C::binding::peripheral.flags = kSAI_FIFOErrorFlag;
      assert((A::template transmit<C, nullptr>(samples) == status::io_error));
      C::binding::peripheral.flags = 0U;
    }
    else
    {
      assert((A::template transmit<C, completed>(samples) == status::ok));
      assert((A::template transmit<C, alternate>(samples) == status::busy));
      fixture::completion_status = kStatus_SAI_TxIdle;
      fixture::complete();
      assert(last_callback == status::ok);
      fixture::completion_status = 0;
    }
    assert(A::template initialize<R>() == status::ok);
    assert((A::template receive<R, nullptr>(samples) == status::ok));
    if constexpr (Mode != transfer_mode::polled)
      assert((A::template receive<R, completed>(samples) == status::ok));
    else
    {
      fixture::poll_progress = false;
      assert((A::template receive<R, nullptr>(samples) == status::timeout));
      fixture::poll_progress = true;
      assert((A::template receive<R, nullptr>(samples) == status::ok));
    }
    assert(A::template abort<R>() == status::ok);
  }
}

struct no_resources
{
  static constexpr auto transfer_mode_value = transfer_mode::dma;
};
struct missing_dma_binding
{
  inline static I2C_Type peripheral{};
  inline static I2C_Type* const instance = &peripheral;
  static constexpr std::uint32_t source_clock_hz = 48'000'000U;
};
struct missing_dma_config : config<I2C_Type, transfer_mode::dma>
{
  using binding = missing_dma_binding;
};
struct no_clock_binding : binding<SPI_Type>
{
  static void now_ms() = delete;
};
struct no_clock_config : config<SPI_Type, transfer_mode::interrupt>
{
  using binding = no_clock_binding;
};
struct unclocked_binding : binding<UART_Type>
{
  static constexpr auto source_clock_hz = 0U;
};
struct unclocked_config : config<UART_Type, transfer_mode::interrupt>
{
  using binding = unclocked_binding;
};

template <typename Base, transfer_mode Mode> struct slave_config : config<Base, Mode>
{
  static constexpr bool is_slave = true;
  static constexpr std::uint16_t own_address = 0x52U;
};

template <typename Base, peripheral_kind Kind, transfer_mode Mode> void reject_slave()
{
  using C = slave_config<Base, Mode>;
  using A = api<Kind>;
  const auto reject = []
  {
    const auto initializations = fixture::initializations, starts = fixture::starts, creates = fixture::creates,
               dma_creates = fixture::dma_creates, aborts = fixture::aborts, notifications = callbacks;
    const auto pending = fixture::pending;
    const auto context = fixture::pending_context;
    assert(A::template initialize<C>() == status::unsupported);
    assert((A::template write<C, completed>(tx) == status::unsupported));
    assert((A::template read<C, completed>(rx) == status::unsupported));
    assert((A::template write<C, nullptr>(tx) == status::unsupported));
    assert((A::template read<C, nullptr>(rx) == status::unsupported));
    assert(A::template abort<C>() == status::unsupported);
    assert(A::template transfer_status<C>() == status::unsupported);
    assert(A::template handle_interrupt<C>() == status::unsupported);
    assert(A::template handle_dma_interrupt<C>() == status::unsupported);
    if constexpr (Kind == peripheral_kind::spi)
    {
      assert((A::template transfer<C, completed>(tx, rx) == status::unsupported));
      if constexpr (SpiMasterConcept<kinetis_espresso_tag, C>)
      {
        using S = spi::slave<kinetis_espresso_tag, C>;
        assert(S::initialize() == status::unsupported);
        assert(S::transfer(tx, rx) == status::unsupported);
        assert(S::write_blocking(tx) == status::unsupported);
        assert(S::read_blocking(rx) == status::unsupported);
        assert(S::template transfer_async<completed>(tx, rx) == status::unsupported);
        assert(S::template transmit_async<completed>(tx) == status::unsupported);
        assert(S::template receive_async<completed>(rx) == status::unsupported);
      }
    }
    else
    {
      assert((A::template write_read<C, completed>(tx, rx, 0x50U) == status::unsupported));
      if constexpr (I2cMasterConcept<kinetis_espresso_tag, C>)
      {
        using S = i2c::slave<kinetis_espresso_tag, C>;
        assert(S::initialize() == status::unsupported);
        assert(S::template transmit_async<completed>(tx) == status::unsupported);
        assert(S::template receive_async<completed>(rx) == status::unsupported);
      }
    }
    assert(fixture::initializations == initializations && fixture::starts == starts && fixture::creates == creates &&
           fixture::dma_creates == dma_creates && fixture::aborts == aborts);
    assert(callbacks == notifications && fixture::pending == pending && fixture::pending_context == context);
  };
  reject();
  using M = config<Base, transfer_mode::interrupt>;
  assert(A::template initialize<M>() == status::ok);
  if constexpr (Kind == peripheral_kind::spi)
    assert((A::template transfer<M, completed>(tx, rx) == status::ok));
  else
    assert((A::template write<M, completed>(tx, 0x50U) == status::ok));
  assert(A::template transfer_status<M>() == status::busy);
  reject();
  assert(A::template transfer_status<M>() == status::busy);
  fixture::complete();
  assert(A::template transfer_status<M>() == status::ok);
}

template <typename Base, peripheral_kind Kind> void reject_slave_modes()
{
  reject_slave<Base, Kind, transfer_mode::automatic>();
  reject_slave<Base, Kind, transfer_mode::polled>();
  reject_slave<Base, Kind, transfer_mode::interrupt>();
  reject_slave<Base, Kind, transfer_mode::dma>();
}

int main()
{
  static_assert(backend_traits<kinetis_espresso_tag>::i2c_capability().transfer_modes.interrupt);
  static_assert(backend_traits<kinetis_espresso_tag>::spi_capability().transfer_modes.polled);
  static_assert(backend_traits<kinetis_espresso_tag>::uart_capability().transfer_modes.polled);
  static_assert(backend_traits<kinetis_espresso_tag>::i2s_capability().transfer_modes.dma ==
                bool(MICROHAL_KINETIS_SERIAL_SAI_EDMA));
  i2c_tests<I2C_Type>();
  i2c_tests<LPI2C_Type>();
  spi_tests<SPI_Type>();
  spi_tests<LPSPI_Type>();
  uart_tests<UART_Type>();
  uart_tests<LPUART_Type>();
  sai_tests<transfer_mode::interrupt>();
  sai_tests<transfer_mode::dma>();
  sai_tests<transfer_mode::polled>();
  assert(api<peripheral_kind::i2c>::initialize<no_resources>() == status::unsupported);
  assert((api<peripheral_kind::i2c>::initialize<config<UART_Type, transfer_mode::interrupt>>() == status::unsupported));
  assert((api<peripheral_kind::i2c>::probe<config<I2C_Type, transfer_mode::interrupt>>(0x50) == status::unsupported));
  assert(api<peripheral_kind::i2c>::initialize<missing_dma_config>() == status::unsupported);
  assert(api<peripheral_kind::spi>::initialize<no_clock_config>() == status::ok);
  assert((api<peripheral_kind::spi>::transfer<no_clock_config, nullptr>(tx, rx) == status::unsupported));
  assert(api<peripheral_kind::uart>::initialize<unclocked_config>() == status::invalid_argument);
  assert(callbacks >= 7U);
  using I = i2c::master<kinetis_espresso_tag, config<I2C_Type, transfer_mode::interrupt>>;
  assert(I::initialize() == status::ok);
  assert(I::write_blocking(0x50, tx) == status::ok);
  using S = spi::master<kinetis_espresso_tag, config<SPI_Type, transfer_mode::interrupt>>;
  assert(S::initialize() == status::ok);
  assert(S::transfer(tx, rx) == status::ok);
  assert(S::handle_dma_interrupt() == status::unsupported);
  using SpiSlave = spi::slave<kinetis_espresso_tag, slave_config<SPI_Type, transfer_mode::interrupt>>;
  assert(SpiSlave::initialize() == status::unsupported);
  assert(SpiSlave::transfer(tx, rx) == status::unsupported);
  using U = uart::port<kinetis_espresso_tag, config<UART_Type, transfer_mode::polled>>;
  assert(U::initialize() == status::ok);
  assert(U::write_blocking(tx) == status::ok);
  assert(U::tx_status() == status::unsupported);
  assert(U::rx_status() == status::unsupported);
  assert(U::handle_dma_interrupt() == status::unsupported);
  using I2cSlave = i2c::slave<kinetis_espresso_tag, slave_config<I2C_Type, transfer_mode::interrupt>>;
  assert(I2cSlave::initialize() == status::unsupported);
  assert((I2cSlave::template receive_async<completed>(rx) == status::unsupported));
  using Audio = i2s::interface<kinetis_espresso_tag, config<I2S_Type, transfer_mode::interrupt>>;
  assert(Audio::initialize() == status::ok);
  assert(Audio::transmit_blocking(samples) == status::ok);
  reject_slave_modes<SPI_Type, peripheral_kind::spi>();
  reject_slave_modes<LPSPI_Type, peripheral_kind::spi>();
  reject_slave_modes<I2C_Type, peripheral_kind::i2c>();
  reject_slave_modes<LPI2C_Type, peripheral_kind::i2c>();
}
