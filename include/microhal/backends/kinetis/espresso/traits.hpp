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

// NXP Kinetis/RT MCUXpresso SDK detection and optional driver headers.
// This keeps the backend extensible across families without a fixed CPU macro allowlist.
#if __has_include("fsl_common.h")
#define MICROHAL_KINETIS_SDK_PRESENT 1
#include "fsl_common.h"

#if __has_include("fsl_adc16.h")
#include "fsl_adc16.h"
#endif
#if __has_include("fsl_adc.h")
#include "fsl_adc.h"
#endif
#if __has_include("fsl_dac.h")
#include "fsl_dac.h"
#endif
#if __has_include("fsl_dspi.h")
#include "fsl_dspi.h"
#endif
#if __has_include("fsl_edma.h")
#include "fsl_edma.h"
#endif
#if __has_include("fsl_flash.h")
#define MICROHAL_KINETIS_HAS_FLASH_DRIVER 1
#include "fsl_flash.h"
#else
#define MICROHAL_KINETIS_HAS_FLASH_DRIVER 0
#endif
#if __has_include("fsl_flexcan.h")
#include "fsl_flexcan.h"
#endif
#if __has_include("fsl_flexspi.h")
#include "fsl_flexspi.h"
#endif
#if __has_include("fsl_ftm.h")
#include "fsl_ftm.h"
#endif
#if __has_include("fsl_gpt.h")
#include "fsl_gpt.h"
#endif
#if __has_include("fsl_gpio.h")
#include "fsl_gpio.h"
#endif
#if __has_include("fsl_i2c.h")
#define MICROHAL_KINETIS_HAS_I2C_DRIVER 1
#include "fsl_i2c.h"
#else
#define MICROHAL_KINETIS_HAS_I2C_DRIVER 0
#endif
#if __has_include("fsl_lpi2c.h")
#define MICROHAL_KINETIS_HAS_LPI2C_DRIVER 1
#include "fsl_lpi2c.h"
#else
#define MICROHAL_KINETIS_HAS_LPI2C_DRIVER 0
#endif
#if __has_include("fsl_iomuxc.h")
#include "fsl_iomuxc.h"
#endif
#if __has_include("fsl_lpspi.h")
#include "fsl_lpspi.h"
#endif
#if __has_include("fsl_lpuart.h")
#define MICROHAL_KINETIS_HAS_LPUART_DRIVER 1
#include "fsl_lpuart.h"
#else
#define MICROHAL_KINETIS_HAS_LPUART_DRIVER 0
#endif
#if __has_include("fsl_port.h")
#include "fsl_port.h"
#endif
#if __has_include("fsl_pit.h")
#include "fsl_pit.h"
#endif
#if __has_include("fsl_rtc.h")
#include "fsl_rtc.h"
#endif
#if __has_include("fsl_snvs_hp.h")
#include "fsl_snvs_hp.h"
#endif
#if __has_include("fsl_uart.h")
#define MICROHAL_KINETIS_HAS_UART_DRIVER 1
#include "fsl_uart.h"
#else
#define MICROHAL_KINETIS_HAS_UART_DRIVER 0
#endif
#if __has_include("fsl_wdog.h")
#include "fsl_wdog.h"
#endif

#if MICROHAL_KINETIS_HAS_LPUART_DRIVER && defined(LPUART_BASE_PTRS)
#define MICROHAL_KINETIS_HAS_LPUART_INSTANCE 1
#else
#define MICROHAL_KINETIS_HAS_LPUART_INSTANCE 0
#endif

#if MICROHAL_KINETIS_HAS_UART_DRIVER && defined(UART_BASE_PTRS)
#define MICROHAL_KINETIS_HAS_UART_INSTANCE 1
#else
#define MICROHAL_KINETIS_HAS_UART_INSTANCE 0
#endif

#else
#define MICROHAL_KINETIS_SDK_PRESENT 0
#define MICROHAL_KINETIS_HAS_LPUART_DRIVER 0
#define MICROHAL_KINETIS_HAS_UART_DRIVER 0
#define MICROHAL_KINETIS_HAS_FLASH_DRIVER 0
#define MICROHAL_KINETIS_HAS_I2C_DRIVER 0
#define MICROHAL_KINETIS_HAS_LPI2C_DRIVER 0
#define MICROHAL_KINETIS_HAS_LPUART_INSTANCE 0
#define MICROHAL_KINETIS_HAS_UART_INSTANCE 0
#if !defined(MICROHAL_SUPPRESS_MISSING_MCU_WARNINGS)
#warning "No MCUXpresso SDK headers found for Kinetis backend (missing fsl_common.h in include path)."
#endif
#endif

#if MICROHAL_KINETIS_SDK_PRESENT && MICROHAL_KINETIS_HAS_FLASH_DRIVER && defined(FTFA) &&                              \
    defined(FSL_FEATURE_FLASH_PFLASH_BLOCK_WRITE_UNIT_SIZE) && (FSL_FEATURE_FLASH_PFLASH_BLOCK_WRITE_UNIT_SIZE > 0) && \
    defined(FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE) && (FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE > 0)
#define MICROHAL_KINETIS_HAS_FLASH_GEOMETRY 1
#else
#define MICROHAL_KINETIS_HAS_FLASH_GEOMETRY 0
#endif

#include "microhal/backends/kinetis/espresso/serial_modes.hpp"
#include "microhal/core/traits.hpp"

/**
 * @def MICROHAL_KINETIS_SDK_PRESENT
 * @brief True only when an NXP MCUXpresso SDK is actually being compiled against.
 * @details The GPIO helpers below used to be guarded by `#if defined(GPIOA)`, but `GPIOA` is a
 *          generic name that ST's CMSIS headers also define. In a mixed translation unit — an
 *          STM32 vendor-validation build that still compiles every backend — that guard fired
 *          against STM32 headers and the Kinetis code failed to parse on types such as
 *          `GPIO_Type` and `PORT_Type` that only NXP defines. Keying off MCUXpresso SDK header
 *          discovery (`fsl_common.h`) keeps the guard accurate across a wider range of Kinetis
 *          and i.MX RT family part macros.
 */
#define MICROHAL_KINETIS_SDK_DETECTED MICROHAL_KINETIS_SDK_PRESENT

namespace microhal::backends::kinetis::espresso
{

#if MICROHAL_KINETIS_SDK_PRESENT && defined(GPIOA)
// Helper to get GPIO base
template <std::uint8_t PortIndex> consteval GPIO_Type* gpio_base() noexcept
{
  constexpr std::array<GPIO_Type*, 5> ports = {GPIOA, GPIOB, GPIOC, GPIOD, GPIOE};
  return ports[PortIndex];
}

// Helper to get PORT base for muxing
template <std::uint8_t PortIndex> consteval PORT_Type* port_base() noexcept
{
  constexpr std::array<PORT_Type*, 5> ports = {PORTA, PORTB, PORTC, PORTD, PORTE};
  return ports[PortIndex];
}

template <typename PinConfig> consteval std::uint8_t gpio_port_index() noexcept
{
  if constexpr (requires { typename PinConfig::binding; })
  {
    if constexpr (requires { PinConfig::binding::port_index; })
    {
      return PinConfig::binding::port_index;
    }
  }
  else if constexpr (requires { PinConfig::port_index; })
  {
    return PinConfig::port_index;
  }

  static_assert(dependent_false_v<PinConfig>,
                "microHAL: Kinetis GPIO bindings must expose binding::port_index or PinConfig::port_index.");
  return 0U;
}

template <typename PinConfig> consteval std::uint8_t gpio_pin_index() noexcept
{
  if constexpr (requires { typename PinConfig::binding; })
  {
    if constexpr (requires { PinConfig::binding::pin_index; })
    {
      return PinConfig::binding::pin_index;
    }
  }
  else if constexpr (requires { PinConfig::pin_index; })
  {
    return PinConfig::pin_index;
  }

  static_assert(dependent_false_v<PinConfig>,
                "microHAL: Kinetis GPIO bindings must expose binding::pin_index or PinConfig::pin_index.");
  return 0U;
}

template <typename PinConfig> consteval port_mux_t gpio_mux() noexcept
{
  if constexpr (requires { typename PinConfig::binding; })
  {
    if constexpr (requires { PinConfig::binding::mux; })
    {
      return static_cast<port_mux_t>(PinConfig::binding::mux);
    }
  }

  return (PinConfig::mode == gpio_mode::alternate) ? static_cast<port_mux_t>(PinConfig::alternate_function)
                                                   : kPORT_MuxAsGpio;
}
#endif

template <typename Config> consteval std::uintptr_t peripheral_base_address() noexcept
{
  if constexpr (requires { typename Config::binding; })
  {
    if constexpr (requires { Config::binding::base_address; })
    {
      return Config::binding::base_address;
    }
  }
  else if constexpr (requires { Config::base_address; })
  {
    return Config::base_address;
  }

  static_assert(dependent_false_v<Config>,
                "microHAL: Kinetis peripheral configs must expose binding::base_address or Config::base_address.");
  return 0U;
}

template <typename PeripheralType, typename Config> constexpr auto peripheral_instance() noexcept -> PeripheralType*
{
  return reinterpret_cast<PeripheralType*>(peripheral_base_address<Config>());
}

template <typename Config> consteval bool config_is_lpuart_instance() noexcept
{
#if MICROHAL_KINETIS_HAS_LPUART_INSTANCE
  constexpr LPUART_Type* instances[] = LPUART_BASE_PTRS;
  constexpr std::uintptr_t base = peripheral_base_address<Config>();
  for (const auto* instance : instances)
  {
    if (reinterpret_cast<std::uintptr_t>(instance) == base)
    {
      return true;
    }
  }
#endif
  return false;
}

template <typename Config> consteval bool config_is_uart_instance() noexcept
{
#if MICROHAL_KINETIS_HAS_UART_INSTANCE
  constexpr UART_Type* instances[] = UART_BASE_PTRS;
  constexpr std::uintptr_t base = peripheral_base_address<Config>();
  for (const auto* instance : instances)
  {
    if (reinterpret_cast<std::uintptr_t>(instance) == base)
    {
      return true;
    }
  }
#endif
  return false;
}
struct gpio_api final
{
  template <typename PinConfig> static void configure() noexcept
  {
#if MICROHAL_KINETIS_SDK_PRESENT && defined(GPIOA)
    // Configure port mux first
    PORT_SetPinMux(port_base<gpio_port_index<PinConfig>()>(), gpio_pin_index<PinConfig>(), gpio_mux<PinConfig>());

    // Configure GPIO
    gpio_pin_config_t config{};
    config.pinDirection = (PinConfig::mode == gpio_mode::output) ? kGPIO_DigitalOutput : kGPIO_DigitalInput;
    config.outputLogic = 0U;
    GPIO_PinInit(gpio_base<gpio_port_index<PinConfig>()>(), gpio_pin_index<PinConfig>(), &config);
#endif
  }

