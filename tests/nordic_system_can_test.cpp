// SPDX-License-Identifier: Apache-2.0
// MCAN register/message-RAM model, independent of nrfx and a physical CAN bus.
#include "microhal/can/can.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
#undef MICROHAL_NORDIC_HAS_MCAN
#if defined(__linux__)
#include <sys/mman.h>
#define MICROHAL_NORDIC_HAS_MCAN 1
#else
#define MICROHAL_NORDIC_HAS_MCAN 0
#endif
#define MCAN_CCCR_INIT_Msk 1U
#define MCAN_CCCR_CCE_Msk 2U
#define MCAN_CCCR_CSR_Msk (1U << 4U)
#define MCAN_CCCR_CSA_Msk (1U << 3U)
#define MCAN_CCCR_MON_Msk (1U << 5U)
#define MCAN_CCCR_TEST_Msk (1U << 7U)
#define MCAN_TEST_LBCK_Msk (1U << 4U)
#define MCAN_PSR_BO_Msk (1U << 7U)
#define CAN_INTENSET_CORE0_Msk 1U
#define NRFX_IRQ_PRIORITY_SET(irq, priority) ((void)0)
#define NRFX_IRQ_PENDING_CLEAR(irq) ((void)0)
#define NRFX_IRQ_ENABLE(irq) ((void)0)
#if MICROHAL_NORDIC_HAS_MCAN
struct mcan_registers
{
  std::uint32_t CCCR{}, NBTP{}, IE{}, ILE{}, IR{}, SIDFC{}, XIDFC{}, RXF0C{}, RXF1C{}, RXESC{}, TXBC{}, TXESC{},
      TXEFC{}, GFC{}, XIDAM{}, TEST{}, PSR{}, TXFQS{}, TXBAR{}, RXF0S{}, RXF0A{}, ILS{};
};
struct can_registers
{
  std::uint32_t INTENCLR{}, EVENTS_CORE[2]{}, TASKS_START{}, INTENSET{};
};
static mcan_registers core{};
static can_registers wrapper{};
#define NRF_MCAN120 (&core)
#define NRF_CAN120 (&wrapper)
static void __DMB() {}
static std::uint32_t interrupt_mask{};
static std::uint32_t __get_PRIMASK()
{
  return interrupt_mask;
}
static void __disable_irq()
{
  interrupt_mask = 1U;
}
static void __set_PRIMASK(std::uint32_t saved)
{
  interrupt_mask = saved;
}
#endif
namespace nordic_can_model
{
using namespace microhal;
namespace detail
{
template <auto Handler> void notify(status value)
{
  Handler(value);
}
} // namespace detail
#include "microhal/backends/nordic/nrfx/can_engine.inc"
} // namespace nordic_can_model
namespace nordic = nordic_can_model;
struct can_model_tag
{
};
namespace microhal
{
template <> struct backend_traits<can_model_tag>
{
  using backend_tag = can_model_tag;
  using can_api = nordic::can_api;
  static constexpr std::string_view name{"nordic-can-model"};
  static constexpr vendor_kind vendor{vendor_kind::mock};
  static constexpr backend_kind kind{backend_kind::custom};
  static consteval struct can_capability can_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .instance_count = 1U,
            .mailbox_count = 3U,
            .filter_banks = 32U,
            .extended_ids = true};
  }
};
} // namespace microhal
struct binding
{
  static constexpr std::uint32_t can_clock_hz{80000000U};
  static constexpr std::uintptr_t can_message_ram_address{0x2FBE8000U}, can_message_ram_base{0x2FBE8000U};
  static constexpr std::uint32_t can_message_ram_bytes{1024U};
  static microhal::status can_prepare() noexcept
  {
    return microhal::status::ok;
  }
};
struct config
{
  using binding = ::binding;
  static constexpr std::uint8_t instance_index{0U};
  static constexpr bool enable_can_fd{false};
  static constexpr bool enable_interrupts{false};
  static constexpr microhal::can_mode mode{microhal::can_mode::normal};
  static constexpr std::uint32_t baudrate{500000U};
};
static_assert(nordic::system_detail::can_nominal_timing(0U, 500000U).error() == microhal::status::invalid_argument);
static_assert(nordic::system_detail::can_nominal_timing(80000000U, 0U).error() == microhal::status::invalid_argument);
static_assert(nordic::system_detail::can_nominal_timing(80000000U, 2000000U).error() ==
              microhal::status::invalid_argument);
