// SPDX-License-Identifier: Apache-2.0
// USBD driver model verifying receive staging and endpoint ownership.
#include "microhal/core/traits.hpp"
#include <array>
#include <atomic>
#include <cassert>
#include <cstring>
#include <span>

#define MICROHAL_NORDIC_HAS_USBD 1
#define MICROHAL_NORDIC_HAS_CLOCK 0
using nrfx_usbd_ep_t = std::uint8_t;
enum nrfx_usbd_ep_status_t
{
  NRFX_USBD_EP_OK,
  NRFX_USBD_EP_BUSY,
  NRFX_USBD_EP_WAITING,
  NRFX_USBD_EP_ABORTED
};
enum
{
  NRFX_USBD_EVT_RESET,
  NRFX_USBD_EVT_EPTRANSFER,
  NRFX_USBD_EVT_SETUP
};
struct nrfx_usbd_setup_t
{
};
struct nrfx_usbd_evt_t
{
  int type{};
  struct
  {
    struct
    {
      nrfx_usbd_ep_t ep{};
      nrfx_usbd_ep_status_t status{};
    } eptransfer;
  } data;
};
struct transfer
{
  void const* buffer{};
  std::size_t bytes{};
};
#define NRFX_USBD_EPIN(ep) static_cast<nrfx_usbd_ep_t>((ep) | 0x80U)
#define NRFX_USBD_EPOUT(ep) static_cast<nrfx_usbd_ep_t>(ep)
#define NRFX_USBD_TRANSFER_IN(name, ptr, len, flags)                                                                   \
  struct transfer name                                                                                                 \
  {                                                                                                                    \
    ptr, len                                                                                                           \
  }
#define NRFX_USBD_TRANSFER_OUT(name, ptr, len)                                                                         \
  struct transfer name                                                                                                 \
  {                                                                                                                    \
    ptr, len                                                                                                           \
  }
