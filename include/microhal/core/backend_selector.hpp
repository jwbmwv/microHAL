// SPDX-License-Identifier: Apache-2.0
/**
 * @file backend_selector.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "microhal/core/capabilities.hpp"

namespace microhal
{

/// @brief Maps a backend kind to its registered backend tag.
/// @tparam Kind Backend kind with an enabled backend specialization.
template <backend_kind Kind> struct backend_tag_from_kind;

#if MICROHAL_ENABLE_STM32_HAL
/// @brief Maps a backend kind to its registered backend tag.
template <> struct backend_tag_from_kind<backend_kind::stm32_cube_hal> final
{
  using type = stm32_hal_tag;
};
#endif

#if MICROHAL_ENABLE_STM32_LL
/// @brief Maps a backend kind to its registered backend tag.
template <> struct backend_tag_from_kind<backend_kind::stm32_cube_ll> final
{
  using type = stm32_ll_tag;
};
#endif

#if MICROHAL_ENABLE_KINETIS_ESPRESSO
/// @brief Maps a backend kind to its registered backend tag.
template <> struct backend_tag_from_kind<backend_kind::kinetis_espresso> final
{
  using type = kinetis_espresso_tag;
};
#endif

#if MICROHAL_ENABLE_ZEPHYR
/// @brief Maps a backend kind to its registered backend tag.
template <> struct backend_tag_from_kind<backend_kind::zephyr_driver_model> final
{
  using type = zephyr_tag;
};
#endif

#if MICROHAL_ENABLE_NUTTX
/// @brief Maps a backend kind to its registered backend tag.
template <> struct backend_tag_from_kind<backend_kind::nuttx_driver_model> final
{
  using type = nuttx_tag;
};
#endif

#if MICROHAL_ENABLE_NORDIC_NRFX
/// @brief Maps a backend kind to its registered backend tag.
template <> struct backend_tag_from_kind<backend_kind::nordic_nrfx> final
{
  using type = nordic_nrf_tag;
};
#endif

#if MICROHAL_ENABLE_MOCK
/// @brief Maps a backend kind to its registered backend tag.
template <> struct backend_tag_from_kind<backend_kind::mock_recording> final
{
  using type = mock_tag;
};
#endif

/// @brief Names the backend tag corresponding to a backend kind.
/// @tparam Kind Backend kind with an enabled backend specialization.
template <backend_kind Kind> using backend_tag_from_kind_t = typename backend_tag_from_kind<Kind>::type;

namespace detail
{

/// @brief Selects the backend's unadapted GPIO API type.
template <typename Backend> using raw_gpio_api_t = typename backend_traits<Backend>::gpio_api;
/// @brief Selects the backend's unadapted I2C API type.
template <typename Backend> using raw_i2c_api_t = typename backend_traits<Backend>::i2c_api;
/// @brief Selects the backend's unadapted SPI API type.
template <typename Backend> using raw_spi_api_t = typename backend_traits<Backend>::spi_api;
/// @brief Selects the backend's unadapted QSPI API type.
template <typename Backend> using raw_qspi_api_t = typename backend_traits<Backend>::qspi_api;
/// @brief Selects the backend's unadapted UART API type.
template <typename Backend> using raw_uart_api_t = typename backend_traits<Backend>::uart_api;
/// @brief Selects the backend's unadapted RTC API type.
template <typename Backend> using raw_rtc_api_t = typename backend_traits<Backend>::rtc_api;
/// @brief Selects the backend's unadapted flash API type.
template <typename Backend> using raw_flash_api_t = typename backend_traits<Backend>::flash_api;
/// @brief Selects the backend's unadapted timer API type.
template <typename Backend> using raw_timer_api_t = typename backend_traits<Backend>::timer_api;
/// @brief Selects the backend's unadapted ADC API type.
template <typename Backend> using raw_adc_api_t = typename backend_traits<Backend>::adc_api;
/// @brief Selects the backend's unadapted DAC API type.
template <typename Backend> using raw_dac_api_t = typename backend_traits<Backend>::dac_api;
/// @brief Selects the backend's unadapted CAN API type.
template <typename Backend> using raw_can_api_t = typename backend_traits<Backend>::can_api;
/// @brief Selects the backend's unadapted USB API type.
template <typename Backend> using raw_usb_api_t = typename backend_traits<Backend>::usb_api;
/// @brief Selects the backend's unadapted Ethernet API type.
template <typename Backend> using raw_ethernet_api_t = typename backend_traits<Backend>::ethernet_api;
/// @brief Selects the backend's unadapted SD API type.
template <typename Backend> using raw_sd_api_t = typename backend_traits<Backend>::sd_api;
/// @brief Selects the backend's unadapted DMA API type.
template <typename Backend> using raw_dma_api_t = typename backend_traits<Backend>::dma_api;
/// @brief Selects the backend's unadapted PWM API type.
template <typename Backend> using raw_pwm_api_t = typename backend_traits<Backend>::pwm_api;
/// @brief Selects the backend's unadapted watchdog API type.
template <typename Backend> using raw_watchdog_api_t = typename backend_traits<Backend>::watchdog_api;
/// @brief Selects the backend's unadapted I2S API type.
template <typename Backend> using raw_i2s_api_t = typename backend_traits<Backend>::i2s_api;
/// @brief Selects the backend's unadapted RCC API type.
template <typename Backend> using raw_rcc_api_t = typename backend_traits<Backend>::rcc_api;
/// @brief Selects the backend's unadapted CRC API type.
template <typename Backend> using raw_crc_api_t = typename backend_traits<Backend>::crc_api;

/// @brief Adapts the selected backend's I2C API to the common facade.
template <typename Backend> struct i2c_api_adapter final
{
  /// @copybrief microhal::i2c::master::initialize
  template <typename Config> static auto initialize() noexcept
  {
    return raw_i2c_api_t<Backend>::template initialize<Config>();
  }

  /// @copybrief microhal::i2c::master::write_blocking
  template <typename Config>
  static auto write_blocking(const std::span<const std::uint8_t> payload, const std::uint16_t address) noexcept
  {
    if constexpr (requires { raw_i2c_api_t<Backend>::template write_blocking<Config>(payload, address); })
    {
      return raw_i2c_api_t<Backend>::template write_blocking<Config>(payload, address);
    }
    else
    {
      return raw_i2c_api_t<Backend>::template write_blocking<Config>(static_cast<std::uint8_t>(address), payload.data(),
                                                                     payload.size());
    }
  }

  /// @copybrief microhal::i2c::master::read_blocking
  template <typename Config>
  static auto read_blocking(const std::span<std::uint8_t> payload, const std::uint16_t address) noexcept
  {
    if constexpr (requires { raw_i2c_api_t<Backend>::template read_blocking<Config>(payload, address); })
    {
      return raw_i2c_api_t<Backend>::template read_blocking<Config>(payload, address);
    }
    else if constexpr (requires {
                         raw_i2c_api_t<Backend>::template read_blocking<Config>(static_cast<std::uint8_t>(address),
                                                                                payload.data(), payload.size());
                       })
    {
      return raw_i2c_api_t<Backend>::template read_blocking<Config>(static_cast<std::uint8_t>(address), payload.data(),
                                                                    payload.size());
    }
    else
    {
      (void)payload;
      (void)address;
      return false;
    }
  }

  /// @brief Writes data and then reads data using the blocking transfer path.
  template <typename Config>
  static auto write_read_blocking(const std::span<const std::uint8_t> tx_payload,
                                  const std::span<std::uint8_t> rx_payload, const std::uint16_t address) noexcept
  {
    if constexpr (requires {
                    raw_i2c_api_t<Backend>::template write_read_blocking<Config>(tx_payload, rx_payload, address);
                  })
    {
      return raw_i2c_api_t<Backend>::template write_read_blocking<Config>(tx_payload, rx_payload, address);
    }
    else
    {
      (void)tx_payload;
      (void)rx_payload;
      (void)address;
      return status::unsupported;
    }
  }

  /// @brief Starts an asynchronous write followed by a read.
  template <typename Config, auto Handler>
  static auto write_read_async(const std::span<const std::uint8_t> tx_payload, const std::span<std::uint8_t> rx_payload,
                               const std::uint16_t address) noexcept
  {
    if constexpr (requires {
                    raw_i2c_api_t<Backend>::template write_read_async<Config, Handler>(tx_payload, rx_payload, address);
                  })
    {
      return raw_i2c_api_t<Backend>::template write_read_async<Config, Handler>(tx_payload, rx_payload, address);
    }
    else
    {
      (void)tx_payload;
      (void)rx_payload;
      (void)address;
      (void)Handler;
      return status::unsupported;
    }
  }

  /// @copybrief microhal::i2c::master::probe
  template <typename Config> static auto probe(const std::uint16_t address) noexcept
  {
    if constexpr (requires { raw_i2c_api_t<Backend>::template probe<Config>(address); })
    {
      return raw_i2c_api_t<Backend>::template probe<Config>(address);
    }
    else
    {
      (void)address;
      return status::unsupported;
    }
  }

  /// @brief Starts the transfer for the I2C peripheral.
  template <typename Config, auto Handler>
  static auto start_transfer(const std::span<std::uint8_t> payload, const std::uint16_t address) noexcept
  {
    if constexpr (requires { raw_i2c_api_t<Backend>::template start_transfer<Config, Handler>(payload, address); })
    {
      return raw_i2c_api_t<Backend>::template start_transfer<Config, Handler>(payload, address);
    }
    else
    {
      (void)payload;
      (void)address;
      (void)Handler;
      return false;
    }
  }
};

/// @brief Adapts the selected backend's QSPI API to the common facade.
template <typename Backend> struct qspi_api_adapter final
{
  /// @brief Initializes the configured backend resource.
  template <typename Config> static auto initialize() noexcept
  {
    return raw_qspi_api_t<Backend>::template initialize<Config>();
  }

  /// @copybrief microhal::qspi::controller::command_write
  template <typename Config>
  static auto command_write(const qspi_command& request, const std::span<const std::uint8_t> payload) noexcept
  {
    if constexpr (requires { raw_qspi_api_t<Backend>::template command_write<Config>(request, payload); })
    {
      return raw_qspi_api_t<Backend>::template command_write<Config>(request, payload);
    }
    else
    {
      (void)request;
      (void)payload;
      return status::unsupported;
    }
  }

  /// @copybrief microhal::qspi::controller::command_read
  template <typename Config>
  static auto command_read(const qspi_command& request, const std::span<std::uint8_t> payload) noexcept
  {
    if constexpr (requires { raw_qspi_api_t<Backend>::template command_read<Config>(request, payload); })
    {
      return raw_qspi_api_t<Backend>::template command_read<Config>(request, payload);
    }
    else
    {
      (void)request;
      (void)payload;
      return status::unsupported;
    }
  }

  /// @brief Enters the configured QSPI memory-mapped mode.
  template <typename Config> static auto enter_memory_mapped(const qspi_command& request) noexcept
  {
    if constexpr (requires { raw_qspi_api_t<Backend>::template enter_memory_mapped<Config>(request); })
    {
      return raw_qspi_api_t<Backend>::template enter_memory_mapped<Config>(request);
    }
    else
    {
      (void)request;
      return status::unsupported;
    }
  }

  /// @copybrief microhal::qspi::controller::exit_memory_mapped
  template <typename Config> static auto exit_memory_mapped() noexcept
  {
    if constexpr (requires { raw_qspi_api_t<Backend>::template exit_memory_mapped<Config>(); })
    {
      return raw_qspi_api_t<Backend>::template exit_memory_mapped<Config>();
    }
    else
    {
      return status::unsupported;
    }
  }
};

/// @brief Adapts the selected backend's SPI API to the common facade.
template <typename Backend> struct spi_api_adapter final
{
  /// @copybrief microhal::spi::master::initialize
  template <typename Config> static auto initialize() noexcept
  {
    return raw_spi_api_t<Backend>::template initialize<Config>();
  }

  /// @brief Transfers data using the configured peripheral.
  template <typename Config>
  static auto transfer(const std::span<const std::uint8_t> tx_payload,
                       const std::span<std::uint8_t> rx_payload) noexcept
  {
    if constexpr (requires { raw_spi_api_t<Backend>::template transfer<Config>(tx_payload, rx_payload); })
    {
      return raw_spi_api_t<Backend>::template transfer<Config>(tx_payload, rx_payload);
    }
    else
    {
      return raw_spi_api_t<Backend>::template transfer<Config>(
          tx_payload.data(), rx_payload.data(),
          (tx_payload.size() < rx_payload.size()) ? tx_payload.size() : rx_payload.size());
    }
  }

  /// @copybrief microhal::spi::master::write_blocking
  template <typename Config> static auto write_blocking(const std::span<const std::uint8_t> payload) noexcept
  {
    if constexpr (requires { raw_spi_api_t<Backend>::template write_blocking<Config>(payload); })
    {
      return raw_spi_api_t<Backend>::template write_blocking<Config>(payload);
    }
    else
    {
      (void)payload;
      return status::unsupported;
    }
  }

  /// @copybrief microhal::spi::master::read_blocking
  template <typename Config> static auto read_blocking(const std::span<std::uint8_t> payload) noexcept
  {
    if constexpr (requires { raw_spi_api_t<Backend>::template read_blocking<Config>(payload); })
    {
      return raw_spi_api_t<Backend>::template read_blocking<Config>(payload);
    }
    else
    {
      (void)payload;
      return status::unsupported;
    }
  }

  /// @brief Starts an asynchronous transfer using the configured peripheral.
  template <typename Config, auto Handler>
  static auto transfer_async(const std::span<const std::uint8_t> tx_payload,
                             const std::span<std::uint8_t> rx_payload) noexcept
  {
    if constexpr (requires {
                    raw_spi_api_t<Backend>::template transfer_async<Config, Handler>(tx_payload, rx_payload);
                  })
    {
      return raw_spi_api_t<Backend>::template transfer_async<Config, Handler>(tx_payload, rx_payload);
    }
    else
    {
      (void)tx_payload;
      (void)rx_payload;
      (void)Handler;
      return false;
    }
  }
};

/// @brief Adapts the selected backend's UART API to the common facade.
template <typename Backend> struct uart_api_adapter final
{
  /// @copybrief microhal::uart::port::initialize
  template <typename Config> static auto initialize() noexcept
  {
    return raw_uart_api_t<Backend>::template initialize<Config>();
  }

  /// @copybrief microhal::uart::port::write_blocking
  template <typename Config> static auto write_blocking(const std::span<const std::uint8_t> payload) noexcept
  {
    return raw_uart_api_t<Backend>::template write_blocking<Config>(payload);
  }

  /// @copybrief microhal::uart::port::read_blocking
  template <typename Config> static auto read_blocking(const std::span<std::uint8_t> payload) noexcept
  {
    return raw_uart_api_t<Backend>::template read_blocking<Config>(payload);
  }

  /// @brief Starts an asynchronous receive operation.
  template <typename Config, auto Handler> static auto start_async_rx(const std::span<std::uint8_t> payload) noexcept
  {
    return raw_uart_api_t<Backend>::template start_async_rx<Config, Handler>(payload);
  }

  /// @brief Starts an asynchronous transmit operation.
  template <typename Config, auto Handler>
  static auto start_async_tx(const std::span<const std::uint8_t> payload) noexcept
  {
    if constexpr (requires { raw_uart_api_t<Backend>::template start_async_tx<Config, Handler>(payload); })
    {
      return raw_uart_api_t<Backend>::template start_async_tx<Config, Handler>(payload);
    }
    else
    {
      (void)payload;
      (void)Handler;
      return false;
    }
  }

  /// @brief Arms the autobaud for the UART peripheral.
  template <typename UartConfig, typename AutobaudConfig> static auto arm_autobaud() noexcept
  {
    if constexpr (requires { raw_uart_api_t<Backend>::template arm_autobaud<UartConfig, AutobaudConfig>(); })
    {
      return raw_uart_api_t<Backend>::template arm_autobaud<UartConfig, AutobaudConfig>();
    }
    else
    {
      return false;
    }
  }

  /// @brief Applies the autobaud for the UART peripheral.
  template <typename UartConfig> static auto apply_autobaud(const std::uint32_t baud_rate) noexcept
  {
    if constexpr (requires { raw_uart_api_t<Backend>::template apply_autobaud<UartConfig>(baud_rate); })
    {
      return raw_uart_api_t<Backend>::template apply_autobaud<UartConfig>(baud_rate);
    }
    else
    {
      (void)baud_rate;
      return false;
    }
  }
};

/// @brief Adapts the selected backend's GPIO API to the common facade.
template <typename Backend> struct gpio_api_adapter final
{
  /// @brief Configures the selected backend resource.
  template <typename PinConfig> static auto configure() noexcept
  {
    return raw_gpio_api_t<Backend>::template configure<PinConfig>();
  }

  /// @brief Writes data using the configured backend.
  template <typename PinConfig> static auto write(const bool level) noexcept
  {
    return raw_gpio_api_t<Backend>::template write<PinConfig>(level);
  }

  /// @copybrief microhal::gpio::pin::read
  template <typename PinConfig> static auto read() noexcept
  {
    return raw_gpio_api_t<Backend>::template read<PinConfig>();
  }

  /// @brief Enables the interrupt for the GPIO pin.
  template <typename PinConfig, auto Handler> static auto enable_interrupt() noexcept
  {
    return raw_gpio_api_t<Backend>::template enable_interrupt<PinConfig, Handler>();
  }

  /// @copybrief microhal::gpio::pin::toggle
  template <typename PinConfig> static auto toggle() noexcept
  {
    if constexpr (requires { raw_gpio_api_t<Backend>::template toggle<PinConfig>(); })
    {
      return raw_gpio_api_t<Backend>::template toggle<PinConfig>();
    }
    else
    {
      return status::unsupported;
    }
  }
};

/// @brief Adapts the selected backend's RTC API to the common facade.
template <typename Backend> struct rtc_api_adapter final
{
  /// @brief Sets the date and time for the RTC.
  template <typename Config> static status set_date_time(const rtc::date_time value) noexcept
  {
    if constexpr (requires { raw_rtc_api_t<Backend>::template set_date_time<Config>(value); })
      return detail::as_status([value] { return raw_rtc_api_t<Backend>::template set_date_time<Config>(value); });
    else
      return status::unsupported;
  }

  /// @brief Returns the date and time for the RTC.
  template <typename Config> static result<rtc::date_time> get_date_time() noexcept
  {
    if constexpr (requires { raw_rtc_api_t<Backend>::template get_date_time<Config>(); })
      return detail::as_result<rtc::date_time>([] { return raw_rtc_api_t<Backend>::template get_date_time<Config>(); });
    else
    {
      rtc::date_time value{};
      using return_type =
          decltype(raw_rtc_api_t<Backend>::template get_time<Config>(value.hour, value.minute, value.second));
      if constexpr (std::is_same_v<std::remove_cv_t<return_type>, status>)
      {
        const auto read_status =
            raw_rtc_api_t<Backend>::template get_time<Config>(value.hour, value.minute, value.second);
        if (read_status != status::ok)
          return read_status;
      }
      else if constexpr (std::is_same_v<std::remove_cv_t<return_type>, bool>)
      {
        if (!raw_rtc_api_t<Backend>::template get_time<Config>(value.hour, value.minute, value.second))
          return status::io_error;
      }
      else
        raw_rtc_api_t<Backend>::template get_time<Config>(value.hour, value.minute, value.second);
      return value;
    }
  }

  /// @copybrief microhal::rtc::controller::read_subsecond_us
  template <typename Config> static result<std::uint32_t> read_subsecond_us() noexcept
  {
    if constexpr (requires { raw_rtc_api_t<Backend>::template read_subsecond_us<Config>(); })
      return detail::as_result<std::uint32_t>([]
                                              { return raw_rtc_api_t<Backend>::template read_subsecond_us<Config>(); });
    else
      return status::unsupported;
  }

  /// @brief Enables the periodic interrupt for the RTC.
  template <typename Config, auto Handler> static status enable_periodic_irq(std::uint32_t frequency_hz) noexcept
  {
    if constexpr (requires { raw_rtc_api_t<Backend>::template enable_periodic_irq<Config, Handler>(frequency_hz); })
      return detail::as_status(
          [frequency_hz]
          { return raw_rtc_api_t<Backend>::template enable_periodic_irq<Config, Handler>(frequency_hz); });
    else
      return status::unsupported;
  }

  /// @copybrief microhal::rtc::controller::disable_periodic_irq
  template <typename Config> static status disable_periodic_irq() noexcept
  {
    if constexpr (requires { raw_rtc_api_t<Backend>::template disable_periodic_irq<Config>(); })
      return detail::as_status([] { return raw_rtc_api_t<Backend>::template disable_periodic_irq<Config>(); });
    else
      return status::unsupported;
  }

  /// @brief Handles the interrupt for the RTC.
  template <typename Config> static status handle_interrupt() noexcept
  {
    if constexpr (requires { raw_rtc_api_t<Backend>::template handle_interrupt<Config>(); })
      return detail::as_status([] { return raw_rtc_api_t<Backend>::template handle_interrupt<Config>(); });
    else if constexpr (requires { raw_rtc_api_t<Backend>::handle_interrupt(Config::instance_index); })
      return detail::as_status([] { return raw_rtc_api_t<Backend>::handle_interrupt(Config::instance_index); });
    else if constexpr (requires { raw_rtc_api_t<Backend>::handle_interrupt(0U); })
      return detail::as_status([] { return raw_rtc_api_t<Backend>::handle_interrupt(0U); });
    else if constexpr (requires { raw_rtc_api_t<Backend>::handle_interrupt(); })
      return detail::as_status([] { return raw_rtc_api_t<Backend>::handle_interrupt(); });
    else
      return status::unsupported;
  }

  /// @copybrief microhal::rtc::controller::initialize
  template <typename Config> static auto initialize() noexcept
  {
    return raw_rtc_api_t<Backend>::template initialize<Config>();
  }

  /// @brief Sets the time for the RTC.
  template <typename Config>
  static auto set_time(const std::uint8_t hours, const std::uint8_t minutes, const std::uint8_t seconds) noexcept
  {
    return raw_rtc_api_t<Backend>::template set_time<Config>(hours, minutes, seconds);
  }

  /// @brief Returns the time for the RTC.
  template <typename Config>
  static auto get_time(std::uint8_t& hours, std::uint8_t& minutes, std::uint8_t& seconds) noexcept
  {
    return raw_rtc_api_t<Backend>::template get_time<Config>(hours, minutes, seconds);
  }

  /// @copybrief microhal::rtc::controller::enable_alarm
  template <typename Config, auto Handler, typename AlarmTime>
  static auto enable_alarm(const AlarmTime& alarm_time) noexcept
  {
    if constexpr (requires { raw_rtc_api_t<Backend>::template enable_alarm<Config, Handler>(alarm_time); })
    {
      return raw_rtc_api_t<Backend>::template enable_alarm<Config, Handler>(alarm_time);
    }
    else
    {
      (void)alarm_time;
      (void)Handler;
      return false;
    }
  }
};

/// @brief Adapts the selected backend's flash API to the common facade.
template <typename Backend> struct flash_api_adapter final
{
  /// @brief Erases the requested storage range.
  template <typename Config> static auto erase(const std::uint32_t address, const std::uint32_t bytes) noexcept
  {
    return raw_flash_api_t<Backend>::template erase<Config>(address, bytes);
  }

  /// @brief Programs the supplied data into storage.
  template <typename Config>
  static auto program(const std::uint32_t address, const std::span<const std::uint8_t> payload) noexcept
  {
    return raw_flash_api_t<Backend>::template program<Config>(address, payload);
  }

  /// @copybrief microhal::flash::controller::read
  template <typename Config>
  static auto read(const std::uint32_t address, const std::span<std::uint8_t> payload) noexcept
  {
    if constexpr (requires { raw_flash_api_t<Backend>::template read<Config>(address, payload); })
    {
      return raw_flash_api_t<Backend>::template read<Config>(address, payload);
    }
    else
    {
      (void)address;
      (void)payload;
      return false;
    }
  }

  /// @copybrief microhal::flash::controller::erase_region
  template <typename Config> static auto erase_region(const std::uint32_t address) noexcept
  {
    if constexpr (requires { raw_flash_api_t<Backend>::template erase_region<Config>(address); })
    {
      return raw_flash_api_t<Backend>::template erase_region<Config>(address);
    }
    else
    {
      (void)address;
      return result<flash_erase_region>{status::unsupported};
    }
  }
};

/// @brief Adapts the selected backend's timer API to the common facade.
template <typename Backend> struct timer_api_adapter final
{
  /// @brief Initializes the configured backend resource.
  template <typename Config> static auto initialize() noexcept
  {
    return raw_timer_api_t<Backend>::template initialize<Config>();
  }

  /// @copybrief microhal::timers::timer::counter
  template <typename Config> static auto counter() noexcept
  {
    if constexpr (requires { raw_timer_api_t<Backend>::template counter<Config>(); })
    {
      return raw_timer_api_t<Backend>::template counter<Config>();
    }
    else
    {
      return raw_timer_api_t<Backend>::template get_counter<Config>();
    }
  }

  /// @copybrief microhal::timers::timer::period
  template <typename Config> static auto period() noexcept
  {
    if constexpr (requires { raw_timer_api_t<Backend>::template period<Config>(); })
    {
      return detail::as_result<std::uint32_t>([] { return raw_timer_api_t<Backend>::template period<Config>(); });
    }
    else
    {
      return result<std::uint32_t>{status::unsupported};
    }
  }

  /// @copybrief microhal::timers::timer::start
  template <typename Config> static auto start() noexcept
  {
    if constexpr (requires { raw_timer_api_t<Backend>::template start<Config>(); })
    {
      return raw_timer_api_t<Backend>::template start<Config>();
    }
    else
    {
      return status::unsupported;
    }
  }

  /// @copybrief microhal::timers::timer::stop
  template <typename Config> static auto stop() noexcept
  {
    if constexpr (requires { raw_timer_api_t<Backend>::template stop<Config>(); })
    {
      return raw_timer_api_t<Backend>::template stop<Config>();
    }
    else
    {
      return status::unsupported;
    }
  }

  /// @brief Resets the counter for the timer.
  template <typename Config> static auto reset_counter() noexcept
  {
    if constexpr (requires { raw_timer_api_t<Backend>::template reset_counter<Config>(); })
    {
      return raw_timer_api_t<Backend>::template reset_counter<Config>();
    }
    else
    {
      return status::unsupported;
    }
  }

  /// @copybrief microhal::timers::timer::set_period
  template <typename Config> static auto set_period(const std::uint32_t period) noexcept
  {
    if constexpr (requires { raw_timer_api_t<Backend>::template set_period<Config>(period); })
    {
      return raw_timer_api_t<Backend>::template set_period<Config>(period);
    }
    else
    {
      (void)period;
      return status::unsupported;
    }
  }

  /// @brief Enables the configured timer capture/compare channel.
  template <typename Config, typename ChannelConfig, auto Handler> static auto enable_capture_compare() noexcept
  {
    if constexpr (requires {
                    raw_timer_api_t<Backend>::template enable_capture_compare<Config, ChannelConfig, Handler>();
                  })
    {
      return raw_timer_api_t<Backend>::template enable_capture_compare<Config, ChannelConfig, Handler>();
    }
    else
    {
      (void)Handler;
      return status::unsupported;
    }
  }

  /// @brief Disables the configured timer capture/compare channel.
  template <typename Config, typename ChannelConfig> static status disable_capture_compare() noexcept
  {
    if constexpr (requires { raw_timer_api_t<Backend>::template disable_capture_compare<Config, ChannelConfig>(); })
      return detail::as_status(
          [] { return raw_timer_api_t<Backend>::template disable_capture_compare<Config, ChannelConfig>(); });
    else
      return status::unsupported;
  }

  /// @brief Handles the interrupt for the timer.
  template <typename Config> static status handle_interrupt() noexcept
  {
    if constexpr (requires { raw_timer_api_t<Backend>::template handle_interrupt<Config>(); })
      return detail::as_status([] { return raw_timer_api_t<Backend>::template handle_interrupt<Config>(); });
    else if constexpr (requires { raw_timer_api_t<Backend>::handle_interrupt(std::uint8_t{}); })
      return detail::as_status(
          []
          {
            if constexpr (requires { Config::instance_index; })
              return raw_timer_api_t<Backend>::handle_interrupt(Config::instance_index);
            else
              return raw_timer_api_t<Backend>::handle_interrupt(0U);
          });
    else
      return status::unsupported;
  }

  /// @copybrief microhal::timers::timer::capture_value
  template <typename Config, typename ChannelConfig> static auto capture_value() noexcept
  {
    if constexpr (requires { raw_timer_api_t<Backend>::template capture_value<Config, ChannelConfig>(); })
    {
      return detail::as_result<std::uint32_t>(
          [] { return raw_timer_api_t<Backend>::template capture_value<Config, ChannelConfig>(); });
    }
    else
    {
      return result<std::uint32_t>{status::unsupported};
    }
  }
};

/// @brief Adapts the selected backend's DMA API to the common facade.
template <typename Backend> struct dma_api_adapter final
{
  /// @copybrief microhal::dma::controller::initialize
  template <typename Config> static auto initialize() noexcept
  {
    return raw_dma_api_t<Backend>::template initialize<Config>();
  }

  /// @copybrief microhal::dma::controller::start_transfer
  template <typename Config, auto Handler>
  static auto start_transfer(const std::uint32_t src_addr, const std::uint32_t dst_addr,
                             const std::uint32_t length) noexcept
  {
    return raw_dma_api_t<Backend>::template start_transfer<Config, Handler>(src_addr, dst_addr, length);
  }

  /// @copybrief microhal::dma::controller::abort
  template <typename Config> static auto abort() noexcept
  {
    return raw_dma_api_t<Backend>::template abort<Config>();
  }

  /// @copybrief microhal::dma::controller::transfer_status
  template <typename Config> static auto transfer_status() noexcept
  {
    if constexpr (requires { raw_dma_api_t<Backend>::template transfer_status<Config>(); })
    {
      return detail::as_result<dma_transfer_state>(
          [] { return raw_dma_api_t<Backend>::template transfer_status<Config>(); });
    }
    else
    {
      return result<dma_transfer_state>{status::unsupported};
    }
  }
};

/// @brief Adapts the selected backend's ADC API to the common facade.
template <typename Backend> struct adc_api_adapter final
{
  /// @brief Performs a blocking differential ADC conversion.
  template <typename Config> static result<std::int16_t> read_differential_blocking(std::uint8_t channel) noexcept
  {
    if constexpr (requires { raw_adc_api_t<Backend>::template read_differential_blocking<Config>(channel); })
      return detail::as_result<std::int16_t>(
          [channel] { return raw_adc_api_t<Backend>::template read_differential_blocking<Config>(channel); });
    else
      return status::unsupported;
  }

  /// @brief Starts DMA streaming using the configured peripheral.
  template <typename Config, auto Handler>
  static status start_dma_stream(std::span<const std::uint8_t> channels, std::span<std::uint16_t> results,
                                 bool circular = true) noexcept
  {
    if constexpr (requires {
                    raw_adc_api_t<Backend>::template start_dma_stream<Config, Handler>(channels, results, circular);
                  })
      return detail::as_status(
          [=]
          { return raw_adc_api_t<Backend>::template start_dma_stream<Config, Handler>(channels, results, circular); });
    else
      return status::unsupported;
  }
  /// @brief Stops the active peripheral operation.
  template <typename Config> static status stop() noexcept
  {
    if constexpr (requires { raw_adc_api_t<Backend>::template stop<Config>(); })
      return detail::as_status([] { return raw_adc_api_t<Backend>::template stop<Config>(); });
    else
      return status::unsupported;
  }
  /// @copybrief microhal::adc::converter::abort
  template <typename Config> static status abort() noexcept
  {
    if constexpr (requires { raw_adc_api_t<Backend>::template abort<Config>(); })
      return detail::as_status([] { return raw_adc_api_t<Backend>::template abort<Config>(); });
    else
      return status::unsupported;
  }
  /// @brief Queries the current or most recently completed transfer status.
  template <typename Config> static result<dma_transfer_state> transfer_status() noexcept
  {
    if constexpr (requires { raw_adc_api_t<Backend>::template transfer_status<Config>(); })
      return detail::as_result<dma_transfer_state>(
          [] { return raw_adc_api_t<Backend>::template transfer_status<Config>(); });
    else
      return status::unsupported;
  }
  /// @brief Handles the interrupt for the ADC.
  template <typename Config> static void handle_interrupt() noexcept
  {
    if constexpr (requires { raw_adc_api_t<Backend>::template handle_interrupt<Config>(); })
      raw_adc_api_t<Backend>::template handle_interrupt<Config>();
  }
  /// @brief Handles the DMA interrupt for the ADC.
  template <typename Config> static void handle_dma_interrupt() noexcept
  {
    if constexpr (requires { raw_adc_api_t<Backend>::template handle_dma_interrupt<Config>(); })
      raw_adc_api_t<Backend>::template handle_dma_interrupt<Config>();
  }
  /// @copybrief microhal::adc::converter::initialize
  template <typename Config> static auto initialize() noexcept
  {
    return raw_adc_api_t<Backend>::template initialize<Config>();
  }

  /// @copybrief microhal::adc::converter::read_blocking
  template <typename Config> static auto read_blocking(const std::uint8_t channel) noexcept
  {
    return detail::as_result<std::uint16_t>(
        [channel] { return raw_adc_api_t<Backend>::template read_blocking<Config>(channel); });
  }

  /// @copybrief microhal::adc::converter::start_continuous
  template <typename Config> static auto start_continuous(const std::span<const std::uint8_t> channels) noexcept
  {
    if constexpr (requires { raw_adc_api_t<Backend>::template start_continuous<Config>(channels); })
    {
      return raw_adc_api_t<Backend>::template start_continuous<Config>(channels);
    }
    else
    {
      (void)channels;
      return status::unsupported;
    }
  }

  /// @copybrief microhal::adc::converter::start_async
  template <typename Config, auto Handler> static auto start_async(const std::uint8_t channel) noexcept
  {
    if constexpr (requires { raw_adc_api_t<Backend>::template start_async<Config, Handler>(channel); })
    {
      return raw_adc_api_t<Backend>::template start_async<Config, Handler>(channel);
    }
    else if constexpr (requires { raw_adc_api_t<Backend>::template start_interrupt<Config, Handler>(channel); })
    {
      return raw_adc_api_t<Backend>::template start_interrupt<Config, Handler>(channel);
    }
    else
    {
      (void)channel;
      (void)Handler;
      return false;
    }
  }

  /// @copybrief microhal::adc::converter::start_dma_scan
  template <typename Config, auto Handler>
  static auto start_dma_scan(const std::span<const std::uint8_t> channels,
                             const std::span<std::uint16_t> results) noexcept
  {
    if constexpr (requires { raw_adc_api_t<Backend>::template start_dma_scan<Config, Handler>(channels, results); })
    {
      return raw_adc_api_t<Backend>::template start_dma_scan<Config, Handler>(channels, results);
    }
    else if constexpr (requires { raw_adc_api_t<Backend>::template start_dma<Config, Handler>(channels[0U], results); })
    {
      if (channels.empty())
      {
        return false;
      }
      return raw_adc_api_t<Backend>::template start_dma<Config, Handler>(channels[0U], results);
    }
    else
    {
      (void)channels;
      (void)results;
      (void)Handler;
      return false;
    }
  }
};

/// @brief Adapts the selected backend's DAC API to the common facade.
template <typename Backend> struct dac_api_adapter final
{
  /// @copybrief microhal::dac::converter::initialize
  template <typename Config> static auto initialize() noexcept
  {
    return raw_dac_api_t<Backend>::template initialize<Config>();
  }

  /// @copybrief microhal::dac::converter::write_blocking
  template <typename Config> static auto write_blocking(const std::uint16_t value) noexcept
  {
    if constexpr (requires { raw_dac_api_t<Backend>::template write_blocking<Config>(value); })
    {
      return raw_dac_api_t<Backend>::template write_blocking<Config>(value);
    }
    else
    {
      return raw_dac_api_t<Backend>::template write<Config>(value, Config::channel);
    }
  }

  /// @copybrief microhal::dac::converter::write_dual
  template <typename Config> static auto write_dual(const std::uint16_t value1, const std::uint16_t value2) noexcept
  {
    return raw_dac_api_t<Backend>::template write_dual<Config>(value1, value2);
  }

  /// @copybrief microhal::dac::converter::start_dma_waveform
  template <typename Config, auto Handler>
  static auto start_dma_waveform(const std::span<const std::uint16_t> samples) noexcept
  {
    if constexpr (requires { raw_dac_api_t<Backend>::template start_dma_waveform<Config, Handler>(samples); })
    {
      return raw_dac_api_t<Backend>::template start_dma_waveform<Config, Handler>(samples);
    }
    else
    {
      return raw_dac_api_t<Backend>::template start_dma<Config, Handler>(samples, Config::channel);
    }
  }
};

} // namespace detail

/**
 * @brief Per-peripheral backend API aliases.
 * @details These are alias templates rather than members of a single struct so that a backend only
 *          has to provide the peripheral APIs it actually implements. An alias template is
 *          instantiated on use, so referencing `uart_api_t<B>` never forces `B` to define an
 *          `ethernet_api`.
 */
