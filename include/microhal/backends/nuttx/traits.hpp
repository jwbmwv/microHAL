// SPDX-License-Identifier: Apache-2.0
/**
 * @file traits.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <array>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string_view>

// NuttX RTOS driver includes - conditional compilation when building for NuttX
#ifdef __NUTTX__
#include <arch/board/board.h>
#include <fcntl.h>
#include <nuttx/analog/adc.h>
#include <nuttx/analog/dac.h>
#include <nuttx/can/can.h>
#include <nuttx/config.h>
#include <nuttx/drivers/drivers.h>
#include <nuttx/fs/ioctl.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/mtd/mtd.h>
#include <nuttx/serial/serial.h>
#include <nuttx/spi/spi.h>
#if defined(CONFIG_SPI_EXCHANGE)
#include <nuttx/spi/spi_transfer.h>
#endif
#include <nuttx/timers/pwm.h>
#include <nuttx/timers/rtc.h>
#include <nuttx/timers/timer.h>
#include <nuttx/timers/watchdog.h>
#include <nuttx/usb/usbdev.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#if defined(CONFIG_NET) && defined(CONFIG_NET_PKT) && (defined(CONFIG_NET_ETHERNET) || defined(CONFIG_STM32H7_ETHMAC))
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netpacket/packet.h>
#include <nuttx/net/ioctl.h>
#include <sys/socket.h>
#endif
#endif

#include "microhal/core/traits.hpp"

namespace microhal::backends::nuttx
{

template <typename Config> consteval const char* i2c_device_path() noexcept
{
  if constexpr (requires { typename Config::binding; } && requires { Config::binding::device_path; })
  {
    return Config::binding::device_path;
  }
  else if constexpr (requires { Config::device_path; })
  {
    return Config::device_path;
  }
  else
  {
    return "/dev/i2c0";
  }
}

template <typename Config> consteval const char* spi_device_path() noexcept
{
  if constexpr (requires { typename Config::binding; } && requires { Config::binding::device_path; })
  {
    return Config::binding::device_path;
  }
  else if constexpr (requires { Config::device_path; })
  {
    return Config::device_path;
  }
  else
  {
    return "/dev/spi0";
  }
}

template <typename Config> consteval const char* qspi_device_path() noexcept
{
  if constexpr (requires { typename Config::binding; } && requires { Config::binding::device_path; })
  {
    return Config::binding::device_path;
  }
  else if constexpr (requires { Config::device_path; })
  {
    return Config::device_path;
  }
  else
  {
    return "/dev/mtdblock0";
  }
}

template <typename Config> consteval const char* sd_device_path() noexcept
{
  if constexpr (requires { typename Config::binding; } && requires { Config::binding::device_path; })
  {
    return Config::binding::device_path;
  }
  else if constexpr (requires { Config::device_path; })
  {
    return Config::device_path;
  }
  else
  {
    return "/dev/mmcsd0";
  }
}

template <typename Config>
static const char* runtime_device_path(char* storage, const std::size_t capacity, const char* device_name) noexcept
{
  if constexpr (requires { typename Config::binding; } && requires { Config::binding::device_path; })
  {
    return Config::binding::device_path;
  }
  else if constexpr (requires { Config::device_path; })
  {
    return Config::device_path;
  }
  else
  {
    constexpr unsigned instance = []
    {
      if constexpr (requires { Config::instance_index; })
      {
        return static_cast<unsigned>(Config::instance_index);
      }
      return 0U;
    }();
    const int length = std::snprintf(storage, capacity, "/dev/%s%u", device_name, instance);
    return ((length >= 0) && (static_cast<std::size_t>(length) < capacity)) ? storage : nullptr;
  }
}

struct gpio_api final
{
  template <typename PinConfig> static void configure() noexcept
  {
#if defined(__NUTTX__)
    // NuttX GPIO is board-specific, typically:
    // stm32_configgpio(GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_PORTX | GPIO_PINX);
    // User implements based on board configuration
#endif
  }

  template <typename PinConfig> static void write(const bool level) noexcept
  {
#if defined(__NUTTX__)
    // stm32_gpiowrite(GPIO_PORTX_PINX, level);
    (void)level;
#else
    (void)level;
#endif
  }

  template <typename PinConfig> static bool read() noexcept
  {
#if defined(__NUTTX__)
    // return stm32_gpioread(GPIO_PORTX_PINX);
    return false;
#else
    return false;
#endif
  }

  template <typename PinConfig, auto Handler> static bool enable_interrupt() noexcept
  {
    (void)Handler;
#if defined(__NUTTX__) && defined(CONFIG_GPIO)
    (void)sizeof(PinConfig);
    return false;
#else
    return false;
#endif
  }
};

struct i2c_api final
{
  template <typename Config> static constexpr bool valid_address(const std::uint16_t address) noexcept
  {
    constexpr std::uint16_t max_address = (Config::address_mode == i2c_address_mode::ten_bit) ? 0x3FFU : 0x7FU;
    return address <= max_address;
  }

  // The NuttX I2C character-device ioctl has no timeout argument. A reported
  // ETIMEDOUT is preserved, but Config::timeout_ms cannot bound this call.
  static status transfer_status(const int result) noexcept
  {
    if (result >= 0)
    {
      return status::ok;
    }
    if (errno == ETIMEDOUT)
    {
      return status::timeout;
    }
    if (errno == EBUSY)
    {
      return status::busy;
    }
    if ((errno == ENXIO) || (errno == EIO))
    {
      return status::io_error;
    }
#ifdef ENOTSUP
    if (errno == ENOTSUP)
    {
      return status::unsupported;
    }
#endif
    return status::error;
  }

  template <typename Config> static bool initialize() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_I2C) && defined(CONFIG_I2C_DRIVER)
    const int fd = open(i2c_device_path<Config>(), O_RDWR);
    if (fd < 0)
    {
      return false;
    }
    close(fd);
    return true;
#else
    return false;
#endif
  }

  template <typename Config>
  static bool write_blocking(const std::uint8_t addr, const std::uint8_t* data, const std::size_t length) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_I2C) && defined(CONFIG_I2C_DRIVER)
    if ((length > UINT16_MAX) || (addr > 0x7FU))
    {
      return false;
    }

    const int fd = open(i2c_device_path<Config>(), O_RDWR);
    if (fd < 0)
    {
      return false;
    }

    i2c_msg_s message{};
    message.frequency = Config::clock_hz;
    message.addr = addr;
    message.flags = 0U;
    message.buffer = const_cast<std::uint8_t*>(data);
    message.length = static_cast<std::uint16_t>(length);

    i2c_transfer_s transfer{};
    transfer.msgv = &message;
    transfer.msgc = 1U;

    const int rc = ioctl(fd, I2CIOC_TRANSFER, reinterpret_cast<unsigned long>(&transfer));
    const status transfer_result = transfer_status(rc);
    const int close_result = close(fd);
    return (transfer_result == status::ok) && (close_result == 0);
#else
    (void)addr;
    (void)data;
    (void)length;
    return false;
#endif
  }

  template <typename Config>
  static status write_blocking(const std::span<const std::uint8_t> payload, const std::uint16_t address) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_I2C) && defined(CONFIG_I2C_DRIVER)
    if ((payload.size() > UINT16_MAX) || !valid_address<Config>(address))
    {
      return status::invalid_argument;
    }

    const int fd = open(i2c_device_path<Config>(), O_RDWR);
    if (fd < 0)
    {
      return status::error;
    }

    i2c_msg_s message{};
    message.frequency = Config::clock_hz;
    message.addr = address;
    if constexpr (Config::address_mode == i2c_address_mode::ten_bit)
    {
#if defined(I2C_M_TEN)
      message.flags = I2C_M_TEN;
#else
      close(fd);
      return status::unsupported;
#endif
    }
    else
    {
      message.flags = 0U;
    }
    message.buffer = const_cast<std::uint8_t*>(payload.data());
    message.length = static_cast<std::uint16_t>(payload.size());

    i2c_transfer_s transfer{};
    transfer.msgv = &message;
    transfer.msgc = 1U;

    const int rc = ioctl(fd, I2CIOC_TRANSFER, reinterpret_cast<unsigned long>(&transfer));
    const status transfer_result = transfer_status(rc);
    const int close_result = close(fd);
    if ((transfer_result == status::ok) && (close_result < 0))
    {
      return status::error;
    }
    return transfer_result;
#else
    (void)payload;
    (void)address;
    return status::unsupported;
#endif
  }

  template <typename Config>
  static status read_blocking(const std::span<std::uint8_t> payload, const std::uint16_t address) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_I2C) && defined(CONFIG_I2C_DRIVER)
    if ((payload.size() > UINT16_MAX) || !valid_address<Config>(address))
    {
      return status::invalid_argument;
    }

    const int fd = open(i2c_device_path<Config>(), O_RDWR);
    if (fd < 0)
    {
      return status::error;
    }

    i2c_msg_s message{};
    message.frequency = Config::clock_hz;
    message.addr = address;
    if constexpr (Config::address_mode == i2c_address_mode::ten_bit)
    {
#if defined(I2C_M_TEN)
      message.flags = I2C_M_READ | I2C_M_TEN;
#else
      close(fd);
      return status::unsupported;
#endif
    }
    else
    {
      message.flags = I2C_M_READ;
    }
    message.buffer = payload.data();
    message.length = static_cast<std::uint16_t>(payload.size());

    i2c_transfer_s transfer{};
    transfer.msgv = &message;
    transfer.msgc = 1U;

    const int rc = ioctl(fd, I2CIOC_TRANSFER, reinterpret_cast<unsigned long>(&transfer));
    const status transfer_result = transfer_status(rc);
    const int close_result = close(fd);
    if ((transfer_result == status::ok) && (close_result < 0))
    {
      return status::error;
    }
    return transfer_result;
#else
    (void)payload;
    (void)address;
    return status::unsupported;
#endif
  }

  template <typename Config> static status probe(const std::uint16_t address) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_I2C) && defined(CONFIG_I2C_DRIVER)
    if (!valid_address<Config>(address))
    {
      return status::invalid_argument;
    }

    const int fd = open(i2c_device_path<Config>(), O_RDWR);
    if (fd < 0)
    {
      return status::error;
    }

    i2c_msg_s message{};
    message.frequency = Config::clock_hz;
    message.addr = address;
    message.buffer = nullptr;
    message.length = 0U;
    if constexpr (Config::address_mode == i2c_address_mode::ten_bit)
    {
#if defined(I2C_M_TEN)
      message.flags = I2C_M_TEN;
#else
      close(fd);
      return status::unsupported;
#endif
    }

    i2c_transfer_s transfer{};
    transfer.msgv = &message;
    transfer.msgc = 1U;
    const status transfer_result =
        transfer_status(ioctl(fd, I2CIOC_TRANSFER, reinterpret_cast<unsigned long>(&transfer)));
    const int close_result = close(fd);
    if ((transfer_result == status::ok) && (close_result < 0))
    {
      return status::error;
    }
    return transfer_result;
#else
    (void)address;
    return status::unsupported;
#endif
  }

  template <typename Config>
  static status write_read_blocking(const std::span<const std::uint8_t> tx_payload,
                                    const std::span<std::uint8_t> rx_payload, const std::uint16_t address) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_I2C) && defined(CONFIG_I2C_DRIVER)
    if (!valid_address<Config>(address) || (tx_payload.size() > UINT16_MAX) || (rx_payload.size() > UINT16_MAX))
    {
      return status::invalid_argument;
    }
#if !defined(I2C_M_NOSTOP)
    return status::unsupported;
#else
    const int fd = open(i2c_device_path<Config>(), O_RDWR);
    if (fd < 0)
    {
      return status::error;
    }

    auto write_flags = I2C_M_NOSTOP;
    auto read_flags = I2C_M_READ;
    if constexpr (Config::address_mode == i2c_address_mode::ten_bit)
    {
#if defined(I2C_M_TEN)
      write_flags |= I2C_M_TEN;
      read_flags |= I2C_M_TEN;
#else
      close(fd);
      return status::unsupported;
#endif
    }

    i2c_msg_s messages[2]{};
    messages[0].frequency = Config::clock_hz;
    messages[0].addr = address;
    messages[0].flags = write_flags;
    messages[0].buffer = const_cast<std::uint8_t*>(tx_payload.data());
    messages[0].length = static_cast<std::uint16_t>(tx_payload.size());
    messages[1].frequency = Config::clock_hz;
    messages[1].addr = address;
    messages[1].flags = read_flags;
    messages[1].buffer = rx_payload.data();
    messages[1].length = static_cast<std::uint16_t>(rx_payload.size());

    i2c_transfer_s transfer{};
    transfer.msgv = messages;
    transfer.msgc = 2U;

    const int rc = ioctl(fd, I2CIOC_TRANSFER, reinterpret_cast<unsigned long>(&transfer));
    const status transfer_result = transfer_status(rc);
    const int close_result = close(fd);
    if ((transfer_result == status::ok) && (close_result < 0))
    {
      return status::error;
    }
    return transfer_result;
#endif
#else
    (void)tx_payload;
    (void)rx_payload;
    (void)address;
    return status::unsupported;
#endif
  }

  template <typename Config, auto Handler>
  static status start_transfer(const std::span<std::uint8_t> payload, const std::uint16_t address) noexcept
  {
    (void)sizeof(Config);
    (void)payload;
    (void)address;
    (void)Handler;
    return status::unsupported;
  }
};

struct spi_api final
{
  static status transfer_status(const int result) noexcept
  {
    if (result >= 0)
    {
      return status::ok;
    }
    if (errno == ETIMEDOUT)
    {
      return status::timeout;
    }
    if (errno == EBUSY)
    {
      return status::busy;
    }
    if ((errno == EIO) || (errno == ENXIO))
    {
      return status::io_error;
    }
#ifdef ENOTSUP
    if (errno == ENOTSUP)
    {
      return status::unsupported;
    }
#endif
    return status::error;
  }

  template <typename Config> static bool initialize() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_SPI) && defined(CONFIG_SPI_DRIVER) && defined(CONFIG_SPI_EXCHANGE)
    const int fd = open(spi_device_path<Config>(), O_RDWR);
    if (fd < 0)
    {
      return false;
    }
    return close(fd) == 0;
#else
    return false;
#endif
  }

  template <typename Config>
  static status transfer(const std::uint8_t* tx_data, std::uint8_t* rx_data, const std::size_t length) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_SPI) && defined(CONFIG_SPI_DRIVER) && defined(CONFIG_SPI_EXCHANGE)
    if (length > UINT16_MAX)
    {
      return status::invalid_argument;
    }

    if constexpr (requires { Config::device_id; } ||
                  (requires { typename Config::binding; } && requires { Config::binding::device_id; }))
    {
      constexpr std::uint32_t device_id = []
      {
        if constexpr (requires { Config::device_id; })
        {
          return static_cast<std::uint32_t>(Config::device_id);
        }
        else
        {
          return static_cast<std::uint32_t>(Config::binding::device_id);
        }
      }();
      constexpr std::uint8_t mode =
          static_cast<std::uint8_t>((Config::clock_polarity == spi_clock_polarity::idle_high ? 2U : 0U) |
                                    (Config::clock_phase == spi_clock_phase::second_edge ? 1U : 0U));
      struct spi_trans_s transaction
      {
      };
      transaction.deselect = true;
      transaction.nwords = length;
      transaction.txbuffer = tx_data;
      transaction.rxbuffer = rx_data;

      struct spi_sequence_s sequence
      {
      };
      sequence.dev = device_id;
      sequence.mode = mode;
      sequence.nbits = 8U;
      sequence.ntrans = 1U;
      sequence.frequency = Config::clock_hz;
      sequence.trans = &transaction;

      const int fd = open(spi_device_path<Config>(), O_RDWR);
      if (fd < 0)
      {
        return status::error;
      }
      const int rc = ioctl(fd, SPIIOC_TRANSFER, reinterpret_cast<unsigned long>(&sequence));
      const status transfer_result = transfer_status(rc);
      const int close_result = close(fd);
      if ((transfer_result == status::ok) && (close_result < 0))
      {
        return status::error;
      }
      return transfer_result;
    }
    else
    {
      (void)tx_data;
      (void)rx_data;
      return status::unsupported;
    }
#else
    (void)sizeof(Config);
    (void)tx_data;
    (void)rx_data;
    (void)length;
    return status::unsupported;
#endif
  }

  template <typename Config>
  static status transfer(const std::span<const std::uint8_t> tx_payload,
                         const std::span<std::uint8_t> rx_payload) noexcept
  {
    if (tx_payload.size() != rx_payload.size())
    {
      return status::invalid_argument;
    }
    return transfer<Config>(tx_payload.data(), rx_payload.data(), tx_payload.size());
  }

  template <typename Config> static status write_blocking(const std::span<const std::uint8_t> payload) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_SPI) && defined(CONFIG_SPI_DRIVER) && defined(CONFIG_SPI_EXCHANGE)
    return transfer<Config>(payload.data(), nullptr, payload.size());
#else
    (void)sizeof(Config);
    (void)payload;
    return status::unsupported;
#endif
  }

  template <typename Config> static status read_blocking(const std::span<std::uint8_t> payload) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_SPI) && defined(CONFIG_SPI_DRIVER) && defined(CONFIG_SPI_EXCHANGE)
    return transfer<Config>(nullptr, payload.data(), payload.size());
#else
    (void)sizeof(Config);
    (void)payload;
    return status::unsupported;
#endif
  }

  template <typename Config, auto Handler>
  static status transfer_async(const std::span<const std::uint8_t> tx_payload,
                               const std::span<std::uint8_t> rx_payload) noexcept
  {
    (void)sizeof(Config);
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
  template <typename Config> static int baud_constant() noexcept
  {
#if defined(__NUTTX__) && (defined(CONFIG_SERIAL) || defined(CONFIG_USART3_SERIAL_CONSOLE))
    if constexpr (requires { Config::baud_rate; })
    {
      switch (Config::baud_rate)
      {
#ifdef B1200
      case 1200U:
        return B1200;
#endif
#ifdef B2400
      case 2400U:
        return B2400;
#endif
#ifdef B4800
      case 4800U:
        return B4800;
#endif
#ifdef B9600
      case 9600U:
        return B9600;
#endif
#ifdef B19200
      case 19200U:
        return B19200;
#endif
#ifdef B38400
      case 38400U:
        return B38400;
#endif
#ifdef B57600
      case 57600U:
        return B57600;
#endif
#ifdef B115200
      case 115200U:
        return B115200;
#endif
#ifdef B230400
      case 230400U:
        return B230400;
#endif
#ifdef B460800
      case 460800U:
        return B460800;
#endif
      default:
        return 0;
      }
    }
    return 0;
#else
    return 0;
#endif
  }

  template <typename Config> static int open_device() noexcept
  {
#if defined(__NUTTX__) && (defined(CONFIG_SERIAL) || defined(CONFIG_USART3_SERIAL_CONSOLE))
    char path[32]{};
    const char* device{};
    if constexpr ((
                      requires { typename Config::binding; } && requires { Config::binding::device_path; }) ||
                  requires { Config::device_path; })
    {
      device = runtime_device_path<Config>(path, sizeof(path), "ttyS");
    }
    else
    {
#if defined(CONFIG_USART3_SERIAL_CONSOLE)
      if constexpr (requires { Config::instance_index; })
      {
        if constexpr (Config::instance_index == 2U)
        {
          device = "/dev/ttyS0";
        }
      }
#endif
      if (device == nullptr)
      {
        device = runtime_device_path<Config>(path, sizeof(path), "ttyS");
      }
    }
    return (device != nullptr) ? open(device, O_RDWR | O_NONBLOCK) : -1;
#else
    (void)sizeof(Config);
    return -1;
#endif
  }

  template <typename Config> static status configure(const int fd) noexcept
  {
#if defined(__NUTTX__) && (defined(CONFIG_SERIAL) || defined(CONFIG_USART3_SERIAL_CONSOLE))
    if constexpr (requires {
                    Config::require_fifo;
                    Config::require_lpuart;
                  })
    {
      if (Config::require_fifo || Config::require_lpuart)
      {
        return status::unsupported;
      }
    }
    const int speed = baud_constant<Config>();
    if (speed == 0)
    {
      return status::unsupported;
    }
    struct termios settings
    {
    };
    if (tcgetattr(fd, &settings) < 0)
    {
      return status::error;
    }
    settings.c_iflag = 0;
    settings.c_oflag = 0;
    settings.c_lflag = 0;
    settings.c_cflag &= static_cast<tcflag_t>(~(CSIZE | PARENB | PARODD | CSTOPB));
    settings.c_cflag |= static_cast<tcflag_t>(CLOCAL | CREAD | CS8);
    if constexpr (requires { Config::parity; })
    {
      if (Config::parity == uart_parity::even)
      {
        settings.c_cflag |= PARENB;
      }
      else if (Config::parity == uart_parity::odd)
      {
        settings.c_cflag |= static_cast<tcflag_t>(PARENB | PARODD);
      }
    }
    if constexpr (requires { Config::stop_bits; })
    {
      if (Config::stop_bits == uart_stop_bits::two)
      {
        settings.c_cflag |= CSTOPB;
      }
      else if (Config::stop_bits == uart_stop_bits::one_and_half)
      {
        return status::unsupported;
      }
    }
    settings.c_cc[VMIN] = 0;
    settings.c_cc[VTIME] = 0;
    if ((cfsetispeed(&settings, speed) < 0) || (cfsetospeed(&settings, speed) < 0) ||
        (tcsetattr(fd, TCSANOW, &settings) < 0))
    {
      return status::error;
    }
    return status::ok;
#else
    (void)sizeof(Config);
    (void)fd;
    return status::unsupported;
#endif
  }

  template <typename Config> static status initialize() noexcept
  {
#if defined(__NUTTX__) && (defined(CONFIG_SERIAL) || defined(CONFIG_USART3_SERIAL_CONSOLE))
    const int fd = open_device<Config>();
    if (fd < 0)
    {
      return status::error;
    }
    const status configured = configure<Config>(fd);
    const int close_result = close(fd);
    return (configured == status::ok) ? ((close_result == 0) ? status::ok : status::error) : configured;
#else
    (void)sizeof(Config);
    return status::unsupported;
#endif
  }

  template <typename Config>
  static status transfer(const std::uint8_t* data, const std::size_t length, const short events,
                         const bool write_transfer) noexcept
  {
    if (length == 0U)
    {
      return status::ok;
    }
#if defined(__NUTTX__) && (defined(CONFIG_SERIAL) || defined(CONFIG_USART3_SERIAL_CONSOLE))
    if (length > static_cast<std::size_t>(std::numeric_limits<ssize_t>::max()))
    {
      return status::invalid_argument;
    }
    const int fd = open_device<Config>();
    if (fd < 0)
    {
      return status::error;
    }
    status result = configure<Config>(fd);
    if (result == status::ok)
    {
      struct pollfd descriptor
      {
      };
      descriptor.fd = fd;
      descriptor.events = static_cast<decltype(descriptor.events)>(events);
      constexpr std::uint32_t timeout_ms = []() consteval
      {
        if constexpr (requires { Config::timeout_ms; })
        {
          return static_cast<std::uint32_t>(Config::timeout_ms);
        }
        return 1000U;
      }();
      const int timeout = (timeout_ms > static_cast<std::uint32_t>(INT_MAX)) ? INT_MAX : static_cast<int>(timeout_ms);
      const int ready = poll(&descriptor, 1, timeout);
      if (ready == 0)
      {
        result = status::timeout;
      }
      else if (ready < 0)
      {
        result = (errno == ETIMEDOUT) ? status::timeout : status::io_error;
      }
      else
      {
        const ssize_t transferred =
            write_transfer ? write(fd, data, length) : read(fd, const_cast<std::uint8_t*>(data), length);
        result = (transferred == static_cast<ssize_t>(length)) ? status::ok : status::io_error;
      }
    }
    const int close_result = close(fd);
    return (result == status::ok && close_result < 0) ? status::error : result;
#else
    (void)sizeof(Config);
    (void)data;
    (void)events;
    (void)write_transfer;
    return status::unsupported;
#endif
  }

  template <typename Config> static status write_blocking(const std::span<const std::uint8_t> payload) noexcept
  {
#if defined(__NUTTX__) && (defined(CONFIG_SERIAL) || defined(CONFIG_USART3_SERIAL_CONSOLE))
    return transfer<Config>(payload.data(), payload.size(), POLLOUT, true);
#else
    (void)sizeof(Config);
    (void)payload;
    return status::unsupported;
#endif
  }

  template <typename Config> static status read_blocking(const std::span<std::uint8_t> payload) noexcept
  {
#if defined(__NUTTX__) && (defined(CONFIG_SERIAL) || defined(CONFIG_USART3_SERIAL_CONSOLE))
    return transfer<Config>(payload.data(), payload.size(), POLLIN, false);
#else
    (void)sizeof(Config);
    (void)payload;
    return status::unsupported;
#endif
  }

  template <typename Config, auto Handler> static status start_async_rx(const std::span<std::uint8_t> payload) noexcept
  {
    (void)sizeof(Config);
    (void)payload;
    (void)Handler;
    return status::unsupported;
  }

  template <typename Config, auto Handler>
  static status start_async_tx(const std::span<const std::uint8_t> payload) noexcept
  {
    (void)sizeof(Config);
    (void)payload;
    (void)Handler;
    return status::unsupported;
  }

  template <typename UartConfig, typename AutobaudConfig> static status arm_autobaud() noexcept
  {
    (void)sizeof(UartConfig);
    (void)sizeof(AutobaudConfig);
    return status::unsupported;
  }

  template <typename UartConfig> static status apply_autobaud(const std::uint32_t baud_rate) noexcept
  {
    (void)sizeof(UartConfig);
    (void)baud_rate;
    return status::unsupported;
  }
};

struct rtc_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_RTC) && defined(CONFIG_RTC_DRIVER)
    const int fd = open("/dev/rtc0", O_RDWR);
    if (fd < 0)
    {
      return false;
    }
    return close(fd) == 0;
#else
    return false;
#endif
  }

  template <typename Config>
  static bool set_time(const std::uint8_t hours, const std::uint8_t minutes, const std::uint8_t seconds) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_RTC) && defined(CONFIG_RTC_DRIVER)
    if ((hours >= 24U) || (minutes >= 60U) || (seconds >= 60U))
    {
      return false;
    }
    const int fd = open("/dev/rtc0", O_RDWR);
    if (fd < 0)
    {
      return false;
    }
    struct rtc_time time
    {
    };
    bool ok = ioctl(fd, RTC_RD_TIME, reinterpret_cast<unsigned long>(&time)) == 0;
    if (ok)
    {
      time.tm_hour = hours;
      time.tm_min = minutes;
      time.tm_sec = seconds;
      ok = ioctl(fd, RTC_SET_TIME, reinterpret_cast<unsigned long>(&time)) == 0;
    }
    const int close_result = close(fd);
    return ok && (close_result == 0);
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
#if defined(__NUTTX__) && defined(CONFIG_RTC) && defined(CONFIG_RTC_DRIVER)
    const int fd = open("/dev/rtc0", O_RDONLY);
    if (fd < 0)
    {
      return false;
    }
    struct rtc_time time
    {
    };
    const bool ok = ioctl(fd, RTC_RD_TIME, reinterpret_cast<unsigned long>(&time)) == 0 && (time.tm_hour >= 0) &&
                    (time.tm_hour < 24) && (time.tm_min >= 0) && (time.tm_min < 60) && (time.tm_sec >= 0) &&
                    (time.tm_sec < 60);
    const int close_result = close(fd);
    if (!ok || (close_result < 0))
    {
      return false;
    }
    hours = static_cast<std::uint8_t>(time.tm_hour);
    minutes = static_cast<std::uint8_t>(time.tm_min);
    seconds = static_cast<std::uint8_t>(time.tm_sec);
    return true;
#else
    (void)hours;
    (void)minutes;
    (void)seconds;
    return false;
#endif
  }

  template <typename Config, auto Handler, typename AlarmType>
  static status enable_alarm(const AlarmType& alarm_time) noexcept
  {
    (void)sizeof(Config);
    (void)Handler;
    (void)alarm_time;
    return status::unsupported;
  }
};

struct flash_api final
{
#if defined(__NUTTX__) && defined(CONFIG_MTD)
  template <typename Config> static mtd_dev_s* device() noexcept
  {
    if constexpr (
        requires { typename Config::binding; } && requires { Config::binding::template get_mtd_device<Config>(); })
    {
      return Config::binding::template get_mtd_device<Config>();
    }
    else if constexpr (requires { typename Config::binding; } && requires { Config::binding::get_mtd_device(); })
    {
      return Config::binding::get_mtd_device();
    }
    else if constexpr (requires { typename Config::binding; } && requires { Config::binding::mtd_device; })
    {
      return Config::binding::mtd_device;
    }
    else if constexpr (requires { Config::mtd_device; })
    {
      return Config::mtd_device;
    }
    else
    {
      return nullptr;
    }
  }
#endif

  static status mtd_status(const int result) noexcept
  {
    if (result >= 0)
    {
      return status::ok;
    }

    switch (-result)
    {
    case EINVAL:
      return status::invalid_argument;
    case ETIMEDOUT:
      return status::timeout;
    case EBUSY:
      return status::busy;
    case ENOTTY:
    case ENOSYS:
      return status::unsupported;
    case EIO:
      return status::io_error;
    default:
      return status::error;
    }
  }

#if defined(__NUTTX__) && defined(CONFIG_MTD)
  static status get_geometry(mtd_dev_s* const mtd, mtd_geometry_s& geometry) noexcept
  {
    if ((mtd == nullptr) || (mtd->ioctl == nullptr))
    {
      return status::unsupported;
    }
    const int rc = MTD_IOCTL(mtd, MTDIOC_GEOMETRY, reinterpret_cast<unsigned long>(&geometry));
    if (rc < 0)
    {
      return mtd_status(rc);
    }
    if ((geometry.blocksize == 0U) || (geometry.erasesize == 0U) || (geometry.neraseblocks == 0U) ||
        ((geometry.erasesize % geometry.blocksize) != 0U))
    {
      return status::io_error;
    }
    return status::ok;
  }

  template <typename Config> static std::uint64_t base_address() noexcept
  {
    if constexpr (requires { Config::base_address; })
    {
      return Config::base_address;
    }
    else if constexpr (requires { Config::flash_base_address; })
    {
      return Config::flash_base_address;
    }
    else
    {
      return 0U;
    }
  }

  template <typename Config>
  static bool range_offset(const std::uint32_t address, const std::size_t length, const mtd_geometry_s& geometry,
                           std::uint64_t& offset) noexcept
  {
    const std::uint64_t base = base_address<Config>();
    const std::uint64_t capacity = static_cast<std::uint64_t>(geometry.erasesize) * geometry.neraseblocks;
    if ((address < base) || (static_cast<std::uint64_t>(address) + length > UINT32_MAX + 1ULL))
    {
      return false;
    }
    offset = static_cast<std::uint64_t>(address) - base;
    return (offset <= capacity) && (length <= capacity - offset);
  }
#endif

  template <typename Config> static status erase(const std::uint32_t address, const std::uint32_t bytes) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_MTD)
    mtd_dev_s* const mtd = device<Config>();
    mtd_geometry_s geometry{};
    const status geometry_status = get_geometry(mtd, geometry);
    if (geometry_status != status::ok)
    {
      return geometry_status;
    }

    std::uint64_t offset{};
    if (!range_offset<Config>(address, bytes, geometry, offset) || (bytes == 0U) ||
        ((address % geometry.erasesize) != base_address<Config>() % geometry.erasesize) ||
        ((bytes % geometry.erasesize) != 0U))
    {
      return status::invalid_argument;
    }
    if constexpr (requires { Config::erase_size_bytes; })
    {
      if (Config::erase_size_bytes != geometry.erasesize)
      {
        return status::unsupported;
      }
    }
    if (mtd->erase == nullptr)
    {
      return status::unsupported;
    }
    return mtd_status(MTD_ERASE(mtd, static_cast<off_t>(offset / geometry.erasesize),
                                static_cast<size_t>(bytes / geometry.erasesize)));
#else
    (void)sizeof(Config);
    (void)address;
    (void)bytes;
    return status::unsupported;
#endif
  }

  template <typename Config>
  static status program(const std::uint32_t address, const std::span<const std::uint8_t> data) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_MTD)
    mtd_dev_s* const mtd = device<Config>();
    mtd_geometry_s geometry{};
    const status geometry_status = get_geometry(mtd, geometry);
    if (geometry_status != status::ok)
    {
      return geometry_status;
    }

    std::uint64_t offset{};
    if (data.empty() || (Config::program_alignment_bytes == 0U) ||
        !range_offset<Config>(address, data.size(), geometry, offset) || ((offset % geometry.blocksize) != 0U) ||
        ((data.size() % geometry.blocksize) != 0U) || ((offset % Config::program_alignment_bytes) != 0U) ||
        ((data.size() % Config::program_alignment_bytes) != 0U))
    {
      return status::invalid_argument;
    }
    if (mtd->bwrite == nullptr)
    {
      return status::unsupported;
    }

    const size_t blocks = data.size() / geometry.blocksize;
    const ssize_t written = MTD_BWRITE(mtd, static_cast<off_t>(offset / geometry.blocksize), blocks, data.data());
    if (written < 0)
    {
      return mtd_status(static_cast<int>(written));
    }
    return (static_cast<size_t>(written) == blocks) ? status::ok : status::io_error;
#else
    (void)sizeof(Config);
    (void)address;
    (void)data;
    return status::unsupported;
#endif
  }

  template <typename Config>
  static status read(const std::uint32_t address, const std::span<std::uint8_t> data) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_MTD)
    mtd_dev_s* const mtd = device<Config>();
    mtd_geometry_s geometry{};
    const status geometry_status = get_geometry(mtd, geometry);
    if (geometry_status != status::ok)
    {
      return geometry_status;
    }

    std::uint64_t offset{};
    if (!range_offset<Config>(address, data.size(), geometry, offset))
    {
      return status::invalid_argument;
    }
    if (data.empty())
    {
      return status::ok;
    }
    if (mtd->read != nullptr)
    {
      const ssize_t bytes_read = MTD_READ(mtd, static_cast<off_t>(offset), data.size(), data.data());
      if (bytes_read < 0)
      {
        return mtd_status(static_cast<int>(bytes_read));
      }
      return (static_cast<size_t>(bytes_read) == data.size()) ? status::ok : status::io_error;
    }
    if ((mtd->bread == nullptr) || ((offset % geometry.blocksize) != 0U) || ((data.size() % geometry.blocksize) != 0U))
    {
      return status::unsupported;
    }
    const size_t blocks = data.size() / geometry.blocksize;
    const ssize_t blocks_read = MTD_BREAD(mtd, static_cast<off_t>(offset / geometry.blocksize), blocks, data.data());
    if (blocks_read < 0)
    {
      return mtd_status(static_cast<int>(blocks_read));
    }
    return (static_cast<size_t>(blocks_read) == blocks) ? status::ok : status::io_error;
#else
    (void)sizeof(Config);
    (void)address;
    (void)data;
    return status::unsupported;
#endif
  }

  template <typename Config> static result<flash_erase_region> erase_region(const std::uint32_t address) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_MTD)
    mtd_geometry_s geometry{};
    const status geometry_status = get_geometry(device<Config>(), geometry);
    if (geometry_status != status::ok)
    {
      return geometry_status;
    }

    std::uint64_t offset{};
    if (!range_offset<Config>(address, 1U, geometry, offset))
    {
      return status::invalid_argument;
    }
    const std::uint64_t start = base_address<Config>() + (offset / geometry.erasesize) * geometry.erasesize;
    if (start > UINT32_MAX)
    {
      return status::invalid_argument;
    }
    return flash_erase_region{static_cast<std::uint32_t>(start), geometry.erasesize};
#else
    (void)sizeof(Config);
    (void)address;
    return status::unsupported;
#endif
  }
};

struct timer_api final
{
  template <typename Config> static int open_device(const int flags) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_TIMER)
    char path[32]{};
    const char* const device = runtime_device_path<Config>(path, sizeof(path), "timer");
    return (device != nullptr) ? open(device, flags) : -1;
#else
    (void)sizeof(Config);
    (void)flags;
    return -1;
#endif
  }

  template <typename Config> static std::uint32_t timeout_us(const std::uint32_t period_ticks) noexcept
  {
    if constexpr (requires { Config::clock_hz; })
    {
      if (Config::clock_hz == 0U)
      {
        return 0U;
      }
      const std::uint64_t microseconds =
          (static_cast<std::uint64_t>(period_ticks) * 1'000'000U + Config::clock_hz - 1U) / Config::clock_hz;
      return (microseconds > UINT32_MAX) ? 0U : static_cast<std::uint32_t>(microseconds);
    }
    return 0U;
  }

  template <typename Config> static std::uint32_t ticks_from_us(const std::uint32_t microseconds) noexcept
  {
    if constexpr (requires { Config::clock_hz; })
    {
      const std::uint64_t ticks = static_cast<std::uint64_t>(microseconds) * Config::clock_hz / 1'000'000U;
      return (ticks > UINT32_MAX) ? UINT32_MAX : static_cast<std::uint32_t>(ticks);
    }
    return microseconds;
  }

  template <typename Config> static bool initialize() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_TIMER)
    const int fd = open_device<Config>(O_RDWR);
    if (fd < 0)
    {
      return false;
    }
    bool ok = true;
    if constexpr (requires { Config::period; })
    {
      const std::uint32_t timeout = timeout_us<Config>(Config::period);
      ok = (timeout > 0U) && (ioctl(fd, TCIOC_SETTIMEOUT, timeout) == 0);
    }
    const int close_result = close(fd);
    return ok && (close_result == 0);
#else
    return false;
#endif
  }

  template <typename Config> static status start() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_TIMER)
    const int fd = open_device<Config>(O_RDWR);
    if (fd < 0)
    {
      return status::error;
    }
    const int rc = ioctl(fd, TCIOC_START, 0);
    const int close_result = close(fd);
    return (rc == 0) ? ((close_result == 0) ? status::ok : status::error) : status::io_error;
#else
    (void)sizeof(Config);
    return status::unsupported;
#endif
  }

  template <typename Config> static status stop() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_TIMER)
    const int fd = open_device<Config>(O_RDWR);
    if (fd < 0)
    {
      return status::error;
    }
    const int rc = ioctl(fd, TCIOC_STOP, 0);
    const int close_result = close(fd);
    return (rc == 0) ? ((close_result == 0) ? status::ok : status::error) : status::io_error;
#else
    (void)sizeof(Config);
    return status::unsupported;
#endif
  }

  template <typename Config> static status reset_counter() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_TIMER)
    const int fd = open_device<Config>(O_RDWR);
    if (fd < 0)
    {
      return status::error;
    }
    struct timer_status_s timer_status
    {
    };
    bool ok = ioctl(fd, TCIOC_GETSTATUS, reinterpret_cast<unsigned long>(&timer_status)) == 0;
    const bool was_running = (timer_status.flags & TCFLAGS_ACTIVE) != 0U;
    if (ok)
    {
      ok = ioctl(fd, TCIOC_STOP, 0) == 0;
    }
    if (ok)
    {
      ok = ioctl(fd, TCIOC_START, 0) == 0;
    }
    if (ok && !was_running)
    {
      ok = ioctl(fd, TCIOC_STOP, 0) == 0;
    }
    const int close_result = close(fd);
    return ok && (close_result == 0) ? status::ok : status::io_error;
#else
    (void)sizeof(Config);
    return status::unsupported;
#endif
  }

  template <typename Config> static status set_period(const std::uint32_t period_ticks) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_TIMER)
    const std::uint32_t timeout = timeout_us<Config>(period_ticks);
    if (timeout == 0U)
    {
      return status::invalid_argument;
    }
    const int fd = open_device<Config>(O_RDWR);
    if (fd < 0)
    {
      return status::error;
    }
    const int rc = ioctl(fd, TCIOC_SETTIMEOUT, timeout);
    const int close_result = close(fd);
    return (rc == 0) ? ((close_result == 0) ? status::ok : status::error) : status::io_error;
#else
    (void)sizeof(Config);
    (void)period_ticks;
    return status::unsupported;
#endif
  }

  template <typename Config> static std::uint32_t get_counter() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_TIMER)
    const int fd = open_device<Config>(O_RDONLY);
    if (fd < 0)
    {
      return 0U;
    }
    struct timer_status_s timer_status
    {
    };
    const bool ok = ioctl(fd, TCIOC_GETSTATUS, reinterpret_cast<unsigned long>(&timer_status)) == 0;
    close(fd);
    return ok ? ticks_from_us<Config>(timer_status.timeout - timer_status.timeleft) : 0U;
#else
    return 0U;
#endif
  }

  template <typename Config> static result<std::uint32_t> period() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_TIMER)
    const int fd = open_device<Config>(O_RDONLY);
    if (fd < 0)
    {
      return status::error;
    }
    struct timer_status_s timer_status
    {
    };
    const bool ok = ioctl(fd, TCIOC_GETSTATUS, reinterpret_cast<unsigned long>(&timer_status)) == 0;
    const int close_result = close(fd);
    if (!ok || (close_result < 0))
    {
      return status::io_error;
    }
    return ticks_from_us<Config>(timer_status.timeout);
#else
    (void)sizeof(Config);
    return status::unsupported;
#endif
  }

  template <typename Config> static std::uint32_t counter() noexcept
  {
    return get_counter<Config>();
  }
};

struct adc_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_ADC)
    // NuttX ADC via /dev/adc0
    // int fd = open("/dev/adc0", O_RDONLY);
    // adc_register("/dev/adc0", adc_dev);
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static result<std::uint16_t> read_blocking(const std::uint8_t channel) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_ADC)
    // struct adc_msg_s sample[1];
    // ssize_t nbytes = read(fd, sample, sizeof(sample));
    // return sample[0].am_data;
    (void)channel;
    return status::unsupported; // Requires fd from Config
#else
    (void)channel;
    return status::unsupported;
#endif
  }

  template <typename Config, auto Handler> static bool start_interrupt(const std::uint8_t channel) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_ADC)
    // ADC interrupts handled by driver
    (void)channel;
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
#if defined(__NUTTX__) && defined(CONFIG_ADC)
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
    (void)sizeof(Config);
    (void)channels;
    return false;
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
#if defined(__NUTTX__) && defined(CONFIG_DAC)
    char path[32]{};
    const char* const device = runtime_device_path<Config>(path, sizeof(path), "dac");
    if (device == nullptr)
    {
      return false;
    }
    const int fd = open(device, O_WRONLY);
    if (fd < 0)
    {
      return false;
    }
    return close(fd) == 0;
#else
    return false;
#endif
  }

  template <typename Config> static status write(const std::uint16_t value, const std::uint8_t channel) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_DAC)
    if constexpr (requires { Config::resolution; })
    {
      constexpr std::uint32_t bits = static_cast<std::uint32_t>(Config::resolution);
      if constexpr ((bits == 0U) || (bits > 16U))
      {
        return status::unsupported;
      }
      if (value > ((1U << bits) - 1U))
      {
        return status::invalid_argument;
      }
    }
    char path[32]{};
    const char* const device = runtime_device_path<Config>(path, sizeof(path), "dac");
    if (device == nullptr)
    {
      return status::invalid_argument;
    }
    const int fd = open(device, O_WRONLY);
    if (fd < 0)
    {
      return status::error;
    }
    struct dac_msg_s sample
    {
    };
    sample.am_channel = channel;
    sample.am_data = value;
    const ssize_t written = ::write(fd, &sample, sizeof(sample));
    const int close_result = close(fd);
    if (written < 0)
    {
      return (errno == ETIMEDOUT) ? status::timeout : status::io_error;
    }
    return (written == static_cast<ssize_t>(sizeof(sample)) && close_result == 0) ? status::ok : status::io_error;
#else
    (void)sizeof(Config);
    (void)value;
    (void)channel;
    return status::unsupported;
#endif
  }

  template <typename Config> static status write_dual(const std::uint16_t value1, const std::uint16_t value2) noexcept
  {
    (void)sizeof(Config);
    (void)value1;
    (void)value2;
    return status::unsupported;
  }

  template <typename Config, auto Handler>
  static bool start_dma(const std::span<const std::uint16_t> data, const std::uint8_t channel) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_DAC)
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

  template <typename Config> static bool write_blocking(const std::uint16_t value) noexcept
  {
    return write<Config>(value, Config::channel) == status::ok;
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
#if defined(__NUTTX__) && defined(CONFIG_CAN)
    char path[32]{};
    const char* const device = runtime_device_path<Config>(path, sizeof(path), "can");
    if (device == nullptr)
    {
      return false;
    }
    const int fd = open(device, O_RDWR);
    if (fd < 0)
    {
      return false;
    }
    return close(fd) == 0;
#else
    return false;
#endif
  }

  template <typename Config> static status transmit(const microhal::can::message& message) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_CAN)
    if ((message.dlc > sizeof(message.data)) || (!message.extended_id && (message.id > CAN_MAX_STDMSGID)))
    {
      return status::invalid_argument;
    }
#ifdef CONFIG_CAN_EXTID
    if (message.extended_id && (message.id > CAN_MAX_EXTMSGID))
    {
      return status::invalid_argument;
    }
#else
    if (message.extended_id)
    {
      return status::unsupported;
    }
#endif
    if constexpr (requires { Config::enable_can_fd; })
    {
      if (Config::enable_can_fd)
      {
        return status::unsupported;
      }
    }

    char path[32]{};
    const char* const device = runtime_device_path<Config>(path, sizeof(path), "can");
    const int fd = (device != nullptr) ? open(device, O_WRONLY) : -1;
    if (fd < 0)
    {
      return status::error;
    }
    struct can_msg_s native
    {
    };
    native.cm_hdr.ch_id = message.id;
    native.cm_hdr.ch_dlc = message.dlc;
    native.cm_hdr.ch_rtr = message.remote_frame;
#ifdef CONFIG_CAN_EXTID
    native.cm_hdr.ch_extid = message.extended_id;
#endif
    if (!message.remote_frame)
    {
      for (std::size_t i = 0U; i < message.dlc; ++i)
      {
        native.cm_data[i] = message.data[i];
      }
    }
    const std::size_t bytes = CAN_MSGLEN(message.dlc);
    const ssize_t written = ::write(fd, &native, bytes);
    const int write_errno = (written < 0) ? errno : 0;
    const int close_result = close(fd);
    if (written < 0)
    {
      return (write_errno == ETIMEDOUT) ? status::timeout : status::io_error;
    }
    return (static_cast<std::size_t>(written) == bytes && close_result == 0) ? status::ok : status::io_error;
#else
    (void)sizeof(Config);
    (void)message;
    return status::unsupported;
#endif
  }

  template <typename Config> static status receive(microhal::can::message& message) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_CAN)
    char path[32]{};
    const char* const device = runtime_device_path<Config>(path, sizeof(path), "can");
    const int fd = (device != nullptr) ? open(device, O_RDONLY) : -1;
    if (fd < 0)
    {
      return status::error;
    }
    struct can_msg_s native
    {
    };
    const ssize_t received = ::read(fd, &native, sizeof(native));
    const int read_errno = (received < 0) ? errno : 0;
    const int close_result = close(fd);
    if (received < 0)
    {
      return (read_errno == ETIMEDOUT) ? status::timeout : status::io_error;
    }
    if ((close_result < 0) || (received < static_cast<ssize_t>(CAN_MSGLEN(0))) ||
        (native.cm_hdr.ch_dlc > sizeof(message.data)) ||
        (received < static_cast<ssize_t>(CAN_MSGLEN(native.cm_hdr.ch_dlc))))
    {
      return status::io_error;
    }
    message.id = native.cm_hdr.ch_id;
    message.dlc = native.cm_hdr.ch_dlc;
    message.remote_frame = native.cm_hdr.ch_rtr;
#ifdef CONFIG_CAN_EXTID
    message.extended_id = native.cm_hdr.ch_extid;
#else
    message.extended_id = false;
#endif
    for (std::size_t i = 0U; i < message.dlc; ++i)
    {
      message.data[i] = native.cm_data[i];
    }
    return status::ok;
#else
    (void)sizeof(Config);
    (void)message;
    return status::unsupported;
#endif
  }

  template <typename Config> static status transmit(const std::uint8_t* data, std::size_t length) noexcept
  {
    (void)sizeof(Config);
    (void)data;
    (void)length;
    return status::unsupported;
  }

  template <typename Config> static status receive(std::uint8_t* data, std::size_t& length) noexcept
  {
    (void)sizeof(Config);
    (void)data;
    (void)length;
    return status::unsupported;
  }

  template <typename Config>
  static status configure_filter(const std::uint32_t id, const std::uint32_t mask, const std::uint8_t fifo) noexcept
  {
    (void)sizeof(Config);
    (void)id;
    (void)mask;
    (void)fifo;
    return status::unsupported;
  }

  template <typename Config, auto Handler> static status enable_rx_interrupt() noexcept
  {
    (void)sizeof(Config);
    (void)Handler;
    return status::unsupported;
  }
};

struct usb_api final
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

  template <typename Config> static bool connect() noexcept
  {
    if constexpr (requires { typename Config::binding; } &&
                  requires { Config::binding::template connect<Config>(); })
    {
      return microhal::detail::as_status([] { return Config::binding::template connect<Config>(); }) == status::ok;
    }
    return false;
  }

  template <typename Config> static bool disconnect() noexcept
  {
    if constexpr (requires { typename Config::binding; } &&
                  requires { Config::binding::template disconnect<Config>(); })
    {
      return microhal::detail::as_status([] { return Config::binding::template disconnect<Config>(); }) == status::ok;
    }
    return false;
  }

  template <typename Config>
  static bool endpoint_transmit(const std::uint8_t endpoint, const std::span<const std::uint8_t> data) noexcept
  {
    if constexpr (requires { typename Config::binding; } &&
                  requires { Config::binding::template endpoint_transmit<Config>(endpoint, data); })
    {
      return microhal::detail::as_status(
                 [&] { return Config::binding::template endpoint_transmit<Config>(endpoint, data); }) == status::ok;
    }
    return false;
  }

  template <typename Config>
  static std::uint32_t endpoint_receive(const std::uint8_t endpoint, const std::span<std::uint8_t> buffer) noexcept
  {
    if constexpr (
        requires { typename Config::binding; } &&
        requires { Config::binding::template endpoint_receive<Config>(endpoint, buffer); })
    {
      return Config::binding::template endpoint_receive<Config>(endpoint, buffer);
    }
    return 0U;
  }
};

struct ethernet_api final
{
#if defined(__NUTTX__) && defined(CONFIG_NET) && defined(CONFIG_NET_PKT) &&                                            \
    (defined(CONFIG_NET_ETHERNET) || defined(CONFIG_STM32H7_ETHMAC))
  template <typename Config> static bool interface_request(struct ifreq& request) noexcept
  {
    std::string_view name{"eth0"};
    char generated_name[IFNAMSIZ]{};
    if constexpr (requires { typename Config::binding; } && requires { Config::binding::interface_name; })
    {
      name = Config::binding::interface_name;
    }
    else if constexpr (requires { Config::interface_name; })
    {
      name = Config::interface_name;
    }
    else if constexpr (requires { Config::instance_index; })
    {
      const int length =
          std::snprintf(generated_name, sizeof(generated_name), "eth%u", static_cast<unsigned>(Config::instance_index));
      if ((length < 0) || (static_cast<std::size_t>(length) >= sizeof(generated_name)))
      {
        return false;
      }
      name = generated_name;
    }
    if (name.empty() || (name.size() >= sizeof(request.ifr_name)))
    {
      return false;
    }
    for (std::size_t i = 0U; i < name.size(); ++i)
    {
      request.ifr_name[i] = name[i];
    }
    request.ifr_name[name.size()] = '\0';
    return true;
  }
#endif

  template <typename Config> static int packet_socket(const bool non_blocking = false) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_NET) && defined(CONFIG_NET_PKT) &&                                            \
    (defined(CONFIG_NET_ETHERNET) || defined(CONFIG_STM32H7_ETHMAC))
    const int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (fd < 0)
    {
      return -1;
    }

    struct ifreq request
    {
    };
    if (!interface_request<Config>(request) || (ioctl(fd, SIOCGIFINDEX, reinterpret_cast<unsigned long>(&request)) < 0))
    {
      close(fd);
      return -1;
    }

    struct sockaddr_ll address
    {
    };
    address.sll_family = AF_PACKET;
    address.sll_protocol = htons(ETH_P_ALL);
    address.sll_ifindex = request.ifr_ifindex;
    if ((address.sll_ifindex <= 0) ||
        (bind(fd, reinterpret_cast<const struct sockaddr*>(&address), sizeof(address)) < 0))
    {
      close(fd);
      return -1;
    }

    if (non_blocking)
    {
      const int flags = fcntl(fd, F_GETFL, 0);
      if ((flags < 0) || (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0))
      {
        close(fd);
        return -1;
      }
    }
    return fd;
#else
    (void)sizeof(Config);
    (void)non_blocking;
    return -1;
#endif
  }

  template <typename Config> static bool initialize() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_NET) && defined(CONFIG_NET_PKT) &&                                            \
    (defined(CONFIG_NET_ETHERNET) || defined(CONFIG_STM32H7_ETHMAC))
    const int fd = packet_socket<Config>();
    if (fd < 0)
    {
      return false;
    }
    return close(fd) == 0;
#else
    (void)sizeof(Config);
    return false;
#endif
  }

  template <typename Config> static bool set_interface_up(const bool enabled) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_NET) && defined(CONFIG_NET_PKT) &&                                            \
    (defined(CONFIG_NET_ETHERNET) || defined(CONFIG_STM32H7_ETHMAC))
    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
      return false;
    }

    struct ifreq request
    {
    };
    bool ok =
        interface_request<Config>(request) && (ioctl(fd, SIOCGIFFLAGS, reinterpret_cast<unsigned long>(&request)) == 0);
    if (ok)
    {
      if (enabled)
      {
        request.ifr_flags |= IFF_UP;
      }
      else
      {
        request.ifr_flags &= ~static_cast<std::uint32_t>(IFF_UP);
      }
      ok = ioctl(fd, SIOCSIFFLAGS, reinterpret_cast<unsigned long>(&request)) == 0;
    }
    const int close_result = close(fd);
    return ok && (close_result == 0);
#else
    (void)sizeof(Config);
    (void)enabled;
    return false;
#endif
  }

  template <typename Config> static bool start() noexcept
  {
    return set_interface_up<Config>(true);
  }

  template <typename Config> static bool stop() noexcept
  {
    return set_interface_up<Config>(false);
  }

  template <typename Config> static bool transmit_frame(const std::span<const std::uint8_t> frame) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_NET) && defined(CONFIG_NET_PKT) &&                                            \
    (defined(CONFIG_NET_ETHERNET) || defined(CONFIG_STM32H7_ETHMAC))
    if (frame.empty() || (frame.size() > static_cast<std::size_t>(std::numeric_limits<ssize_t>::max())))
    {
      return false;
    }
    const int fd = packet_socket<Config>();
    if (fd < 0)
    {
      return false;
    }

    struct ifreq request
    {
    };
    ssize_t sent = -1;
    if (interface_request<Config>(request) && (ioctl(fd, SIOCGIFINDEX, reinterpret_cast<unsigned long>(&request)) == 0))
    {
      struct sockaddr_ll address
      {
      };
      address.sll_family = AF_PACKET;
      address.sll_protocol = htons(ETH_P_ALL);
      address.sll_ifindex = request.ifr_ifindex;
      sent = sendto(fd, frame.data(), frame.size(), 0, reinterpret_cast<const struct sockaddr*>(&address),
                    sizeof(address));
    }
    const int close_result = close(fd);
    return (sent == static_cast<ssize_t>(frame.size())) && (close_result == 0);
#else
    (void)sizeof(Config);
    (void)frame;
    return false;
#endif
  }

  template <typename Config> static std::uint32_t receive_frame(const std::span<std::uint8_t> buffer) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_NET) && defined(CONFIG_NET_PKT) &&                                            \
    (defined(CONFIG_NET_ETHERNET) || defined(CONFIG_STM32H7_ETHMAC))
    if (buffer.empty())
    {
      return 0U;
    }
    const int fd = packet_socket<Config>(true);
    if (fd < 0)
    {
      return 0U;
    }
    const ssize_t received = recv(fd, buffer.data(), buffer.size(), 0);
    close(fd);
    return (received > 0) ? static_cast<std::uint32_t>(received) : 0U;
#else
    (void)sizeof(Config);
    (void)buffer;
    return 0U;
#endif
  }

  template <typename Config> static bool set_mac_address(const std::array<std::uint8_t, 6>& address) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_NET) && defined(CONFIG_NET_PKT) &&                                            \
    (defined(CONFIG_NET_ETHERNET) || defined(CONFIG_STM32H7_ETHMAC))
    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
      return false;
    }
    struct ifreq request
    {
    };
    bool ok = interface_request<Config>(request);
    if (ok)
    {
      request.ifr_hwaddr.sa_family = ARPHRD_ETHER;
      for (std::size_t i = 0U; i < address.size(); ++i)
      {
        request.ifr_hwaddr.sa_data[i] = static_cast<char>(address[i]);
      }
      ok = ioctl(fd, SIOCSIFHWADDR, reinterpret_cast<unsigned long>(&request)) == 0;
    }
    const int close_result = close(fd);
    return ok && (close_result == 0);
#else
    (void)sizeof(Config);
    (void)address;
    return false;
#endif
  }
};

struct sd_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_MMCSD)
    const int fd = open(sd_device_path<Config>(), O_RDWR);
    if (fd < 0)
    {
      return false;
    }
    close(fd);
    return true;
#else
    return false;
#endif
  }

  template <typename Config>
  static bool read_blocks(const std::uint32_t start_block, const std::span<std::uint8_t> buffer) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_MMCSD)
    const int fd = open(sd_device_path<Config>(), O_RDONLY);
    if (fd < 0)
    {
      return false;
    }

    constexpr off_t block_size = 512;
    if (lseek(fd, static_cast<off_t>(start_block) * block_size, SEEK_SET) < 0)
    {
      close(fd);
      return false;
    }

    const ssize_t n = read(fd, buffer.data(), buffer.size());
    close(fd);
    return n == static_cast<ssize_t>(buffer.size());
#else
    (void)start_block;
    (void)buffer;
    return false;
#endif
  }

  template <typename Config>
  static bool write_blocks(const std::uint32_t start_block, const std::span<const std::uint8_t> buffer) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_MMCSD)
    const int fd = open(sd_device_path<Config>(), O_WRONLY);
    if (fd < 0)
    {
      return false;
    }

    constexpr off_t block_size = 512;
    if (lseek(fd, static_cast<off_t>(start_block) * block_size, SEEK_SET) < 0)
    {
      close(fd);
      return false;
    }

    const ssize_t n = write(fd, buffer.data(), buffer.size());
    close(fd);
    return n == static_cast<ssize_t>(buffer.size());
#else
    (void)start_block;
    (void)buffer;
    return false;
#endif
  }

  template <typename Config>
  static bool erase_blocks(const std::uint32_t start_block, const std::uint32_t block_count) noexcept
  {
    (void)start_block;
    (void)block_count;
    return false;
  }

  template <typename Config> static std::uint32_t capacity_blocks() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_MMCSD)
    const int fd = open(sd_device_path<Config>(), O_RDONLY);
    if (fd < 0)
    {
      return 0U;
    }

    const off_t current = lseek(fd, 0, SEEK_CUR);
    const off_t end = lseek(fd, 0, SEEK_END);
    (void)lseek(fd, current, SEEK_SET);
    close(fd);
    if (end <= 0)
    {
      return 0U;
    }
    return static_cast<std::uint32_t>(end / 512);
#else
    return 0U;
#endif
  }
};

struct dma_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_ARCH_DMA)
    // NuttX DMA is architecture-specific
    // For STM32: stm32_dmasetup(), stm32_dmastart()
    return true;
#else
    return false;
#endif
  }

  template <typename Config, auto Handler>
  static bool start_transfer(const std::uint32_t src_addr, const std::uint32_t dst_addr,
                             const std::uint32_t length) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_ARCH_DMA)
    // DMA_HANDLE dma = stm32_dmachannel(Config::channel);
    // stm32_dmasetup(dma, ...);
    // stm32_dmastart(dma, Handler, ...);
    (void)src_addr;
    (void)dst_addr;
    (void)length;
    (void)Handler;
    return false; // Requires arch-specific DMA handle
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
#if defined(__NUTTX__) && defined(CONFIG_ARCH_DMA)
    // stm32_dmastop(dma);
    return false;
#else
    return false;
#endif
  }
};

struct pwm_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_PWM)
    char path[32]{};
    const char* const device = runtime_device_path<Config>(path, sizeof(path), "pwm");
    if (device == nullptr)
    {
      return false;
    }
    const int fd = open(device, O_RDWR);
    if (fd < 0)
    {
      return false;
    }
    return close(fd) == 0;
#else
    return false;
#endif
  }

  template <typename Config> static status set_duty_cycle(const std::uint16_t duty_permille) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_PWM)
    if (duty_permille > 1'000U)
    {
      return status::invalid_argument;
    }
    if constexpr (!requires { Config::frequency_hz; })
    {
      return status::unsupported;
    }
    else
    {
      struct pwm_info_s info
      {
      };
      info.frequency = Config::frequency_hz;
#if defined(CONFIG_PWM_MULTICHAN)
      for (std::size_t i = 0U; i < CONFIG_PWM_NCHANNELS; ++i)
      {
        info.channels[i].channel = -1;
      }
      if constexpr (!requires { Config::channel; })
      {
        return status::unsupported;
      }
      else if constexpr ((Config::channel > INT8_MAX) || (CONFIG_PWM_NCHANNELS == 0))
      {
        return status::unsupported;
      }
      else
      {
        info.channels[0].channel = static_cast<std::int8_t>(Config::channel);
        info.channels[0].duty = static_cast<ub16_t>((static_cast<std::uint32_t>(duty_permille) * 65'536U) / 1'000U);
        if constexpr (requires { Config::polarity; })
        {
          info.channels[0].cpol = (Config::polarity == pwm_polarity::active_low) ? 1U : 0U;
        }
      }
#else
      info.duty = static_cast<ub16_t>((static_cast<std::uint32_t>(duty_permille) * 65'536U) / 1'000U);
      if constexpr (requires { Config::polarity; })
      {
        info.cpol = (Config::polarity == pwm_polarity::active_low) ? 1U : 0U;
      }
#endif
      char path[32]{};
      const char* const device = runtime_device_path<Config>(path, sizeof(path), "pwm");
      if (device == nullptr)
      {
        return status::invalid_argument;
      }
      const int fd = open(device, O_RDWR);
      if (fd < 0)
      {
        return status::error;
      }
      const int rc = ioctl(fd, PWMIOC_SETCHARACTERISTICS, reinterpret_cast<unsigned long>(&info));
      const int close_result = close(fd);
      return (rc == 0) ? ((close_result == 0) ? status::ok : status::error) : status::io_error;
    }
#else
    (void)sizeof(Config);
    (void)duty_permille;
    return status::unsupported;
#endif
  }

  template <typename Config> static status start() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_PWM)
    char path[32]{};
    const char* const device = runtime_device_path<Config>(path, sizeof(path), "pwm");
    const int fd = (device != nullptr) ? open(device, O_RDWR) : -1;
    if (fd < 0)
    {
      return status::error;
    }
    const int rc = ioctl(fd, PWMIOC_START, 0);
    const int close_result = close(fd);
    return (rc == 0) ? ((close_result == 0) ? status::ok : status::error) : status::io_error;
#else
    (void)sizeof(Config);
    return status::unsupported;
#endif
  }

  template <typename Config> static status stop() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_PWM)
    char path[32]{};
    const char* const device = runtime_device_path<Config>(path, sizeof(path), "pwm");
    const int fd = (device != nullptr) ? open(device, O_RDWR) : -1;
    if (fd < 0)
    {
      return status::error;
    }
    const int rc = ioctl(fd, PWMIOC_STOP, 0);
    const int close_result = close(fd);
    return (rc == 0) ? ((close_result == 0) ? status::ok : status::error) : status::io_error;
#else
    (void)sizeof(Config);
    return status::unsupported;
#endif
  }
};

struct watchdog_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_WATCHDOG)
    char path[32]{};
    const char* const device = runtime_device_path<Config>(path, sizeof(path), "watchdog");
    if (device == nullptr)
    {
      return false;
    }
    const int fd = open(device, O_RDWR);
    if (fd < 0)
    {
      return false;
    }
    return close(fd) == 0;
#else
    return false;
#endif
  }

  template <typename Config> static status start() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_WATCHDOG)
    char path[32]{};
    const char* const device = runtime_device_path<Config>(path, sizeof(path), "watchdog");
    const int fd = (device != nullptr) ? open(device, O_RDWR) : -1;
    if (fd < 0)
    {
      return status::error;
    }
    const int rc = ioctl(fd, WDIOC_START, 0);
    const int close_result = close(fd);
    return (rc == 0) ? ((close_result == 0) ? status::ok : status::error) : status::io_error;
#else
    (void)sizeof(Config);
    return status::unsupported;
#endif
  }

  template <typename Config> static status refresh() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_WATCHDOG)
    char path[32]{};
    const char* const device = runtime_device_path<Config>(path, sizeof(path), "watchdog");
    const int fd = (device != nullptr) ? open(device, O_RDWR) : -1;
    if (fd < 0)
    {
      return status::error;
    }
    const int rc = ioctl(fd, WDIOC_KEEPALIVE, 0);
    const int close_result = close(fd);
    return (rc == 0) ? ((close_result == 0) ? status::ok : status::error) : status::io_error;
#else
    (void)sizeof(Config);
    return status::unsupported;
#endif
  }
};

struct i2s_api final
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

  template <typename Config> static bool transmit_blocking(const std::span<const std::uint16_t> samples) noexcept
  {
    if constexpr (requires { typename Config::binding; } &&
                  requires { Config::binding::template transmit_blocking<Config>(samples); })
    {
      return microhal::detail::as_status(
                 [&] { return Config::binding::template transmit_blocking<Config>(samples); }) == status::ok;
    }
    return false;
  }

  template <typename Config> static bool receive_blocking(const std::span<std::uint16_t> samples) noexcept
  {
    if constexpr (requires { typename Config::binding; } &&
                  requires { Config::binding::template receive_blocking<Config>(samples); })
    {
      return microhal::detail::as_status(
                 [&] { return Config::binding::template receive_blocking<Config>(samples); }) == status::ok;
    }
    return false;
  }

  template <typename Config, auto Handler>
  static bool start_dma_tx(const std::span<const std::uint16_t> samples) noexcept
  {
    if constexpr (requires { typename Config::binding; } &&
                  requires { Config::binding::template start_dma_tx<Config, Handler>(samples); })
    {
      return microhal::detail::as_status(
                 [&] { return Config::binding::template start_dma_tx<Config, Handler>(samples); }) == status::ok;
    }
    return false;
  }
};

struct rcc_api final
{
  template <typename Config> static bool configure() noexcept
  {
#if defined(__NUTTX__)
    // NuttX clock configuration via board_clockconfig()
    // Called during board initialization in arch/arm/src/*/xxx_clockconfig.c
    // stm32_clockconfig() for STM32, etc.
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static std::uint32_t get_system_clock_hz() noexcept
  {
#if defined(__NUTTX__)
    // Architecture-specific clock frequency getters
    // For STM32: STM32_HCLK_FREQUENCY, STM32_SYSCLK_FREQUENCY
    return 0U; // Board-specific constant
#else
    return 0U;
#endif
  }

  template <typename Config>
  static void enable_peripheral_clock(const peripheral_kind peripheral, const std::uint8_t instance) noexcept
  {
#if defined(__NUTTX__)
    // Architecture-specific peripheral clock enable
    // For STM32: stm32_modifyreg32(STM32_RCC_APB1ENR, 0, RCC_APB1ENR_TIM2EN);
    (void)peripheral;
    (void)instance;
#else
    (void)peripheral;
    (void)instance;
#endif
  }

  template <typename Config>
  static void disable_peripheral_clock(const peripheral_kind peripheral, const std::uint8_t instance) noexcept
  {
#if defined(__NUTTX__)
    (void)peripheral;
    (void)instance;
#else
    (void)peripheral;
    (void)instance;
#endif
  }
};