  template <typename PinConfig> static void write(const bool level) noexcept
  {
#if MICROHAL_KINETIS_SDK_PRESENT && defined(GPIOA)
    GPIO_PinWrite(gpio_base<gpio_port_index<PinConfig>()>(), gpio_pin_index<PinConfig>(), level ? 1U : 0U);
#else
    (void)level;
#endif
  }

  template <typename PinConfig> static bool read() noexcept
  {
#if MICROHAL_KINETIS_SDK_PRESENT && defined(GPIOA)
    return GPIO_PinRead(gpio_base<gpio_port_index<PinConfig>()>(), gpio_pin_index<PinConfig>()) != 0U;
#else
    return false;
#endif
  }

  template <typename PinConfig, auto Handler> static bool enable_interrupt() noexcept
  {
    (void)Handler;
#if MICROHAL_KINETIS_SDK_PRESENT && defined(GPIOA)
    (void)sizeof(PinConfig);
    return false;
#else
    return false;
#endif
  }
};

struct i2c_api final
{
#if MICROHAL_KINETIS_SDK_PRESENT
  template <typename SdkStatus> static status map_status(const SdkStatus result) noexcept
  {
    if (result == kStatus_Success)
    {
      return status::ok;
    }
    if (result == kStatus_InvalidArgument)
    {
      return status::invalid_argument;
    }
#if MICROHAL_KINETIS_HAS_I2C_DRIVER
    if ((result == kStatus_I2C_Busy))
    {
      return status::busy;
    }
    if (result == kStatus_I2C_Timeout)
    {
      return status::timeout;
    }
    if ((result == kStatus_I2C_Nak) || (result == kStatus_I2C_Addr_Nak) || (result == kStatus_I2C_ArbitrationLost))
    {
      return status::io_error;
    }
#endif
#if MICROHAL_KINETIS_HAS_LPI2C_DRIVER
    if (result == kStatus_LPI2C_Busy)
    {
      return status::busy;
    }
    if (result == kStatus_LPI2C_Timeout)
    {
      return status::timeout;
    }
    if (result == kStatus_LPI2C_PinLowTimeout)
    {
      return status::timeout;
    }
    if ((result == kStatus_LPI2C_Nak) || (result == kStatus_LPI2C_ArbitrationLost) ||
        (result == kStatus_LPI2C_FifoError) || (result == kStatus_LPI2C_BitError))
    {
      return status::io_error;
    }
#endif
    return status::error;
  }
#endif

#if MICROHAL_KINETIS_HAS_I2C_DRIVER && defined(I2C0)
  inline static i2c_master_handle_t handle_storage{};
#elif MICROHAL_KINETIS_HAS_LPI2C_DRIVER && defined(LPI2C1)
  inline static lpi2c_master_handle_t handle_storage{};
#endif

  template <typename Config> static bool initialize() noexcept
  {
#if MICROHAL_KINETIS_HAS_I2C_DRIVER && defined(I2C0)
    i2c_master_config_t config;
    I2C_MasterGetDefaultConfig(&config);
    config.baudRate_Bps = Config::clock_hz;

    I2C_Type* instance = peripheral_instance<I2C_Type, Config>();
    I2C_MasterInit(instance, &config, CLOCK_GetFreq(kCLOCK_BusClk));
    I2C_MasterTransferCreateHandle(instance, &handle_storage, nullptr, nullptr);
    return true;
#elif MICROHAL_KINETIS_HAS_LPI2C_DRIVER && defined(LPI2C1)
    lpi2c_master_config_t config;
    LPI2C_MasterGetDefaultConfig(&config);
    config.baudRate_Hz = Config::clock_hz;

    std::uint32_t source_clock_hz = 0U;
#if defined(kCLOCK_IpgClk)
    source_clock_hz = CLOCK_GetFreq(kCLOCK_IpgClk);
#elif defined(kCLOCK_BusClk)
    source_clock_hz = CLOCK_GetFreq(kCLOCK_BusClk);
#elif defined(kCLOCK_OscClk)
    source_clock_hz = CLOCK_GetFreq(kCLOCK_OscClk);
#endif
    if (source_clock_hz == 0U)
    {
      return false;
    }

    LPI2C_Type* instance = peripheral_instance<LPI2C_Type, Config>();
    LPI2C_MasterInit(instance, &config, source_clock_hz);
    LPI2C_MasterTransferCreateHandle(instance, &handle_storage, nullptr, nullptr);
    return true;
#else
    return false;
#endif
  }

  template <typename Config>
  static bool write_blocking(const std::uint8_t addr, const std::uint8_t* data, const std::size_t length) noexcept
  {
#if MICROHAL_KINETIS_HAS_I2C_DRIVER && defined(I2C0)
    i2c_master_transfer_t transfer{};
    transfer.slaveAddress = addr;
    transfer.direction = kI2C_Write;
    transfer.data = const_cast<std::uint8_t*>(data);
    transfer.dataSize = length;
    transfer.flags = kI2C_TransferDefaultFlag;

    I2C_Type* instance = peripheral_instance<I2C_Type, Config>();
    return I2C_MasterTransferBlocking(instance, &transfer) == kStatus_Success;
#elif MICROHAL_KINETIS_HAS_LPI2C_DRIVER && defined(LPI2C1)
    lpi2c_master_transfer_t transfer{};
    transfer.slaveAddress = addr;
    transfer.direction = kLPI2C_Write;
    transfer.data = const_cast<std::uint8_t*>(data);
    transfer.dataSize = length;
    transfer.flags = kLPI2C_TransferDefaultFlag;

    LPI2C_Type* instance = peripheral_instance<LPI2C_Type, Config>();
    return LPI2C_MasterTransferBlocking(instance, &transfer) == kStatus_Success;
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
#if MICROHAL_KINETIS_HAS_I2C_DRIVER && defined(I2C0)
    i2c_master_transfer_t transfer{};
    transfer.slaveAddress = static_cast<std::uint8_t>(address);
    transfer.direction = kI2C_Read;
    transfer.data = payload.data();
    transfer.dataSize = payload.size();
    transfer.flags = kI2C_TransferDefaultFlag;

    I2C_Type* instance = peripheral_instance<I2C_Type, Config>();
    return I2C_MasterTransferBlocking(instance, &transfer) == kStatus_Success;
#elif MICROHAL_KINETIS_HAS_LPI2C_DRIVER && defined(LPI2C1)
    lpi2c_master_transfer_t transfer{};
    transfer.slaveAddress = static_cast<std::uint8_t>(address);
    transfer.direction = kLPI2C_Read;
    transfer.data = payload.data();
    transfer.dataSize = payload.size();
    transfer.flags = kLPI2C_TransferDefaultFlag;

    LPI2C_Type* instance = peripheral_instance<LPI2C_Type, Config>();
    return LPI2C_MasterTransferBlocking(instance, &transfer) == kStatus_Success;
#else
    (void)payload;
    (void)address;
    return false;
#endif
  }

