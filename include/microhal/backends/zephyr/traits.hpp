// SPDX-License-Identifier: Apache-2.0
/**
 * @file traits.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <array>
#include <cstdint>

// Zephyr RTOS driver includes - conditional compilation when building for Zephyr
#ifdef CONFIG_ZEPHYR_KERNEL
#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/uart.h>
#if __has_include(<zephyr/usb/usb_device.h>)
#include <zephyr/usb/usb_device.h>
#elif __has_include(<zephyr/drivers/usb/usb_device.h>)
#include <zephyr/drivers/usb/usb_device.h>
#endif
#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/sys/crc.h>
#if __has_include(<zephyr/random/random.h>)
#include <zephyr/random/random.h>
#elif __has_include(<zephyr/sys/rand32.h>)
#include <zephyr/sys/rand32.h>
#endif
#include <zephyr/sys/util.h>
#endif

#include "microhal/core/traits.hpp"

namespace microhal::backends::zephyr
{

#ifdef CONFIG_ZEPHYR_KERNEL
template <typename Binding> static auto device_from_binding() noexcept -> const struct device*
{
  if constexpr (requires { Binding::device; })
  {
    return Binding::device;
  }
  else
  {
    return nullptr;
  }
}

template <typename Config> static auto device_from_config() noexcept -> const struct device*
{
  if constexpr (requires { typename Config::binding; })
  {
    return device_from_binding<typename Config::binding>();
  }
  else if constexpr (requires { Config::device; })
  {
    return Config::device;
  }
  else
  {
    return nullptr;
  }
}
#endif

#if defined(CONFIG_GPIO)
consteval gpio_flags_t zephyr_gpio_flags(const gpio_mode mode, const gpio_pull pull) noexcept
{
  gpio_flags_t flags = 0U;
  switch (mode)
  {
  case gpio_mode::input:
    flags |= GPIO_INPUT;
    break;
  case gpio_mode::output:
    flags |= GPIO_OUTPUT;
    break;
  default:
    flags |= GPIO_INPUT;
    break;
  }

  switch (pull)
  {
  case gpio_pull::up:
    flags |= GPIO_PULL_UP;
    break;
  case gpio_pull::down:
    flags |= GPIO_PULL_DOWN;
    break;
  default:
    break;
  }

  return flags;
}

consteval gpio_flags_t zephyr_gpio_interrupt_flags(const gpio_interrupt_trigger trigger) noexcept
{
  switch (trigger)
  {
  case gpio_interrupt_trigger::rising:
    return GPIO_INT_EDGE_RISING;
  case gpio_interrupt_trigger::falling:
    return GPIO_INT_EDGE_FALLING;
  case gpio_interrupt_trigger::both:
    return GPIO_INT_EDGE_BOTH;
  case gpio_interrupt_trigger::none:
  default:
    return GPIO_INT_DISABLE;
  }
}

template <typename PinConfig> static auto gpio_spec_from_config() noexcept -> const struct gpio_dt_spec*
{
  if constexpr (requires { typename PinConfig::binding; })
  {
    if constexpr (requires { PinConfig::binding::spec; })
    {
      return &PinConfig::binding::spec;
    }
  }
  return nullptr;
}
#endif

#if defined(CONFIG_SPI)
template <typename Config> static auto spi_config_from_config() noexcept -> const struct spi_config*
{
  if constexpr (requires { typename Config::binding; })
  {
    if constexpr (requires { Config::binding::spi_config; })
    {
      return &Config::binding::spi_config;
    }
    else if constexpr (requires { Config::binding::config; })
    {
      return &Config::binding::config;
    }
  }
  else if constexpr (requires { Config::spi_config; })
  {
    return &Config::spi_config;
  }

  return nullptr;
}
#endif

#if defined(CONFIG_COUNTER)
template <typename Config> static auto rtc_epoch_seconds() noexcept -> std::uint32_t&
{
  static std::uint32_t seconds = 0U;
  return seconds;
}

template <typename Config> static auto rtc_epoch_ticks() noexcept -> std::uint32_t&
{
  static std::uint32_t ticks = 0U;
  return ticks;
}
#endif

#if defined(CONFIG_CAN)
template <typename Config> static auto can_rx_msgq_from_config() noexcept -> struct k_msgq*
{
  if constexpr (requires { typename Config::binding; })
  {
    if constexpr (requires { Config::binding::rx_msgq; })
    {
      return &Config::binding::rx_msgq;
    }
  }
  else if constexpr (requires { Config::rx_msgq; })
  {
    return &Config::rx_msgq;
  }

  return nullptr;
}

template <typename Config> static auto can_filter_handle() noexcept -> int&
{
  static int handle = -1;
  return handle;
}
#endif

#if defined(CONFIG_ADC)
template <typename Config> consteval std::uint8_t adc_resolution_bits() noexcept
{
  if constexpr (requires { Config::resolution; })
  {
    return static_cast<std::uint8_t>(Config::resolution);
  }
  return 12U;
}

template <typename Config> consteval std::uint8_t adc_oversampling() noexcept
{
  if constexpr (requires { Config::oversampling; })
  {
    return static_cast<std::uint8_t>(Config::oversampling);
  }
  return 0U;
}

template <typename Config> consteval std::uint8_t adc_default_channel() noexcept
{
  if constexpr (requires { Config::channel; })
  {
    return static_cast<std::uint8_t>(Config::channel);
  }
  return 0U;
}

template <typename Config> static auto adc_channel_config_from_config() noexcept -> const struct adc_channel_cfg*
{
  if constexpr (requires { typename Config::binding; })
  {
    if constexpr (requires { Config::binding::channel_cfg; })
    {
      return &Config::binding::channel_cfg;
    }
  }
  else if constexpr (requires { Config::channel_cfg; })
  {
    return &Config::channel_cfg;
  }

  return nullptr;
}
#endif

#if defined(CONFIG_DAC)
template <typename Config> consteval std::uint8_t dac_channel_index() noexcept
{
  if constexpr (requires { Config::channel; })
  {
    return static_cast<std::uint8_t>(Config::channel);
  }
  return 0U;
}
#endif

#if defined(CONFIG_NET_L2_ETHERNET)
template <typename Config> static auto ethernet_iface_from_config() noexcept -> struct net_if*
{
  if constexpr (requires { typename Config::binding; })
  {
    if constexpr (requires { Config::binding::iface; })
    {
      return Config::binding::iface;
    }
  }
  else if constexpr (requires { Config::iface; })
  {
    return Config::iface;
  }

  return net_if_get_default();
}
#endif

#if defined(CONFIG_SDHC) || defined(CONFIG_DISK_ACCESS)
template <typename Config> static auto sd_disk_name_from_config() noexcept -> const char*
{
  if constexpr (requires { typename Config::binding; })
  {
    if constexpr (requires { Config::binding::disk_name; })
    {
      return Config::binding::disk_name;
    }
  }
  else if constexpr (requires { Config::disk_name; })
  {
    return Config::disk_name;
  }

  return "SD";
}

template <typename Config> consteval std::uint32_t sd_block_size_bytes() noexcept
{
  if constexpr (requires { Config::block_size_bytes; })
  {
    return Config::block_size_bytes;
  }
  else if constexpr (requires { Config::block_size; })
  {
    return Config::block_size;
  }
  return 512U;
}
#endif

#if defined(CONFIG_CLOCK_CONTROL)
template <typename Config> static auto clock_subsys_from_config() noexcept -> clock_control_subsys_t
{
  if constexpr (requires { typename Config::binding; })
  {
    if constexpr (requires { Config::binding::subsys; })
    {
      return static_cast<clock_control_subsys_t>(Config::binding::subsys);
    }
  }
  else if constexpr (requires { Config::subsys; })
  {
    return static_cast<clock_control_subsys_t>(Config::subsys);
  }
  return nullptr;
}
#endif

#if defined(CONFIG_DMA)
template <typename Config> consteval std::uint32_t dma_channel_index() noexcept
{
  if constexpr (requires { typename Config::binding; })
  {
    if constexpr (requires { Config::binding::channel; })
    {
      return Config::binding::channel;
    }
  }
  else if constexpr (requires { Config::channel; })
  {
    return Config::channel;
  }

  return 0U;
}
#endif

#if defined(CONFIG_PWM)
template <typename Config> static auto pwm_spec_from_config() noexcept -> const struct pwm_dt_spec*
{
  if constexpr (requires { typename Config::binding; })
  {
    if constexpr (requires { Config::binding::spec; })
    {
      return &Config::binding::spec;
    }
  }
  else if constexpr (requires { Config::spec; })
  {
    return &Config::spec;
  }

  return nullptr;
}
#endif

#if defined(CONFIG_WATCHDOG)
template <typename Config> static auto watchdog_channel_id() noexcept -> int&
{
  static int channel = -1;
  return channel;
}
#endif

#if defined(CONFIG_I2S)
template <typename Config> static auto i2s_tx_config_from_config() noexcept -> const struct i2s_config*
{
  if constexpr (requires { typename Config::binding; })
  {
    if constexpr (requires { Config::binding::tx_config; })
    {
      return &Config::binding::tx_config;
    }
    else if constexpr (requires { Config::binding::config; })
    {
      return &Config::binding::config;
    }
  }
  else if constexpr (requires { Config::tx_config; })
  {
    return &Config::tx_config;
  }
  else if constexpr (requires { Config::config; })
  {
    return &Config::config;
  }

  return nullptr;
}

template <typename Config> static auto i2s_rx_config_from_config() noexcept -> const struct i2s_config*
{
  if constexpr (requires { typename Config::binding; })
  {
    if constexpr (requires { Config::binding::rx_config; })
    {
      return &Config::binding::rx_config;
    }
    else if constexpr (requires { Config::binding::config; })
    {
      return &Config::binding::config;
    }
  }
  else if constexpr (requires { Config::rx_config; })
  {
    return &Config::rx_config;
  }
  else if constexpr (requires { Config::config; })
  {
    return &Config::config;
  }

  return nullptr;
}
#endif

struct gpio_api final
{
  template <typename PinConfig> static void configure() noexcept
  {
#if defined(CONFIG_GPIO)
    const auto* spec = gpio_spec_from_config<PinConfig>();
    if ((spec == nullptr) || !device_is_ready(spec->port))
    {
      return;
    }

    (void)gpio_pin_configure(spec->port, spec->pin,
                             spec->dt_flags | zephyr_gpio_flags(PinConfig::mode, PinConfig::pull));
#endif
  }

  template <typename PinConfig> static void write(const bool level) noexcept
  {
#if defined(CONFIG_GPIO)
    const auto* spec = gpio_spec_from_config<PinConfig>();
    if ((spec == nullptr) || !device_is_ready(spec->port))
    {
      return;
    }

    (void)gpio_pin_set(spec->port, spec->pin, level ? 1 : 0);
    (void)level;
#else
    (void)level;
#endif
  }

  template <typename PinConfig> static bool read() noexcept
  {
#if defined(CONFIG_GPIO)
    const auto* spec = gpio_spec_from_config<PinConfig>();
    if ((spec == nullptr) || !device_is_ready(spec->port))
    {
      return false;
    }

    return gpio_pin_get(spec->port, spec->pin) > 0;
#else
    return false;
#endif
  }

  template <typename PinConfig> static status toggle() noexcept
  {
#if defined(CONFIG_GPIO)
    const auto* spec = gpio_spec_from_config<PinConfig>();
    if ((spec == nullptr) || !device_is_ready(spec->port))
    {
      return status::error;
    }

    return (gpio_pin_toggle(spec->port, spec->pin) == 0) ? status::ok : status::error;
#else
    (void)sizeof(PinConfig);
    return status::unsupported;
#endif
  }

  template <typename PinConfig, auto Handler> static void enable_interrupt() noexcept
  {
#if defined(CONFIG_GPIO)
    const auto* spec = gpio_spec_from_config<PinConfig>();
    if ((spec == nullptr) || !device_is_ready(spec->port))
    {
      return;
    }

    (void)gpio_pin_interrupt_configure(spec->port, spec->pin,
                                       zephyr_gpio_interrupt_flags(PinConfig::interrupt_trigger));
    (void)Handler;
#else
    (void)Handler;
#endif
  }
};

struct i2c_api final
{
#ifdef CONFIG_ZEPHYR_KERNEL
  static status transfer_status(const int result) noexcept
  {
    if (result == 0)
    {
      return status::ok;
    }
    if (result == -ETIMEDOUT)
    {
      return status::timeout;
    }
    if (result == -EBUSY)
    {
      return status::busy;
    }
    if ((result == -ENOTSUP) || (result == -ENOSYS))
    {
      return status::unsupported;
    }
    if ((result == -EIO) || (result == -ENXIO))
    {
      return status::io_error;
    }
    return status::error;
  }
#endif

  template <typename Config> static bool initialize() noexcept
  {
#if defined(CONFIG_I2C)
    const struct device* dev = device_from_config<Config>();
    return (dev != nullptr) && device_is_ready(dev);
#else
    return false;
#endif
  }

  template <typename Config>
  static status write_blocking(const std::span<const std::uint8_t> payload, const std::uint16_t address) noexcept
  {
#if defined(CONFIG_I2C)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return status::error;
    }

    return transfer_status(i2c_write(dev, payload.data(), payload.size(), address));
#else
    (void)payload;
    (void)address;
    return status::unsupported;
#endif
  }

  template <typename Config>
  static status read_blocking(const std::span<std::uint8_t> payload, const std::uint16_t address) noexcept
  {
#if defined(CONFIG_I2C)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return status::error;
    }

    return transfer_status(i2c_read(dev, payload.data(), payload.size(), address));
#else
    (void)payload;
    (void)address;
    return status::unsupported;
#endif
  }

  template <typename Config> static status probe(const std::uint16_t address) noexcept
  {
#if defined(CONFIG_I2C)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return status::error;
    }

    return transfer_status(i2c_write(dev, nullptr, 0U, address));
#else
    (void)address;
    return status::unsupported;
#endif
  }

  template <typename Config>
  static status write_read_blocking(const std::span<const std::uint8_t> tx_payload,
                                    const std::span<std::uint8_t> rx_payload, const std::uint16_t address) noexcept
  {
#if defined(CONFIG_I2C)
    if constexpr (Config::address_mode != i2c_address_mode::seven_bit)
    {
      return status::unsupported;
    }

    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return status::error;
    }

    return transfer_status(
        i2c_write_read(dev, address, tx_payload.data(), tx_payload.size(), rx_payload.data(), rx_payload.size()));
#else
    (void)tx_payload;
    (void)rx_payload;
    (void)address;
    return status::unsupported;
#endif
  }

  template <typename Config, auto Handler>
  static bool start_transfer(const std::span<std::uint8_t> payload, const std::uint16_t address) noexcept
  {
    (void)payload;
    (void)address;
    (void)Handler;
    return false;
  }
};

struct spi_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(CONFIG_SPI)
    const struct device* dev = device_from_config<Config>();
    return (dev != nullptr) && device_is_ready(dev) && (spi_config_from_config<Config>() != nullptr);
#else
    return false;
#endif
  }

  template <typename Config>
  static bool transfer(const std::span<const std::uint8_t> tx_payload, std::span<std::uint8_t> rx_payload) noexcept
  {
#if defined(CONFIG_SPI)
    const struct device* dev = device_from_config<Config>();
    const struct spi_config* config = spi_config_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev) || (config == nullptr) || (tx_payload.size() != rx_payload.size()))
    {
      return false;
    }

    const struct spi_buf tx_buf = {.buf = const_cast<std::uint8_t*>(tx_payload.data()), .len = tx_payload.size()};
    const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1U};
    struct spi_buf rx_buf = {.buf = rx_payload.data(), .len = rx_payload.size()};
    const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1U};
    return spi_transceive(dev, config, &tx, &rx) == 0;
#else
    (void)tx_payload;
    (void)rx_payload;
    return false;
#endif
  }

  template <typename Config> static status write_blocking(const std::span<const std::uint8_t> payload) noexcept
  {
#if defined(CONFIG_SPI)
    const struct device* dev = device_from_config<Config>();
    const struct spi_config* config = spi_config_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev) || (config == nullptr))
    {
      return status::error;
    }

    const struct spi_buf buffer = {.buf = const_cast<std::uint8_t*>(payload.data()), .len = payload.size()};
    const struct spi_buf_set buffers = {.buffers = &buffer, .count = 1U};
    return (spi_write(dev, config, &buffers) == 0) ? status::ok : status::error;
#else
    (void)payload;
    return status::unsupported;
#endif
  }

  template <typename Config> static status read_blocking(const std::span<std::uint8_t> payload) noexcept
  {
#if defined(CONFIG_SPI)
    const struct device* dev = device_from_config<Config>();
    const struct spi_config* config = spi_config_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev) || (config == nullptr))
    {
      return status::error;
    }

    struct spi_buf buffer = {.buf = payload.data(), .len = payload.size()};
    const struct spi_buf_set buffers = {.buffers = &buffer, .count = 1U};
    return (spi_read(dev, config, &buffers) == 0) ? status::ok : status::error;
#else
    (void)payload;
    return status::unsupported;
#endif
  }

  template <typename Config, auto Handler>
  static status transfer_async(const std::span<const std::uint8_t> tx_payload,
                               std::span<std::uint8_t> rx_payload) noexcept
  {
    (void)tx_payload;
    (void)rx_payload;
    (void)Handler;
    return status::unsupported;
  }
};

struct qspi_api final
{
  template <typename Config> static bool initialize() noexcept
  {
    if constexpr (requires { typename Config::binding; } &&
                  requires { Config::binding::template initialize<Config>(); })
    {
      return microhal::detail::as_status([] { return Config::binding::template initialize<Config>(); }) == status::ok;
    }
    return false;
  }

  template <typename Config>
  static status command_write(const qspi_command& request, const std::span<const std::uint8_t> payload) noexcept
  {
    if constexpr (requires { typename Config::binding; } &&
                  requires { Config::binding::template command_write<Config>(request, payload); })
    {
      return microhal::detail::as_status(
          [&] { return Config::binding::template command_write<Config>(request, payload); });
    }
    return status::unsupported;
  }

  template <typename Config>
  static status command_read(const qspi_command& request, const std::span<std::uint8_t> payload) noexcept
  {
    if constexpr (requires { typename Config::binding; } &&
                  requires { Config::binding::template command_read<Config>(request, payload); })
    {
      return microhal::detail::as_status(
          [&] { return Config::binding::template command_read<Config>(request, payload); });
    }
    return status::unsupported;
  }

  template <typename Config> static status enter_memory_mapped(const qspi_command& request) noexcept
  {
    if constexpr (requires { typename Config::binding; } &&
                  requires { Config::binding::template enter_memory_mapped<Config>(request); })
    {
      return microhal::detail::as_status(
          [&] { return Config::binding::template enter_memory_mapped<Config>(request); });
    }
    return status::unsupported;
  }

  template <typename Config> static status exit_memory_mapped() noexcept
  {
    if constexpr (requires { typename Config::binding; } &&
                  requires { Config::binding::template exit_memory_mapped<Config>(); })
    {
      return microhal::detail::as_status([] { return Config::binding::template exit_memory_mapped<Config>(); });
    }
    return status::unsupported;
  }
};

struct uart_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(CONFIG_UART)
    const struct device* dev = device_from_config<Config>();
    return (dev != nullptr) && device_is_ready(dev);
#else
    return false;
#endif
  }

  template <typename Config> static bool write_blocking(std::span<const std::uint8_t> payload) noexcept
  {
#if defined(CONFIG_UART)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return false;
    }

    for (const std::uint8_t byte : payload)
    {
      uart_poll_out(dev, byte);
    }
    return true;
#else
    (void)payload;
    return false;
#endif
  }

  template <typename Config> static bool read_blocking(std::span<std::uint8_t> payload) noexcept
  {
#if defined(CONFIG_UART)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return false;
    }

    for (std::uint8_t& byte : payload)
    {
      if (uart_poll_in(dev, reinterpret_cast<unsigned char*>(&byte)) != 0)
      {
        return false;
      }
    }
    return true;
#else
    (void)payload;
    return false;
#endif
  }

  template <typename Config, auto Handler> static bool start_async_rx(std::span<std::uint8_t> payload) noexcept
  {
#if defined(CONFIG_UART)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return false;
    }

    if constexpr (requires { uart_irq_callback_set(dev, Handler); })
    {
      uart_irq_callback_set(dev, Handler);
      uart_irq_rx_enable(dev);
      (void)payload;
      return true;
    }

    (void)payload;
    (void)Handler;
    return false;
#else
    (void)payload;
    (void)Handler;
    return false;
#endif
  }

  template <typename Config, auto Handler> static bool start_async_tx(std::span<const std::uint8_t> payload) noexcept
  {
    (void)payload;
    (void)Handler;
    return false;
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
#if defined(CONFIG_COUNTER)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return false;
    }

    std::uint32_t ticks = 0U;
    if (counter_get_value(dev, &ticks) != 0)
    {
      ticks = 0U;
    }
    rtc_epoch_ticks<Config>() = ticks;
    rtc_epoch_seconds<Config>() = 0U;
    return (counter_start(dev) == 0) || true;
#else
    return false;
#endif
  }

  template <typename Config>
  static bool set_time(const std::uint8_t hours, const std::uint8_t minutes, const std::uint8_t seconds) noexcept
  {
#if defined(CONFIG_COUNTER)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return false;
    }

    std::uint32_t ticks = 0U;
    if (counter_get_value(dev, &ticks) != 0)
    {
      return false;
    }

    rtc_epoch_ticks<Config>() = ticks;
    rtc_epoch_seconds<Config>() = (static_cast<std::uint32_t>(hours) * 3600U) +
                                  (static_cast<std::uint32_t>(minutes) * 60U) + static_cast<std::uint32_t>(seconds);
    return true;
#else
    (void)hours;
    (void)minutes;
    (void)seconds;
    return false;
#endif
  }

  template <typename Config>
  static bool get_time(std::uint8_t& hours, std::uint8_t& minutes, std::uint8_t& seconds) noexcept
  {
#if defined(CONFIG_COUNTER)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return false;
    }

    std::uint32_t ticks = 0U;
    if (counter_get_value(dev, &ticks) != 0)
    {
      return false;
    }

    const std::uint32_t frequency = counter_get_frequency(dev);
    if (frequency == 0U)
    {
      return false;
    }

    const std::uint32_t delta_ticks = (ticks >= rtc_epoch_ticks<Config>()) ? (ticks - rtc_epoch_ticks<Config>()) : 0U;
    const std::uint32_t total_seconds = rtc_epoch_seconds<Config>() + (delta_ticks / frequency);
    hours = static_cast<std::uint8_t>((total_seconds / 3600U) % 24U);
    minutes = static_cast<std::uint8_t>((total_seconds / 60U) % 60U);
    seconds = static_cast<std::uint8_t>(total_seconds % 60U);
    return true;
#else
    (void)hours;
    (void)minutes;
    (void)seconds;
    return false;
#endif
  }

  template <typename Config, auto Handler, typename AlarmType>
  static bool enable_alarm(const AlarmType& alarm_time) noexcept
  {
    (void)sizeof(Config);
    (void)Handler;
    (void)alarm_time;
    return false;
  }
};

struct flash_api final
{
  template <typename Config> static bool erase(const std::uint32_t address, const std::uint32_t bytes) noexcept
  {
#if defined(CONFIG_FLASH)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return false;
    }

    return flash_erase(dev, address, bytes) == 0;
#else
    (void)address;
    (void)bytes;
    return false;
#endif
  }

  template <typename Config>
  static bool program(const std::uint32_t address, const std::span<const std::uint8_t> data) noexcept
  {
#if defined(CONFIG_FLASH)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return false;
    }

    return flash_write(dev, address, data.data(), data.size()) == 0;
#else
    (void)address;
    (void)data;
    return false;
#endif
  }

  template <typename Config> static bool read(const std::uint32_t address, const std::span<std::uint8_t> data) noexcept
  {
#if defined(CONFIG_FLASH)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return false;
    }

    return flash_read(dev, address, data.data(), data.size()) == 0;
#else
    (void)address;
    (void)data;
    return false;
#endif
  }

  template <typename Config> static result<flash_erase_region> erase_region(const std::uint32_t address) noexcept
  {
#if defined(CONFIG_FLASH)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return status::error;
    }

    struct flash_pages_info info{};
    const int rc = flash_get_page_info_by_offs(dev, static_cast<off_t>(address), &info);
    if (rc == 0)
    {
      return flash_erase_region{.start_address = static_cast<std::uint32_t>(info.start_offset),
                                .size_bytes = info.size};
    }
    if ((rc == -ENOTSUP) || (rc == -ENOSYS))
    {
      return status::unsupported;
    }
    if (rc == -EINVAL)
    {
      return status::invalid_argument;
    }
    return status::error;
#else
    (void)address;
    return status::unsupported;
#endif
  }
};

struct timer_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(CONFIG_COUNTER)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return false;
    }

    if constexpr (requires { Config::period; })
    {
      if (Config::period == 0U)
      {
        return false;
      }

      struct counter_top_cfg top_cfg{};
      top_cfg.ticks = Config::period - 1U;
      if (counter_set_top_value(dev, &top_cfg) != 0)
      {
        return false;
      }
    }

    const int start_result = counter_start(dev);
    return (start_result == 0) || (start_result == -EALREADY);
#else
    return false;
#endif
  }

  template <typename Config> static status start() noexcept
  {
#if defined(CONFIG_COUNTER)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return status::error;
    }

    const int start_result = counter_start(dev);
    return ((start_result == 0) || (start_result == -EALREADY)) ? status::ok : status::error;
#else
    return status::unsupported;
#endif
  }

  template <typename Config> static status stop() noexcept
  {
#if defined(CONFIG_COUNTER)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return status::error;
    }

    const int stop_result = counter_stop(dev);
    return ((stop_result == 0) || (stop_result == -EALREADY)) ? status::ok : status::error;
#else
    return status::unsupported;
#endif
  }

  template <typename Config> static status set_period(const std::uint32_t period) noexcept
  {
#if defined(CONFIG_COUNTER)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return status::error;
    }
    if ((period == 0U) || ((period - 1U) > counter_get_max_top_value(dev)))
    {
      return status::invalid_argument;
    }

    struct counter_top_cfg top_cfg{};
    top_cfg.ticks = period - 1U;
    return (counter_set_top_value(dev, &top_cfg) == 0) ? status::ok : status::error;
#else
    (void)period;
    return status::unsupported;
#endif
  }

  template <typename Config> static std::uint32_t counter() noexcept
  {
#if defined(CONFIG_COUNTER)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return 0U;
    }

    std::uint32_t ticks = 0U;
    if (counter_get_value(dev, &ticks) != 0)
    {
      return 0U;
    }

    return ticks;
#else
    return 0U;
#endif
  }

  template <typename Config> static result<std::uint32_t> period() noexcept
  {
#if defined(CONFIG_COUNTER)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return status::error;
    }

    const std::uint32_t top_value = counter_get_top_value(dev);
    if (top_value == 0xFFFF'FFFFU)
    {
      return status::error;
    }
    return top_value + 1U;
#else
    return status::unsupported;
#endif
  }
};

struct adc_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(CONFIG_ADC)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return false;
    }

    if (const struct adc_channel_cfg* channel_cfg = adc_channel_config_from_config<Config>(); channel_cfg != nullptr)
    {
      return adc_channel_setup(dev, channel_cfg) == 0;
    }

    return true;
#else
    return false;
#endif
  }

  template <typename Config> static result<std::uint16_t> read_blocking(const std::uint8_t channel) noexcept
  {
#if defined(CONFIG_ADC)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return status::error;
    }

    std::uint16_t sample = 0U;
    struct adc_sequence sequence{};
    sequence.channels = BIT(channel);
    sequence.buffer = &sample;
    sequence.buffer_size = sizeof(sample);
    sequence.resolution = adc_resolution_bits<Config>();
    sequence.oversampling = adc_oversampling<Config>();

    if (adc_read(dev, &sequence) != 0)
    {
      return status::error;
    }

    return sample;
#else
    (void)channel;
    return status::unsupported;
#endif
  }

  template <typename Config, auto Handler> static bool start_interrupt(const std::uint8_t channel) noexcept
  {
#if defined(CONFIG_ADC)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return false;
    }

    static std::uint16_t sample = 0U;
    static struct k_poll_signal signal;
    k_poll_signal_init(&signal);

    struct adc_sequence sequence{};
    sequence.channels = BIT(channel);
    sequence.buffer = &sample;
    sequence.buffer_size = sizeof(sample);
    sequence.resolution = adc_resolution_bits<Config>();
    sequence.oversampling = adc_oversampling<Config>();

    if constexpr (requires { adc_read_async(dev, &sequence, &signal); })
    {
      return adc_read_async(dev, &sequence, &signal) == 0;
    }

    (void)Handler;
    return false;
#else
    (void)channel;
    (void)Handler;
    return false;
#endif
  }

  template <typename Config, auto Handler>
  static bool start_dma(const std::uint8_t channel, std::span<std::uint16_t> buffer) noexcept
  {
#if defined(CONFIG_ADC)
    (void)channel;
    (void)buffer;
    (void)Handler;
    return false;
#else
    (void)channel;
    (void)buffer;
    (void)Handler;
    return false;
#endif
  }

  template <typename Config> static bool start_continuous(const std::span<const std::uint8_t> channels) noexcept
  {
    if (channels.empty())
    {
      return false;
    }
    return read_blocking<Config>(channels.front()).has_value();
  }

  template <typename Config, auto Handler> static bool start_async(const std::uint8_t channel) noexcept
  {
    return start_interrupt<Config, Handler>(channel);
  }

  template <typename Config, auto Handler>
  static bool start_dma_scan(const std::span<const std::uint8_t> channels, std::span<std::uint16_t> results) noexcept
  {
    if (channels.empty())
    {
      return false;
    }
    return start_dma<Config, Handler>(channels.front(), results);
  }
};

struct dac_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(CONFIG_DAC)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return false;
    }

    struct dac_channel_cfg channel_cfg{};
    channel_cfg.channel_id = dac_channel_index<Config>();
#if defined(CONFIG_DAC_INIT_PRIORITY)
    channel_cfg.resolution = 12U;
#endif
    return dac_channel_setup(dev, &channel_cfg) == 0;
#else
    return false;
#endif
  }

  template <typename Config> static status write(const std::uint16_t value, const std::uint8_t channel) noexcept
  {
#if defined(CONFIG_DAC)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return status::error;
    }

    return (dac_write_value(dev, channel, value) == 0) ? status::ok : status::error;
#else
    (void)value;
    (void)channel;
    return status::unsupported;
#endif
  }

  template <typename Config> static status write_dual(const std::uint16_t value1, const std::uint16_t value2) noexcept
  {
#if defined(CONFIG_DAC)
    const status first = write<Config>(value1, 0U);
    return (first == status::ok) ? write<Config>(value2, 1U) : first;
#else
    (void)value1;
    (void)value2;
    return status::unsupported;
#endif
  }

  template <typename Config, auto Handler>
  static bool start_dma(const std::span<const std::uint16_t> data, const std::uint8_t channel) noexcept
  {
#if defined(CONFIG_DAC)
    (void)data;
    (void)channel;
    (void)Handler;
    return false;
#else
    (void)data;
    (void)channel;
    (void)Handler;
    return false;
#endif
  }

  template <typename Config> static status write_blocking(const std::uint16_t value) noexcept
  {
    return write<Config>(value, Config::channel);
  }

  template <typename Config, auto Handler>
  static bool start_dma_waveform(const std::span<const std::uint16_t> samples) noexcept
  {
    return start_dma<Config, Handler>(samples, Config::channel);
  }
};

struct can_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(CONFIG_CAN)
    const struct device* dev = device_from_config<Config>();
    return (dev != nullptr) && device_is_ready(dev) && (can_start(dev) == 0);
#else
    return false;
#endif
  }

  template <typename Config> static bool transmit(const microhal::can::message& msg) noexcept
  {
#if defined(CONFIG_CAN)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return false;
    }

    struct can_frame frame{};
    frame.id = msg.id;
    frame.dlc = static_cast<std::uint8_t>((msg.dlc > 8U) ? 8U : msg.dlc);
#if defined(CAN_FRAME_IDE)
    if constexpr (requires { frame.flags; })
    {
      if (msg.extended_id)
      {
        frame.flags |= CAN_FRAME_IDE;
      }
      if (msg.remote_frame)
      {
        frame.flags |= CAN_FRAME_RTR;
      }
    }
#endif
    for (std::uint8_t index = 0U; index < frame.dlc; ++index)
    {
      frame.data[index] = msg.data[index];
    }

    return can_send(dev, &frame, K_MSEC(100), nullptr, nullptr) == 0;
#else
    (void)msg;
    return false;
#endif
  }

  template <typename Config> static bool receive(microhal::can::message& msg) noexcept
  {
#if defined(CONFIG_CAN)
    struct k_msgq* rx_msgq = can_rx_msgq_from_config<Config>();
    if (rx_msgq == nullptr)
    {
      return false;
    }

    struct can_frame frame{};
    if (k_msgq_get(rx_msgq, &frame, K_NO_WAIT) != 0)
    {
      return false;
    }

    msg.id = frame.id;
    msg.dlc = static_cast<std::uint8_t>((frame.dlc > 8U) ? 8U : frame.dlc);
    msg.extended_id = false;
    msg.remote_frame = false;
#if defined(CAN_FRAME_IDE)
    if constexpr (requires { frame.flags; })
    {
      msg.extended_id = (frame.flags & CAN_FRAME_IDE) != 0U;
      msg.remote_frame = (frame.flags & CAN_FRAME_RTR) != 0U;
    }
#endif
    for (std::uint8_t index = 0U; index < msg.dlc; ++index)
    {
      msg.data[index] = frame.data[index];
    }

    return true;
#else
    (void)msg;
    return false;
#endif
  }

  template <typename Config> static bool transmit(const std::uint8_t* data, std::size_t length) noexcept
  {
    microhal::can::message msg{};
    msg.dlc = static_cast<std::uint8_t>((length > 8U) ? 8U : length);
    for (std::uint8_t index = 0U; index < msg.dlc; ++index)
    {
      msg.data[index] = data[index];
    }
    return transmit<Config>(msg);
  }

  template <typename Config> static bool receive(std::uint8_t* data, std::size_t& length) noexcept
  {
    microhal::can::message msg{};
    if (!receive<Config>(msg))
    {
      return false;
    }

    length = msg.dlc;
    for (std::size_t index = 0U; index < length; ++index)
    {
      data[index] = msg.data[index];
    }
    return true;
  }

  template <typename Config>
  static bool configure_filter(const std::uint32_t id, const std::uint32_t mask, const std::uint8_t fifo) noexcept
  {
#if defined(CONFIG_CAN)
    const struct device* dev = device_from_config<Config>();
    struct k_msgq* rx_msgq = can_rx_msgq_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev) || (rx_msgq == nullptr))
    {
      return false;
    }

    struct can_filter filter{};
    filter.id = id;
    filter.mask = mask;
    (void)fifo;

    if constexpr (requires { can_add_rx_filter_msgq(dev, rx_msgq, &filter); })
    {
      if (can_filter_handle<Config>() >= 0)
      {
        can_remove_rx_filter(dev, can_filter_handle<Config>());
      }
      can_filter_handle<Config>() = can_add_rx_filter_msgq(dev, rx_msgq, &filter);
      return can_filter_handle<Config>() >= 0;
    }

    return false;
#else
    (void)id;
    (void)mask;
    (void)fifo;
    return false;
#endif
  }

  template <typename Config, auto Handler> static bool enable_rx_interrupt() noexcept
  {
#if defined(CONFIG_CAN)
    (void)device_from_config<Config>();
    (void)Handler;
    return false;
#else
    (void)Handler;
    return false;
#endif
  }
};

struct usb_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(CONFIG_USB_DEVICE_STACK)
    (void)sizeof(Config);
    return usb_enable(nullptr) == 0;
#else
    return false;
#endif
  }

  template <typename Config> static bool connect() noexcept
  {
#if defined(CONFIG_USB_DEVICE_STACK)
    // Already connected via usb_enable()
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static bool disconnect() noexcept
  {
#if defined(CONFIG_USB_DEVICE_STACK)
    if constexpr (requires { usb_disable(); })
    {
      return usb_disable() == 0;
    }
    return false;
#else
    return false;
#endif
  }

  template <typename Config>
  static bool endpoint_transmit(const std::uint8_t endpoint, const std::span<const std::uint8_t> data) noexcept
  {
#if defined(CONFIG_USB_DEVICE_STACK)
    std::uint32_t written = 0U;
    return (usb_write(endpoint, data.data(), static_cast<std::uint32_t>(data.size()), &written) == 0) &&
           (written == static_cast<std::uint32_t>(data.size()));
#else
    (void)endpoint;
    (void)data;
    return false;
#endif
  }

  template <typename Config>
  static std::uint32_t endpoint_receive(const std::uint8_t endpoint, const std::span<std::uint8_t> buffer) noexcept
  {
#if defined(CONFIG_USB_DEVICE_STACK)
    std::uint32_t bytes_read = 0U;
    if (usb_read(endpoint, buffer.data(), static_cast<std::uint32_t>(buffer.size()), &bytes_read) != 0)
    {
      return 0U;
    }
    return bytes_read;
#else
    (void)endpoint;
    (void)buffer;
    return 0U;
#endif
  }
};

struct ethernet_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(CONFIG_NET_L2_ETHERNET)
    struct net_if* iface = ethernet_iface_from_config<Config>();
    return iface != nullptr;
#else
    return false;
#endif
  }

  template <typename Config> static bool start() noexcept
  {
#if defined(CONFIG_NET_L2_ETHERNET)
    return initialize<Config>();
#else
    return false;
#endif
  }

  template <typename Config> static bool stop() noexcept
  {
#if defined(CONFIG_NET_L2_ETHERNET)
    struct net_if* iface = ethernet_iface_from_config<Config>();
    if (iface == nullptr)
    {
      return false;
    }
    return net_if_down(iface) == 0;
#else
    return false;
#endif
  }

  template <typename Config> static bool transmit_frame(const std::span<const std::uint8_t> frame) noexcept
  {
#if defined(CONFIG_NET_L2_ETHERNET)
    struct net_if* iface = ethernet_iface_from_config<Config>();
    if ((iface == nullptr) || frame.empty())
    {
      return false;
    }

    struct net_pkt* pkt = net_pkt_alloc_with_buffer(iface, frame.size(), AF_UNSPEC, 0, K_NO_WAIT);
    if (pkt == nullptr)
    {
      return false;
    }

    if (net_pkt_write(pkt, frame.data(), frame.size()) != 0)
    {
      net_pkt_unref(pkt);
      return false;
    }

    return net_if_queue_tx(iface, pkt) == 0;
#else
    (void)frame;
    return false;
#endif
  }

  template <typename Config> static std::uint32_t receive_frame(const std::span<std::uint8_t> buffer) noexcept
  {
    (void)buffer;
    return 0U;
  }

  template <typename Config> static bool set_mac_address(const std::array<std::uint8_t, 6>& address) noexcept
  {
#if defined(CONFIG_NET_L2_ETHERNET)
    struct net_if* iface = ethernet_iface_from_config<Config>();
    if (iface == nullptr)
    {
      return false;
    }

    return net_if_set_link_addr(iface, const_cast<std::uint8_t*>(address.data()), address.size(), NET_LINK_ETHERNET) ==
           0;
#else
    (void)address;
    return false;
#endif
  }
};

struct sd_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(CONFIG_SDHC) || defined(CONFIG_DISK_ACCESS)
    return disk_access_init(sd_disk_name_from_config<Config>()) == 0;
#else
    return false;
#endif
  }

  template <typename Config>
  static bool read_blocks(const std::uint32_t start_block, const std::span<std::uint8_t> buffer) noexcept
  {
#if defined(CONFIG_SDHC) || defined(CONFIG_DISK_ACCESS)
    constexpr std::uint32_t block_size = sd_block_size_bytes<Config>();
    if ((buffer.size() == 0U) || ((buffer.size() % block_size) != 0U))
    {
      return false;
    }

    const std::uint32_t sector_count = static_cast<std::uint32_t>(buffer.size() / block_size);
    return disk_access_read(sd_disk_name_from_config<Config>(), buffer.data(), start_block, sector_count) == 0;
#else
    (void)start_block;
    (void)buffer;
    return false;
#endif
  }

  template <typename Config>
  static bool write_blocks(const std::uint32_t start_block, const std::span<const std::uint8_t> buffer) noexcept
  {
#if defined(CONFIG_SDHC) || defined(CONFIG_DISK_ACCESS)
    constexpr std::uint32_t block_size = sd_block_size_bytes<Config>();
    if ((buffer.size() == 0U) || ((buffer.size() % block_size) != 0U))
    {
      return false;
    }

    const std::uint32_t sector_count = static_cast<std::uint32_t>(buffer.size() / block_size);
    return disk_access_write(sd_disk_name_from_config<Config>(), buffer.data(), start_block, sector_count) == 0;
#else
    (void)start_block;
    (void)buffer;
    return false;
#endif
  }

  template <typename Config>
  static bool erase_blocks(const std::uint32_t start_block, const std::uint32_t block_count) noexcept
  {
#if defined(CONFIG_SDHC) || defined(CONFIG_DISK_ACCESS)
    (void)start_block;
    (void)block_count;
    // Zephyr disk access API does not provide a generic block erase call across all media.
    return false;
#else
    (void)start_block;
    (void)block_count;
    return false;
#endif
  }

  template <typename Config> static std::uint32_t capacity_blocks() noexcept
  {
#if defined(CONFIG_SDHC) || defined(CONFIG_DISK_ACCESS)
    std::uint32_t capacity = 0U;
    if (disk_access_ioctl(sd_disk_name_from_config<Config>(), DISK_IOCTL_GET_SECTOR_COUNT, &capacity) != 0)
    {
      return 0U;
    }
    return capacity;
#else
    return 0U;
#endif
  }
};

struct dma_api final
{
#if defined(CONFIG_DMA)
  template <typename Config> static atomic_t& transfer_state_storage() noexcept
  {
    static atomic_t state = static_cast<atomic_t>(dma_transfer_state::idle);
    return state;
  }

  template <typename Config> static void set_transfer_state(const dma_transfer_state state) noexcept
  {
    atomic_set(&transfer_state_storage<Config>(), static_cast<atomic_val_t>(state));
  }

  template <typename Config> static auto transfer_state() noexcept -> dma_transfer_state
  {
    return static_cast<dma_transfer_state>(atomic_get(&transfer_state_storage<Config>()));
  }

  template <typename Config, auto Handler>
  static void completion_callback(const struct device* dev, void* user_data, const std::uint32_t channel,
                                  const int callback_status) noexcept
  {
    set_transfer_state<Config>((callback_status == DMA_STATUS_COMPLETE) || (callback_status == DMA_STATUS_BLOCK)
                                   ? dma_transfer_state::complete
                                   : dma_transfer_state::error);
    if constexpr (Handler != nullptr)
    {
      Handler(dev, user_data, channel, callback_status);
    }
  }
#endif

  template <typename Config> static bool initialize() noexcept
  {
#if defined(CONFIG_DMA)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return false;
    }
    set_transfer_state<Config>(dma_transfer_state::idle);
    return true;
#else
    return false;
#endif
  }

  template <typename Config, auto Handler>
  static bool start_transfer(const std::uint32_t src_addr, const std::uint32_t dst_addr,
                             const std::uint32_t length) noexcept
  {
#if defined(CONFIG_DMA)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return false;
    }

    struct dma_block_config block_cfg{};
    block_cfg.source_address = src_addr;
    block_cfg.dest_address = dst_addr;
    block_cfg.block_size = length;

    struct dma_config dma_cfg{};
    dma_cfg.channel_direction = MEMORY_TO_MEMORY;
    dma_cfg.source_data_size = 1U;
    dma_cfg.dest_data_size = 1U;
    dma_cfg.source_burst_length = 1U;
    dma_cfg.dest_burst_length = 1U;
    dma_cfg.block_count = 1U;
    dma_cfg.complete_callback_en = 1U;
    dma_cfg.dma_callback = completion_callback<Config, Handler>;
    dma_cfg.head_block = &block_cfg;

    set_transfer_state<Config>(dma_transfer_state::active);
    if ((dma_config(dev, dma_channel_index<Config>(), &dma_cfg) == 0) &&
        (dma_start(dev, dma_channel_index<Config>()) == 0))
    {
      return true;
    }
    set_transfer_state<Config>(dma_transfer_state::error);
    return false;
#else
    (void)src_addr;
    (void)dst_addr;
    (void)length;
    (void)Handler;
    return false;
#endif
  }

  template <typename Config> static bool abort() noexcept
  {
#if defined(CONFIG_DMA)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev) || (dma_stop(dev, dma_channel_index<Config>()) != 0))
    {
      return false;
    }
    set_transfer_state<Config>(dma_transfer_state::idle);
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static result<dma_transfer_state> transfer_status() noexcept
  {
#if defined(CONFIG_DMA)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return status::error;
    }

    struct dma_status dma_status{};
    const int status_result = dma_get_status(dev, dma_channel_index<Config>(), &dma_status);
    if (status_result == -ENOSYS)
    {
      return status::unsupported;
    }
    if (status_result != 0)
    {
      return status::error;
    }
    if (dma_status.busy)
    {
      return dma_transfer_state::active;
    }

    const dma_transfer_state current_state = transfer_state<Config>();
    return (current_state == dma_transfer_state::active) ? dma_transfer_state::complete : current_state;
#else
    return status::unsupported;
#endif
  }
};

struct pwm_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(CONFIG_PWM)
    const struct pwm_dt_spec* spec = pwm_spec_from_config<Config>();
    return (spec != nullptr) && device_is_ready(spec->dev);
#else
    return false;
#endif
  }

  template <typename Config> static void set_duty_cycle(const std::uint16_t duty_permille) noexcept
  {
#if defined(CONFIG_PWM)
    const struct pwm_dt_spec* spec = pwm_spec_from_config<Config>();
    if ((spec == nullptr) || !device_is_ready(spec->dev) || (Config::frequency_hz == 0U))
    {
      return;
    }

    const std::uint32_t period_ns = static_cast<std::uint32_t>(1000000000ULL / Config::frequency_hz);
    const std::uint32_t pulse_ns =
        static_cast<std::uint32_t>((static_cast<std::uint64_t>(period_ns) * duty_permille) / 1000U);
    (void)pwm_set(spec->dev, spec->channel, period_ns, pulse_ns, spec->flags);
    (void)duty_permille;
#else
    (void)duty_permille;
#endif
  }

  template <typename Config> static bool start() noexcept
  {
#if defined(CONFIG_PWM)
    return initialize<Config>();
#else
    return false;
#endif
  }

  template <typename Config> static bool stop() noexcept
  {
#if defined(CONFIG_PWM)
    const struct pwm_dt_spec* spec = pwm_spec_from_config<Config>();
    if ((spec == nullptr) || !device_is_ready(spec->dev) || (Config::frequency_hz == 0U))
    {
      return false;
    }

    const std::uint32_t period_ns = static_cast<std::uint32_t>(1000000000ULL / Config::frequency_hz);
    return pwm_set(spec->dev, spec->channel, period_ns, 0U, spec->flags) == 0;
#else
    return false;
#endif
  }
};

struct watchdog_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(CONFIG_WATCHDOG)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return false;
    }

    struct wdt_timeout_cfg timeout_cfg{};
    timeout_cfg.window.min = Config::window_mode ? (Config::timeout_ms / 2U) : 0U;
    timeout_cfg.window.max = Config::timeout_ms;
    timeout_cfg.callback = nullptr;
    timeout_cfg.flags = WDT_FLAG_RESET_SOC;

    watchdog_channel_id<Config>() = wdt_install_timeout(dev, &timeout_cfg);
    return (watchdog_channel_id<Config>() >= 0) && (wdt_setup(dev, 0U) == 0);
#else
    return false;
#endif
  }

  template <typename Config> static bool start() noexcept
  {
#if defined(CONFIG_WATCHDOG)
    return initialize<Config>();
#else
    return false;
#endif
  }

  template <typename Config> static bool refresh() noexcept
  {
#if defined(CONFIG_WATCHDOG)
    const struct device* dev = device_from_config<Config>();
    return (dev != nullptr) && device_is_ready(dev) && (watchdog_channel_id<Config>() >= 0) &&
           (wdt_feed(dev, watchdog_channel_id<Config>()) == 0);
#else
    return false;
#endif
  }
};

struct i2s_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(CONFIG_I2S)
    const struct device* dev = device_from_config<Config>();
    const struct i2s_config* tx_config = i2s_tx_config_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev) || (tx_config == nullptr))
    {
      return false;
    }

    bool configured = i2s_configure(dev, I2S_DIR_TX, tx_config) == 0;
    if (const struct i2s_config* rx_config = i2s_rx_config_from_config<Config>(); rx_config != nullptr)
    {
      configured = configured && (i2s_configure(dev, I2S_DIR_RX, rx_config) == 0);
    }
    return configured;
#else
    return false;
#endif
  }

  template <typename Config> static bool transmit_blocking(const std::span<const std::uint16_t> samples) noexcept
  {
#if defined(CONFIG_I2S)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return false;
    }

    if (i2s_trigger(dev, I2S_DIR_TX, I2S_TRIGGER_START) != 0)
    {
      return false;
    }

    return i2s_write(dev, const_cast<std::uint16_t*>(samples.data()), samples.size() * sizeof(std::uint16_t)) == 0;
#else
    (void)samples;
    return false;
#endif
  }

  template <typename Config> static bool receive_blocking(const std::span<std::uint16_t> samples) noexcept
  {
#if defined(CONFIG_I2S)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return false;
    }

    if (i2s_trigger(dev, I2S_DIR_RX, I2S_TRIGGER_START) != 0)
    {
      return false;
    }

    void* block = nullptr;
    std::size_t bytes_read = 0U;
    if (i2s_read(dev, &block, &bytes_read) != 0)
    {
      return false;
    }

    const std::size_t sample_count =
        ((bytes_read / sizeof(std::uint16_t)) < samples.size()) ? (bytes_read / sizeof(std::uint16_t)) : samples.size();
    auto* input = static_cast<std::uint16_t*>(block);
    for (std::size_t index = 0U; index < sample_count; ++index)
    {
      samples[index] = input[index];
    }
    return true;
#else
    (void)samples;
    return false;
#endif
  }

  template <typename Config, auto Handler>
  static bool start_dma_tx(const std::span<const std::uint16_t> samples) noexcept
  {
#if defined(CONFIG_I2S)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return false;
    }

    if (i2s_trigger(dev, I2S_DIR_TX, I2S_TRIGGER_START) != 0)
    {
      return false;
    }

    (void)Handler;
    return i2s_write(dev, const_cast<std::uint16_t*>(samples.data()), samples.size() * sizeof(std::uint16_t)) == 0;
#else
    (void)samples;
    (void)Handler;
    return false;
#endif
  }
};

struct rcc_api final
{
  template <typename Config> static bool configure() noexcept
  {
#if defined(CONFIG_CLOCK_CONTROL)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return false;
    }

    clock_control_subsys_t subsys = clock_subsys_from_config<Config>();
    if (subsys == nullptr)
    {
      return true;
    }

    return clock_control_on(dev, subsys) == 0;
#else
    return false;
#endif
  }

  template <typename Config> static std::uint32_t get_system_clock_hz() noexcept
  {
#if defined(CONFIG_CLOCK_CONTROL)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return 0U;
    }

    clock_control_subsys_t subsys = clock_subsys_from_config<Config>();
    if (subsys == nullptr)
    {
      return 0U;
    }

    std::uint32_t rate = 0U;
    if (clock_control_get_rate(dev, subsys, &rate) != 0)
    {
      return 0U;
    }

    return rate;
#else
    return 0U;
#endif
  }

  template <typename Config>
  static status enable_peripheral_clock(const peripheral_kind peripheral, const std::uint8_t instance) noexcept
  {
#if defined(CONFIG_CLOCK_CONTROL)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return status::error;
    }

    clock_control_subsys_t subsys = clock_subsys_from_config<Config>();
    if (subsys != nullptr)
    {
      return (clock_control_on(dev, subsys) == 0) ? status::ok : status::error;
    }

    (void)peripheral;
    (void)instance;
    return status::unsupported;
#else
    (void)peripheral;
    (void)instance;
    return status::unsupported;
#endif
  }

  template <typename Config>
  static status disable_peripheral_clock(const peripheral_kind peripheral, const std::uint8_t instance) noexcept
  {
#if defined(CONFIG_CLOCK_CONTROL)
    const struct device* dev = device_from_config<Config>();
    if ((dev == nullptr) || !device_is_ready(dev))
    {
      return status::error;
    }

    clock_control_subsys_t subsys = clock_subsys_from_config<Config>();
    if (subsys != nullptr)
    {
      return (clock_control_off(dev, subsys) == 0) ? status::ok : status::error;
    }

    (void)peripheral;
    (void)instance;
    return status::unsupported;
#else
    (void)peripheral;
    (void)instance;
    return status::unsupported;
#endif
  }
};

struct crc_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(CONFIG_CRC)
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static std::uint32_t calculate(const std::span<const std::uint8_t> data) noexcept
  {
#if defined(CONFIG_CRC)
    return crc32_ieee(data.data(), data.size());
#else
    (void)data;
    return 0U;
#endif
  }

  template <typename Config> static void reset() noexcept
  {
#if defined(CONFIG_CRC)
    // Software CRC doesn't need reset
#endif
  }

  template <typename Config> static std::uint32_t get_random() noexcept
  {
#if defined(CONFIG_ENTROPY_GENERATOR)
    (void)sizeof(Config);
    return sys_rand32_get();
#else
    return 0U;
#endif
  }
};

} // namespace microhal::backends::zephyr

namespace microhal
{

template <> struct backend_traits<zephyr_tag> final
{
  using backend_tag = zephyr_tag;
  using gpio_api = backends::zephyr::gpio_api;
  using i2c_api = backends::zephyr::i2c_api;
  using spi_api = backends::zephyr::spi_api;
  using qspi_api = backends::zephyr::qspi_api;
  using uart_api = backends::zephyr::uart_api;
  using rtc_api = backends::zephyr::rtc_api;
  using flash_api = backends::zephyr::flash_api;
  using timer_api = backends::zephyr::timer_api;
  using adc_api = backends::zephyr::adc_api;
  using dac_api = backends::zephyr::dac_api;
  using can_api = backends::zephyr::can_api;
  using usb_api = backends::zephyr::usb_api;
  using ethernet_api = backends::zephyr::ethernet_api;
  using sd_api = backends::zephyr::sd_api;
  using dma_api = backends::zephyr::dma_api;
  using pwm_api = backends::zephyr::pwm_api;
  using watchdog_api = backends::zephyr::watchdog_api;
  using i2s_api = backends::zephyr::i2s_api;
  using rcc_api = backends::zephyr::rcc_api;
  using crc_api = backends::zephyr::crc_api;

  static constexpr vendor_kind vendor = vendor_kind::zephyr;
  static constexpr backend_kind kind = backend_kind::zephyr_driver_model;
  static constexpr std::string_view name{"zephyr-driver-model"};
  static constexpr std::uint32_t gpio_port_count{32U};
  static constexpr std::uint32_t gpio_pins_per_port{32U};
  static constexpr std::uint32_t gpio_alternate_function_count{32U};

  static consteval struct gpio_capability gpio_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = gpio_port_count,
            .max_frequency_hz = 0U,
            .alternate_function = true,
            .output_toggle = true};
  }

  static consteval struct i2c_capability i2c_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 8U,
            .max_frequency_hz = 1'000'000U,
            .ten_bit_addressing = false,
            .repeated_start = true,
            .address_probe = true};
  }

  static consteval struct spi_capability spi_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 8U,
            .max_frequency_hz = 80'000'000U,
            .transmit_only = true,
            .receive_only = true,
            .async_transfer = false};
  }

  static consteval struct qspi_capability qspi_capability() noexcept
  {
#if defined(CONFIG_FLASH)
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 96'000'000U,
            .quad_mode = true,
            .dual_mode = true,
            .octo_mode = false,
            .ddr_mode = false,
            .command_transfer = true,
            .memory_mapped = false,
            .max_address_bytes = 4U};
#else
    return {};
#endif
  }

  static consteval struct uart_capability uart_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 12U,
            .max_frequency_hz = 12'500'000U,
            .fifo = true,
            .lpuart = true,
            .autobaud = true,
            .fifo_depth = 64U};
  }

  static consteval struct rtc_capability rtc_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = false,
            .instance_count = 2U,
            .max_frequency_hz = 0U,
            .periodic = true,
            .alarm = false,
            .calendar = false,
            .subsecond = false};
  }

  static consteval struct flash_capability flash_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 2U,
            .max_frequency_hz = 0U,
            .alignment_bytes = 4U,
            .erase_bytes = 4'096U,
            .erase_region_query = true,
            .program_readback = true};
  }

  static consteval struct timer_capability timer_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 16U,
            .max_frequency_hz = 200'000'000U,
            .capture_compare = true,
            .periodic = true,
            .period_readback = true};
  }

  static consteval struct adc_capability adc_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = false,
            .instance_count = 3U,
            .max_frequency_hz = 36'000'000U,
            .resolution_bits = 12U,
            .channel_count = 16U,
            .differential = true,
            .oversampling = true,
            .sequential_scan = true,
            .voltage_conversion = true};
  }

  static consteval struct dac_capability dac_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 2U,
            .max_frequency_hz = 0U,
            .resolution_bits = 12U,
            .dual_mode = true,
            .waveform_generation = false,
            .voltage_conversion = true,
            .sequence_write = true};
  }

  static consteval struct can_capability can_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 2U,
            .max_frequency_hz = 1'000'000U,
            .can_fd = false,
            .time_triggered = false,
            .mailbox_count = 3U,
            .filter_banks = 14U,
            .extended_ids = true,
            .remote_frames = true,
            .rx_interrupt = true};
  }

  static consteval struct usb_capability usb_capability() noexcept
  {
#if defined(CONFIG_USB_DEVICE_STACK)
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .device = true,
            .host = false,
            .otg = false,
            .high_speed = false,
            .endpoint_count = 6U,
            .endpoint_transfer = true,
            .endpoint_stall = true,
            .max_packet_size_bytes = 64U};
#else
    return {};
#endif
  }

  static consteval struct ethernet_capability ethernet_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 100'000'000U,
            .checksum_offload = false,
            .ptp_timestamp = false,
            .vlan_filter = false,
            .gigabit = false,
            .mtu_bytes = 1518U};
  }

  static consteval struct sd_capability sd_capability() noexcept
  {
#if defined(CONFIG_SDHC) || defined(CONFIG_DISK_ACCESS)
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 50'000'000U,
            .high_speed = false,
            .uhs = false,
            .bus_width_4 = true,
            .bus_width_8 = false,
            .block_size_bytes = 512U,
            .block_io = true,
            .erase = false,
            .capacity_query = true,
            .bus_width_1 = true};
#else
    return {};
#endif
  }

  static consteval struct dma_capability dma_capability() noexcept
  {
    return {.supported = true,
            .blocking = false,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 2U,
            .max_frequency_hz = 0U,
            .stream_count = 8U,
            .channel_count = 8U,
            .mem_to_mem = true,
            .fifo = false,
            .double_buffer = false,
            .transfer_status = true,
            .abort = true,
            .completion_callback = true,
            .address_width_bits = 32U};
  }

  static consteval struct pwm_capability pwm_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 16U,
            .max_frequency_hz = 200'000'000U,
            .channel_count = 4U,
            .complementary_output = true,
            .dead_time = true,
            .runtime_duty_update = true,
            .runtime_start_stop = true};
  }

  static consteval struct watchdog_capability watchdog_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = true,
            .dma = false,
            .instance_count = 2U,
            .max_frequency_hz = 0U,
            .independent = true,
            .window = true,
            .min_timeout_ms = 0U,
            .max_timeout_ms = 32'000U,
            .runtime_refresh = true,
            .reset_on_timeout = true};
  }

  static consteval struct i2s_capability i2s_capability() noexcept
  {
#if defined(CONFIG_I2S)
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 3U,
            .max_frequency_hz = 42'000'000U,
            .full_duplex = true,
            .master_mode = true,
            .slave_mode = true,
            .master_clock_output = true,
            .max_sample_rate = 192'000U,
            .transmit_dma = false,
            .receive_dma = false,
            .sample_bits = 16U};
#else
    return {};
#endif
  }

  static consteval struct rcc_capability rcc_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .pll_config = false,
            .hse = false,
            .lse = false,
            .max_system_clock_hz = 200'000'000U,
            .clock_configuration = true,
            .peripheral_clock_gating = true,
            .system_clock_readback = true};
  }

  static consteval struct crc_capability crc_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .hardware_crc32 = false,
            .programmable_polynomial = false,
            .random_number_generator = true,
            .seed_configurable = false,
            .output_xor_configurable = false,
            .checksum_width_bits = 32U};
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
