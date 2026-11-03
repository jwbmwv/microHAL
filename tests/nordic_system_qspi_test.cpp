// SPDX-License-Identifier: Apache-2.0
// Compile twice with NRF_QSPI_HAS_XIPEN=0/1 to model the native nRF52/nRF53 lifecycles.
#include "microhal/core/traits.hpp"
#include <array>
#include <atomic>
#include <cassert>
#include <cerrno>
#include <span>

#ifndef NRF_QSPI_HAS_XIPEN
#define NRF_QSPI_HAS_XIPEN 0
#endif
#define MICROHAL_NORDIC_HAS_QSPI 1
#define MICROHAL_NORDIC_QSPI_HAS_CACHE NRF_QSPI_HAS_XIPEN
#define QSPI_WRITE_CNT_CNT_Msk 0xFFFFFFU
#define QSPI_WRITE_CNT_CNT_Pos 0U
#define QSPI_READ_CNT_CNT_Msk 0xFFFFFFU
#define QSPI_READ_CNT_CNT_Pos 0U
enum
{
  NRF_QSPI_ADDRMODE_24BIT,
  NRF_QSPI_ADDRMODE_32BIT,
  NRF_QSPI_WRITEOC_PP,
  NRF_QSPI_WRITEOC_PP2O,
  NRF_QSPI_WRITEOC_PP4O,
  NRF_QSPI_WRITEOC_PP4IO,
  NRF_QSPI_READOC_FASTREAD,
  NRF_QSPI_READOC_READ2O,
  NRF_QSPI_READOC_READ4O,
  NRF_QSPI_READOC_READ2IO,
  NRF_QSPI_READOC_READ4IO,
  NRF_QSPI_CINSTR_LEN_1B
};
using nrf_qspi_cinstr_len_t = int;
struct nrf_qspi_cinstr_conf_t
{
  std::uint8_t opcode{};
  nrf_qspi_cinstr_len_t length{};
  bool io2_level{}, io3_level{};
};
struct nrfx_qspi_config_t
{
  std::uint32_t xip_offset{}, timeout{};
  struct
  {
    int addrmode{NRF_QSPI_ADDRMODE_24BIT}, writeoc{NRF_QSPI_WRITEOC_PP}, readoc{NRF_QSPI_READOC_FASTREAD};
  } prot_if;
  struct
  {
    std::uint8_t sck_freq{1U};
  } phy_if;
};
struct qspi_registers
{
  std::uint32_t offset{};
  bool xip{true};
};
static qspi_registers hardware{};
#define NRF_QSPI (&hardware)
static bool initialized{}, activated{};
static int activation_error{}, readiness_error{}, deactivation_error{}, transfer_error{};
static unsigned reads{}, writes{}, instructions{}, deactivations{}, barriers{};
static void (*during_transfer)(){};
static void (*during_activation)(){};
static bool nrfx_qspi_init_check()
{
  return initialized;
}
static int nrfx_qspi_init(nrfx_qspi_config_t const*, void*, void*)
{
  initialized = true;
  return 0;
}
static void nrfx_qspi_uninit()
{
  initialized = false;
  activated = false;
}
static int nrfx_qspi_activate(bool wait)
{
  assert(wait);
  if (during_activation)
    during_activation();
  if (activated)
    return -EALREADY;
  if (activation_error)
    return activation_error;
  activated = true;
  return 0;
}
static int nrfx_qspi_mem_busy_check()
{
  return readiness_error;
}
static int nrfx_qspi_deactivate()
{
  ++deactivations;
  if (deactivation_error)
    return deactivation_error;
  activated = false;
  return 0;
}
static void nrf_qspi_xip_offset_set(qspi_registers* reg, std::uint32_t offset)
{
  reg->offset = offset;
}
#if NRF_QSPI_HAS_XIPEN
static bool ignore_xip_write{};
static void nrf_qspi_xip_set(qspi_registers* reg, bool enabled)
{
  if (!ignore_xip_write)
    reg->xip = enabled;
}
static bool nrf_qspi_xip_check(qspi_registers const* reg)
{
  return reg->xip;
}
static bool cache_enabled{true};
static unsigned invalidations{};
#define NRF_CACHE (&cache_enabled)
static bool nrf_cache_enable_check(bool const* enabled)
{
  return *enabled;
}
static void nrf_cache_invalidate(bool*)
{
  ++invalidations;
}
#endif
static void __DSB()
{
  ++barriers;
}
static void __ISB()
{
  ++barriers;
}
static int nrfx_qspi_read(void*, std::size_t, std::uint32_t)
{
  ++reads;
  if (during_transfer)
    during_transfer();
  activated = true;
  return transfer_error;
}
static int nrfx_qspi_write(void const*, std::size_t, std::uint32_t)
{
  ++writes;
  if (during_transfer)
    during_transfer();
  activated = true;
  return transfer_error;
}
static int nrfx_qspi_cinstr_xfer(nrf_qspi_cinstr_conf_t const*, void const*, void*)
{
  ++instructions;
  if (during_transfer)
    during_transfer();
  return transfer_error;
}
static int nrfx_qspi_lfm_start(nrf_qspi_cinstr_conf_t const*)
{
  return 0;
}
static int nrfx_qspi_lfm_xfer(void const*, void*, std::size_t, bool)
{
  return transfer_error;
}
namespace nordic_qspi_model
{
using namespace microhal;
namespace detail
{
constexpr status driver_result(int value)
{
  switch (value)
  {
  case 0:
    return status::ok;
  case -EBUSY:
    return status::busy;
  case -ETIMEDOUT:
    return status::timeout;
  default:
    return status::io_error;
  }
}
template <typename Config> constexpr std::uint32_t timeout_ms()
{
  return 100U;
}
#include "microhal/backends/nordic/nrfx/qspi_state.inc"
} // namespace detail
#include "microhal/backends/nordic/nrfx/qspi_engine.inc"
} // namespace nordic_qspi_model
struct binding
{
  static inline nrfx_qspi_config_t configuration{.xip_offset = 0x1000U, .timeout = 0U, .prot_if = {}, .phy_if = {}};
  static constexpr bool owns_xip_window{true};
};
struct config
{
  using binding = ::binding;
  static constexpr std::uint32_t clock_hz{16000000U};
};
struct other_config : config
{
};
struct unowned_binding : binding
{
  static constexpr bool owns_xip_window{false};
};
struct unowned_config : config
{
  using binding = unowned_binding;
};
using api = nordic_qspi_model::qspi_api;
constexpr microhal::qspi_command read_command{.instruction = 0x0BU,
                                              .instruction_mode = microhal::qspi_io_mode::single,
                                              .address_size_bytes = 3U,
                                              .address_mode = microhal::qspi_io_mode::single,
                                              .dummy_cycles = 8U,
                                              .data_mode = microhal::qspi_io_mode::single};