  template <typename Config>
  static status write_read_blocking(const std::span<const std::uint8_t> tx_payload,
                                    const std::span<std::uint8_t> rx_payload, const std::uint16_t address) noexcept
  {
    if constexpr (Config::address_mode != i2c_address_mode::seven_bit)
    {
      return status::unsupported;
    }
    if ((tx_payload.empty()) || (rx_payload.empty()) || (address > 0x7FU))
    {
      return status::invalid_argument;
    }

#if MICROHAL_KINETIS_SDK_PRESENT && MICROHAL_KINETIS_HAS_I2C_DRIVER && defined(I2C0)
    I2C_Type* instance = peripheral_instance<I2C_Type, Config>();

    i2c_master_transfer_t write{};
    write.slaveAddress = static_cast<std::uint8_t>(address);
    write.direction = kI2C_Write;
    write.data = const_cast<std::uint8_t*>(tx_payload.data());
    write.dataSize = tx_payload.size();
    write.flags = kI2C_TransferNoStopFlag;

    const status_t write_result = I2C_MasterTransferBlocking(instance, &write);
    if (write_result != kStatus_Success)
    {
      (void)I2C_MasterStop(instance);
      return map_status(write_result);
    }

    i2c_master_transfer_t read{};
    read.slaveAddress = static_cast<std::uint8_t>(address);
    read.direction = kI2C_Read;
    read.data = rx_payload.data();
    read.dataSize = rx_payload.size();
    read.flags = kI2C_TransferRepeatedStartFlag;
    return map_status(I2C_MasterTransferBlocking(instance, &read));
#elif MICROHAL_KINETIS_SDK_PRESENT && MICROHAL_KINETIS_HAS_LPI2C_DRIVER && defined(LPI2C1)
    LPI2C_Type* instance = peripheral_instance<LPI2C_Type, Config>();

    lpi2c_master_transfer_t write{};
    write.slaveAddress = address;
    write.direction = kLPI2C_Write;
    write.data = const_cast<std::uint8_t*>(tx_payload.data());
    write.dataSize = tx_payload.size();
    write.flags = kLPI2C_TransferNoStopFlag;

    const status_t write_result = LPI2C_MasterTransferBlocking(instance, &write);
    if (write_result != kStatus_Success)
    {
      (void)LPI2C_MasterStop(instance);
      return map_status(write_result);
    }

    lpi2c_master_transfer_t read{};
    read.slaveAddress = address;
    read.direction = kLPI2C_Read;
    read.data = rx_payload.data();
    read.dataSize = rx_payload.size();
    read.flags = kLPI2C_TransferRepeatedStartFlag;
    return map_status(LPI2C_MasterTransferBlocking(instance, &read));
#else
    (void)sizeof(Config);
    return status::unsupported;
#endif
  }

  template <typename Config, auto Handler>
  static bool start_transfer(const std::span<std::uint8_t> payload, const std::uint16_t address) noexcept
  {
#if MICROHAL_KINETIS_HAS_I2C_DRIVER && defined(I2C0)
    i2c_master_transfer_t transfer{};
    transfer.slaveAddress = static_cast<std::uint8_t>(address);
    transfer.direction = kI2C_Write;
    transfer.data = payload.data();
    transfer.dataSize = payload.size();
    transfer.flags = kI2C_TransferDefaultFlag;

    I2C_Type* instance = peripheral_instance<I2C_Type, Config>();
    I2C_MasterTransferCreateHandle(instance, &handle_storage, nullptr, nullptr);
    (void)Handler;
    return I2C_MasterTransferNonBlocking(instance, &handle_storage, &transfer) == kStatus_Success;
#elif MICROHAL_KINETIS_HAS_LPI2C_DRIVER && defined(LPI2C1)
    lpi2c_master_transfer_t transfer{};
    transfer.slaveAddress = static_cast<std::uint8_t>(address);
    transfer.direction = kLPI2C_Write;
    transfer.data = payload.data();
    transfer.dataSize = payload.size();
    transfer.flags = kLPI2C_TransferDefaultFlag;

    LPI2C_Type* instance = peripheral_instance<LPI2C_Type, Config>();
    LPI2C_MasterTransferCreateHandle(instance, &handle_storage, nullptr, nullptr);
    (void)Handler;
    return LPI2C_MasterTransferNonBlocking(instance, &handle_storage, &transfer) == kStatus_Success;
#else
    (void)payload;
    (void)address;
    (void)Handler;
    return false;
#endif
  }
};

struct spi_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if MICROHAL_KINETIS_SERIAL_DSPI && defined(SPI0)
    dspi_master_config_t config;
    DSPI_MasterGetDefaultConfig(&config);
    config.ctarConfig.baudRate = 1'000'000U;
    config.ctarConfig.bitsPerFrame = 8U;
    config.ctarConfig.cpol = kDSPI_ClockPolarityActiveHigh;
    config.ctarConfig.cpha = kDSPI_ClockPhaseFirstEdge;

    SPI_Type* instance = peripheral_instance<SPI_Type, Config>();
    DSPI_MasterInit(instance, &config, CLOCK_GetFreq(kCLOCK_BusClk));
    return true;
#elif MICROHAL_KINETIS_SERIAL_LPSPI && defined(LPSPI1)
    lpspi_master_config_t config;
    LPSPI_MasterGetDefaultConfig(&config);
    config.baudRate = 1'000'000U;

    LPSPI_Type* instance = peripheral_instance<LPSPI_Type, Config>();
    LPSPI_MasterInit(instance, &config, CLOCK_GetFreq(kCLOCK_Usb1PllPfd0Clk));
    return true;
#else
    return false;
#endif
  }

  template <typename Config>
  static bool transfer(const std::uint8_t* tx_data, std::uint8_t* rx_data, const std::size_t length) noexcept
  {
#if MICROHAL_KINETIS_SERIAL_DSPI && defined(SPI0)
    dspi_transfer_t transfer{};
    transfer.txData = const_cast<std::uint8_t*>(tx_data);
    transfer.rxData = rx_data;
    transfer.dataSize = length;

    SPI_Type* instance = peripheral_instance<SPI_Type, Config>();
    return DSPI_MasterTransferBlocking(instance, &transfer) == kStatus_Success;
#elif MICROHAL_KINETIS_SERIAL_LPSPI && defined(LPSPI1)
    lpspi_transfer_t transfer{};
    transfer.txData = const_cast<std::uint8_t*>(tx_data);
    transfer.rxData = rx_data;
    transfer.dataSize = length;

    LPSPI_Type* instance = peripheral_instance<LPSPI_Type, Config>();
    return LPSPI_MasterTransferBlocking(instance, &transfer) == kStatus_Success;
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
  static status transfer_async(const std::span<const std::uint8_t> tx_payload,
                               const std::span<std::uint8_t> rx_payload) noexcept
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
    if constexpr (
        requires { typename Config::binding; } && requires { Config::binding::template initialize<Config>(); })
    {
      return microhal::detail::as_status([] { return Config::binding::template initialize<Config>(); }) == status::ok;
    }
#if defined(FLEXSPI)
    // i.MX RT series uses FlexSPI for QSPI
    flexspi_config_t config;
    FLEXSPI_GetDefaultConfig(&config);
    FLEXSPI_Init(FLEXSPI, &config);
    return true;
#else
    return false; // K64/K66 don't have QSPI
#endif
  }

  template <typename Config>
  static status command_write(const qspi_command& request, const std::span<const std::uint8_t> payload) noexcept
  {
    if constexpr (
        requires { typename Config::binding; } &&
        requires { Config::binding::template command_write<Config>(request, payload); })
    {
      return microhal::detail::as_status([&]
                                         { return Config::binding::template command_write<Config>(request, payload); });
    }
    return status::unsupported;
  }

  template <typename Config>
  static status command_read(const qspi_command& request, const std::span<std::uint8_t> payload) noexcept
  {
    if constexpr (
        requires { typename Config::binding; } &&
        requires { Config::binding::template command_read<Config>(request, payload); })
    {
      return microhal::detail::as_status([&]
                                         { return Config::binding::template command_read<Config>(request, payload); });
    }
    return status::unsupported;
  }

  template <typename Config> static status enter_memory_mapped(const qspi_command& request) noexcept
  {
    if constexpr (
        requires { typename Config::binding; } &&
        requires { Config::binding::template enter_memory_mapped<Config>(request); })
    {
      return microhal::detail::as_status([&]
                                         { return Config::binding::template enter_memory_mapped<Config>(request); });
    }
    return status::unsupported;
  }

  template <typename Config> static status exit_memory_mapped() noexcept
  {
    if constexpr (
        requires { typename Config::binding; } && requires { Config::binding::template exit_memory_mapped<Config>(); })
    {
      return microhal::detail::as_status([] { return Config::binding::template exit_memory_mapped<Config>(); });
    }
    return status::unsupported;
  }
};

struct uart_api final
{
#if MICROHAL_KINETIS_HAS_LPUART_INSTANCE
  inline static lpuart_handle_t lpuart_handle_storage{};
#endif
#if MICROHAL_KINETIS_HAS_UART_INSTANCE
  inline static uart_handle_t uart_handle_storage{};
#endif