struct crc_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(__NUTTX__)
    // NuttX CRC via software implementation or hardware driver
    // #include <crc32.h>
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static std::uint32_t calculate(const std::span<const std::uint8_t> data) noexcept
  {
#if defined(__NUTTX__)
    // return crc32(data.data(), data.size());
    // Or crc32part(data.data(), data.size(), 0xFFFFFFFF);
    (void)data;
    return 0U; // Software CRC calculation
#else
    (void)data;
    return 0U;
#endif
  }

  template <typename Config> static void reset() noexcept
  {
#if defined(__NUTTX__)
    // Software CRC doesn't need reset
#endif
  }

  template <typename Config> static std::uint32_t get_random() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_DEV_RANDOM)
    // int fd = open("/dev/random", O_RDONLY);
    // uint32_t random;
    // read(fd, &random, sizeof(random));
    // return random;
    return 0U;
#else
    return 0U;
#endif
  }
};

} // namespace microhal::backends::nuttx

namespace microhal
{

template <> struct backend_traits<nuttx_tag> final
{
  using backend_tag = nuttx_tag;
  using gpio_api = backends::nuttx::gpio_api;
  using i2c_api = backends::nuttx::i2c_api;
  using spi_api = backends::nuttx::spi_api;
  using qspi_api = backends::nuttx::qspi_api;
  using uart_api = backends::nuttx::uart_api;
  using rtc_api = backends::nuttx::rtc_api;
  using flash_api = backends::nuttx::flash_api;
  using timer_api = backends::nuttx::timer_api;
  using adc_api = backends::nuttx::adc_api;
  using dac_api = backends::nuttx::dac_api;
  using can_api = backends::nuttx::can_api;
  using usb_api = backends::nuttx::usb_api;
  using ethernet_api = backends::nuttx::ethernet_api;
  using sd_api = backends::nuttx::sd_api;
  using dma_api = backends::nuttx::dma_api;
  using pwm_api = backends::nuttx::pwm_api;
  using watchdog_api = backends::nuttx::watchdog_api;
  using i2s_api = backends::nuttx::i2s_api;
  using rcc_api = backends::nuttx::rcc_api;
  using crc_api = backends::nuttx::crc_api;

  static constexpr vendor_kind vendor = vendor_kind::nuttx;
  static constexpr backend_kind kind = backend_kind::nuttx_driver_model;
  static constexpr std::string_view name{"nuttx-driver-model"};
  static constexpr std::uint32_t gpio_port_count{16U};
  static constexpr std::uint32_t gpio_pins_per_port{32U};
  static constexpr std::uint32_t gpio_alternate_function_count{32U};

  static consteval struct gpio_capability gpio_capability() noexcept
  {
    return {};
  }

  static consteval struct i2c_capability i2c_capability() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_I2C) && defined(CONFIG_I2C_DRIVER)
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 4U,
            .max_frequency_hz = 1'000'000U,
#if defined(I2C_M_TEN)
            .ten_bit_addressing = true,
#else
            .ten_bit_addressing = false,
#endif
#if defined(I2C_M_NOSTOP)
            .repeated_start = true,
#else
            .repeated_start = false,
#endif
            .address_probe = true};
