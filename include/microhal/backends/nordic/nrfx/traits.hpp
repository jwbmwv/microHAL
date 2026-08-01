// SPDX-License-Identifier: Apache-2.0
/**
 * @file traits.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <array>
#include <cstring>
#include <cstdint>
#include <span>

#if __has_include("nrfx.h")
#define MICROHAL_NORDIC_NRFX_PRESENT 1
#else
#define MICROHAL_NORDIC_NRFX_PRESENT 0
#endif

#if __has_include("nrfx_twim.h")
#define MICROHAL_NORDIC_HAS_TWIM 1
#else
#define MICROHAL_NORDIC_HAS_TWIM 0
#endif

#if __has_include("nrfx_spim.h")
#define MICROHAL_NORDIC_HAS_SPIM 1
#else
#define MICROHAL_NORDIC_HAS_SPIM 0
#endif

#if __has_include("nrfx_qspi.h")
#define MICROHAL_NORDIC_HAS_QSPI 1
#else
#define MICROHAL_NORDIC_HAS_QSPI 0
#endif

#if __has_include("nrfx_sdhc.h")
#define MICROHAL_NORDIC_HAS_SDHC 1
#else
#define MICROHAL_NORDIC_HAS_SDHC 0
#endif

#if __has_include("nrfx_uarte.h")
#define MICROHAL_NORDIC_HAS_UARTE 1
#else
#define MICROHAL_NORDIC_HAS_UARTE 0
#endif

#if __has_include("nrfx_rtc.h")
#define MICROHAL_NORDIC_HAS_RTC 1
#else
#define MICROHAL_NORDIC_HAS_RTC 0
#endif

#if __has_include("nrfx_timer.h")
#define MICROHAL_NORDIC_HAS_TIMER 1
#else
#define MICROHAL_NORDIC_HAS_TIMER 0
#endif

#if __has_include("nrfx_saadc.h")
#define MICROHAL_NORDIC_HAS_SAADC 1
#else
#define MICROHAL_NORDIC_HAS_SAADC 0
#endif

#if __has_include("nrfx_dac.h")
#define MICROHAL_NORDIC_HAS_DAC 1
#else
#define MICROHAL_NORDIC_HAS_DAC 0
#endif

#if __has_include("nrfx_nvmc.h")
#define MICROHAL_NORDIC_HAS_NVMC 1
#else
#define MICROHAL_NORDIC_HAS_NVMC 0
#endif

#if __has_include("nrfx_pwm.h")
#define MICROHAL_NORDIC_HAS_PWM 1
#else
#define MICROHAL_NORDIC_HAS_PWM 0
#endif

#if __has_include("nrfx_wdt.h")
#define MICROHAL_NORDIC_HAS_WDT 1
#else
#define MICROHAL_NORDIC_HAS_WDT 0
#endif

#include "microhal/core/traits.hpp"

namespace microhal::backends::nordic::nrfx
{

struct state final
{
  inline static std::uint32_t timer_counter{0U};
  inline static std::uint8_t rtc_hour{0U};
  inline static std::uint8_t rtc_minute{0U};
  inline static std::uint8_t rtc_second{0U};
  inline static std::array<std::uint8_t, 64U * 1024U> flash_storage{};
  inline static std::uint16_t dac_last_a{0U};
  inline static std::uint16_t dac_last_b{0U};
  inline static std::uint16_t pwm_last_duty{0U};
};

struct gpio_api final
{
  template <typename PinConfig> static bool configure() noexcept
  {
    (void)sizeof(PinConfig);
    return MICROHAL_NORDIC_NRFX_PRESENT != 0;
  }

  template <typename PinConfig> static void write(const bool level) noexcept
  {
    (void)sizeof(PinConfig);
    (void)level;
  }

  template <typename PinConfig> static bool read() noexcept
  {
    (void)sizeof(PinConfig);
    return false;
  }

  template <typename PinConfig, auto Handler> static bool enable_interrupt() noexcept
  {
    (void)sizeof(PinConfig);
    (void)Handler;
    return MICROHAL_NORDIC_NRFX_PRESENT != 0;
  }
};

struct i2c_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return MICROHAL_NORDIC_HAS_TWIM != 0;
  }

  template <typename Config>
  static bool write_blocking(const std::span<const std::uint8_t> payload, const std::uint16_t address) noexcept
  {
    (void)sizeof(Config);
    (void)payload;
    (void)address;
    return MICROHAL_NORDIC_HAS_TWIM != 0;
  }

  template <typename Config>
  static bool read_blocking(const std::span<std::uint8_t> payload, const std::uint16_t address) noexcept
  {
    for (std::uint8_t& byte : payload)
    {
      byte = 0U;
    }
    (void)sizeof(Config);
    (void)address;
    return MICROHAL_NORDIC_HAS_TWIM != 0;
  }

  template <typename Config, auto Handler>
  static bool start_transfer(const std::span<std::uint8_t> payload, const std::uint16_t address) noexcept
  {
    (void)sizeof(Config);
    (void)payload;
    (void)address;
    (void)Handler;
    return MICROHAL_NORDIC_HAS_TWIM != 0;
  }
};

struct spi_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return MICROHAL_NORDIC_HAS_SPIM != 0;
  }

  template <typename Config>
  static bool transfer(const std::span<const std::uint8_t> tx_payload, const std::span<std::uint8_t> rx_payload) noexcept
  {
    const std::size_t count = (tx_payload.size() < rx_payload.size()) ? tx_payload.size() : rx_payload.size();
    for (std::size_t index = 0U; index < count; ++index)
    {
      rx_payload[index] = tx_payload[index];
    }
    (void)sizeof(Config);
    return MICROHAL_NORDIC_HAS_SPIM != 0;
  }

  template <typename Config, auto Handler>
  static bool transfer_async(const std::span<const std::uint8_t> tx_payload,
                             const std::span<std::uint8_t> rx_payload) noexcept
  {
    (void)Handler;
    return transfer<Config>(tx_payload, rx_payload);
  }
};

struct qspi_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return MICROHAL_NORDIC_HAS_QSPI != 0;
  }
};

struct uart_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return MICROHAL_NORDIC_HAS_UARTE != 0;
  }

  template <typename Config> static bool write_blocking(const std::span<const std::uint8_t> payload) noexcept
  {
    (void)sizeof(Config);
    return (MICROHAL_NORDIC_HAS_UARTE != 0) && !payload.empty();
  }

  template <typename Config> static bool read_blocking(const std::span<std::uint8_t> payload) noexcept
  {
    (void)sizeof(Config);
    for (std::uint8_t& byte : payload)
    {
      byte = 0U;
    }
    return MICROHAL_NORDIC_HAS_UARTE != 0;
  }

  template <typename Config, auto Handler> static bool start_async_rx(const std::span<std::uint8_t> payload) noexcept
  {
    (void)Handler;
    return read_blocking<Config>(payload);
  }

  template <typename Config, auto Handler> static bool start_async_tx(const std::span<const std::uint8_t> payload) noexcept
  {
    (void)Handler;
    return write_blocking<Config>(payload);
  }

  template <typename UartConfig, typename AutobaudConfig> static bool arm_autobaud() noexcept
  {
    (void)sizeof(UartConfig);
    (void)sizeof(AutobaudConfig);
    return false;
  }

  template <typename UartConfig> static bool apply_autobaud(const std::uint32_t baud_rate) noexcept
  {
    (void)sizeof(UartConfig);
    (void)baud_rate;
    return false;
  }
};

struct rtc_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    state::rtc_hour = 0U;
    state::rtc_minute = 0U;
    state::rtc_second = 0U;
    return MICROHAL_NORDIC_HAS_RTC != 0;
  }

  template <typename Config>
  static bool set_time(const std::uint8_t hours, const std::uint8_t minutes, const std::uint8_t seconds) noexcept
  {
    (void)sizeof(Config);
    state::rtc_hour = hours;
    state::rtc_minute = minutes;
    state::rtc_second = seconds;
    return MICROHAL_NORDIC_HAS_RTC != 0;
  }

  template <typename Config>
  static bool get_time(std::uint8_t& hours, std::uint8_t& minutes, std::uint8_t& seconds) noexcept
  {
    (void)sizeof(Config);
    hours = state::rtc_hour;
    minutes = state::rtc_minute;
    seconds = state::rtc_second;
    return MICROHAL_NORDIC_HAS_RTC != 0;
  }

  template <typename Config, auto Handler, typename AlarmTime> static bool enable_alarm(const AlarmTime&) noexcept
  {
    (void)sizeof(Config);
    (void)Handler;
    return MICROHAL_NORDIC_HAS_RTC != 0;
  }
};

struct flash_api final
{
  template <typename Config> static bool erase(const std::uint32_t address, const std::uint32_t bytes) noexcept
  {
    (void)sizeof(Config);
    if ((address + bytes) > state::flash_storage.size())
    {
      return false;
    }
    for (std::uint32_t index = 0U; index < bytes; ++index)
    {
      state::flash_storage[address + index] = 0xFFU;
    }
    return MICROHAL_NORDIC_HAS_NVMC != 0;
  }

  template <typename Config>
  static bool program(const std::uint32_t address, const std::span<const std::uint8_t> data) noexcept
  {
    (void)sizeof(Config);
    if ((address + data.size()) > state::flash_storage.size())
    {
      return false;
    }
    std::memcpy(state::flash_storage.data() + address, data.data(), data.size());
    return MICROHAL_NORDIC_HAS_NVMC != 0;
  }

  template <typename Config> static bool read(const std::uint32_t address, const std::span<std::uint8_t> data) noexcept
  {
    (void)sizeof(Config);
    if ((address + data.size()) > state::flash_storage.size())
    {
      return false;
    }
    std::memcpy(data.data(), state::flash_storage.data() + address, data.size());
    return MICROHAL_NORDIC_HAS_NVMC != 0;
  }
};

struct timer_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    state::timer_counter = 0U;
    return MICROHAL_NORDIC_HAS_TIMER != 0;
  }

  template <typename Config> static std::uint32_t counter() noexcept
  {
    (void)sizeof(Config);
    return state::timer_counter++;
  }
};

struct adc_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return MICROHAL_NORDIC_HAS_SAADC != 0;
  }

  template <typename Config> static std::uint16_t read_blocking(const std::uint8_t channel) noexcept
  {
    (void)sizeof(Config);
    return static_cast<std::uint16_t>(0x100U + channel);
  }

  template <typename Config> static bool start_continuous(const std::span<const std::uint8_t> channels) noexcept
  {
    (void)sizeof(Config);
    return (MICROHAL_NORDIC_HAS_SAADC != 0) && !channels.empty();
  }

  template <typename Config, auto Handler> static bool start_async(const std::uint8_t channel) noexcept
  {
    (void)sizeof(Config);
    (void)channel;
    (void)Handler;
    return MICROHAL_NORDIC_HAS_SAADC != 0;
  }

  template <typename Config, auto Handler>
  static bool start_dma_scan(const std::span<const std::uint8_t> channels,
                             const std::span<std::uint16_t> buffer) noexcept
  {
    (void)sizeof(Config);
    (void)Handler;
    const std::size_t count = (channels.size() < buffer.size()) ? channels.size() : buffer.size();
    for (std::size_t index = 0U; index < count; ++index)
    {
      buffer[index] = static_cast<std::uint16_t>(0x200U + channels[index]);
    }
    return MICROHAL_NORDIC_HAS_SAADC != 0;
  }
};

struct dac_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return MICROHAL_NORDIC_HAS_DAC != 0;
  }

  template <typename Config> static bool write_blocking(const std::uint16_t value) noexcept
  {
    (void)sizeof(Config);
    state::dac_last_a = value;
    state::dac_last_b = value;
    return MICROHAL_NORDIC_HAS_DAC != 0;
  }

  template <typename Config> static bool write_dual(const std::uint16_t a, const std::uint16_t b) noexcept
  {
    (void)sizeof(Config);
    state::dac_last_a = a;
    state::dac_last_b = b;
    return MICROHAL_NORDIC_HAS_DAC != 0;
  }

  template <typename Config, auto Handler>
  static bool start_dma_waveform(const std::span<const std::uint16_t> waveform) noexcept
  {
    (void)sizeof(Config);
    (void)Handler;
    if (waveform.empty())
    {
      return false;
    }
    state::dac_last_a = waveform.front();
    state::dac_last_b = waveform.back();
    return MICROHAL_NORDIC_HAS_DAC != 0;
  }
};

struct can_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return false;
  }

  template <typename Config> static bool transmit(const microhal::can::message&) noexcept
  {
    (void)sizeof(Config);
    return false;
  }

  template <typename Config> static bool receive(microhal::can::message&) noexcept
  {
    (void)sizeof(Config);
    return false;
  }

  template <typename Config> static bool configure_filter(const std::uint32_t, const std::uint32_t, const std::uint32_t) noexcept
  {
    (void)sizeof(Config);
    return false;
  }

  template <typename Config, auto Handler> static bool enable_rx_interrupt() noexcept
  {
    (void)sizeof(Config);
    (void)Handler;
    return false;
  }
};

struct usb_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return false;
  }

  template <typename Config> static bool connect() noexcept
  {
    (void)sizeof(Config);
    return false;
  }

  template <typename Config> static bool disconnect() noexcept
  {
    (void)sizeof(Config);
    return false;
  }

  template <typename Config>
  static bool endpoint_transmit(const std::uint8_t endpoint, const std::span<const std::uint8_t> data) noexcept
  {
    (void)sizeof(Config);
    (void)endpoint;
    (void)data;
    return false;
  }

  template <typename Config>
  static std::uint32_t endpoint_receive(const std::uint8_t endpoint, const std::span<std::uint8_t> buffer) noexcept
  {
    (void)sizeof(Config);
    (void)endpoint;
    (void)buffer;
    return 0U;
  }
};

struct ethernet_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return false;
  }

  template <typename Config> static bool start() noexcept
  {
    (void)sizeof(Config);
    return false;
  }

  template <typename Config> static bool stop() noexcept
  {
    (void)sizeof(Config);
    return false;
  }

  template <typename Config> static bool transmit_frame(const std::span<const std::uint8_t> frame) noexcept
  {
    (void)sizeof(Config);
    (void)frame;
    return false;
  }

  template <typename Config> static std::uint32_t receive_frame(const std::span<std::uint8_t> buffer) noexcept
  {
    (void)sizeof(Config);
    (void)buffer;
    return 0U;
  }

  template <typename Config> static bool set_mac_address(const std::array<std::uint8_t, 6>& address) noexcept
  {
    (void)sizeof(Config);
    (void)address;
    return false;
  }
};

struct sd_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return MICROHAL_NORDIC_HAS_SDHC != 0;
  }

  template <typename Config>
  static bool read_blocks(const std::uint32_t start_block, const std::span<std::uint8_t> buffer) noexcept
  {
    (void)sizeof(Config);
    (void)start_block;
    for (std::uint8_t& byte : buffer)
    {
      byte = 0U;
    }
    return MICROHAL_NORDIC_HAS_SDHC != 0;
  }

  template <typename Config>
  static bool write_blocks(const std::uint32_t start_block, const std::span<const std::uint8_t> buffer) noexcept
  {
    (void)sizeof(Config);
    (void)start_block;
    (void)buffer;
    return MICROHAL_NORDIC_HAS_SDHC != 0;
  }

  template <typename Config>
  static bool erase_blocks(const std::uint32_t start_block, const std::uint32_t block_count) noexcept
  {
    (void)sizeof(Config);
    (void)start_block;
    (void)block_count;
    return MICROHAL_NORDIC_HAS_SDHC != 0;
  }

  template <typename Config> static std::uint32_t capacity_blocks() noexcept
  {
    (void)sizeof(Config);
    return (MICROHAL_NORDIC_HAS_SDHC != 0) ? 65'536U : 0U;
  }
};

struct dma_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return MICROHAL_NORDIC_NRFX_PRESENT != 0;
  }

  template <typename Config, auto Handler>
  static bool start_transfer(const std::uint32_t src_addr, const std::uint32_t dst_addr,
                             const std::uint32_t length) noexcept
  {
    (void)sizeof(Config);
    (void)Handler;
    if ((src_addr == 0U) || (dst_addr == 0U) || (length == 0U))
    {
      return false;
    }
    std::memcpy(reinterpret_cast<void*>(dst_addr), reinterpret_cast<const void*>(src_addr), length);
    return MICROHAL_NORDIC_NRFX_PRESENT != 0;
  }

  template <typename Config> static bool abort() noexcept
  {
    (void)sizeof(Config);
    return MICROHAL_NORDIC_NRFX_PRESENT != 0;
  }
};

struct pwm_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    state::pwm_last_duty = 0U;
    return MICROHAL_NORDIC_HAS_PWM != 0;
  }

  template <typename Config> static bool set_duty_cycle(const std::uint16_t duty_permille) noexcept
  {
    (void)sizeof(Config);
    state::pwm_last_duty = duty_permille;
    return (MICROHAL_NORDIC_HAS_PWM != 0) && (duty_permille <= 1000U);
  }

  template <typename Config> static bool start() noexcept
  {
    (void)sizeof(Config);
    return MICROHAL_NORDIC_HAS_PWM != 0;
  }

  template <typename Config> static bool stop() noexcept
  {
    (void)sizeof(Config);
    return MICROHAL_NORDIC_HAS_PWM != 0;
  }
};

struct watchdog_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return MICROHAL_NORDIC_HAS_WDT != 0;
  }

  template <typename Config> static bool start() noexcept
  {
    (void)sizeof(Config);
    return MICROHAL_NORDIC_HAS_WDT != 0;
  }

  template <typename Config> static bool refresh() noexcept
  {
    (void)sizeof(Config);
    return MICROHAL_NORDIC_HAS_WDT != 0;
  }
};

struct i2s_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return false;
  }

  template <typename Config> static bool transmit_blocking(const std::span<const std::uint16_t> samples) noexcept
  {
    (void)sizeof(Config);
    (void)samples;
    return false;
  }

  template <typename Config> static bool receive_blocking(const std::span<std::uint16_t> samples) noexcept
  {
    (void)sizeof(Config);
    (void)samples;
    return false;
  }

  template <typename Config, auto Handler>
  static bool start_dma_tx(const std::span<const std::uint16_t> samples) noexcept
  {
    (void)sizeof(Config);
    (void)samples;
    (void)Handler;
    return false;
  }
};

struct rcc_api final
{
  template <typename Config> static bool configure() noexcept
  {
    (void)sizeof(Config);
    return false;
  }

  template <typename Config> static std::uint32_t get_system_clock_hz() noexcept
  {
    (void)sizeof(Config);
    return 0U;
  }

  template <typename Config>
  static void enable_peripheral_clock(const peripheral_kind peripheral, const std::uint8_t instance) noexcept
  {
    (void)sizeof(Config);
    (void)peripheral;
    (void)instance;
  }

  template <typename Config>
  static void disable_peripheral_clock(const peripheral_kind peripheral, const std::uint8_t instance) noexcept
  {
    (void)sizeof(Config);
    (void)peripheral;
    (void)instance;
  }
};

struct crc_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    (void)sizeof(Config);
    return false;
  }

  template <typename Config> static std::uint32_t calculate(const std::span<const std::uint8_t> data) noexcept
  {
    (void)sizeof(Config);
    (void)data;
    return 0U;
  }

  template <typename Config> static void reset() noexcept
  {
    (void)sizeof(Config);
  }

  template <typename Config> static std::uint32_t get_random() noexcept
  {
    (void)sizeof(Config);
    return 0U;
  }
};

} // namespace microhal::backends::nordic::nrfx

namespace microhal
{

template <> struct backend_traits<nordic_nrf_tag> final
{
  using backend_tag = nordic_nrf_tag;
  using gpio_api = backends::nordic::nrfx::gpio_api;
  using i2c_api = backends::nordic::nrfx::i2c_api;
  using spi_api = backends::nordic::nrfx::spi_api;
  using qspi_api = backends::nordic::nrfx::qspi_api;
  using uart_api = backends::nordic::nrfx::uart_api;
  using rtc_api = backends::nordic::nrfx::rtc_api;
  using flash_api = backends::nordic::nrfx::flash_api;
  using timer_api = backends::nordic::nrfx::timer_api;
  using adc_api = backends::nordic::nrfx::adc_api;
  using dac_api = backends::nordic::nrfx::dac_api;
  using can_api = backends::nordic::nrfx::can_api;
  using usb_api = backends::nordic::nrfx::usb_api;
  using ethernet_api = backends::nordic::nrfx::ethernet_api;
  using sd_api = backends::nordic::nrfx::sd_api;
  using dma_api = backends::nordic::nrfx::dma_api;
  using pwm_api = backends::nordic::nrfx::pwm_api;
  using watchdog_api = backends::nordic::nrfx::watchdog_api;
  using i2s_api = backends::nordic::nrfx::i2s_api;
  using rcc_api = backends::nordic::nrfx::rcc_api;
  using crc_api = backends::nordic::nrfx::crc_api;

  static constexpr vendor_kind vendor = vendor_kind::nordic_nrf;
  static constexpr backend_kind kind = backend_kind::nordic_nrfx;
  static constexpr std::string_view name{"nordic-nrfx"};
  static constexpr std::uint32_t gpio_port_count{2U};
  static constexpr std::uint32_t gpio_pins_per_port{32U};
  static constexpr std::uint32_t gpio_alternate_function_count{16U};

  static consteval struct gpio_capability gpio_capability() noexcept
  {
    return {.supported = MICROHAL_NORDIC_NRFX_PRESENT != 0,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = false,
            .instance_count = gpio_port_count,
            .max_frequency_hz = 0U,
            .alternate_function = true,
            .alternate_function_count = gpio_alternate_function_count,
            .port_count = gpio_port_count,
            .pins_per_port = gpio_pins_per_port};
  }

  static consteval struct i2c_capability i2c_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_TWIM != 0;
    return {.supported = supported,
            .blocking = supported,
            .non_blocking = supported,
            .isr = supported,
            .dma = false,
            .instance_count = supported ? 2U : 0U,
            .max_frequency_hz = 1'000'000U};
  }

  static consteval struct spi_capability spi_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_SPIM != 0;
    return {.supported = supported,
            .blocking = supported,
            .non_blocking = supported,
            .isr = supported,
            .dma = supported,
            .instance_count = supported ? 4U : 0U,
            .max_frequency_hz = 32'000'000U};
  }

  static consteval struct qspi_capability qspi_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_QSPI != 0;
    return {.supported = supported,
            .blocking = supported,
            .non_blocking = false,
            .isr = false,
            .dma = supported,
            .instance_count = supported ? 1U : 0U,
            .max_frequency_hz = 96'000'000U,
            .quad_mode = supported,
            .octo_mode = false,
            .ddr_mode = false};
  }

  static consteval struct uart_capability uart_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_UARTE != 0;
    return {.supported = supported,
            .blocking = supported,
            .non_blocking = supported,
            .isr = supported,
            .dma = supported,
            .instance_count = supported ? 2U : 0U,
            .max_frequency_hz = 8'000'000U,
            .fifo = supported,
            .lpuart = false,
            .autobaud = false,
            .fifo_depth = supported ? 8U : 0U};
  }

  static consteval struct rtc_capability rtc_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_RTC != 0;
    return {.supported = supported,
            .blocking = supported,
            .non_blocking = supported,
            .isr = supported,
            .dma = false,
            .instance_count = supported ? 3U : 0U,
            .max_frequency_hz = 32'768U,
            .periodic = supported,
            .alarm = supported};
  }

  static consteval struct flash_capability flash_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_NVMC != 0;
    return {.supported = supported,
            .blocking = supported,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = supported ? 1U : 0U,
            .max_frequency_hz = 0U,
            .alignment_bytes = 4U,
            .erase_bytes = 4096U,
            .total_size_kb = 64U};
  }

  static consteval struct timer_capability timer_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_TIMER != 0;
    return {.supported = supported,
            .blocking = supported,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = supported ? 5U : 0U,
            .max_frequency_hz = 16'000'000U,
            .capture_compare = supported,
            .periodic = supported,
            .pwm = false,
            .channel_count = supported ? 4U : 0U};
  }

  static consteval struct adc_capability adc_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_SAADC != 0;
    return {.supported = supported,
            .blocking = supported,
            .non_blocking = supported,
            .isr = supported,
            .dma = supported,
            .instance_count = supported ? 1U : 0U,
            .max_frequency_hz = 200'000U,
            .resolution_bits = 12U,
            .channel_count = supported ? 8U : 0U,
            .differential = supported,
            .oversampling = supported,
            .injected = false};
  }

  static consteval struct dac_capability dac_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_DAC != 0;
    return {.supported = supported,
            .blocking = supported,
            .non_blocking = supported,
            .isr = supported,
            .dma = supported,
            .instance_count = supported ? 1U : 0U,
            .max_frequency_hz = 1'000'000U,
            .resolution_bits = 12U,
            .channel_count = supported ? 2U : 0U,
            .dual_mode = supported,
            .waveform_generation = supported};
  }

  static consteval struct can_capability can_capability() noexcept
  {
    return {};
  }

  static consteval struct usb_capability usb_capability() noexcept
  {
    return {};
  }

  static consteval struct ethernet_capability ethernet_capability() noexcept
  {
    return {};
  }

  static consteval struct sd_capability sd_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_SDHC != 0;
    return {.supported = supported,
            .blocking = supported,
            .non_blocking = supported,
            .isr = supported,
            .dma = supported,
            .instance_count = supported ? 1U : 0U,
            .max_frequency_hz = 50'000'000U,
            .high_speed = supported,
            .uhs = false,
            .bus_width_4 = supported,
            .bus_width_8 = false,
            .block_size_bytes = 512U};
  }

  static consteval struct dma_capability dma_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_NRFX_PRESENT != 0;
    return {.supported = supported,
            .blocking = false,
            .non_blocking = supported,
            .isr = supported,
            .dma = false,
            .instance_count = supported ? 1U : 0U,
            .max_frequency_hz = 0U,
            .stream_count = supported ? 16U : 0U,
            .channel_count = supported ? 16U : 0U,
            .mem_to_mem = supported,
            .fifo = false,
            .double_buffer = false};
  }

  static consteval struct pwm_capability pwm_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_PWM != 0;
    return {.supported = supported,
            .blocking = supported,
            .non_blocking = supported,
            .isr = supported,
            .dma = supported,
            .instance_count = supported ? 4U : 0U,
            .max_frequency_hz = 16'000'000U,
            .channel_count = supported ? 4U : 0U,
            .complementary_output = false,
            .dead_time = false};
  }

  static consteval struct watchdog_capability watchdog_capability() noexcept
  {
    constexpr bool supported = MICROHAL_NORDIC_HAS_WDT != 0;
    return {.supported = supported,
            .blocking = supported,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = supported ? 1U : 0U,
            .max_frequency_hz = 0U,
            .independent = supported,
            .window = supported,
            .min_timeout_ms = supported ? 1U : 0U,
            .max_timeout_ms = supported ? 120'000U : 0U};
  }

  static consteval struct i2s_capability i2s_capability() noexcept
  {
    return {};
  }

  static consteval struct rcc_capability rcc_capability() noexcept
  {
    return {};
  }

  static consteval struct crc_capability crc_capability() noexcept
  {
    return {};
  }

  template <typename PinConfig> static consteval bool is_valid_gpio_pin() noexcept
  {
    return (PinConfig::port_index < gpio_port_count) && (PinConfig::pin_index < gpio_pins_per_port);
  }

  template <typename PinConfig> static consteval bool is_valid_alternate_function() noexcept
  {
    return (PinConfig::alternate_function == gpio_no_alternate) ||
           (PinConfig::alternate_function < gpio_alternate_function_count);
  }
};

} // namespace microhal