  template <typename Config> static bool initialize() noexcept
  {
#if MICROHAL_KINETIS_HAS_LPUART_INSTANCE
    if constexpr (config_is_lpuart_instance<Config>())
    {
      lpuart_config_t config;
      LPUART_GetDefaultConfig(&config);
      config.baudRate_Bps = Config::baud_rate;
      config.enableTx = true;
      config.enableRx = true;

      std::uint32_t source_clock_hz = 0U;
#if defined(kCLOCK_OscClk)
      source_clock_hz = CLOCK_GetFreq(kCLOCK_OscClk);
#elif defined(kCLOCK_IpgClk)
      source_clock_hz = CLOCK_GetFreq(kCLOCK_IpgClk);
#elif defined(kCLOCK_BusClk)
      source_clock_hz = CLOCK_GetFreq(kCLOCK_BusClk);
#elif defined(kCLOCK_CoreSysClk)
      source_clock_hz = CLOCK_GetFreq(kCLOCK_CoreSysClk);
#endif
      if (source_clock_hz == 0U)
      {
        return false;
      }

      LPUART_Type* instance = peripheral_instance<LPUART_Type, Config>();
      LPUART_Init(instance, &config, source_clock_hz);
      return true;
    }
#endif
#if MICROHAL_KINETIS_HAS_UART_INSTANCE
    if constexpr (config_is_uart_instance<Config>())
    {
      uart_config_t config;
      UART_GetDefaultConfig(&config);
      config.baudRate_Bps = Config::baud_rate;
      config.enableTx = true;
      config.enableRx = true;

      std::uint32_t source_clock_hz = 0U;
#if defined(kCLOCK_CoreSysClk)
      source_clock_hz = CLOCK_GetFreq(kCLOCK_CoreSysClk);
#elif defined(kCLOCK_BusClk)
      source_clock_hz = CLOCK_GetFreq(kCLOCK_BusClk);
#elif defined(kCLOCK_OscClk)
      source_clock_hz = CLOCK_GetFreq(kCLOCK_OscClk);
#endif
      if (source_clock_hz == 0U)
      {
        return false;
      }

      UART_Type* instance = peripheral_instance<UART_Type, Config>();
      UART_Init(instance, &config, source_clock_hz);
      return true;
    }
#endif
    return false;
  }

  template <typename Config> static bool write_blocking(std::span<const std::uint8_t> payload) noexcept
  {
#if MICROHAL_KINETIS_HAS_LPUART_INSTANCE
    if constexpr (config_is_lpuart_instance<Config>())
    {
      LPUART_Type* instance = peripheral_instance<LPUART_Type, Config>();
      return LPUART_WriteBlocking(instance, payload.data(), payload.size()) == kStatus_Success;
    }
#endif
#if MICROHAL_KINETIS_HAS_UART_INSTANCE
    if constexpr (config_is_uart_instance<Config>())
    {
      UART_Type* instance = peripheral_instance<UART_Type, Config>();
      return UART_WriteBlocking(instance, payload.data(), payload.size()) == kStatus_Success;
    }
#endif
    (void)payload;
    return false;
  }

  template <typename Config> static bool read_blocking(std::span<std::uint8_t> payload) noexcept
  {
#if MICROHAL_KINETIS_HAS_LPUART_INSTANCE
    if constexpr (config_is_lpuart_instance<Config>())
    {
      LPUART_Type* instance = peripheral_instance<LPUART_Type, Config>();
      return LPUART_ReadBlocking(instance, payload.data(), payload.size()) == kStatus_Success;
    }
#endif
#if MICROHAL_KINETIS_HAS_UART_INSTANCE
    if constexpr (config_is_uart_instance<Config>())
    {
      UART_Type* instance = peripheral_instance<UART_Type, Config>();
      return UART_ReadBlocking(instance, payload.data(), payload.size()) == kStatus_Success;
    }
#endif
    (void)payload;
    return false;
  }

  template <typename Config, auto Handler> static bool start_async_rx(std::span<std::uint8_t> payload) noexcept
  {
#if MICROHAL_KINETIS_HAS_LPUART_INSTANCE
    if constexpr (config_is_lpuart_instance<Config>())
    {
      LPUART_Type* instance = peripheral_instance<LPUART_Type, Config>();
      LPUART_TransferCreateHandle(instance, &lpuart_handle_storage, Handler, nullptr);
      lpuart_transfer_t transfer{};
      transfer.data = payload.data();
      transfer.dataSize = payload.size();
      return LPUART_TransferReceiveNonBlocking(instance, &lpuart_handle_storage, &transfer, nullptr) == kStatus_Success;
    }
#endif
#if MICROHAL_KINETIS_HAS_UART_INSTANCE
    if constexpr (config_is_uart_instance<Config>())
    {
      UART_Type* instance = peripheral_instance<UART_Type, Config>();
      UART_TransferCreateHandle(instance, &uart_handle_storage, Handler, nullptr);
      uart_transfer_t transfer{};
      transfer.data = payload.data();
      transfer.dataSize = payload.size();
      return UART_TransferReceiveNonBlocking(instance, &uart_handle_storage, &transfer, nullptr) == kStatus_Success;
    }
#endif
    (void)payload;
    (void)Handler;
    return false;
  }

  template <typename Config, auto Handler> static bool start_async_tx(std::span<const std::uint8_t> payload) noexcept
  {
#if MICROHAL_KINETIS_HAS_LPUART_INSTANCE
    if constexpr (config_is_lpuart_instance<Config>())
    {
      LPUART_Type* instance = peripheral_instance<LPUART_Type, Config>();
      LPUART_TransferCreateHandle(instance, &lpuart_handle_storage, Handler, nullptr);
      lpuart_transfer_t transfer{};
      transfer.data = const_cast<std::uint8_t*>(payload.data());
      transfer.dataSize = payload.size();
      return LPUART_TransferSendNonBlocking(instance, &lpuart_handle_storage, &transfer) == kStatus_Success;
    }
#endif
#if MICROHAL_KINETIS_HAS_UART_INSTANCE
    if constexpr (config_is_uart_instance<Config>())
    {
      UART_Type* instance = peripheral_instance<UART_Type, Config>();
      UART_TransferCreateHandle(instance, &uart_handle_storage, Handler, nullptr);
      uart_transfer_t transfer{};
      transfer.data = const_cast<std::uint8_t*>(payload.data());
      transfer.dataSize = payload.size();
      return UART_TransferSendNonBlocking(instance, &uart_handle_storage, &transfer) == kStatus_Success;
    }
#endif
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
#if defined(RTC)
    rtc_config_t config;
    RTC_GetDefaultConfig(&config);
    RTC_Init(RTC, &config);
    RTC_StartTimer(RTC);
    return true;
#elif defined(SNVS)
    // i.MX RT uses SNVS_HP for RTC
    snvs_hp_rtc_config_t config;
    SNVS_HP_RTC_GetDefaultConfig(&config);
    SNVS_HP_RTC_Init(SNVS, &config);
    return true;
#else
    return false;
#endif
  }

  template <typename Config>
  static bool set_time(const std::uint8_t hours, const std::uint8_t minutes, const std::uint8_t seconds) noexcept
  {
#if defined(RTC)
    rtc_datetime_t datetime{};
    datetime.year = 2026U;
    datetime.month = 1U;
    datetime.day = 1U;
    datetime.hour = hours;
    datetime.minute = minutes;
    datetime.second = seconds;
    return RTC_SetDatetime(RTC, &datetime) == kStatus_Success;
#elif defined(SNVS)
    snvs_hp_rtc_datetime_t datetime{};
    datetime.year = 2026U;
    datetime.month = 1U;
    datetime.day = 1U;
    datetime.hour = hours;
    datetime.minute = minutes;
    datetime.second = seconds;
    SNVS_HP_RTC_SetDatetime(SNVS, &datetime);
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
#if defined(RTC)
    rtc_datetime_t datetime{};
    RTC_GetDatetime(RTC, &datetime);
    hours = datetime.hour;
    minutes = datetime.minute;
    seconds = datetime.second;
    return true;
#elif defined(SNVS)
    snvs_hp_rtc_datetime_t datetime{};
    SNVS_HP_RTC_GetDatetime(SNVS, &datetime);
    hours = datetime.hour;
    minutes = datetime.minute;
    seconds = datetime.second;
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
#if MICROHAL_KINETIS_HAS_FLASH_GEOMETRY
  static flash_config_t flash_config{};

  static status map_status(const status_t result) noexcept
  {
    if (result == kStatus_FLASH_Success)
    {
      return status::ok;
    }
    if ((result == kStatus_FTFx_InvalidArgument) || (result == kStatus_FTFx_SizeError) ||
        (result == kStatus_FTFx_AlignmentError) || (result == kStatus_FTFx_AddressError) ||
        (result == kStatus_FTFx_EraseKeyError))
    {
      return status::invalid_argument;
    }
    if (result == kStatus_FTFx_UnknownProperty)
    {
      return status::unsupported;
    }
    return status::io_error;
  }
#endif

  template <typename Config> static status erase(const std::uint32_t address, const std::uint32_t bytes) noexcept
  {
#if MICROHAL_KINETIS_HAS_FLASH_GEOMETRY
    status_t sdk_status = FLASH_Init(&flash_config);
    if (sdk_status != kStatus_FLASH_Success)
    {
      return map_status(sdk_status);
    }
    sdk_status = FLASH_Erase(&flash_config, address, bytes, kFLASH_ApiEraseKey);
    return map_status(sdk_status);
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
#if MICROHAL_KINETIS_HAS_FLASH_GEOMETRY
    constexpr std::uint32_t program_unit = FSL_FEATURE_FLASH_PFLASH_BLOCK_WRITE_UNIT_SIZE;
    if (data.empty() || (data.size() > UINT32_MAX) || ((address % program_unit) != 0U) ||
        ((data.size() % program_unit) != 0U))
    {
      return status::invalid_argument;
    }

    status_t sdk_status = FLASH_Init(&flash_config);
    if (sdk_status != kStatus_FLASH_Success)
    {
      return map_status(sdk_status);
    }
    sdk_status = FLASH_Program(&flash_config, address, const_cast<std::uint8_t*>(data.data()),
                               static_cast<std::uint32_t>(data.size()));
    return map_status(sdk_status);
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
#if MICROHAL_KINETIS_HAS_FLASH_GEOMETRY
    const auto* source = reinterpret_cast<const std::uint8_t*>(address);
    for (std::size_t i = 0U; i < data.size(); ++i)
    {
      data[i] = source[i];
    }
    (void)sizeof(Config);
    return status::ok;
#else
    (void)sizeof(Config);
    (void)address;
    (void)data;
    return status::unsupported;
#endif
  }

  template <typename Config> static result<flash_erase_region> erase_region(const std::uint32_t address) noexcept
  {
#if MICROHAL_KINETIS_HAS_FLASH_GEOMETRY
    const status_t init_status = FLASH_Init(&flash_config);
    if (init_status != kStatus_FLASH_Success)
    {
      return map_status(init_status);
    }

    constexpr std::array<flash_property_tag_t, 2> base_properties{kFLASH_PropertyPflash0BlockBaseAddr,
                                                                  kFLASH_PropertyPflash1BlockBaseAddr};
    constexpr std::array<flash_property_tag_t, 2> size_properties{kFLASH_PropertyPflash0TotalSize,
                                                                  kFLASH_PropertyPflash1TotalSize};
    constexpr std::array<flash_property_tag_t, 2> sector_properties{kFLASH_PropertyPflash0SectorSize,
                                                                    kFLASH_PropertyPflash1SectorSize};

    for (std::size_t block = 0U; block < base_properties.size(); ++block)
    {
      std::uint32_t base_address = 0U;
      std::uint32_t total_size = 0U;
      std::uint32_t sector_size = 0U;
      const status_t base_status = FLASH_GetProperty(&flash_config, base_properties[block], &base_address);
      const status_t size_status = FLASH_GetProperty(&flash_config, size_properties[block], &total_size);
      const status_t sector_status = FLASH_GetProperty(&flash_config, sector_properties[block], &sector_size);
      if ((base_status != kStatus_FLASH_Success) || (size_status != kStatus_FLASH_Success) ||
          (sector_status != kStatus_FLASH_Success))
      {
        if (block == 0U)
        {
          return map_status((base_status != kStatus_FLASH_Success)
                                ? base_status
                                : ((size_status != kStatus_FLASH_Success) ? size_status : sector_status));
        }
        continue;
      }

      if ((total_size == 0U) || (sector_size == 0U) || (address < base_address) ||
          ((static_cast<std::uint64_t>(address) - base_address) >= total_size))
      {
        continue;
      }

      const std::uint32_t offset = address - base_address;
      const std::uint32_t region_start = base_address + (offset / sector_size) * sector_size;
      return flash_erase_region{.start_address = region_start, .size_bytes = sector_size};
    }

    return status::invalid_argument;
#else
    (void)sizeof(Config);
    (void)address;
    return status::unsupported;
#endif
  }
};

struct timer_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if MICROHAL_KINETIS_SDK_PRESENT && defined(FTM0)
    ftm_config_t config;
    FTM_GetDefaultConfig(&config);
    config.prescale = kFTM_Prescale_Divide_1;

