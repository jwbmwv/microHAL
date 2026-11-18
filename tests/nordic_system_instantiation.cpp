// SPDX-License-Identifier: Apache-2.0
// Compile with the real nrfx SDK; calls force template bodies to be checked.
#include "microhal/backends/nordic/nrfx/traits.hpp"
#include "microhal/can/can.hpp"
#include "microhal/crc/crc.hpp"
#include "microhal/dma/dma.hpp"
#include "microhal/flash/flash.hpp"
#include "microhal/gpio/gpio.hpp"
#include "microhal/qspi/qspi.hpp"
#include "microhal/rcc/rcc.hpp"
#include "microhal/rtc/rtc.hpp"
#include "microhal/usb/usb.hpp"

#if MICROHAL_NORDIC_NATIVE_TARGET
#if defined(NRF54H20_XXAA)
static_assert(MICROHAL_NORDIC_HAS_H20_MRAM && MICROHAL_NORDIC_HAS_GRTC && MICROHAL_NORDIC_HAS_MVDMA &&
              MICROHAL_NORDIC_HAS_MCAN);
static_assert(!MICROHAL_NORDIC_HAS_USBD && !MICROHAL_NORDIC_HAS_QSPI && !MICROHAL_NORDIC_HAS_RTC);
#elif defined(NRF54L15_XXAA)
static_assert(MICROHAL_NORDIC_HAS_RRAMC && MICROHAL_NORDIC_HAS_GRTC);
static_assert(!MICROHAL_NORDIC_HAS_USBD && !MICROHAL_NORDIC_HAS_QSPI && !MICROHAL_NORDIC_HAS_RTC);
#elif defined(NRF52840_XXAA) || defined(NRF5340_XXAA_APPLICATION)
static_assert(MICROHAL_NORDIC_HAS_NVMC && MICROHAL_NORDIC_HAS_USBD && MICROHAL_NORDIC_HAS_QSPI &&
              MICROHAL_NORDIC_HAS_RTC);
#endif
#endif