int main()
{
  using microhal::status;
  for (const auto clock : {16000000U, 32000000U, 64000000U, 80000000U})
    for (const auto baud : {125000U, 250000U, 500000U, 1000000U})
    {
      const auto timing = nordic::system_detail::can_nominal_timing(clock, baud);
      assert(timing);
      const auto bits = timing.value();
      const auto prescale = ((bits >> 16U) & 511U) + 1U;
      const auto segment1 = ((bits >> 8U) & 255U) + 1U;
      const auto segment2 = (bits & 127U) + 1U;
      assert(clock == baud * prescale * (1U + segment1 + segment2));
    }
#if MICROHAL_NORDIC_HAS_MCAN
  auto* ram = mmap(reinterpret_cast<void*>(binding::can_message_ram_address), 4096U, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
  assert(ram != MAP_FAILED);
  using api = nordic::can_api;
  assert(api::initialize<config>() == status::ok && wrapper.TASKS_START == 1U);
  assert(api::initialize<config>() == status::busy);
  assert(core.RXF0C == (192U | (8U << 16U)) && core.TXBC == (320U | (3U << 24U)));
  using bus = microhal::can::bus<can_model_tag, config>;
  assert(bus::configure_filter(0x123U, 0x7FFU, 0U) == status::ok);
  auto* words = static_cast<std::uint32_t*>(ram);
  assert(words[0] == ((2U << 30U) | (1U << 27U) | (0x123U << 16U) | 0x7FFU));
  assert(bus::configure_filter(0x456U, 0x7F0U, 0U) == status::ok);
  const auto second_standard = words[1];
  assert(second_standard == ((2U << 30U) | (1U << 27U) | (0x456U << 16U) | 0x7F0U));
  assert(bus::configure_filter(0x123U, 0x7F0U, 0U) == status::ok);
  assert(words[0] == ((2U << 30U) | (1U << 27U) | (0x123U << 16U) | 0x7F0U));
  assert(words[1] == second_standard && nordic::system_detail::can.standard_count == 2U);
  assert(bus::configure_filter(0x777U, 0x7FFU, 1U) == status::unsupported);
  assert(nordic::system_detail::can.standard_count == 2U && words[2] == 0U);
  assert(bus::configure_filter(0x12345U, 0x1FFFFFFFU, 0U) == status::ok);
  assert(words[16] == (0x12345U | (1U << 29U)) && words[17] == 0x9FFFFFFFU);
  assert(bus::configure_filter(0x23456U, 0x1FFFFFF0U, 0U) == status::ok);
  assert(words[18] == (0x23456U | (1U << 29U)) && words[19] == 0x9FFFFFF0U);
  assert(bus::configure_filter(0x12345U, 0x1FFFFF00U, 0U) == status::ok);
  assert(words[16] == (0x12345U | (1U << 29U)) && words[17] == 0x9FFFFF00U);
  assert(nordic::system_detail::can.extended_count == 2U);
  for (std::uint32_t index = 2U; index < 16U; ++index)
  {
    assert(bus::configure_filter(0x500U + index, 0x7FFU, 0U) == status::ok);
    assert(bus::configure_filter(0x100000U + index, 0x1FFFFFFFU, 0U) == status::ok);
    assert(words[index] == ((2U << 30U) | (1U << 27U) | ((0x500U + index) << 16U) | 0x7FFU));
    assert(words[16U + index * 2U] == ((0x100000U + index) | (1U << 29U)));
    assert(words[17U + index * 2U] == 0x9FFFFFFFU);
  }
  std::array<std::uint32_t, 48U> saved_filters{};
  std::memcpy(saved_filters.data(), words, sizeof(saved_filters));
  assert(bus::configure_filter(0x777U, 0x7FFU, 0U) == status::busy);
  assert(bus::configure_filter(0x2FFFFFFU, 0x1FFFFFFFU, 0U) == status::busy);
  assert(bus::configure_filter(0x123U, 0x7FFU, 1U) == status::unsupported);
  assert(bus::configure_filter(0x12345U, 0x1FFFFFFFU, 1U) == status::unsupported);
  assert(bus::configure_filter(0x123U, 0x7FFU, 2U) == status::invalid_argument);
  assert(bus::configure_filter(0x20000000U, 0x1FFFFFFFU, 0U) == status::invalid_argument);
  assert(bus::configure_filter(0x123U, 0x20000000U, 0U) == status::invalid_argument);
  assert(std::memcmp(saved_filters.data(), words, sizeof(saved_filters)) == 0);
  assert(bus::configure_filter(0x123U, 0x7FFU, 0U) == status::ok);
  assert(bus::configure_filter(0x12345U, 0x1FFFFFFFU, 0U) == status::ok);
  assert(nordic::system_detail::can.standard_count == 16U && nordic::system_detail::can.extended_count == 16U);
  assert(words[0] == ((2U << 30U) | (1U << 27U) | (0x123U << 16U) | 0x7FFU));
  assert(words[16] == (0x12345U | (1U << 29U)) && words[17] == 0x9FFFFFFFU);
  assert(words[1] == second_standard && core.RXF1C == 0U && interrupt_mask == 0U);
  interrupt_mask = 1U;
  assert(bus::configure_filter(0x123U, 0x7FFU, 0U) == status::ok && interrupt_mask == 1U);
  interrupt_mask = 0U;
  microhal::can::message message{.id = 0x123U, .data = {1U, 2U, 3U, 4U}, .dlc = 4U};
  assert(api::transmit<config>(message) == status::ok);
  assert(words[80] == (0x123U << 18U) && words[81] == (4U << 16U));
  assert(words[82] == 0x04030201U && core.TXBAR == 1U);
  core.TXFQS = 1U << 21U;
  assert(api::transmit<config>(message) == status::busy);
  core.TXFQS = 0U;
  core.PSR = MCAN_PSR_BO_Msk;
  assert(api::transmit<config>(message) == status::io_error);
  core.PSR = 0U;
  core.RXF0S = 1U;
  words[48] = 0x12345U | (1U << 30U);
  words[49] = 4U << 16U;
  words[50] = 0x08070605U;
  assert(api::receive<config>(message) == status::ok);
  assert(message.extended_id && message.id == 0x12345U && message.dlc == 4U);
  assert(message.data[0] == 5U && message.data[3] == 8U && core.RXF0A == 0U);
  assert(munmap(ram, 4096U) == 0);
#endif
}