static bool initialized{}, started{}, enabled{}, fail_submission{}, stuck_abort{};
static std::array<bool, 256> endpoints{}, stalled{};
static std::array<nrfx_usbd_ep_status_t, 256> statuses{};
static void (*callback)(nrfx_usbd_evt_t const*){};
static transfer submitted{};
static std::size_t received{};
static std::uint32_t __get_PRIMASK()
{
  return 0U;
}
static void __disable_irq() {}
static void __set_PRIMASK(std::uint32_t) {}
static bool nrfx_usbd_init_check()
{
  return initialized;
}
static int nrfx_usbd_init(void (*handler)(nrfx_usbd_evt_t const*))
{
  callback = handler;
  initialized = true;
  return 0;
}
static bool nrfx_usbd_is_started()
{
  return started;
}
static bool nrfx_usbd_is_enabled()
{
  return enabled;
}
static void nrfx_usbd_enable()
{
  enabled = true;
}
static void nrfx_usbd_disable()
{
  enabled = false;
}
static void nrfx_usbd_start(bool)
{
  started = true;
}
static void nrfx_usbd_stop()
{
  started = false;
}
static void nrfx_usbd_setup_get(nrfx_usbd_setup_t*) {}
static bool nrfx_usbd_ep_enable_check(nrfx_usbd_ep_t ep)
{
  return endpoints[ep];
}
static void nrfx_usbd_ep_enable(nrfx_usbd_ep_t ep)
{
  endpoints[ep] = true;
}
static void nrfx_usbd_ep_max_packet_size_set(nrfx_usbd_ep_t, unsigned) {}
static int nrfx_usbd_ep_transfer(nrfx_usbd_ep_t ep, transfer const* next)
{
  if (fail_submission)
    return -1;
  submitted = *next;
  statuses[ep] = NRFX_USBD_EP_BUSY;
  return 0;
}
static nrfx_usbd_ep_status_t nrfx_usbd_ep_status_get(nrfx_usbd_ep_t ep, std::size_t* bytes)
{
  *bytes = received;
  return statuses[ep];
}
static void nrfx_usbd_ep_abort(nrfx_usbd_ep_t ep)
{
  statuses[ep] = stuck_abort ? NRFX_USBD_EP_WAITING : NRFX_USBD_EP_ABORTED;
}
static void nrfx_usbd_ep_stall(nrfx_usbd_ep_t ep)
{
  stalled[ep] = true;
}
static void nrfx_usbd_ep_stall_clear(nrfx_usbd_ep_t ep)
{
  stalled[ep] = false;
}
static bool nrfx_usbd_ep_stall_check(nrfx_usbd_ep_t ep)
{
  return stalled[ep];
}
static void nrfx_usbd_irq_handler() {}
namespace microhal::backends::nordic::nrfx
{
namespace detail
{
static status driver_result(int value)
{
  return value == 0 ? status::ok : status::busy;
}
} // namespace detail
#include "microhal/backends/nordic/nrfx/usb_engine.inc"
} // namespace microhal::backends::nordic::nrfx
struct config
{
  static constexpr bool device_mode = true, host_mode = false, otg_mode = false;
  static constexpr auto speed = microhal::usb_speed::full;
  struct binding
  {
    static void handle_setup(nrfx_usbd_setup_t const&) {}
    static microhal::status request_usb_clock()
    {
      return microhal::status::ok;
    }
  };
};
int main()
{
  using namespace microhal;
  using namespace microhal::backends::nordic::nrfx;
  assert(usb_api::initialize<config>() == status::ok);
  assert(usb_api::connect<config>() == status::ok);
  std::array<std::uint8_t, 8> buffer{};
  fail_submission = true;
  assert(usb_api::endpoint_receive<config>(1U, buffer) == 0U);
  assert(!system_detail::usb_rx[1].pending.load());
  fail_submission = false;
  assert(usb_api::endpoint_receive<config>(1U, buffer) == 0U);
  assert(submitted.buffer == system_detail::usb_rx[1].storage.data());
  assert(submitted.buffer != buffer.data() && buffer[0] == 0U);
  assert(usb_api::endpoint_status<config>(1U) == status::busy);
  constexpr std::array<std::uint8_t, 6> packet{1U, 2U, 3U, 4U, 5U, 6U};
  std::memcpy(const_cast<void*>(submitted.buffer), packet.data(), packet.size());
  received = packet.size();
  statuses[1] = NRFX_USBD_EP_OK;
  nrfx_usbd_evt_t complete{NRFX_USBD_EVT_EPTRANSFER, {{1U, NRFX_USBD_EP_OK}}};
  callback(&complete);
  assert(usb_api::endpoint_receive<config>(1U, std::span(buffer).first(4)) == 0U);
  assert(system_detail::usb_rx[1].complete.load() && buffer[0] == 0U);
  assert(usb_api::endpoint_receive<config>(1U, buffer) == packet.size());
  assert(std::memcmp(buffer.data(), packet.data(), packet.size()) == 0);
  assert(usb_api::endpoint_receive<config>(1U, buffer) == 0U);
  stuck_abort = true;
  assert(usb_api::endpoint_abort<config>(1U) == status::busy);
  assert(system_detail::usb_rx[1].pending.load());
  stuck_abort = false;
  assert(usb_api::endpoint_abort<config>(1U) == status::ok);
  assert(!system_detail::usb_rx[1].pending.load());
  assert(usb_api::endpoint_transmit<config>(0x81U, buffer) == status::ok);
  assert(submitted.buffer == buffer.data());
  assert(usb_api::endpoint_stall<config>(0x81U, true) == status::ok && stalled[0x81]);
  assert(usb_api::endpoint_stall<config>(0x81U, false) == status::ok && !stalled[0x81]);
  assert(usb_api::endpoint_receive<config>(1U, buffer) == 0U);
  nrfx_usbd_evt_t reset{NRFX_USBD_EVT_RESET, {}};
  callback(&reset);
  assert(!system_detail::usb_rx[1].pending.load() && !system_detail::usb_rx[1].complete.load());
  assert(usb_api::disconnect<config>() == status::ok && !started && !enabled);
}