constexpr microhal::qspi_command write_command{.instruction = 0x02U,
                                               .instruction_mode = microhal::qspi_io_mode::single,
                                               .address_size_bytes = 3U,
                                               .address_mode = microhal::qspi_io_mode::single,
                                               .data_mode = microhal::qspi_io_mode::single};
static std::array<std::uint8_t, 4U> bytes{};
static void nested_transfer()
{
  assert(api::enter_memory_mapped<config>(read_command) == microhal::status::busy);
  assert(api::exit_memory_mapped<config>() == microhal::status::busy);
  assert(api::initialize<config>() == microhal::status::busy);
}
static void nested_activation()
{
  assert(api::command_read<config>(read_command, bytes) == microhal::status::busy);
  assert(api::command_write<config>(write_command, bytes) == microhal::status::busy);
}
int main()
{
  using microhal::status;
  namespace state = nordic_qspi_model::detail;
  assert(api::enter_memory_mapped<config>(read_command) == status::unsupported);
  initialized = true;
  assert(api::initialize<config>() == status::busy);
  assert(api::enter_memory_mapped<config>(read_command) == status::unsupported);
  initialized = false;
  binding::configuration.xip_offset = 1U;
  assert(api::initialize<config>() == status::invalid_argument);
  binding::configuration.xip_offset = 0x1000000U;
  assert(api::initialize<config>() == status::invalid_argument);
  binding::configuration.xip_offset = 0x1000U;
  assert(api::initialize<config>() == status::ok);
  binding::configuration.xip_offset = 0x2000U;
  assert(api::initialize<other_config>() == status::busy);
  assert(api::enter_memory_mapped<other_config>(read_command) == status::unsupported);
  assert(api::exit_memory_mapped<config>() == status::unsupported);
  auto unsupported = read_command;
  unsupported.dummy_cycles = 0U;
  assert(api::enter_memory_mapped<config>(unsupported) == status::unsupported);
  unsupported = read_command;
  unsupported.address = 4U;
  assert(api::enter_memory_mapped<config>(unsupported) == status::unsupported);
  readiness_error = -EBUSY;
  assert(api::enter_memory_mapped<config>(read_command) == status::busy && !state::qspi_mapped);
  readiness_error = 0;
  during_transfer = nested_transfer;
  assert(api::command_write<config>(write_command, bytes) == status::ok && writes == 1U);
  during_transfer = nullptr;
  during_activation = nested_activation;
  assert(api::enter_memory_mapped<config>(read_command) == status::ok);
  during_activation = nullptr;
  assert(state::qspi_mapped && activated && hardware.offset == 0x1000U && barriers > 0U);
  binding::configuration.xip_offset = 0x1000U;
  assert(api::enter_memory_mapped<config>(read_command) == status::busy);
  assert(api::command_read<config>(read_command, bytes) == status::busy && reads == 0U);
  assert(api::command_write<config>(write_command, bytes) == status::busy && writes == 1U);
  microhal::qspi_command custom{.instruction = 0x9FU,
                                .instruction_mode = microhal::qspi_io_mode::single,
                                .data_mode = microhal::qspi_io_mode::single};
  assert(api::command_read<config>(custom, bytes) == status::busy && instructions == 0U);
  assert(api::exit_memory_mapped<other_config>() == status::unsupported && state::qspi_mapped);
  deactivation_error = -EBUSY;
  assert(api::exit_memory_mapped<config>() == status::busy && state::qspi_mapped && activated);
  assert(api::command_read<config>(read_command, bytes) == status::busy);
  deactivation_error = 0;
  assert(api::exit_memory_mapped<config>() == status::ok && !state::qspi_mapped && !activated);
  assert(deactivations == 2U && !state::qspi_active.load());
#if NRF_QSPI_HAS_XIPEN
  assert(!hardware.xip && invalidations >= 3U);
  ignore_xip_write = true;
  assert(api::enter_memory_mapped<config>(read_command) == status::io_error && !state::qspi_mapped);
  ignore_xip_write = false;
  assert(api::enter_memory_mapped<config>(read_command) == status::ok);
  ignore_xip_write = true;
  assert(api::exit_memory_mapped<config>() == status::io_error && state::qspi_mapped);
  assert(api::command_read<config>(read_command, bytes) == status::busy);
  ignore_xip_write = false;
  assert(api::exit_memory_mapped<config>() == status::ok);
  cache_enabled = false;
  const auto cached = invalidations;
  assert(api::enter_memory_mapped<config>(read_command) == status::ok);
  assert(api::exit_memory_mapped<config>() == status::ok && invalidations == cached);
  cache_enabled = true;
#endif
  assert(api::command_read<config>(read_command, bytes) == status::ok && reads == 1U);
  during_transfer = nested_transfer;
  assert(api::command_read<config>(custom, bytes) == status::ok && instructions == 1U);
  during_transfer = nullptr;
  readiness_error = -ETIMEDOUT;
  assert(api::enter_memory_mapped<config>(read_command) == status::timeout);
  assert(!initialized && !state::qspi_mapped && !state::qspi_active.load() && state::qspi_owner == nullptr);
  readiness_error = 0;
  assert(api::initialize<config>() == status::ok);
  activation_error = -ETIMEDOUT;
  assert(api::enter_memory_mapped<config>(read_command) == status::timeout && !initialized);
  activation_error = 0;
  assert(api::initialize<unowned_config>() == status::ok);
  assert(api::enter_memory_mapped<unowned_config>(read_command) == status::unsupported);
  nrfx_qspi_uninit();
  assert(api::initialize<config>() == status::ok);
  auto long_command = read_command;
  long_command.instruction = 0x13U;
  transfer_error = -EPERM;
  assert(api::command_read<config>(long_command, bytes) == status::io_error);
  assert(!initialized && state::qspi_owner == nullptr && !state::qspi_active.load());
  transfer_error = 0;
  assert(api::initialize<config>() == status::ok);
  assert(api::enter_memory_mapped<config>(read_command) == status::ok);
  assert(api::exit_memory_mapped<config>() == status::ok);
}
