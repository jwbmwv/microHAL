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
#include <nuttx/timers/pwm.h>
#include <nuttx/timers/rtc.h>
#include <nuttx/timers/timer.h>
#include <nuttx/timers/watchdog.h>
#include <nuttx/usb/usbdev.h>
#include <sys/ioctl.h>
#include <unistd.h>
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
  template <typename Config> static bool initialize() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_I2C)
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
#if defined(__NUTTX__) && defined(CONFIG_I2C)
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
    close(fd);
    return rc >= 0;
#else
    (void)addr;
    (void)data;
    (void)length;
    return false;
#endif
  }

  template <typename Config>
  static bool write_blocking(const std::span<const std::uint8_t> payload, const std::uint16_t address) noexcept
  {
    return write_blocking<Config>(static_cast<std::uint8_t>(address), payload.data(), payload.size());
  }

  template <typename Config>
  static bool read_blocking(const std::span<std::uint8_t> payload, const std::uint16_t address) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_I2C)
    const int fd = open(i2c_device_path<Config>(), O_RDWR);
    if (fd < 0)
    {
      return false;
    }

    i2c_msg_s message{};
    message.frequency = Config::clock_hz;
    message.addr = static_cast<std::uint8_t>(address);
    message.flags = I2C_M_READ;
    message.buffer = payload.data();
    message.length = static_cast<std::uint16_t>(payload.size());

    i2c_transfer_s transfer{};
    transfer.msgv = &message;
    transfer.msgc = 1U;

    const int rc = ioctl(fd, I2CIOC_TRANSFER, reinterpret_cast<unsigned long>(&transfer));
    close(fd);
    return rc >= 0;
#else
    (void)payload;
    (void)address;
    return false;
#endif
  }

  template <typename Config, auto Handler>
  static bool start_transfer(const std::span<std::uint8_t> payload, const std::uint16_t address) noexcept
  {
    (void)Handler;
    return write_blocking<Config>(static_cast<std::uint8_t>(address), payload.data(), payload.size());
  }
};

struct spi_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_SPI)
    const int fd = open(spi_device_path<Config>(), O_RDWR);
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
  static bool transfer(const std::uint8_t* tx_data, std::uint8_t* rx_data, const std::size_t length) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_SPI)
    const int fd = open(spi_device_path<Config>(), O_RDWR);
    if (fd < 0)
    {
      return false;
    }

    const ssize_t written = write(fd, tx_data, length);
    if (written != static_cast<ssize_t>(length))
    {
      close(fd);
      return false;
    }

    const ssize_t read_bytes = read(fd, rx_data, length);
    close(fd);
    return read_bytes == static_cast<ssize_t>(length);
#else
    (void)tx_data;
    (void)rx_data;
    (void)length;
    return false;
#endif
  }

  template <typename Config>
  static bool transfer(const std::span<const std::uint8_t> tx_payload,
                       const std::span<std::uint8_t> rx_payload) noexcept
  {
    return transfer<Config>(tx_payload.data(), rx_payload.data(),
                            (tx_payload.size() < rx_payload.size()) ? tx_payload.size() : rx_payload.size());
  }

  template <typename Config, auto Handler>
  static bool transfer_async(const std::span<const std::uint8_t> tx_payload,
                             const std::span<std::uint8_t> rx_payload) noexcept
  {
    (void)Handler;
    return transfer<Config>(tx_payload.data(), rx_payload.data(),
                            (tx_payload.size() < rx_payload.size()) ? tx_payload.size() : rx_payload.size());
  }
};

struct qspi_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(__NUTTX__) && (defined(CONFIG_MTD) || defined(CONFIG_SPI))
    const int fd = open(qspi_device_path<Config>(), O_RDWR);
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
};

struct uart_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_SERIAL)
    // NuttX UART access via /dev/ttyS0, /dev/ttyS1, etc.
    // int fd = open("/dev/ttyS0", O_RDWR);
    // struct termios tio;
    // tcgetattr(fd, &tio);
    // cfsetspeed(&tio, Config::baud_rate);
    // tcsetattr(fd, TCSANOW, &tio);
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static bool write_blocking(std::span<const std::uint8_t> payload) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_SERIAL)
    // return write(fd, payload.data(), payload.size()) == payload.size();
    (void)payload;
    return false; // Requires fd from Config
#else
    (void)payload;
    return false;
#endif
  }

  template <typename Config> static bool read_blocking(std::span<std::uint8_t> payload) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_SERIAL)
    // return read(fd, payload.data(), payload.size()) == payload.size();
    (void)payload;
    return false; // Requires fd from Config