namespace nordic_system_validation
{
using namespace microhal;
using namespace microhal::backends::nordic::nrfx;
struct binding
{
  static constexpr bool owns_clock{true};
#if MICROHAL_NORDIC_HAS_H20_MRAM
  static constexpr std::uint32_t mram_start_address{0x0E0A0000U}, mram_size_bytes{0x40000U};
  static constexpr std::uint32_t mram_bank11_address{0x0E100000U};
  static status mram_acquire() noexcept
  {
    return status::unsupported;
  }
  static void mram_release() noexcept {}
#endif
#if MICROHAL_NORDIC_HAS_MCAN
  static constexpr std::uint32_t can_clock_hz{80000000U};
  static constexpr std::uintptr_t can_message_ram_address{0x2FBE8000U}, can_message_ram_base{0x2FBE8000U};
  static constexpr std::uint32_t can_message_ram_bytes{1024U};
  static status can_prepare() noexcept
  {
    return status::unsupported;
  }
#endif
#if MICROHAL_NORDIC_HAS_USBD
  static void handle_setup(nrfx_usbd_setup_t const&) noexcept
  {
    nrfx_usbd_setup_stall();
  }
#endif
};
struct config
{
  using binding = nordic_system_validation::binding;
  static constexpr std::uint8_t instance_index{0U};
  static constexpr bool enable_periodic_irq{true}, enable_alarm_irq{true};
  static constexpr bool host_mode{false}, otg_mode{false}, device_mode{true}, enable_rng{true}, enable_pll{false};
  static constexpr usb_speed speed{usb_speed::full};
  static constexpr rcc::clock_source source{rcc::clock_source::hsi};
  static constexpr std::uint32_t system_clock_hz{64000000U};
  static constexpr std::uint8_t stream{0U}, channel{0U};
  static constexpr dma::dma_direction direction{dma::dma_direction::memory_to_memory};
  static constexpr bool circular_mode{false}, double_buffer{false}, enable_can_fd{false};
  static constexpr std::uint32_t baudrate{500000U};
  static constexpr std::uint32_t polynomial{0x04C11DB7U};
  static constexpr bool enable_interrupts{true};
  static constexpr can_mode mode{can_mode::normal};
};
#if MICROHAL_NORDIC_HAS_H20_MRAM
struct storage_config
{
  using binding = nordic_system_validation::binding;
  static constexpr std::uint32_t program_alignment_bytes{16U}, erase_size_bytes{16U};
  static constexpr bool enable_dma{false}, enable_interrupts{false};
};
#endif
constexpr auto callback = [](status) noexcept {};
#if MICROHAL_NORDIC_HAS_QSPI
struct qspi_binding
{
  static constexpr bool owns_xip_window{true};
  static constexpr nrfx_qspi_config_t configuration = []
  {
    nrfx_qspi_config_t value = NRFX_QSPI_DEFAULT_CONFIG(0U, 1U, 2U, 3U, 4U, 5U);
    value.xip_offset = 0x1000U;
    value.phy_if.sck_freq = NRF_QSPI_FREQ_DIV2;
    value.prot_if.readoc = NRF_QSPI_READOC_FASTREAD;
    value.prot_if.writeoc = NRF_QSPI_WRITEOC_PP;
    value.prot_if.addrmode = NRF_QSPI_ADDRMODE_24BIT;
    return value;
  }();
};
#endif
void instantiate(std::span<std::uint8_t> data)
{
  (void)flash_api::erase<config>(0U, 4U);
  (void)flash_api::program<config>(0U, data);
  (void)flash_api::read<config>(0U, data);
  (void)flash_api::erase_region<config>(0U);
  (void)rram_api::erase(0U, 4U);
  (void)mram_api::erase(0U, 4U);
  (void)mram_api::program(0U, data);
  (void)mram_api::read(0U, data);
  (void)rtc_api::initialize<config>();
  (void)rtc_api::set_time<config>(23U, 59U, 59U);
  rtc::date_time date{.year = 2024U, .month = 2U, .day = 29U};
  (void)rtc_api::set_calendar<config>(date);
  (void)rtc_api::get_calendar<config>(date);
  (void)rtc_api::set_date_time<config>(date);
  (void)rtc_api::get_date_time<config>();
  (void)rtc_api::read_subsecond_us<config>();
  (void)rtc_api::enable_periodic_irq<config, callback>(32768U);
  (void)rtc_api::disable_periodic_irq<config>();
  (void)rtc_api::get_subsecond_us<config>();
  (void)rtc_api::enable_alarm<config, callback>(date);
  (void)rtc_api::enable_periodic<config, callback>(1000000U);
  (void)rtc_api::disable_periodic<config>();
  rtc_api::handle_interrupt();
  (void)usb_api::initialize<config>();
  (void)usb_api::connect<config>();
  (void)usb_api::endpoint_transmit<config>(1U, data);
  (void)usb_api::endpoint_receive<config>(1U, data);
  (void)usb_api::endpoint_status<config>(0x81U);
  (void)usb_api::endpoint_abort<config>(0x81U);
  (void)usb_api::endpoint_stall<config>(0x81U, true);
  (void)usb_api::disconnect<config>();
  usb_api::handle_interrupt<config>();
  (void)rcc_api::configure<config>();
  (void)rcc_api::get_system_clock_hz<config>();
  (void)rcc_api::enable_peripheral_clock<config>(peripheral_kind::rtc, 0U);
  (void)rcc_api::disable_peripheral_clock<config>(peripheral_kind::rtc, 0U);
  (void)crc_api::initialize<config>();
  (void)crc_api::get_random<config>();
  (void)dma_api::initialize<config>();
  (void)dma_api::start_transfer<config, callback>(0x20000000U, 0x20001000U, 16U);
  (void)dma_api::transfer_status<config>();
  (void)dma_api::abort<config>();
  dma_api::handle_interrupt();
  (void)can_api::initialize<config>();
  microhal::can::message message{};
  (void)can_api::transmit<config>(message);
  (void)can_api::receive<config>(message);
  (void)can_api::configure_filter<config>(1U, 0x7FFU, 0U);
  (void)can_api::enable_rx_interrupt<config, callback>();
  can_api::handle_interrupt();
}

// These are compile probes, never an executable hardware initialization sequence.
void instantiate_public(std::span<std::uint8_t> data)
{
#if MICROHAL_NORDIC_HAS_QSPI
  using memory = qspi::controller<nordic_nrf_tag, qspi::bound_controller_config<qspi_binding,
#if defined(NRF5340_XXAA_APPLICATION)
                                                                                48000000U
#else
                                                                                16000000U
#endif
                                                                                >>;
  constexpr qspi::command request{.instruction = 0x0BU,
                                  .instruction_mode = qspi_io_mode::single,
                                  .address_size_bytes = 3U,
                                  .address_mode = qspi_io_mode::single,
                                  .dummy_cycles = 8U,
                                  .data_mode = qspi_io_mode::single};
  static_assert(memory::is_valid_command(request) && memory::supports_command_mode(request));
  (void)memory::initialize();
  (void)memory::enter_memory_mapped(request);
  (void)memory::command_read(request, data);
  (void)memory::exit_memory_mapped();
  (void)memory::command_read(request, data);
#endif
#if MICROHAL_NORDIC_HAS_RTC || MICROHAL_NORDIC_HAS_GRTC
  using clock = rtc::controller<nordic_nrf_tag, config>;
  static_assert(backend_traits<nordic_nrf_tag>::rtc_capability().calendar);
  static_assert(backend_traits<nordic_nrf_tag>::rtc_capability().subsecond);
  (void)clock::initialize();
  (void)clock::set({.year = 2024U, .month = 2U, .day = 29U, .hour = 12U});
  (void)clock::read();
  (void)clock::read_subsecond_us();
  (void)clock::enable_alarm<callback>({.year = 2024U, .month = 3U, .day = 1U});
  (void)clock::enable_periodic_irq<callback>(1U);
  (void)clock::disable_periodic_irq();
  (void)clock::handle_interrupt();
#endif
#if MICROHAL_NORDIC_HAS_USBD
  using device = usb::device<nordic_nrf_tag, usb::bound_usb_config<binding>>;
  static_assert(device::endpoint_count == 8U && device::max_packet_size_bytes == 64U);
  (void)device::initialize();
  (void)device::connect();
  (void)device::endpoint_transmit(0x81U, data);
  (void)device::endpoint_receive(1U, data);
  (void)device::endpoint_status(0x81U);
  (void)device::endpoint_abort(0x81U);
  (void)device::endpoint_stall(0x81U, true);
  (void)device::handle_interrupt();
  (void)device::disconnect();
#endif
#if MICROHAL_NORDIC_HAS_RNG
  using entropy = crc::accelerator<nordic_nrf_tag, config>;
  static_assert(entropy::has_rng && !entropy::has_hardware_crc);
  (void)entropy::initialize();
  (void)entropy::read_random();
#endif
#if MICROHAL_NORDIC_HAS_H20_MRAM
  using storage = flash::controller<nordic_nrf_tag, storage_config>;
  static_assert(backend_traits<nordic_nrf_tag>::flash_capability().alignment_bytes == 16U);
  static_assert(backend_traits<nordic_nrf_tag>::flash_capability().erase_bytes == 16U);
  (void)storage::erase(binding::mram_start_address, 16U);
  (void)storage::program(binding::mram_start_address, data);
  (void)storage::read(binding::mram_start_address, data);
  (void)storage::erase_region(binding::mram_start_address);
  using pin95 = gpio::pin_config<9U, 5U, gpio_mode::output>;
  using pin613 = gpio::pin_config<6U, 13U, gpio_mode::output>;
  static_assert(GpioPinConcept<nordic_nrf_tag, pin95> && GpioPinConcept<nordic_nrf_tag, pin613>);
  static_assert(!GpioPinConcept<nordic_nrf_tag, gpio::pin_config<9U, 6U, gpio_mode::output>>);
  static_assert(!GpioPinConcept<nordic_nrf_tag, gpio::pin_config<6U, 14U, gpio_mode::output>>);
  (void)gpio::pin<nordic_nrf_tag, pin95>::configure();
  gpio::pin<nordic_nrf_tag, pin95>::set();
  (void)gpio::pin<nordic_nrf_tag, pin95>::read();
  (void)gpio::pin<nordic_nrf_tag, pin613>::configure();
  gpio::pin<nordic_nrf_tag, pin613>::clear();
  (void)gpio::pin<nordic_nrf_tag, pin613>::toggle();
#endif
#if MICROHAL_NORDIC_HAS_MCAN
  using bus = can::bus<nordic_nrf_tag, config>;
  static_assert(!bus::has_can_fd && bus::mailbox_count == 3U);
  static_assert(backend_traits<nordic_nrf_tag>::can_capability().filter_banks == 32U);
  static_assert(std::is_same_v<decltype(&can_api::configure_filter<config>),
                               status (*)(std::uint32_t, std::uint32_t, std::uint8_t) noexcept>);
  (void)bus::initialize();
  can::message message{};
  (void)bus::transmit(message);
  (void)bus::receive(message);
  (void)bus::configure_filter(1U, 0x7FFU, 0U);
  (void)bus::configure_filter(2U, 0x7F0U, 0U);
  (void)bus::configure_filter(1U, 0x7F0U, 0U);
  (void)bus::configure_filter(0x12345U, 0x1FFFFFFFU, 0U);
  (void)bus::configure_filter(0x23456U, 0x1FFFFFF0U, 0U);
  (void)bus::configure_filter(0x12345U, 0x1FFFFF00U, 0U);
  for (std::uint32_t index = 2U; index < 16U; ++index)
  {
    (void)bus::configure_filter(0x100U + index, 0x7FFU, 0U);
    (void)bus::configure_filter(0x100000U + index, 0x1FFFFFFFU, 0U);
  }
  (void)bus::configure_filter(0x777U, 0x7FFU, 0U);
  (void)bus::configure_filter(0x2FFFFFFU, 0x1FFFFFFFU, 0U);
  (void)bus::configure_filter(1U, 0x7FFU, 1U);
  (void)bus::enable_rx_interrupt<callback>();
#endif
#if MICROHAL_NORDIC_HAS_MVDMA
  using mover = dma::controller<nordic_nrf_tag, dma::dma_config<0U, 0U, 0U, dma::dma_direction::memory_to_memory>>;
  static_assert(mover::has_mem_to_mem);
  (void)mover::initialize();
  (void)mover::start_memcpy<callback>(0x20000000U, 0x20001000U, 16U);
  (void)mover::transfer_status();
  (void)mover::transfer_complete();
  (void)mover::abort();
#endif
  (void)data;
}
} // namespace nordic_system_validation