    FTM_Type* instance = peripheral_instance<FTM_Type, Config>();
    FTM_Init(instance, &config);
    FTM_SetTimerPeriod(instance, Config::period);
    FTM_StartTimer(instance, kFTM_SystemClock);
    return true;
#elif MICROHAL_KINETIS_SDK_PRESENT && defined(GPT1)
    gpt_config_t config;
    GPT_GetDefaultConfig(&config);

    GPT_Type* instance = peripheral_instance<GPT_Type, Config>();
    GPT_Init(instance, &config);
    GPT_SetTimerPeriod(instance, Config::period);
    GPT_StartTimer(instance);
    return true;
#elif MICROHAL_KINETIS_SDK_PRESENT && defined(PIT)
    pit_config_t config;
    PIT_GetDefaultConfig(&config);
    PIT_Init(PIT, &config);

    constexpr std::uint32_t pit_channel = 0U;
    PIT_SetTimerPeriod(PIT, pit_channel, Config::period);
    PIT_StartTimer(PIT, pit_channel);
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static std::uint32_t get_counter() noexcept
  {
#if MICROHAL_KINETIS_SDK_PRESENT && defined(FTM0)
    FTM_Type* instance = peripheral_instance<FTM_Type, Config>();
    return FTM_GetCurrentTimerCount(instance);
#elif MICROHAL_KINETIS_SDK_PRESENT && defined(GPT1)
    GPT_Type* instance = peripheral_instance<GPT_Type, Config>();
    return GPT_GetCurrentTimerCount(instance);
#elif MICROHAL_KINETIS_SDK_PRESENT && defined(PIT)
    return PIT_GetCurrentTimerCount(PIT, 0U);
#else
    return 0U;
#endif
  }

  template <typename Config> static std::uint32_t counter() noexcept
  {
    return get_counter<Config>();
  }

  template <typename Config> static status reset_counter() noexcept
  {
#if MICROHAL_KINETIS_SDK_PRESENT && defined(FTM0)
    FTM_SetTimerCount(peripheral_instance<FTM_Type, Config>(), 0U);
    return status::ok;
#else
    return status::unsupported;
#endif
  }

  template <typename Config> static status start() noexcept
  {
#if MICROHAL_KINETIS_SDK_PRESENT && defined(FTM0)
    FTM_StartTimer(peripheral_instance<FTM_Type, Config>(), kFTM_SystemClock);
    return status::ok;
#elif MICROHAL_KINETIS_SDK_PRESENT && defined(GPT1)
    GPT_StartTimer(peripheral_instance<GPT_Type, Config>());
    return status::ok;
#elif MICROHAL_KINETIS_SDK_PRESENT && defined(PIT)
    PIT_StartTimer(PIT, 0U);
    return status::ok;
#else
    return status::unsupported;
#endif
  }

  template <typename Config> static status stop() noexcept
  {
#if MICROHAL_KINETIS_SDK_PRESENT && defined(FTM0)
    FTM_StopTimer(peripheral_instance<FTM_Type, Config>());
    return status::ok;
#elif MICROHAL_KINETIS_SDK_PRESENT && defined(GPT1)
    GPT_StopTimer(peripheral_instance<GPT_Type, Config>());
    return status::ok;
#elif MICROHAL_KINETIS_SDK_PRESENT && defined(PIT)
    PIT_StopTimer(PIT, 0U);
    return status::ok;
#else
    return status::unsupported;
#endif
  }

  template <typename Config> static status set_period(const std::uint32_t period) noexcept
  {
#if MICROHAL_KINETIS_SDK_PRESENT && defined(FTM0)
    FTM_SetTimerPeriod(peripheral_instance<FTM_Type, Config>(), period);
    return status::ok;
#elif MICROHAL_KINETIS_SDK_PRESENT && defined(GPT1)
    GPT_SetTimerPeriod(peripheral_instance<GPT_Type, Config>(), period);
    return status::ok;
#elif MICROHAL_KINETIS_SDK_PRESENT && defined(PIT)
    PIT_SetTimerPeriod(PIT, 0U, period);
    return status::ok;
#else
    (void)period;
    return status::unsupported;
#endif
  }
};

