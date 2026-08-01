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

template <backend_kind Kind> struct backend_tag_from_kind;

#if MICROHAL_ENABLE_STM32_HAL
template <> struct backend_tag_from_kind<backend_kind::stm32_cube_hal> final
{
  using type = stm32_hal_tag;
};
#endif

#if MICROHAL_ENABLE_STM32_LL
template <> struct backend_tag_from_kind<backend_kind::stm32_cube_ll> final
{
  using type = stm32_ll_tag;
};
#endif

#if MICROHAL_ENABLE_KINETIS_ESPRESSO
template <> struct backend_tag_from_kind<backend_kind::kinetis_espresso> final
{
  using type = kinetis_espresso_tag;
};
#endif

#if MICROHAL_ENABLE_ZEPHYR
template <> struct backend_tag_from_kind<backend_kind::zephyr_driver_model> final
{
  using type = zephyr_tag;
};
#endif

#if MICROHAL_ENABLE_NUTTX
template <> struct backend_tag_from_kind<backend_kind::nuttx_driver_model> final
{
  using type = nuttx_tag;
};
#endif

#if MICROHAL_ENABLE_NORDIC_NRFX
template <> struct backend_tag_from_kind<backend_kind::nordic_nrfx> final
{
  using type = nordic_nrf_tag;
};
#endif

#if MICROHAL_ENABLE_MOCK
template <> struct backend_tag_from_kind<backend_kind::mock_recording> final
{
  using type = mock_tag;
};
#endif

template <backend_kind Kind> using backend_tag_from_kind_t = typename backend_tag_from_kind<Kind>::type;