/// @{
/// @brief Selects the backend's GPIO API type.
/// @tparam Backend Registered backend tag with an enabled traits specialization.
template <typename Backend> using gpio_api_t = detail::gpio_api_adapter<Backend>;
/// @brief Selects the backend's I2C API type.
/// @tparam Backend Registered backend tag with an enabled traits specialization.
template <typename Backend> using i2c_api_t = detail::i2c_api_adapter<Backend>;
/// @brief Selects the backend's SPI API type.
/// @tparam Backend Registered backend tag with an enabled traits specialization.
template <typename Backend> using spi_api_t = detail::spi_api_adapter<Backend>;
/// @brief Selects the backend's QSPI API type.
/// @tparam Backend Registered backend tag with an enabled traits specialization.
template <typename Backend> using qspi_api_t = detail::qspi_api_adapter<Backend>;
/// @brief Selects the backend's UART API type.
/// @tparam Backend Registered backend tag with an enabled traits specialization.
template <typename Backend> using uart_api_t = detail::uart_api_adapter<Backend>;
/// @brief Selects the backend's RTC API type.
/// @tparam Backend Registered backend tag with an enabled traits specialization.
template <typename Backend> using rtc_api_t = detail::rtc_api_adapter<Backend>;
/// @brief Selects the backend's flash API type.
/// @tparam Backend Registered backend tag with an enabled traits specialization.
template <typename Backend> using flash_api_t = detail::flash_api_adapter<Backend>;
/// @brief Selects the backend's timer API type.
/// @tparam Backend Registered backend tag with an enabled traits specialization.
template <typename Backend> using timer_api_t = detail::timer_api_adapter<Backend>;
/// @brief Selects the backend's ADC API type.
/// @tparam Backend Registered backend tag with an enabled traits specialization.
template <typename Backend> using adc_api_t = detail::adc_api_adapter<Backend>;
/// @brief Selects the backend's DAC API type.
/// @tparam Backend Registered backend tag with an enabled traits specialization.
template <typename Backend> using dac_api_t = detail::dac_api_adapter<Backend>;
/// @brief Selects the backend's CAN API type.
/// @tparam Backend Registered backend tag with an enabled traits specialization.
template <typename Backend> using can_api_t = detail::raw_can_api_t<Backend>;
/// @brief Selects the backend's USB API type.
/// @tparam Backend Registered backend tag with an enabled traits specialization.
template <typename Backend> using usb_api_t = detail::raw_usb_api_t<Backend>;
/// @brief Selects the backend's Ethernet API type.
/// @tparam Backend Registered backend tag with an enabled traits specialization.
template <typename Backend> using ethernet_api_t = detail::raw_ethernet_api_t<Backend>;
/// @brief Selects the backend's SD API type.
/// @tparam Backend Registered backend tag with an enabled traits specialization.
template <typename Backend> using sd_api_t = detail::raw_sd_api_t<Backend>;
/// @brief Selects the backend's DMA API type.
/// @tparam Backend Registered backend tag with an enabled traits specialization.
template <typename Backend> using dma_api_t = detail::dma_api_adapter<Backend>;
/// @brief Selects the backend's PWM API type.
/// @tparam Backend Registered backend tag with an enabled traits specialization.
template <typename Backend> using pwm_api_t = detail::raw_pwm_api_t<Backend>;
/// @brief Selects the backend's watchdog API type.
/// @tparam Backend Registered backend tag with an enabled traits specialization.
template <typename Backend> using watchdog_api_t = detail::raw_watchdog_api_t<Backend>;
/// @brief Selects the backend's I2S API type.
/// @tparam Backend Registered backend tag with an enabled traits specialization.
template <typename Backend> using i2s_api_t = detail::raw_i2s_api_t<Backend>;
/// @brief Selects the backend's RCC API type.
/// @tparam Backend Registered backend tag with an enabled traits specialization.
template <typename Backend> using rcc_api_t = detail::raw_rcc_api_t<Backend>;
/// @brief Selects the backend's CRC API type.
/// @tparam Backend Registered backend tag with an enabled traits specialization.
template <typename Backend> using crc_api_t = detail::raw_crc_api_t<Backend>;
/// @}