#else
    return {};
#endif
  }

  static consteval struct spi_capability spi_capability() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_SPI) && defined(CONFIG_SPI_DRIVER) && defined(CONFIG_SPI_EXCHANGE)
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 4U,
            .max_frequency_hz = 0U,
            .transmit_only = true,
            .receive_only = true,
            .async_transfer = false};
#else
    return {};
#endif
  }

  static consteval struct qspi_capability qspi_capability() noexcept
  {
    return {};
  }

  static consteval struct uart_capability uart_capability() noexcept
  {
#if defined(__NUTTX__) && (defined(CONFIG_SERIAL) || defined(CONFIG_USART3_SERIAL_CONSOLE))
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 10U,
            .max_frequency_hz = 0U,
            .fifo = false,
            .lpuart = false,
            .autobaud = false,
            .fifo_depth = 0U,
            .parity = true,
            .two_stop_bits = true,
            .one_and_half_stop_bits = false,
            .hardware_flow_control = false,
            .max_baud_rate = 460'800U};
#else
    return {};
#endif
  }

  static consteval struct rtc_capability rtc_capability() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_RTC) && defined(CONFIG_RTC_DRIVER)
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 1U,
            .periodic = false,
            .alarm = false,
            .calendar = true,
            .subsecond = false};
#else
    return {};
