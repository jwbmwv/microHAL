// SPDX-License-Identifier: Apache-2.0
#include "microhal/backends/kinetis/espresso/traits.hpp"
#include <cassert>
using namespace microhal;
using backend = backend_traits<kinetis_espresso_tag>;
static_assert(!backend::i2c_capability().transfer_modes.interrupt);
static_assert(!backend::spi_capability().supported);
static_assert(!backend::uart_capability().transfer_modes.polled);
static_assert(!backend::i2s_capability().transfer_modes.dma);
struct config
{
  static constexpr auto transfer_mode_value = transfer_mode::dma;
};
int main()
{
  using A = serial_mode_api<kinetis_espresso_tag, peripheral_kind::i2c>;
  assert(A::initialize<config>() == status::unsupported);
  assert((A::write<config, nullptr>({}, 0x50) == status::unsupported));
  assert(A::abort<config>() == status::unsupported);
  assert(A::transfer_status<config>() == status::unsupported);
  assert(A::probe<config>(0x50) == status::unsupported);
  assert(A::handle_interrupt<config>() == status::unsupported);
  assert(A::tx_status<config>() == status::unsupported);
  assert(A::rx_status<config>() == status::unsupported);
  assert(A::abort_tx<config>() == status::unsupported);
  assert(A::abort_rx<config>() == status::unsupported);
  assert(A::handle_dma_interrupt<config>() == status::unsupported);
}
