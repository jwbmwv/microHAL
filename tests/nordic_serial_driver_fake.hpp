// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "microhal/core/traits.hpp"
#include <array>
#include <atomic>
#include <cerrno>
#include <span>
#include <type_traits>
#include <utility>

#define MICROHAL_NORDIC_HAS_UARTE 1
#define MICROHAL_NORDIC_HAS_SERIAL_TIMEOUT 1
#define MICROHAL_NORDIC_HAS_SPIS 1
#define MICROHAL_NORDIC_HAS_SPIM 0
#define MICROHAL_NORDIC_HAS_TWIS 1
#define MICROHAL_NORDIC_HAS_TWIM 1
#define NRF54L15_XXAA 1

struct register_block
{
  alignas(4096) unsigned value{};
  unsigned STATUS{};
};
inline register_block serial_registers[5]{};
#define NRF_SPIS00 (&serial_registers[0])
#define NRF_SPIS20 (&serial_registers[1])
#define NRF_SPIS21 (&serial_registers[2])
#define NRF_SPIS22 (&serial_registers[3])
#define NRF_SPIS30 (&serial_registers[4])
#define NRF_TWIS20 (&serial_registers[1])
#define NRF_TWIS21 (&serial_registers[2])
#define NRF_TWIS22 (&serial_registers[3])
#define NRF_TWIS30 (&serial_registers[4])

