// SPDX-License-Identifier: Apache-2.0
/**
 * @file qspi.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <limits>
#include <span>

#include "microhal/core/concepts.hpp"

namespace microhal::qspi
{

/**
 * @brief Compile-time QSPI controller configuration.
 */
template <std::uint8_t InstanceIndex, std::uint32_t ClockHz, bool EnableDma = false, bool EnableInterrupts = false,
          std::uint32_t TimeoutMs = 1000U>
struct controller_config final
{
  static constexpr std::uint8_t instance_index{InstanceIndex};
  static constexpr std::uint32_t clock_hz{ClockHz};
  static constexpr bool enable_dma{EnableDma};
  static constexpr bool enable_interrupts{EnableInterrupts};
  static constexpr std::uint32_t timeout_ms{TimeoutMs};
};

template <typename Binding, std::uint32_t ClockHz, bool EnableDma = false, bool EnableInterrupts = false,
          std::uint32_t TimeoutMs = 1000U>
struct bound_controller_config final
{
  using binding = Binding;

  static constexpr std::uint32_t clock_hz{ClockHz};
  static constexpr bool enable_dma{EnableDma};
  static constexpr bool enable_interrupts{EnableInterrupts};
  static constexpr std::uint32_t timeout_ms{TimeoutMs};
};

using io_mode = qspi_io_mode;
using command = qspi_command;

/**
 * @brief Zero-overhead QSPI controller facade for command-based and memory-mapped transfers.
 */
template <typename Backend, typename Config>
  requires QspiConcept<Backend, Config>
struct controller final
{
  using backend_tag = Backend;
  using config_type = Config;

  static constexpr peripheral_kind kind{peripheral_kind::qspi};

  /**
   * @brief Initializes the configured QSPI controller.
   * @return Backend initialization status.
   */
  [[nodiscard]] static inline status initialize() noexcept
  {
    return detail::as_status([] { return qspi_api_t<Backend>::template initialize<Config>(); });
  }

  /**
   * @brief Executes a QSPI command and writes its data phase.
   * @param[in] request Command phases and address; must be valid and use backend-supported IO modes.
   * @param[in] payload Bytes for the command data phase; caller-owned storage is used for this call.
   * @return @ref microhal::status::invalid_argument for an invalid command or oversized payload,
   *         @ref microhal::status::unsupported for unsupported IO modes, or the backend transfer status.
   * @note Backends that use @c Config::timeout_ms apply it to the command transfer and report any timeout through
   *       the returned status.
   */
  [[nodiscard]] static inline status command_write(const command& request,
                                                   const std::span<const std::uint8_t> payload) noexcept
  {
    if ((payload.size() > std::numeric_limits<std::uint32_t>::max()) || !is_valid_command(request, payload.empty()))
    {
      return status::invalid_argument;
    }
    if (!supports_command_mode(request))
    {
      return status::unsupported;
    }
    return detail::as_status([&request, payload]
                             { return qspi_api_t<Backend>::template command_write<Config>(request, payload); });
  }

  /**
   * @brief Executes a QSPI command and reads its data phase into caller-provided storage.
   * @param[in]  request Command phases and address; must be valid and use backend-supported IO modes.
   * @param[out] payload Destination storage; caller-owned storage is used for this call.
   * @return @ref microhal::status::invalid_argument for an invalid command or oversized buffer,
   *         @ref microhal::status::unsupported for unsupported IO modes, or the backend transfer status.
   * @note Backends that use @c Config::timeout_ms apply it to the command transfer and report any timeout through
   *       the returned status.
   */
  [[nodiscard]] static inline status command_read(const command& request,
                                                  const std::span<std::uint8_t> payload) noexcept
  {
    if ((payload.size() > std::numeric_limits<std::uint32_t>::max()) || !is_valid_command(request, payload.empty()))
    {
      return status::invalid_argument;
    }
    if (!supports_command_mode(request))
    {
      return status::unsupported;
    }
    return detail::as_status([&request, payload]
                             { return qspi_api_t<Backend>::template command_read<Config>(request, payload); });
  }

  /**
   * @brief Enters memory-mapped mode using a command that includes a data phase.
   * @param[in] request Memory-mapped read command; it must be valid and use backend-supported IO modes.
   * @return @ref microhal::status::invalid_argument for an invalid command,
   *         @ref microhal::status::unsupported for unsupported IO modes, or the backend status.
   * @note Timeout enforcement is backend-defined; backends that use @c Config::timeout_ms report a timeout through
   *       the returned status.
   */
  [[nodiscard]] static inline status enter_memory_mapped(const command& request) noexcept
  {
    if (!is_valid_command(request, false))
    {
      return status::invalid_argument;
    }
    if (!supports_command_mode(request))
    {
      return status::unsupported;
    }
    return detail::as_status([&request] { return qspi_api_t<Backend>::template enter_memory_mapped<Config>(request); });
  }

  /**
   * @brief Exits QSPI memory-mapped mode.
   * @return Backend exit status.
   */
  [[nodiscard]] static inline status exit_memory_mapped() noexcept
  {
    return detail::as_status([] { return qspi_api_t<Backend>::template exit_memory_mapped<Config>(); });
  }

  /**
   * @brief Checks whether the command's phase modes, address size, and data-phase declaration are structurally valid.
   * @param[in] request  QSPI command to check.
   * @param[in] no_data True when the command has no data phase.
   * @return True when the command satisfies the facade's command-format constraints.
   */
  [[nodiscard]] static constexpr bool is_valid_command(const command& request, const bool no_data = false) noexcept
  {
    return valid_command(request, no_data);
  }

  /**
   * @brief Checks whether the backend supports each IO mode selected by a command.
   * @param[in] request Command whose instruction, address, and data modes are checked.
   * @return True when every requested mode is supported by the backend capability declaration.
   */
  [[nodiscard]] static constexpr bool supports_command_mode(const command& request) noexcept
  {
    return supports_mode(request.instruction_mode) && supports_mode(request.address_mode) &&
           supports_mode(request.data_mode);
  }

private:
  [[nodiscard]] static constexpr bool supports_mode(const io_mode mode) noexcept
  {
    const auto capability = backend_traits<Backend>::qspi_capability();
    switch (mode)
    {
    case io_mode::none:
    case io_mode::single:
      return true;
    case io_mode::dual:
      return capability.dual_mode;
    case io_mode::quad:
      return capability.quad_mode;
    case io_mode::octo:
      return capability.octo_mode;
    }
    return false;
  }

  [[nodiscard]] static constexpr bool valid_io_mode(const io_mode mode) noexcept
  {
    switch (mode)
    {
    case io_mode::none:
    case io_mode::single:
    case io_mode::dual:
    case io_mode::quad:
    case io_mode::octo:
      return true;
    }
    return false;
  }

  [[nodiscard]] static constexpr bool valid_command(const command& request, const bool no_data) noexcept
  {
    return valid_io_mode(request.instruction_mode) && valid_io_mode(request.address_mode) &&
           valid_io_mode(request.data_mode) && (request.instruction_mode != io_mode::none) &&
           (request.address_size_bytes <= 4U) &&
           (request.address_size_bytes <= backend_traits<Backend>::qspi_capability().max_address_bytes) &&
           ((request.address_size_bytes == 0U) || (static_cast<std::uint64_t>(request.address) <=
                                                   ((std::uint64_t{1U} << (8U * request.address_size_bytes)) - 1U))) &&
           ((request.address_size_bytes == 0U) == (request.address_mode == io_mode::none)) &&
           (no_data == (request.data_mode == io_mode::none));
  }
};

} // namespace microhal::qspi