/**
 * @brief Compile-time dispatch table that resolves backend namespaces without runtime indirection.
 * @deprecated Prefer the per-peripheral `*_api_t` aliases; naming this struct requires the backend
 *             to define all twenty peripheral APIs.
 */
template <typename Backend> struct backend_dispatch_table final
{
  using gpio = gpio_api_t<Backend>;
  using i2c = i2c_api_t<Backend>;
  using spi = spi_api_t<Backend>;
  using qspi = qspi_api_t<Backend>;
  using uart = uart_api_t<Backend>;
  using rtc = rtc_api_t<Backend>;
  using flash = flash_api_t<Backend>;
  using timer = timer_api_t<Backend>;
  using adc = adc_api_t<Backend>;
  using dac = dac_api_t<Backend>;
  using can = can_api_t<Backend>;
  using usb = usb_api_t<Backend>;
  using ethernet = ethernet_api_t<Backend>;
  using sd = sd_api_t<Backend>;
  using dma = dma_api_t<Backend>;
  using pwm = pwm_api_t<Backend>;
  using watchdog = watchdog_api_t<Backend>;
  using i2s = i2s_api_t<Backend>;
  using rcc = rcc_api_t<Backend>;
  using crc = crc_api_t<Backend>;
};

/**
 * @brief Returns the registry descriptor for a registered backend.
 * @tparam Backend Backend tag whose traits provide the descriptor.
 * @return The backend name, vendor, and backend kind.
 */
template <typename Backend> [[nodiscard]] consteval auto select_backend() noexcept -> backend_descriptor
{
  return make_backend_descriptor<Backend>();
}

/**
 * @brief Requires a backend to support every listed peripheral at compile time.
 * @tparam Backend Backend tag to validate.
 * @tparam RequiredPeripherals Peripheral kinds required by the application.
 */
template <typename Backend, peripheral_kind... RequiredPeripherals>
consteval auto validate_backend_selection() noexcept -> void
{
  static_assert((capability_for<Backend, RequiredPeripherals>().supported && ...),
                "microHAL: selected backend does not support one or more required peripherals.");
}

} // namespace microhal