struct control_block
{
  bool initialized{};
};
struct nrfx_uarte_t
{
  register_block* p_reg{};
  control_block cb{};
};
struct nrfx_spis_t
{
  register_block* p_reg{};
  control_block cb{};
};
struct nrfx_twis_t
{
  register_block* p_reg{};
  control_block cb{};
};
struct nrfx_timer_t
{
  register_block* p_reg{};
  control_block cb{};
};
struct nrfx_twim_t
{
  register_block* p_twim{};
  control_block cb{};
};
enum twim_transfer_type
{
  NRFX_TWIM_XFER_TX,
  NRFX_TWIM_XFER_RX,
  NRFX_TWIM_XFER_TXRX
};
struct nrfx_twim_xfer_desc_t
{
  std::uint8_t address{};
  twim_transfer_type type{};
  std::uint8_t* p_primary_buf{};
  std::size_t primary_length{};
  std::uint8_t* p_secondary_buf{};
  std::size_t secondary_length{};
};
using nrfx_gppi_handle_t = unsigned;
struct nrfx_gppi_t
{
};
inline void nrfx_gppi_init(nrfx_gppi_t*) {}
enum nrf_timer_event_t
{
  NRF_TIMER_EVENT_COMPARE0
};
constexpr auto NRF_TIMER_MODE_TIMER = 0;
constexpr auto NRF_TIMER_BIT_WIDTH_32 = 0;
constexpr auto NRF_TIMER_TASK_CLEAR = 0;
constexpr auto NRF_TIMER_TASK_START = 1;
constexpr auto NRF_TIMER_CC_CHANNEL0 = 0;
constexpr auto NRF_TIMER_SHORT_COMPARE0_STOP_MASK = 1;
constexpr auto NRF_UARTE_EVENT_RXDRDY = 0;
constexpr auto NRFX_UARTE_RX_ENABLE_CONT = 1;
constexpr auto NRFX_UARTE_RX_ENABLE_STOP_ON_END = 2;
constexpr auto NRFX_UARTE_TX_DONE_ABORTED = 1U;
enum uarte_event_type
{
  NRFX_UARTE_EVT_TX_DONE,
  NRFX_UARTE_EVT_RX_DONE,
  NRFX_UARTE_EVT_ERROR,
  NRFX_UARTE_EVT_RX_DISABLED,
  NRFX_UARTE_EVT_RX_BUF_REQUEST,
  NRFX_UARTE_EVT_RX_BUF_TOO_LATE
};
struct nrfx_uarte_event_t
{
  uarte_event_type type{};
  struct
  {
    struct
    {
      std::size_t length{};
      unsigned flags{};
    } tx;
    struct
    {
      std::uint8_t* p_buffer{};
      std::size_t length{};
    } rx;
  } data;
};
constexpr auto NRF_UARTE_PSEL_DISCONNECTED = UINT32_MAX;
constexpr auto NRF_UARTE_HWFC_DISABLED = 0U;
constexpr auto NRF_UARTE_HWFC_ENABLED = 1U;
struct nrfx_uarte_config_t
{
  unsigned txd_pin{}, rxd_pin{}, rts_pin{}, cts_pin{};
  struct
  {
    unsigned hwfc{};
  } config;
};
inline bool nrf_gpio_pin_present_check(std::uint32_t pin)
{
  return pin < 48U;
}
enum spis_event_type
{
  NRFX_SPIS_BUFFERS_SET_DONE,
  NRFX_SPIS_XFER_DONE
};
struct nrfx_spis_event_t
{
  spis_event_type evt_type{};
  std::size_t tx_amount{}, rx_amount{};
};
using nrf_spis_mode_t = unsigned;
constexpr auto NRF_SPIS_BIT_ORDER_MSB_FIRST = 0;
constexpr auto NRF_SPIS_BIT_ORDER_LSB_FIRST = 1;
constexpr auto NRF_SPIS_PIN_NOT_CONNECTED = UINT32_MAX;
constexpr auto NRF_SPIS_STATUS_OVERREAD = 1U;
constexpr auto NRF_SPIS_STATUS_OVERFLOW = 2U;
inline unsigned spis_errors{};
inline unsigned nrf_spis_status_get(const register_block*)
{
  return spis_errors;
}
constexpr auto NRF_GPIO_PIN_PULLUP = 1;
constexpr auto NRF_GPIO_PIN_S0S1 = 0;
struct nrfx_spis_config_t
{
  unsigned sck_pin{}, mosi_pin{}, miso_pin{}, csn_pin{}, mode{}, bit_order{}, csn_pullup{}, miso_drive{};
  unsigned def{}, orc{}, irq_priority{};
};
enum twis_event_type
{
  NRFX_TWIS_EVT_READ_REQ,
  NRFX_TWIS_EVT_WRITE_REQ,
  NRFX_TWIS_EVT_READ_DONE,
  NRFX_TWIS_EVT_WRITE_DONE,
  NRFX_TWIS_EVT_READ_ERROR,
  NRFX_TWIS_EVT_WRITE_ERROR,
  NRFX_TWIS_EVT_GENERAL_ERROR
};
struct nrfx_twis_event_t
{
  twis_event_type type{};
  struct
  {
    std::size_t tx_amount{}, rx_amount{};
  } data;
};
using nrfx_twis_event_handler_t = void (*)(nrfx_twis_event_t const*);
struct nrfx_twis_config_t
{
  unsigned scl_pin{}, sda_pin{}, addr[2]{}, scl_pull{}, sda_pull{}, interrupt_priority{};
};
struct nrfx_timer_config_t
{
  unsigned frequency{}, mode{}, bit_width{}, interrupt_priority{};
  void* p_context{};
};