namespace detail
{

template <typename Backend> using raw_gpio_api_t = typename backend_traits<Backend>::gpio_api;
template <typename Backend> using raw_i2c_api_t = typename backend_traits<Backend>::i2c_api;
template <typename Backend> using raw_spi_api_t = typename backend_traits<Backend>::spi_api;
template <typename Backend> using raw_qspi_api_t = typename backend_traits<Backend>::qspi_api;
template <typename Backend> using raw_uart_api_t = typename backend_traits<Backend>::uart_api;
template <typename Backend> using raw_rtc_api_t = typename backend_traits<Backend>::rtc_api;
template <typename Backend> using raw_flash_api_t = typename backend_traits<Backend>::flash_api;
template <typename Backend> using raw_timer_api_t = typename backend_traits<Backend>::timer_api;
template <typename Backend> using raw_adc_api_t = typename backend_traits<Backend>::adc_api;
template <typename Backend> using raw_dac_api_t = typename backend_traits<Backend>::dac_api;
template <typename Backend> using raw_can_api_t = typename backend_traits<Backend>::can_api;
template <typename Backend> using raw_usb_api_t = typename backend_traits<Backend>::usb_api;
template <typename Backend> using raw_ethernet_api_t = typename backend_traits<Backend>::ethernet_api;
template <typename Backend> using raw_sd_api_t = typename backend_traits<Backend>::sd_api;
template <typename Backend> using raw_dma_api_t = typename backend_traits<Backend>::dma_api;
template <typename Backend> using raw_pwm_api_t = typename backend_traits<Backend>::pwm_api;
template <typename Backend> using raw_watchdog_api_t = typename backend_traits<Backend>::watchdog_api;
template <typename Backend> using raw_i2s_api_t = typename backend_traits<Backend>::i2s_api;
template <typename Backend> using raw_rcc_api_t = typename backend_traits<Backend>::rcc_api;
template <typename Backend> using raw_crc_api_t = typename backend_traits<Backend>::crc_api;

template <typename Backend> struct i2c_api_adapter final
{
  template <typename Config> static auto initialize() noexcept
  {
    return raw_i2c_api_t<Backend>::template initialize<Config>();
  }

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

template <typename Backend> struct spi_api_adapter final
{
  template <typename Config> static auto initialize() noexcept
  {
    return raw_spi_api_t<Backend>::template initialize<Config>();
  }

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

template <typename Backend> struct uart_api_adapter final
{
  template <typename Config> static auto initialize() noexcept
  {
    return raw_uart_api_t<Backend>::template initialize<Config>();
  }

  template <typename Config> static auto write_blocking(const std::span<const std::uint8_t> payload) noexcept
  {
    return raw_uart_api_t<Backend>::template write_blocking<Config>(payload);
  }

  template <typename Config> static auto read_blocking(const std::span<std::uint8_t> payload) noexcept
  {
    return raw_uart_api_t<Backend>::template read_blocking<Config>(payload);
  }

  template <typename Config, auto Handler> static auto start_async_rx(const std::span<std::uint8_t> payload) noexcept
  {
    return raw_uart_api_t<Backend>::template start_async_rx<Config, Handler>(payload);
  }

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

template <typename Backend> struct rtc_api_adapter final
{
  template <typename Config> static auto initialize() noexcept
  {
    return raw_rtc_api_t<Backend>::template initialize<Config>();
  }

  template <typename Config>
  static auto set_time(const std::uint8_t hours, const std::uint8_t minutes, const std::uint8_t seconds) noexcept
  {
    return raw_rtc_api_t<Backend>::template set_time<Config>(hours, minutes, seconds);
  }

  template <typename Config>
  static auto get_time(std::uint8_t& hours, std::uint8_t& minutes, std::uint8_t& seconds) noexcept
  {
    return raw_rtc_api_t<Backend>::template get_time<Config>(hours, minutes, seconds);
  }

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

template <typename Backend> struct flash_api_adapter final
{
  template <typename Config> static auto erase(const std::uint32_t address, const std::uint32_t bytes) noexcept
  {
    return raw_flash_api_t<Backend>::template erase<Config>(address, bytes);
  }

  template <typename Config>
  static auto program(const std::uint32_t address, const std::span<const std::uint8_t> payload) noexcept
  {
    return raw_flash_api_t<Backend>::template program<Config>(address, payload);
  }

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
};

template <typename Backend> struct timer_api_adapter final
{
  template <typename Config> static auto initialize() noexcept
  {
    return raw_timer_api_t<Backend>::template initialize<Config>();
  }

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
      return false;
    }
  }
};

template <typename Backend> struct adc_api_adapter final
{
  template <typename Config> static auto initialize() noexcept
  {
    return raw_adc_api_t<Backend>::template initialize<Config>();
  }

  template <typename Config> static auto read_blocking(const std::uint8_t channel) noexcept
  {
    return raw_adc_api_t<Backend>::template read_blocking<Config>(channel);
  }

  template <typename Config> static auto start_continuous(const std::span<const std::uint8_t> channels) noexcept
  {
    if constexpr (requires { raw_adc_api_t<Backend>::template start_continuous<Config>(channels); })
    {
      return raw_adc_api_t<Backend>::template start_continuous<Config>(channels);
    }
    else
    {
      (void)channels;
      return false;
    }
  }

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

template <typename Backend> struct dac_api_adapter final
{
  template <typename Config> static auto initialize() noexcept
  {
    return raw_dac_api_t<Backend>::template initialize<Config>();
  }

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

  template <typename Config> static auto write_dual(const std::uint16_t value1, const std::uint16_t value2) noexcept
  {
    return raw_dac_api_t<Backend>::template write_dual<Config>(value1, value2);
  }

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
template <typename Backend> using gpio_api_t = detail::raw_gpio_api_t<Backend>;
template <typename Backend> using i2c_api_t = detail::i2c_api_adapter<Backend>;
template <typename Backend> using spi_api_t = detail::spi_api_adapter<Backend>;
template <typename Backend> using qspi_api_t = detail::raw_qspi_api_t<Backend>;
template <typename Backend> using uart_api_t = detail::uart_api_adapter<Backend>;
template <typename Backend> using rtc_api_t = detail::rtc_api_adapter<Backend>;
template <typename Backend> using flash_api_t = detail::flash_api_adapter<Backend>;
template <typename Backend> using timer_api_t = detail::timer_api_adapter<Backend>;
template <typename Backend> using adc_api_t = detail::adc_api_adapter<Backend>;
template <typename Backend> using dac_api_t = detail::dac_api_adapter<Backend>;
template <typename Backend> using can_api_t = detail::raw_can_api_t<Backend>;
template <typename Backend> using usb_api_t = detail::raw_usb_api_t<Backend>;
template <typename Backend> using ethernet_api_t = detail::raw_ethernet_api_t<Backend>;
template <typename Backend> using sd_api_t = detail::raw_sd_api_t<Backend>;
template <typename Backend> using dma_api_t = detail::raw_dma_api_t<Backend>;
template <typename Backend> using pwm_api_t = detail::raw_pwm_api_t<Backend>;
template <typename Backend> using watchdog_api_t = detail::raw_watchdog_api_t<Backend>;
template <typename Backend> using i2s_api_t = detail::raw_i2s_api_t<Backend>;
template <typename Backend> using rcc_api_t = detail::raw_rcc_api_t<Backend>;
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

template <typename Backend> [[nodiscard]] consteval auto select_backend() noexcept -> backend_descriptor
{
  return make_backend_descriptor<Backend>();
}

template <typename Backend, peripheral_kind... RequiredPeripherals>
consteval auto validate_backend_selection() noexcept -> void
{
  static_assert((capability_for<Backend, RequiredPeripherals>().supported && ...),
                "microHAL: selected backend does not support one or more required peripherals.");
}

} // namespace microhal
