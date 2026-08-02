// SPDX-License-Identifier: Apache-2.0
/**
 * @file stm32_flash_data_logging.cpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

// STM32 Flash Data Logging Example
// Demonstrates: Flash sector erase, data programming, wear leveling, circular log buffer

#include "microhal/flash/flash.hpp"
#include "microhal/rtc/rtc.hpp"
#include <array>
#include <cstdint>
#include <span>

namespace
{

using backend = microhal::stm32_hal_tag;

// Flash memory map (example for STM32F4 - adjust for your device)
// Use last 2 sectors for logging: Sector 10 (128KB) and Sector 11 (128KB)
constexpr std::uint32_t log_sector_0_addr = 0x080C0000U; // Sector 10
constexpr std::uint32_t log_sector_1_addr = 0x080E0000U; // Sector 11
constexpr std::uint32_t log_sector_size = 128U * 1024U;  // 128 KB per sector
constexpr std::uint32_t log_total_size = 2U * log_sector_size;

// Flash Configuration: 8-byte program alignment and 128 KB erase size.
using flash_cfg = microhal::flash::flash_config<8U, log_sector_size>;

// RTC timestamps use the default compile-time feature selection.
using rtc_cfg = microhal::rtc::rtc_config<>;

// Log entry structure sized to an 8-byte flash program alignment.
struct alignas(8) log_entry
{
  std::uint32_t timestamp;            // RTC timestamp (seconds since epoch)
  std::uint16_t event_id;             // Event identifier
  std::uint16_t data_length;          // Length of event data
  std::array<std::uint8_t, 28U> data; // Event-specific data (28 bytes)
  std::uint32_t crc;                  // CRC32 checksum

  [[maybe_unused]] static constexpr std::uint32_t magic = 0xDEADBEEFU;
  static constexpr std::size_t size_bytes = 40U; // Total size: 4+2+2+28+4 = 40 bytes
};

static_assert(sizeof(log_entry) == 40U, "log_entry must be 40 bytes");
static_assert((sizeof(log_entry) % 8U) == 0U, "log_entry must be flash-aligned");

// Log manager state
struct flash_logger
{
  std::uint32_t current_write_addr = log_sector_0_addr;
  std::uint32_t entries_written = 0U;
  std::uint32_t current_sector = 0U; // 0 or 1
  bool initialized = false;
};

flash_logger logger{};

// Simple CRC32 calculation (replace with hardware CRC if available)
std::uint32_t calculate_crc32(const std::uint8_t* data, const std::size_t length) noexcept
{
  std::uint32_t crc = 0xFFFFFFFFU;

  for (std::size_t i = 0U; i < length; ++i)
  {
    crc ^= data[i];
    for (std::uint8_t bit = 0U; bit < 8U; ++bit)
    {
      if (crc & 1U)
      {
        crc = (crc >> 1U) ^ 0xEDB88320U;
      }
      else
      {
        crc >>= 1U;
      }
    }
  }

  return ~crc;
}

// Find last written entry in Flash (for initialization after reset)
bool find_last_entry() noexcept
{
  // Scan both sectors to find the last valid entry
  std::uint32_t last_valid_addr = log_sector_0_addr;
  bool found_any = false;

  for (std::uint32_t addr = log_sector_0_addr; addr < log_sector_0_addr + log_total_size; addr += log_entry::size_bytes)
  {
    const auto* entry = reinterpret_cast<const log_entry*>(addr);

    // Check if entry is valid (not erased)
    if (entry->timestamp != 0xFFFFFFFFU)
    {
      // Verify CRC
      const std::uint32_t calc_crc =
          calculate_crc32(reinterpret_cast<const std::uint8_t*>(entry), log_entry::size_bytes - sizeof(std::uint32_t));
      if (calc_crc == entry->crc)
      {
        last_valid_addr = addr;
        found_any = true;
      }
    }
    else
    {
      // Found erased region, use this address
      break;
    }
  }

  if (found_any)
  {
    logger.current_write_addr = last_valid_addr + log_entry::size_bytes;

    // Determine current sector
    if (logger.current_write_addr >= log_sector_1_addr)
    {
      logger.current_sector = 1U;
    }
  }

  return found_any;
}

// Erase sector when full
bool erase_oldest_sector() noexcept
{
  // Determine which sector to erase (implement circular buffer)
  const std::uint32_t sector_to_erase = (logger.current_sector == 0U) ? log_sector_1_addr : log_sector_0_addr;

  // Erasing is the one operation that must not be assumed to succeed: continuing would append
  // records onto un-erased flash.
  if (!microhal::is_ok(microhal::flash::controller<backend, flash_cfg>::erase(sector_to_erase, log_sector_size)))
  {
    return false;
  }

  // Switch to newly erased sector
  logger.current_sector = (logger.current_sector == 0U) ? 1U : 0U;
  logger.current_write_addr = sector_to_erase;

  return true;
}

// Write log entry to Flash
bool write_log_entry(const std::uint16_t event_id, const std::span<const std::uint8_t> data) noexcept
{
  if (!logger.initialized)
  {
    return false;
  }

  // Check if we need to erase next sector
  const std::uint32_t sector_end =
      (logger.current_sector == 0U) ? log_sector_0_addr + log_sector_size : log_sector_1_addr + log_sector_size;

  if (logger.current_write_addr + log_entry::size_bytes > sector_end)
  {
    if (!erase_oldest_sector())
    {
      return false;
    }
  }

  // Create log entry
  log_entry entry{};

  // Get RTC timestamp
  const auto now = microhal::rtc::controller<backend, rtc_cfg>::now();
  entry.timestamp = (static_cast<std::uint32_t>(now.hour) * 3600U) + (static_cast<std::uint32_t>(now.minute) * 60U) +
                    static_cast<std::uint32_t>(now.second);

  entry.event_id = event_id;
  entry.data_length = static_cast<std::uint16_t>(data.size() > entry.data.size() ? entry.data.size() : data.size());

  // Copy data
  for (std::size_t i = 0U; i < entry.data_length; ++i)
  {
    entry.data[i] = data[i];
  }

  // Calculate CRC
  entry.crc =
      calculate_crc32(reinterpret_cast<const std::uint8_t*>(&entry), log_entry::size_bytes - sizeof(std::uint32_t));

  // Program to Flash
  const std::span<const std::uint8_t> entry_bytes{reinterpret_cast<const std::uint8_t*>(&entry), log_entry::size_bytes};

  if (!microhal::is_ok(
          microhal::flash::controller<backend, flash_cfg>::program(logger.current_write_addr, entry_bytes)))
  {
    return false;
  }

  logger.current_write_addr += log_entry::size_bytes;
  logger.entries_written++;

  return true;
}

// Read log entries (for debugging/analysis)
void read_all_logs() noexcept
{
  for (std::uint32_t addr = log_sector_0_addr; addr < log_sector_0_addr + log_total_size; addr += log_entry::size_bytes)
  {
    const auto* entry = reinterpret_cast<const log_entry*>(addr);

    if (entry->timestamp == 0xFFFFFFFFU)
    {
      break; // Reached erased region
    }

    // Verify CRC
    const std::uint32_t calc_crc =
        calculate_crc32(reinterpret_cast<const std::uint8_t*>(entry), log_entry::size_bytes - sizeof(std::uint32_t));
    if (calc_crc == entry->crc)
    {
      // Valid entry - process it (e.g., send via UART, store to SD card, etc.)
    }
  }
}

// Initialize logger
bool initialize_logger() noexcept
{
  // Find last entry in Flash
  find_last_entry();

  logger.initialized = true;
  return true;
}

} // namespace

int main()
{
  // Compile-time validation
  static_assert(microhal::FlashConcept<backend, flash_cfg>);
  static_assert(microhal::RtcConcept<backend, rtc_cfg>);

  // Initialize RTC for timestamps
  if (!microhal::is_ok(microhal::rtc::controller<backend, rtc_cfg>::initialize()) ||
      !microhal::is_ok(microhal::rtc::controller<backend, rtc_cfg>::set(
          {.year = 2026U, .month = 5U, .day = 31U, .hour = 12U, .minute = 30U, .second = 0U})))
  {
    return 1;
  }

  // Initialize Flash logger
  if (!initialize_logger())
  {
    return 1;
  }

  // Example: Write some log entries
  constexpr std::uint16_t event_system_boot = 0x0001U;
  constexpr std::uint16_t event_sensor_reading = 0x0010U;
  constexpr std::uint16_t event_error = 0x00FFU;

  // Log system boot
  std::array<std::uint8_t, 4U> boot_data{0x01, 0x02, 0x03, 0x04};
  write_log_entry(event_system_boot, boot_data);

  // Simulate periodic sensor readings
  for (std::uint32_t i = 0U; i < 100U; ++i)
  {
    std::array<std::uint8_t, 8U> sensor_data{};
    sensor_data[0] = static_cast<std::uint8_t>(i >> 8U);
    sensor_data[1] = static_cast<std::uint8_t>(i & 0xFFU);
    sensor_data[2] = 0x12; // Temperature
    sensor_data[3] = 0x34;
    sensor_data[4] = 0x56; // Pressure
    sensor_data[5] = 0x78;

    write_log_entry(event_sensor_reading, sensor_data);

    // Simulate delay between readings
    for (std::uint32_t delay = 0U; delay < 100000U; ++delay)
    {
    }
  }

  // Log an error condition
  std::array<std::uint8_t, 4U> error_data{0xFF, 0xEE, 0xDD, 0xCC};
  write_log_entry(event_error, error_data);

  // Read back all logs (for analysis)
  read_all_logs();

  return 0;
}