#endif
  }

  static consteval struct flash_capability flash_capability() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_MTD)
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .alignment_bytes = 1U,
            .erase_bytes = 1U,
            .total_size_kb = 0U,
            .erase_region_query = true,
            .program_readback = false};
#else
    return {};
#endif
  }

  static consteval struct timer_capability timer_capability() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_TIMER)
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 1'000'000U,
            .capture_compare = false,
            .periodic = false,
            .pwm = false,
            .channel_count = 0U,
            .period_readback = true,
            .counter_reset = true};
#else
    return {};
#endif
  }

  static consteval struct adc_capability adc_capability() noexcept { return {}; }

  static consteval struct dac_capability dac_capability() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_DAC)
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .resolution_bits = 0U,
            .channel_count = 0U,
            .dual_mode = false,
            .waveform_generation = false,
            .output_buffer = false,
            .voltage_conversion = false,
            .sequence_write = true};
#else
    return {};
#endif
  }

  static consteval struct can_capability can_capability() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_CAN)
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .can_fd = false,
            .time_triggered = false,
            .mailbox_count = 0U,
            .filter_banks = 0U,
#if defined(CONFIG_CAN_EXTID)
            .extended_ids = true,
#else
            .extended_ids = false,
#endif
            .remote_frames = true,
            .rx_interrupt = false};