namespace fake
{
inline int tx_abort_error{}, rx_abort_error{}, buffer_error{}, enable_error{}, connect_error{}, attach_error{};
inline int abort_tx_calls{}, abort_rx_calls{}, queue_calls{}, release_calls{}, timer_releases{}, prepare_calls{};
inline std::uint8_t* queued{};
inline std::size_t queued_size{};
inline bool disable_all{}, sync_abort{};
inline bool slave_enabled{};
inline bool master_enabled{true};
inline void (*tx_abort_hook)(){};
inline void (*rx_abort_hook)(){};
inline nrfx_twim_xfer_desc_t master_descriptor{};
inline nrfx_uarte_config_t uart_configuration{};
inline unsigned uart_initializations{};
inline int uart_init_error{};
inline void (*timer_handler)(nrf_timer_event_t, void*){};
inline void* timer_context{};
} // namespace fake
inline bool nrfx_uarte_init_check(const nrfx_uarte_t* p)
{
  return p->cb.initialized;
}
inline int nrfx_uarte_init(nrfx_uarte_t* instance, const nrfx_uarte_config_t* config,
                           void (*)(const nrfx_uarte_event_t*, void*))
{
  ++fake::uart_initializations;
  fake::uart_configuration = *config;
  if (fake::uart_init_error == 0)
    instance->cb.initialized = true;
  return fake::uart_init_error;
}
inline int nrfx_uarte_tx_abort(nrfx_uarte_t*, bool)
{
  ++fake::abort_tx_calls;
  if (fake::tx_abort_hook != nullptr)
  {
    const auto hook = fake::tx_abort_hook;
    fake::tx_abort_hook = nullptr;
    hook();
  }
  return fake::tx_abort_error;
}
inline int nrfx_uarte_rx_abort(nrfx_uarte_t*, bool all, bool sync)
{
  ++fake::abort_rx_calls;
  fake::disable_all = all;
  fake::sync_abort = sync;
  if (sync && fake::rx_abort_hook != nullptr)
  {
    const auto hook = fake::rx_abort_hook;
    fake::rx_abort_hook = nullptr;
    hook();
  }
  return fake::rx_abort_error;
}
inline int nrfx_uarte_rx_buffer_set(nrfx_uarte_t*, std::uint8_t* bytes, std::size_t size)
{
  ++fake::queue_calls;
  fake::queued = bytes;
  fake::queued_size = size;
  return fake::buffer_error;
}
inline int nrfx_uarte_rx_enable(nrfx_uarte_t*, unsigned)
{
  return fake::enable_error;
}
inline bool nrfx_timer_init_check(const nrfx_timer_t* p)
{
  return p->cb.initialized;
}
inline int nrfx_timer_init(nrfx_timer_t* p, const nrfx_timer_config_t* c, void (*h)(nrf_timer_event_t, void*))
{
  p->cb.initialized = true;
  fake::timer_handler = h;
  fake::timer_context = c->p_context;
  return 0;
}
inline void nrfx_timer_disable(nrfx_timer_t*) {}
inline void nrfx_timer_uninit(nrfx_timer_t* p)
{
  p->cb.initialized = false;
  ++fake::timer_releases;
}
inline unsigned nrfx_timer_task_address_get(const nrfx_timer_t*, int task)
{
  return 100U + static_cast<unsigned>(task);
}
inline unsigned nrf_uarte_event_address_get(const register_block*, int)
{
  return 200U;
}
inline void nrfx_timer_extended_compare(const nrfx_timer_t*, int, unsigned, int, bool) {}
inline int nrfx_gppi_conn_alloc(unsigned, unsigned, unsigned* handle)
{
  *handle = 1U;
  return fake::connect_error;
}
inline int nrfx_gppi_ep_attach(unsigned, unsigned)
{
  return fake::attach_error;
}
inline void nrfx_gppi_conn_free(unsigned, unsigned, unsigned)
{
  ++fake::release_calls;
}
inline void nrfx_gppi_conn_enable(unsigned) {}
inline void nrfx_gppi_conn_disable(unsigned) {}
inline void nrfx_gppi_ep_clear(unsigned) {}
inline void nrfx_gppi_ep_enable(unsigned) {}
inline bool nrfx_spis_init_check(const nrfx_spis_t* p)
{
  return p->cb.initialized;
}
inline int nrfx_spis_init(nrfx_spis_t* p, const nrfx_spis_config_t*, void (*)(const nrfx_spis_event_t*, void*), void*)
{
  p->cb.initialized = true;
  return 0;
}
inline int nrfx_spis_buffers_set(nrfx_spis_t*, const std::uint8_t*, std::size_t, std::uint8_t*, std::size_t)
{
  return fake::buffer_error;
}
inline void nrfx_spis_uninit(nrfx_spis_t* p)
{
  p->cb.initialized = false;
}
inline void nrfx_spis_irq_handler(nrfx_spis_t*) {}
inline bool nrfx_twis_init_check(const nrfx_twis_t* p)
{
  return p->cb.initialized;
}
inline int nrfx_twis_init(nrfx_twis_t* p, const nrfx_twis_config_t*, nrfx_twis_event_handler_t)
{
  p->cb.initialized = true;
  return 0;
}
inline void nrfx_twis_enable(nrfx_twis_t*)
{
  fake::slave_enabled = true;
}
inline void nrfx_twis_disable(nrfx_twis_t*)
{
  fake::slave_enabled = false;
}
inline int nrfx_twis_tx_prepare(nrfx_twis_t*, const std::uint8_t*, std::size_t)
{
  ++fake::prepare_calls;
  return fake::buffer_error;
}
inline int nrfx_twis_rx_prepare(nrfx_twis_t*, std::uint8_t*, std::size_t)
{
  ++fake::prepare_calls;
  return fake::buffer_error;
}
inline void nrfx_twis_irq_handler(nrfx_twis_t*) {}
inline bool nrfx_twim_init_check(const nrfx_twim_t* p)
{
  return p->cb.initialized;
}
inline int nrfx_twim_xfer(nrfx_twim_t*, const nrfx_twim_xfer_desc_t* descriptor, unsigned)
{
  fake::master_descriptor = *descriptor;
  return fake::buffer_error;
}
inline void nrfx_twim_disable(nrfx_twim_t*)
{
  fake::master_enabled = false;
}
inline void nrfx_twim_enable(nrfx_twim_t*)
{
  fake::master_enabled = true;
}
inline void nrfx_twim_irq_handler(nrfx_twim_t*) {}