struct adc_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(ADC0)
    adc16_config_t config;
    ADC16_GetDefaultConfig(&config);
    if constexpr (Config::resolution == microhal::adc_resolution::bits_16)
    {
      config.resolution = kADC16_ResolutionSE16Bit;
    }
    else if constexpr (Config::resolution == microhal::adc_resolution::bits_12)
    {
      config.resolution = kADC16_ResolutionSE12Bit;
    }
    else if constexpr (Config::resolution == microhal::adc_resolution::bits_10)
    {
      config.resolution = kADC16_ResolutionSE10Bit;
    }
    else if constexpr (Config::resolution == microhal::adc_resolution::bits_8)
    {
      config.resolution = kADC16_ResolutionSE8Bit;
    }
    else
    {
      static_assert(dependent_false_v<Config>, "microHAL: requested ADC resolution is not supported by Kinetis.");
    }
    config.enableContinuousConversion = false;

    ADC_Type* instance = peripheral_instance<ADC_Type, Config>();
    ADC16_Init(instance, &config);
    ADC16_EnableHardwareTrigger(instance, false);
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static result<std::uint16_t> read_blocking(const std::uint8_t channel) noexcept
  {
#if defined(ADC0)
    adc16_channel_config_t channel_config{};
    channel_config.channelNumber = channel;
    channel_config.enableInterruptOnConversionCompleted = false;

    ADC_Type* instance = peripheral_instance<ADC_Type, Config>();
    ADC16_SetChannelConfig(instance, 0U, &channel_config);

    while (0U == (kADC16_ChannelConversionDoneFlag & ADC16_GetChannelStatusFlags(instance, 0U)))
    {
    }
    return ADC16_GetChannelConversionValue(instance, 0U);
#else
    (void)channel;
    return status::unsupported;
#endif
  }

  template <typename Config, auto Handler> static bool start_interrupt(const std::uint8_t channel) noexcept
  {
#if defined(ADC0)
    adc16_channel_config_t channel_config{};
    channel_config.channelNumber = channel;
    channel_config.enableInterruptOnConversionCompleted = true;

    ADC_Type* instance = peripheral_instance<ADC_Type, Config>();
    ADC16_SetChannelConfig(instance, 0U, &channel_config);
    return true;
#else
    (void)channel;
    (void)Handler;
    return false;
#endif
  }

  template <typename Config, auto Handler>
  static bool start_dma(const std::uint8_t channel, std::span<std::uint16_t> buffer) noexcept
  {
#if defined(ADC0)
    // ADC16 DMA would require EDMA setup
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
#if defined(DAC0)
    dac_config_t config;
    DAC_GetDefaultConfig(&config);
    config.referenceVoltageSource = kDAC_ReferenceVoltageSourceVref2;
    config.enableLowPowerMode = false;

    DAC_Type* instance = peripheral_instance<DAC_Type, Config>();
    DAC_Init(instance, &config);
    DAC_Enable(instance, true);
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static status write(const std::uint16_t value, const std::uint8_t channel) noexcept
  {
#if defined(DAC0)
    DAC_Type* instance = peripheral_instance<DAC_Type, Config>();
    // Kinetis DAC is 12-bit, channel parameter typically not used (single DAC instance)
    DAC_SetBufferValue(instance, 0U, value & 0x0FFFU);
    (void)channel;
    return status::ok;
#else
    (void)value;
    (void)channel;
    return status::unsupported;
#endif
  }

  template <typename Config> static void write_dual(const std::uint16_t value1, const std::uint16_t value2) noexcept
  {
#if defined(DAC0)
    // Kinetis typically has 1-2 separate DAC instances, not dual-channel
    DAC_SetBufferValue(DAC0, 0U, value1 & 0x0FFFU);
#if defined(DAC1)
    DAC_SetBufferValue(DAC1, 0U, value2 & 0x0FFFU);
#endif
    (void)value2;
#else
    (void)value1;
    (void)value2;
#endif
  }

  template <typename Config, auto Handler>
  static bool start_dma(const std::span<const std::uint16_t> data, const std::uint8_t channel) noexcept
  {
#if defined(DAC0)
    // DAC DMA would require EDMA setup and buffer configuration
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
#if defined(CAN0)
  static flexcan_handle_t handle_storage{};
#endif

  template <typename Config> static bool initialize() noexcept
  {
#if defined(CAN0)
    flexcan_config_t config;
    FLEXCAN_GetDefaultConfig(&config);
    config.baudRate = 500000U; // 500 kbps
    config.maxMbNum = 16U;

    CAN_Type* instance = peripheral_instance<CAN_Type, Config>();
    FLEXCAN_Init(instance, &config, CLOCK_GetFreq(kCLOCK_BusClk));
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static bool transmit(const std::uint8_t* data, std::size_t length) noexcept
  {
#if defined(CAN0)
    flexcan_frame_t frame{};
    frame.id = FLEXCAN_ID_STD(0x123U);
    frame.format = kFLEXCAN_FrameFormatStandard;
    frame.type = kFLEXCAN_FrameTypeData;
    frame.length = static_cast<std::uint8_t>(length);
    for (std::size_t i = 0U; i < length && i < 8U; ++i)
    {
      frame.dataByte[i] = data[i];
    }

    CAN_Type* instance = peripheral_instance<CAN_Type, Config>();
    flexcan_mb_transfer_t transfer{};
    transfer.mbIdx = 8U;
    transfer.frame = &frame;
    return FLEXCAN_TransferSendBlocking(instance, 8U, &frame) == kStatus_Success;
#else
    (void)data;
    (void)length;
    return false;
#endif
  }

  template <typename Config> static bool receive(std::uint8_t* data, std::size_t& length) noexcept
  {
#if defined(CAN0)
    flexcan_frame_t frame{};
    CAN_Type* instance = peripheral_instance<CAN_Type, Config>();

    if (FLEXCAN_TransferReceiveBlocking(instance, 9U, &frame) == kStatus_Success)
    {
      length = frame.length;
      for (std::size_t i = 0U; i < length && i < 8U; ++i)
      {
        data[i] = frame.dataByte[i];
      }
      return true;
    }
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
#if defined(CAN0)
    flexcan_rx_mb_config_t mb_config{};
    mb_config.format = kFLEXCAN_FrameFormatStandard;
    mb_config.type = kFLEXCAN_FrameTypeData;
    mb_config.id = FLEXCAN_ID_STD(id);

    CAN_Type* instance = peripheral_instance<CAN_Type, Config>();
    FLEXCAN_SetRxMbConfig(instance, fifo, &mb_config, true);
    return true;
#else
    (void)id;
    (void)mask;
    (void)fifo;
    return false;
#endif
  }

  template <typename Config, auto Handler> static bool enable_rx_interrupt() noexcept
  {
#if defined(CAN0)
    CAN_Type* instance = peripheral_instance<CAN_Type, Config>();
    FLEXCAN_TransferCreateHandle(instance, &handle_storage, Handler, nullptr);
    FLEXCAN_EnableMbInterrupts(instance, 1U << 9U);
    return true;
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
    if constexpr (
        requires { typename Config::binding; } && requires { Config::binding::template initialize<Config>(); })
    {
      return microhal::detail::as_status([] { return Config::binding::template initialize<Config>(); }) == status::ok;
    }
    return false;
  }

  template <typename Config> static bool connect() noexcept
  {
    if constexpr (requires { typename Config::binding; } && requires { Config::binding::template connect<Config>(); })
    {
      return microhal::detail::as_status([] { return Config::binding::template connect<Config>(); }) == status::ok;
    }
    return false;
  }

  template <typename Config> static bool disconnect() noexcept
  {
    if constexpr (
        requires { typename Config::binding; } && requires { Config::binding::template disconnect<Config>(); })
    {
      return microhal::detail::as_status([] { return Config::binding::template disconnect<Config>(); }) == status::ok;
    }
    return false;
  }

  template <typename Config>
  static bool endpoint_transmit(const std::uint8_t endpoint, const std::span<const std::uint8_t> data) noexcept
  {
    if constexpr (
        requires { typename Config::binding; } &&
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
  template <typename Config> static bool initialize() noexcept
  {
    return false;
  }

  template <typename Config> static bool start() noexcept
  {
    return false;
  }

  template <typename Config> static bool stop() noexcept
  {
    return false;
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
#if defined(SDHC) || defined(USDHC1)
    if constexpr (
        requires { typename Config::binding; } && requires { Config::binding::template initialize<Config>(); })
    {
      return Config::binding::template initialize<Config>();
    }
    return true;
#else
    if constexpr (
        requires { typename Config::binding; } && requires { Config::binding::template initialize<Config>(); })
    {
      return Config::binding::template initialize<Config>();
    }
    return false;
#endif
  }

  template <typename Config>
  static bool read_blocks(const std::uint32_t start_block, const std::span<std::uint8_t> buffer) noexcept
  {
    if constexpr (
        requires { typename Config::binding; } && requires { Config::binding::read_blocks(start_block, buffer); })
    {
      return Config::binding::read_blocks(start_block, buffer);
    }
    (void)start_block;
    (void)buffer;
    return false;
  }

  template <typename Config>
  static bool write_blocks(const std::uint32_t start_block, const std::span<const std::uint8_t> buffer) noexcept
  {
    if constexpr (
        requires { typename Config::binding; } && requires { Config::binding::write_blocks(start_block, buffer); })
    {
      return Config::binding::write_blocks(start_block, buffer);
    }
    (void)start_block;
    (void)buffer;
    return false;
  }

  template <typename Config>
  static bool erase_blocks(const std::uint32_t start_block, const std::uint32_t block_count) noexcept
  {
    if constexpr (
        requires { typename Config::binding; } && requires { Config::binding::erase_blocks(start_block, block_count); })
    {
      return Config::binding::erase_blocks(start_block, block_count);
    }
    (void)start_block;
    (void)block_count;
    return false;
  }

  template <typename Config> static std::uint32_t capacity_blocks() noexcept
  {
    if constexpr (requires { typename Config::binding; } && requires { Config::binding::capacity_blocks(); })
    {
      return Config::binding::capacity_blocks();
    }
    return 0U;
  }
};

struct dma_api final
{
#if defined(DMA0)
  template <typename Config> static edma_handle_t& handle() noexcept
  {
    static edma_handle_t handle_storage{};
    return handle_storage;
  }

  template <typename Config> static volatile dma_transfer_state& transfer_state() noexcept
  {
    static volatile dma_transfer_state state{dma_transfer_state::idle};
    return state;
  }

  template <typename Config, auto Handler>
  static void completion_callback(edma_handle_t* handle, void* user_data, const bool transfer_done,
                                  const std::uint32_t tcds) noexcept
  {
    transfer_state<Config>() = transfer_done ? dma_transfer_state::complete : dma_transfer_state::active;
    if constexpr (Handler != nullptr)
    {
      Handler(handle, user_data, transfer_done, tcds);
    }
  }
#endif

  template <typename Config> static bool initialize() noexcept
  {
#if defined(DMA0)
    edma_config_t config;
    EDMA_GetDefaultConfig(&config);
    EDMA_Init(DMA0, &config);
    transfer_state<Config>() = dma_transfer_state::idle;
    return true;
#else
    return false;
#endif
  }

  template <typename Config, auto Handler>
  static bool start_transfer(const std::uint32_t src_addr, const std::uint32_t dst_addr,
                             const std::uint32_t length) noexcept
  {
#if defined(DMA0)
    edma_transfer_config_t transfer_config;
    EDMA_PrepareTransfer(&transfer_config, reinterpret_cast<void*>(src_addr), sizeof(std::uint8_t),
                         reinterpret_cast<void*>(dst_addr), sizeof(std::uint8_t), length, length, kEDMA_MemoryToMemory);

    auto& handle_storage = handle<Config>();
    EDMA_CreateHandle(&handle_storage, DMA0, Config::channel);
    EDMA_SetCallback(&handle_storage, completion_callback<Config, Handler>, nullptr);
    transfer_state<Config>() = dma_transfer_state::active;
    if (EDMA_SubmitTransfer(&handle_storage, &transfer_config) == kStatus_Success)
    {
      return true;
    }
    transfer_state<Config>() = dma_transfer_state::error;
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
#if defined(DMA0)
    EDMA_AbortTransfer(&handle<Config>());
    transfer_state<Config>() = dma_transfer_state::idle;
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static result<dma_transfer_state> transfer_status() noexcept
  {
#if defined(DMA0)
    return transfer_state<Config>();
#else
    return status::unsupported;
#endif
  }
};

struct pwm_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(FTM0)
    ftm_config_t config;
    FTM_GetDefaultConfig(&config);
    config.prescale = kFTM_Prescale_Divide_1;

    FTM_Type* instance = peripheral_instance<FTM_Type, Config>();
    FTM_Init(instance, &config);

    ftm_chnl_pwm_signal_param_t pwm_config{};
    pwm_config.chnlNumber = kFTM_Chnl_0;
    pwm_config.level = kFTM_HighTrue;
    pwm_config.dutyCyclePercent = 0U;
    pwm_config.firstEdgeDelayPercent = 0U;
    pwm_config.enableComplementary = false;
    pwm_config.enableDeadtime = false;

    FTM_SetupPwm(instance, &pwm_config, 1U, kFTM_EdgeAlignedPwm, 1000U, CLOCK_GetFreq(kCLOCK_BusClk));
    FTM_StartTimer(instance, kFTM_SystemClock);
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static void set_duty_cycle(const std::uint16_t duty_permille) noexcept
  {
#if defined(FTM0)
    FTM_Type* instance = peripheral_instance<FTM_Type, Config>();
    const std::uint32_t mod = FTM_GetModulo(instance);
    const std::uint32_t cv = (mod * duty_permille) / 1000U;
    FTM_UpdateChnlEdgeLevelSelect(instance, kFTM_Chnl_0, 0U);
    FTM_UpdatePwmDutycycle(instance, kFTM_Chnl_0, kFTM_EdgeAlignedPwm, static_cast<std::uint8_t>(duty_permille / 10U));
    FTM_SetSoftwareTrigger(instance, true);
#else
    (void)duty_permille;
#endif
  }

  template <typename Config> static bool start() noexcept
  {
#if defined(FTM0)
    FTM_Type* instance = peripheral_instance<FTM_Type, Config>();
    FTM_StartTimer(instance, kFTM_SystemClock);
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static bool stop() noexcept
  {
#if defined(FTM0)
    FTM_Type* instance = peripheral_instance<FTM_Type, Config>();
    FTM_StopTimer(instance);
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
#if MICROHAL_KINETIS_SDK_PRESENT && defined(WDOG)
    wdog_config_t config;
    WDOG_GetDefaultConfig(&config);
    config.timeoutValue = 0xFFFFU;
    config.enableWdog = true;
    WDOG_Init(WDOG, &config);
    return true;
#elif MICROHAL_KINETIS_SDK_PRESENT && defined(WDOG1)
    wdog_config_t config;
    WDOG_GetDefaultConfig(&config);
    config.timeoutValue = 0xFFFFU;
    config.enableWdog = true;
    WDOG_Init(WDOG1, &config);
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static bool start() noexcept
  {
#if (MICROHAL_KINETIS_SDK_PRESENT && defined(WDOG)) || (MICROHAL_KINETIS_SDK_PRESENT && defined(WDOG1))
    // Watchdog starts automatically after Init
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static bool refresh() noexcept
  {
#if MICROHAL_KINETIS_SDK_PRESENT && defined(WDOG)
    WDOG_Refresh(WDOG);
    return true;
#elif MICROHAL_KINETIS_SDK_PRESENT && defined(WDOG1)
    WDOG_Refresh(WDOG1);
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
    if constexpr (
        requires { typename Config::binding; } && requires { Config::binding::template initialize<Config>(); })
    {
      return microhal::detail::as_status([] { return Config::binding::template initialize<Config>(); }) == status::ok;
    }
    // Automatic mode still requires the existing board adapter. Native SAI
    // resources and handle setup belong to the explicit-mode engine.
    return false;
  }

  template <typename Config> static bool transmit_blocking(const std::span<const std::uint16_t> samples) noexcept
  {
    if constexpr (
        requires { typename Config::binding; } &&
        requires { Config::binding::template transmit_blocking<Config>(samples); })
    {
      return microhal::detail::as_status(
                 [&] { return Config::binding::template transmit_blocking<Config>(samples); }) == status::ok;
    }
#if defined(I2S0)
    // SAI blocking transmit
    (void)samples;
    return false; // Requires full transfer setup
#else
    (void)samples;
    return false;
#endif
  }

  template <typename Config> static bool receive_blocking(const std::span<std::uint16_t> samples) noexcept
  {
    if constexpr (
        requires { typename Config::binding; } &&
        requires { Config::binding::template receive_blocking<Config>(samples); })
    {
      return microhal::detail::as_status([&] { return Config::binding::template receive_blocking<Config>(samples); }) ==
             status::ok;
    }
#if defined(I2S0)
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
    if constexpr (
        requires { typename Config::binding; } &&
        requires { Config::binding::template start_dma_tx<Config, Handler>(samples); })
    {
      return microhal::detail::as_status(
                 [&] { return Config::binding::template start_dma_tx<Config, Handler>(samples); }) == status::ok;
    }
#if defined(I2S0)
    // SAI DMA transfer
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
  template <typename Config> static status configure() noexcept
  {
    if constexpr (requires { typename Config::binding; } && requires { Config::binding::configure(); })
    {
      return microhal::detail::as_status([] { return Config::binding::configure(); });
    }
    return status::unsupported;
  }

  template <typename Config> static std::uint32_t get_system_clock_hz() noexcept
  {
#if defined(MCG) || defined(CCM)
    return CLOCK_GetFreq(kCLOCK_CoreSysClk);
#else
    return 0U;
#endif
  }

  template <typename Config>
  static status enable_peripheral_clock(const peripheral_kind peripheral, const std::uint8_t instance) noexcept
  {
    if constexpr (
        requires { typename Config::binding; } &&
        requires { Config::binding::enable_peripheral_clock(peripheral, instance); })
    {
      return microhal::detail::as_status([peripheral, instance]
                                         { return Config::binding::enable_peripheral_clock(peripheral, instance); });
    }
    return status::unsupported;
  }

  template <typename Config>
  static status disable_peripheral_clock(const peripheral_kind peripheral, const std::uint8_t instance) noexcept
  {
    if constexpr (
        requires { typename Config::binding; } &&
        requires { Config::binding::disable_peripheral_clock(peripheral, instance); })
    {
      return microhal::detail::as_status([peripheral, instance]
                                         { return Config::binding::disable_peripheral_clock(peripheral, instance); });
    }
    return status::unsupported;
  }
};

struct crc_api final
{
  template <typename Config> static bool initialize() noexcept
  {
#if defined(CRC0)
    crc_config_t config;
    CRC_GetDefaultConfig(&config);
    config.polynomial = kCRC_Polynomial_CRC_32;
    config.seed = 0xFFFFFFFFU;
    config.reflectIn = true;
    config.reflectOut = true;
    config.complementChecksum = true;

    CRC_Init(CRC0, &config);
    return true;
#else
    return false;
#endif
  }

  template <typename Config> static std::uint32_t calculate(const std::span<const std::uint8_t> data) noexcept
  {
#if defined(CRC0)
    CRC_WriteData(CRC0, data.data(), data.size());
    return CRC_Get32bitResult(CRC0);
#else
    (void)data;
    return 0U;
#endif
  }

  template <typename Config> static void reset() noexcept
  {
#if defined(CRC0)
    // CRC module reset via seed reload
    crc_config_t config;
    CRC_GetDefaultConfig(&config);
    CRC_Init(CRC0, &config);
#endif
  }

  template <typename Config> static std::uint32_t get_random() noexcept
  {
#if defined(RNG) || defined(TRNG)
    // K64/K66 has RNGA, RT1062 has TRNG
    // TRNG_GetRandomData(TRNG, &random, sizeof(random));
    return 0U; // Placeholder
#else
    return 0U;
#endif
  }
};

} // namespace microhal::backends::kinetis::espresso

namespace microhal
{

template <> struct backend_traits<kinetis_espresso_tag> final
{
  using backend_tag = kinetis_espresso_tag;
  using gpio_api = backends::kinetis::espresso::gpio_api;
  using i2c_api = backends::kinetis::espresso::i2c_api;
  using spi_api = backends::kinetis::espresso::spi_api;
  using qspi_api = backends::kinetis::espresso::qspi_api;
  using uart_api = backends::kinetis::espresso::uart_api;
  using rtc_api = backends::kinetis::espresso::rtc_api;
  using flash_api = backends::kinetis::espresso::flash_api;
  using timer_api = backends::kinetis::espresso::timer_api;
  using adc_api = backends::kinetis::espresso::adc_api;
  using dac_api = backends::kinetis::espresso::dac_api;
  using can_api = backends::kinetis::espresso::can_api;
  using usb_api = backends::kinetis::espresso::usb_api;
  using ethernet_api = backends::kinetis::espresso::ethernet_api;
  using sd_api = backends::kinetis::espresso::sd_api;
  using dma_api = backends::kinetis::espresso::dma_api;
  using pwm_api = backends::kinetis::espresso::pwm_api;
  using watchdog_api = backends::kinetis::espresso::watchdog_api;
  using i2s_api = backends::kinetis::espresso::i2s_api;
  using rcc_api = backends::kinetis::espresso::rcc_api;
  using crc_api = backends::kinetis::espresso::crc_api;

  static constexpr vendor_kind vendor = vendor_kind::kinetis;
  static constexpr backend_kind kind = backend_kind::kinetis_espresso;
  static constexpr std::string_view name{"kinetis-espresso"};
  static constexpr std::uint32_t gpio_port_count{6U};
  static constexpr std::uint32_t gpio_pins_per_port{32U};
  static constexpr std::uint32_t gpio_alternate_function_count{8U};

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
    constexpr bool native =
        MICROHAL_KINETIS_SDK_PRESENT && (MICROHAL_KINETIS_HAS_I2C_DRIVER || MICROHAL_KINETIS_HAS_LPI2C_DRIVER);
    constexpr bool dma =
        MICROHAL_KINETIS_SERIAL_I2C_EDMA || MICROHAL_KINETIS_SERIAL_LPI2C_EDMA || MICROHAL_KINETIS_SERIAL_I2C_DMA;
    return {.supported = native,
            .blocking = native,
            .non_blocking = native,
            .isr = native,
            .dma = dma,
            .instance_count = native ? 1U : 0U,
            .max_frequency_hz = native ? 1'000'000U : 0U,
            .repeated_start = native,
            .transfer_modes = {.polled = native, .interrupt = native, .dma = dma}};
  }

  static consteval struct spi_capability spi_capability() noexcept
  {
    constexpr bool native = MICROHAL_KINETIS_SERIAL_DSPI || MICROHAL_KINETIS_SERIAL_LPSPI;
    constexpr bool dma = MICROHAL_KINETIS_SERIAL_DSPI_EDMA || MICROHAL_KINETIS_SERIAL_LPSPI_EDMA;
    return {.supported = native,
            .blocking = native,
            .non_blocking = native,
            .isr = native,
            .dma = dma,
            .instance_count = native ? 1U : 0U,
            .max_frequency_hz = native ? 60'000'000U : 0U,
            .transmit_only = native,
            .receive_only = native,
            .async_transfer = native,
            .transfer_modes = {.polled = native, .interrupt = native, .dma = dma}};
  }

  static consteval struct qspi_capability qspi_capability() noexcept
  {
#if defined(FLEXSPI)
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = true,
            .instance_count = 1U,
            .max_frequency_hz = 133'000'000U,
            .quad_mode = true,
            .dual_mode = true,
            .octo_mode = false,
            .ddr_mode = false,
            .command_transfer = true,
            .memory_mapped = true,
            .max_address_bytes = 4U};
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
    constexpr bool has_uart = (MICROHAL_KINETIS_HAS_LPUART_INSTANCE != 0) || (MICROHAL_KINETIS_HAS_UART_INSTANCE != 0);
    return {.supported = has_uart,
            .blocking = has_uart,
            .non_blocking = has_uart,
            .isr = has_uart,
            .dma = MICROHAL_KINETIS_SERIAL_UART_EDMA || MICROHAL_KINETIS_SERIAL_LPUART_EDMA ||
                   MICROHAL_KINETIS_SERIAL_UART_DMA || MICROHAL_KINETIS_SERIAL_LPUART_DMA,
            .instance_count = has_uart ? 1U : 0U,
            .max_frequency_hz = 12'500'000U,
            .fifo = has_uart,
            .lpuart = (MICROHAL_KINETIS_HAS_LPUART_INSTANCE != 0),
            .autobaud = false,
            .fifo_depth = 8U,
            .transfer_modes = {.polled = has_uart,
                               .interrupt = has_uart,
                               .dma = MICROHAL_KINETIS_SERIAL_UART_EDMA || MICROHAL_KINETIS_SERIAL_LPUART_EDMA ||
                                      MICROHAL_KINETIS_SERIAL_UART_DMA || MICROHAL_KINETIS_SERIAL_LPUART_DMA}};
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
            .periodic = true,
            .alarm = true,
            .calendar = false,
            .subsecond = false};
  }

  static consteval struct flash_capability flash_capability() noexcept
  {
#if MICROHAL_KINETIS_HAS_FLASH_GEOMETRY
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .alignment_bytes = FSL_FEATURE_FLASH_PFLASH_BLOCK_WRITE_UNIT_SIZE,
            .erase_bytes = FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE,
            .erase_region_query = true,
            .program_readback = true};
#else
    return {.supported = false,
            .blocking = false,
            .non_blocking = false,
            .isr = false,
            .dma = false,
            .instance_count = 0U,
            .max_frequency_hz = 0U,
            .alignment_bytes = 0U,
            .erase_bytes = 0U,
            .erase_region_query = false,
            .program_readback = false};
#endif
  }

  static consteval struct timer_capability timer_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 8U,
            .max_frequency_hz = 120'000'000U,
            .capture_compare = true,
            .periodic = true,
            .channel_count = 8U,
#if MICROHAL_KINETIS_SDK_PRESENT && defined(FTM0)
            .counter_reset = true};
#else
            .counter_reset = false};
#endif
  }

  static consteval struct adc_capability adc_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = true,
            .instance_count = 2U,
            .max_frequency_hz = 25'000'000U,
            .resolution_bits = 16U,
            .channel_count = 24U,
            .differential = true,
            .oversampling = false,
            .sequential_scan = true,
            .voltage_conversion = true};
  }

  static consteval struct dac_capability dac_capability() noexcept
  {
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = true,
            .instance_count = 2U,
            .max_frequency_hz = 0U,
            .resolution_bits = 12U,
            .channel_count = 1U,
            .dual_mode = false,
            .waveform_generation = false,
            .output_buffer = true,
            .voltage_conversion = true,
            .sequence_write = true};
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
            .mailbox_count = 16U,
            .filter_banks = 16U,
            .extended_ids = true,
            .remote_frames = false,
            .rx_interrupt = true};
  }

  static consteval struct usb_capability usb_capability() noexcept
  {
#if MICROHAL_KINETIS_SDK_PRESENT && (defined(USB0) || defined(USBHS) || defined(USB))
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .device = true,
            .host = false,
            .otg = false,
            .high_speed = false,
            .endpoint_count = 8U,
            .endpoint_transfer = true,
            .endpoint_stall = true,
            .max_packet_size_bytes = 64U};