#else
    return {};
#endif
  }

  static consteval struct usb_capability usb_capability() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_USBDEV)
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .device = true,
            .host = false,
            .otg = false,
            .high_speed = false,
            .endpoint_count = 4U,
            .endpoint_transfer = true,
            .endpoint_stall = true,
            .max_packet_size_bytes = 64U};
#else
    return {};
#endif
  }

  static consteval struct ethernet_capability ethernet_capability() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_NET) && defined(CONFIG_NET_PKT) &&                                            \
    (defined(CONFIG_NET_ETHERNET) || defined(CONFIG_STM32H7_ETHMAC))
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = false,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .checksum_offload = false,
            .ptp_timestamp = false,
            .vlan_filter = false,
            .gigabit = false,
#if defined(CONFIG_NET_ETH_PKTSIZE)
            .mtu_bytes = CONFIG_NET_ETH_PKTSIZE};
#else
            .mtu_bytes = 0U};
#endif
#else
    return {};
#endif
  }

  static consteval struct sd_capability sd_capability() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_MMCSD)
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = false,
            .instance_count = 2U,
            .max_frequency_hz = 50'000'000U,
            .high_speed = true,
            .uhs = false,
            .bus_width_4 = true,
            .bus_width_8 = true,
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
    return {};
  }

  static consteval struct pwm_capability pwm_capability() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_PWM)
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
#if defined(CONFIG_PWM_MULTICHAN)
            .channel_count = CONFIG_PWM_NCHANNELS,
#else
            .channel_count = 1U,
#endif
            .complementary_output = false,
            .dead_time = false,
            .runtime_duty_update = true,
            .runtime_start_stop = true};
#else
    return {};
#endif
  }

  static consteval struct watchdog_capability watchdog_capability() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_WATCHDOG)
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .independent = false,
            .window = false,
            .min_timeout_ms = 0U,
            .max_timeout_ms = 0U,
            .runtime_refresh = true,
            .reset_on_timeout = true};
#else
    return {};
#endif
  }

  static consteval struct i2s_capability i2s_capability() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_AUDIO)
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