namespace microhal::backends::nordic::nrfx
{
namespace detail
{
inline status driver_result(int value) noexcept
{
  if (value == 0)
    return status::ok;
  if (value == -EBUSY)
    return status::busy;
  if (value == -EINVAL)
    return status::invalid_argument;
  return status::io_error;
}
constexpr bool serial_count_valid(std::size_t size) noexcept
{
  return size <= UINT16_MAX;
}
template <auto Handler> void notify(status value) noexcept
{
  if constexpr (requires { Handler(value); })
    Handler(value);
}
#include "microhal/backends/nordic/nrfx/serial_transfer_state.inc"

#include "microhal/backends/nordic/nrfx/serial_resource.inc"

#include "microhal/backends/nordic/nrfx/serial_uarte_config.inc"
inline constexpr auto twim_instance_count = 1U;
inline nrfx_twim_t twim_instances[]{{&serial_registers[0], {true}}};
inline std::array<transfer_state, twim_instance_count> twim_states{};
inline nrfx_twim_t* twim_instance(std::uint8_t index) noexcept
{
  return &twim_instances[index];
}
inline nrfx_uarte_t uarte_instances[]{{&serial_registers[0], {true}}};
#include "microhal/backends/nordic/nrfx/serial_spis_state.inc"
#include "microhal/backends/nordic/nrfx/serial_twis_state.inc"
#include "microhal/backends/nordic/nrfx/serial_uarte_state.inc"
} // namespace detail
struct uart_api
{
#include "microhal/backends/nordic/nrfx/serial_uarte_api.inc"
};
struct spi_api
{
  template <typename Config> static status transfer(std::span<const std::uint8_t>, std::span<std::uint8_t>)
  {
    return status::unsupported;
  }
#include "microhal/backends/nordic/nrfx/serial_spis_api.inc"
};
struct i2c_api
{
#include "microhal/backends/nordic/nrfx/serial_twis_api.inc"
};
} // namespace microhal::backends::nordic::nrfx