#else
    return {};
#endif
  }

  static consteval struct ethernet_capability ethernet_capability() noexcept
  {
    return {};
  }

  static consteval struct sd_capability sd_capability() noexcept
  {
#if MICROHAL_KINETIS_SDK_PRESENT && (defined(SDHC) || defined(USDHC1))
    return {.supported = true,
            .blocking = true,
            .non_blocking = false,
            .isr = false,
            .dma = true,
            .instance_count = 1U,
            .max_frequency_hz = 50'000'000U,
            .high_speed = true,
            .uhs = false,
            .bus_width_4 = true,
            .bus_width_8 = false,
            .block_size_bytes = 512U,
            .block_io = true,
            .erase = true,
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
            .dma = false,
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .stream_count = 0U,
            .channel_count = 32U,
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
            .instance_count = 8U,
            .max_frequency_hz = 120'000'000U,
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
            .instance_count = 1U,
            .max_frequency_hz = 0U,
            .independent = true,
            .window = false,
            .min_timeout_ms = 0U,
            .max_timeout_ms = 32'000U,
            .runtime_refresh = true,
            .reset_on_timeout = true};
  }

  static consteval struct i2s_capability i2s_capability() noexcept
  {
#if MICROHAL_KINETIS_SERIAL_SAI_ENGINE
    return {.supported = true,
            .blocking = true,
            .non_blocking = true,
            .isr = true,
            .dma = MICROHAL_KINETIS_SERIAL_SAI_EDMA,
            .instance_count = 1U,
            .max_frequency_hz = 24'000'000U,
            .full_duplex = false,
            .master_mode = true,
            .slave_mode = true,
            .master_clock_output = false,
            .max_sample_rate = 96'000U,
            .transmit_dma = MICROHAL_KINETIS_SERIAL_SAI_EDMA,
            .receive_dma = MICROHAL_KINETIS_SERIAL_SAI_EDMA,
            .sample_bits = 16U,
            .transfer_modes = {.polled = true, .interrupt = true, .dma = MICROHAL_KINETIS_SERIAL_SAI_EDMA}};
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
            .pll_config = true,
            .hse = true,
            .lse = true,
            .max_system_clock_hz = 180'000'000U,
            .clock_configuration = false,
            .peripheral_clock_gating = false,
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
            .hardware_crc32 = true,
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