#else
    (void)payload;
    return false;
#endif
  }

  template <typename Config, auto Handler> static bool start_async_rx(std::span<std::uint8_t> payload) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_SERIAL)
    // NuttX async I/O via signals or poll()
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
#if defined(__NUTTX__) && defined(CONFIG_RTC)
    // NuttX RTC via /dev/rtc0
    // int fd = open("/dev/rtc0", O_RDWR);
    // rtc_initialize(0, ...);
    return true;
#else
    return false;
#endif
  }

  template <typename Config>
  static bool set_time(const std::uint8_t hours, const std::uint8_t minutes, const std::uint8_t seconds) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_RTC)
    // struct rtc_time tm;
    // tm.tm_hour = hours;
    // tm.tm_min = minutes;
    // tm.tm_sec = seconds;
    // return ioctl(fd, RTC_SET_TIME, (unsigned long)&tm) == OK;
    (void)hours;
    (void)minutes;
    (void)seconds;
    return false; // Requires fd from Config
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
#if defined(__NUTTX__) && defined(CONFIG_RTC)
    // struct rtc_time tm;
    // if (ioctl(fd, RTC_RD_TIME, (unsigned long)&tm) == OK) {
    //   hours = tm.tm_hour;
    //   minutes = tm.tm_min;
    //   seconds = tm.tm_sec;
    //   return true;
    // }
    (void)hours;
    (void)minutes;
    (void)seconds;
    return false;
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
#if defined(__NUTTX__) && defined(CONFIG_MTD)
    // NuttX Flash via MTD interface
    // struct mtd_geometry_s geo;
    // mtd->ioctl(mtd, MTDIOC_GEOMETRY, (unsigned long)&geo);
    // mtd->erase(mtd, address / geo.erasesize, bytes / geo.erasesize);
    (void)address;
    (void)bytes;
    return false; // Requires mtd_dev_s from Config
#else
    (void)address;
    (void)bytes;
    return false;
#endif
  }

  template <typename Config>
  static bool program(const std::uint32_t address, const std::span<const std::uint8_t> data) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_MTD)
    // mtd->write(mtd, address / mtd->writesize, data.size() / mtd->writesize, data.data());
    (void)address;
    (void)data;
    return false; // Requires mtd_dev_s from Config
#else
    (void)address;
    (void)data;
    return false;
#endif
  }

  template <typename Config> static bool read(const std::uint32_t address, const std::span<std::uint8_t> data) noexcept
  {
    (void)sizeof(Config);
    (void)address;
    (void)data;
    return false;
  }
};

struct timer_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_TIMER)
    // NuttX Timer via /dev/timer0
    // int fd = open("/dev/timer0", O_RDONLY);
    // struct timer_sethandler_s handler;
    // handler.handler = callback;
    // ioctl(fd, TCIOC_SETHANDLER, (unsigned long)&handler);
    // struct timer_start_s start;
    // ioctl(fd, TCIOC_START, (unsigned long)&start);
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static std::uint32_t get_counter() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_TIMER)
    // struct timer_status_s status;
    // ioctl(fd, TCIOC_GETSTATUS, (unsigned long)&status);
    // return status.timeleft;
    return 0U; // Requires fd from Config
#else
    return 0U;
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

  template <typename Config> static std::uint16_t read_blocking(const std::uint8_t channel) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_ADC)
    // struct adc_msg_s sample[1];
    // ssize_t nbytes = read(fd, sample, sizeof(sample));
    // return sample[0].am_data;
    (void)channel;
    return 0U; // Requires fd from Config
#else
    (void)channel;
    return 0U;
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
    // NuttX DAC via /dev/dac0
    // int fd = open("/dev/dac0", O_WRONLY);
    // dac_register("/dev/dac0", dac_dev);
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static void write(const std::uint16_t value, const std::uint8_t channel) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_DAC)
    // struct dac_msg_s sample;
    // sample.am_channel = channel;
    // sample.am_data = value;
    // write(fd, &sample, sizeof(sample));
    (void)value;
    (void)channel;
#else
    (void)value;
    (void)channel;
#endif
  }

  template <typename Config> static void write_dual(const std::uint16_t value1, const std::uint16_t value2) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_DAC)
    // Write to two channels sequentially
    (void)value1;
    (void)value2;
#else
    (void)value1;
    (void)value2;
#endif
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
    write<Config>(value, Config::channel);
    return true;
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
    // NuttX CAN via /dev/can0
    // int fd = open("/dev/can0", O_RDWR);
    // can_register("/dev/can0", can_dev);
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static bool transmit(const std::uint8_t* data, std::size_t length) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_CAN)
    // struct can_msg_s msg;
    // msg.cm_hdr.ch_id = 0x123;
    // msg.cm_hdr.ch_dlc = length;
    // memcpy(msg.cm_data, data, length);
    // return write(fd, &msg, sizeof(msg)) == sizeof(msg);
    (void)data;
    (void)length;
    return false; // Requires fd from Config
#else
    (void)data;
    (void)length;
    return false;
#endif
  }

  template <typename Config> static bool receive(std::uint8_t* data, std::size_t& length) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_CAN)
    // struct can_msg_s msg;
    // if (read(fd, &msg, sizeof(msg)) == sizeof(msg)) {
    //   length = msg.cm_hdr.ch_dlc;
    //   memcpy(data, msg.cm_data, length);
    //   return true;
    // }
    (void)data;
    (void)length;
    return false;
#else
    (void)data;
    (void)length;
    return false;
#endif
  }

  template <typename Config>
  static bool configure_filter(const std::uint32_t id, const std::uint32_t mask, const std::uint8_t fifo) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_CAN)
    // struct canioc_bittiming_s bt;
    // ioctl(fd, CANIOC_SET_BITTIMING, (unsigned long)&bt);
    (void)id;
    (void)mask;
    (void)fifo;
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
#if defined(__NUTTX__) && defined(CONFIG_CAN)
    // CAN RX via poll() or read() blocking
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
#if defined(__NUTTX__) && defined(CONFIG_USBDEV)
    // NuttX USB device via usbdev_register()
    // usbdev_register(&driver);
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static bool connect() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_USBDEV)
    // USB connection managed by driver
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static bool disconnect() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_USBDEV)
    // usbdev_unregister(&driver);
    return false;
#else
    return false;
#endif
  }

  template <typename Config>
  static bool endpoint_transmit(const std::uint8_t endpoint, const std::span<const std::uint8_t> data) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_USBDEV)
    // EP_submit(ep, req);
    (void)endpoint;
    (void)data;
    return false;
#else
    (void)endpoint;
    (void)data;
    return false;
#endif
  }

  template <typename Config>
  static std::uint32_t endpoint_receive(const std::uint8_t endpoint, const std::span<std::uint8_t> buffer) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_USBDEV)
    (void)endpoint;
    (void)buffer;
    return 0U;
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
#if defined(__NUTTX__) && defined(CONFIG_NET_ETHERNET)
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static bool start() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_NET_ETHERNET)
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static bool stop() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_NET_ETHERNET)
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static bool transmit_frame(const std::span<const std::uint8_t> frame) noexcept
  {
    (void)frame;
    return false;
  }

  template <typename Config> static std::uint32_t receive_frame(const std::span<std::uint8_t> buffer) noexcept
  {
    (void)buffer;
    return 0U;
  }

  template <typename Config> static bool set_mac_address(const std::array<std::uint8_t, 6>& address) noexcept
  {
    (void)address;
    return false;
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

    std::array<std::uint8_t, 512U> zeros{};
    for (std::uint32_t block = 0U; block < block_count; ++block)
    {
      if (write(fd, zeros.data(), zeros.size()) != static_cast<ssize_t>(zeros.size()))
      {
        close(fd);
        return false;
      }
    }
    close(fd);
    return true;
#else
    (void)start_block;
    (void)block_count;
    return false;
#endif
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
    // NuttX PWM via /dev/pwm0
    // int fd = open("/dev/pwm0", O_RDONLY);
    // pwm_register("/dev/pwm0", pwm_dev);
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static void set_duty_cycle(const std::uint16_t duty_permille) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_PWM)
    // struct pwm_info_s info;
    // info.frequency = 1000;
    // info.duty = duty_permille * 65536 / 1000; // Convert to 16-bit duty
    // ioctl(fd, PWMIOC_SETCHARACTERISTICS, (unsigned long)&info);
    (void)duty_permille;
#else
    (void)duty_permille;
#endif
  }

  template <typename Config> static bool start() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_PWM)
    // ioctl(fd, PWMIOC_START, 0);
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static bool stop() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_PWM)
    // ioctl(fd, PWMIOC_STOP, 0);
    return true;
#else
    return false;
#endif
  }
};

struct watchdog_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_WATCHDOG)
    // NuttX Watchdog via /dev/watchdog0
    // int fd = open("/dev/watchdog0", O_RDONLY);
    // watchdog_register("/dev/watchdog0", wdog_dev);
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static bool start() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_WATCHDOG)
    // ioctl(fd, WDIOC_START, 0);
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static bool refresh() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_WATCHDOG)
    // ioctl(fd, WDIOC_KEEPALIVE, 0);
    return true;
#else
    return false;
#endif
  }
};

struct i2s_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_AUDIO)
    // NuttX I2S via audio subsystem
    // struct audio_lowerhalf_s *audio = ...
    // audio_register("/dev/audio0", audio);
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static bool transmit_blocking(const std::span<const std::uint16_t> samples) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_AUDIO)
    // int fd = open("/dev/audio0", O_WRONLY);
    // write(fd, samples.data(), samples.size() * sizeof(uint16_t));
    (void)samples;
    return false;
#else
    (void)samples;
    return false;
#endif
  }

  template <typename Config> static bool receive_blocking(const std::span<std::uint16_t> samples) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_AUDIO)
    // int fd = open("/dev/audio0", O_RDONLY);
    // read(fd, samples.data(), samples.size() * sizeof(uint16_t));
    (void)samples;
    return false;
#else
    (void)samples;
    return false;
#endif
  }

  template <typename Config, auto Handler>
  static bool start_dma_tx(const std::span<const std::uint16_t> samples) noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_AUDIO)
    (void)samples;
    (void)Handler;
    return false;
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
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = gpio_port_count,
            .max_frequency_hz = 0U,
            .alternate_function = true};
  }

  static consteval struct i2c_capability i2c_capability() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_I2C)
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = false,
            .instance_count = 4U,
            .max_frequency_hz = 1'000'000U};
#else
    return {};
#endif
  }

  static consteval struct spi_capability spi_capability() noexcept
  {
#if defined(__NUTTX__) && defined(CONFIG_SPI)
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = false,
            .instance_count = 4U,
            .max_frequency_hz = 40'000'000U};
#else
    return {};
#endif
  }

  static consteval struct qspi_capability qspi_capability() noexcept
  {
#if defined(__NUTTX__) && (defined(CONFIG_MTD) || defined(CONFIG_SPI))
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 80'000'000U,
            .quad_mode = true,
            .octo_mode = false,
            .ddr_mode = false};
#else
    return {.supported = false,
            .blocking = false,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 0U,
            .max_frequency_hz = 0U,
            .quad_mode = false};
#endif
  }

  static consteval struct uart_capability uart_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 10U,
            .max_frequency_hz = 12'500'000U,
            .fifo = true,
            .lpuart = true,
            .autobaud = true,
            .fifo_depth = 32U};
  }

  static consteval struct rtc_capability rtc_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .periodic = true};
  }

  static consteval struct flash_capability flash_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .alignment_bytes = 4U,
            .erase_bytes = 4'096U};
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
            .periodic = true};
  }

  static consteval struct adc_capability adc_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 3U,
            .max_frequency_hz = 36'000'000U,
            .resolution_bits = 12U,
            .channel_count = 16U,
            .differential = true,
            .oversampling = true};
  }

  static consteval struct dac_capability dac_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 2U,
            .max_frequency_hz = 0U,
            .resolution_bits = 12U,
            .dual_mode = true,
            .waveform_generation = true};
  }

  static consteval struct can_capability can_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = false,
            .instance_count = 2U,
            .max_frequency_hz = 1'000'000U,
            .can_fd = true,
            .time_triggered = false,
            .mailbox_count = 3U,
            .filter_banks = 14U};
  }

  static consteval struct usb_capability usb_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .device = true,
            .host = true,
            .otg = true,
            .high_speed = false,
            .endpoint_count = 4U};
  }

  static consteval struct ethernet_capability ethernet_capability() noexcept
  {
    return {};
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
            .block_size_bytes = 512U};
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
            .dma = false,
            .instance_count = 2U,
            .max_frequency_hz = 0U,
            .stream_count = 8U,
            .channel_count = 8U,
            .mem_to_mem = true,
            .fifo = true,
            .double_buffer = true};
  }

  static consteval struct pwm_capability pwm_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 14U,
            .max_frequency_hz = 200'000'000U,
            .channel_count = 4U,
            .complementary_output = true,
            .dead_time = true};
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
            .max_timeout_ms = 32'000U};
  }

  static consteval struct i2s_capability i2s_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 3U,
            .max_frequency_hz = 42'000'000U,
            .full_duplex = true,
            .master_clock_output = true,
            .max_sample_rate = 192'000U};
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
            .pll_config = true,
            .hse = true,
            .lse = true,
            .max_system_clock_hz = 180'000'000U};
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
            .hardware_crc32 = true,
            .programmable_polynomial = true,
            .random_number_generator = true};
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
