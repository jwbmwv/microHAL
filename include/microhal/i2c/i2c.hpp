// SPDX-License-Identifier: Apache-2.0
/**
 * @file i2c.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <limits>
#include <span>

#include "microhal/core/concepts.hpp"

namespace microhal::i2c
{

/**
 * @brief Compile-time I2C controller configuration.
 * @tparam TimeoutMs Deadline applied when the backend exposes a configurable timeout; otherwise the
 *         backend's native blocking-transfer timeout applies.
 * @tparam StretchPolicy Requested clock-stretching policy; @ref i2c_clock_stretch_policy::backend_default
 *         preserves backend behavior.
 * @tparam StretchTimeoutUs Maximum clock-stretch duration when the backend supports a configurable limit.
 */
template <std::uint8_t InstanceIndex, std::uint32_t ClockHz, i2c_address_mode AddressMode = i2c_address_mode::seven_bit,
          bool EnableDma = false, bool EnableInterrupts = false, std::uint32_t TimeoutMs = 1000U,
          transfer_mode Mode = transfer_mode::automatic,
          i2c_clock_stretch_policy StretchPolicy = i2c_clock_stretch_policy::backend_default,
          std::uint32_t StretchTimeoutUs = 0U>
struct master_config final
{
  static_assert(TimeoutMs > 0U, "microHAL: I2C timeout must be non-zero.");

  static constexpr std::uint8_t instance_index{InstanceIndex};
  static constexpr std::uint32_t clock_hz{ClockHz};
  static constexpr i2c_address_mode address_mode{AddressMode};
  static constexpr transfer_mode transfer_mode_value{Mode};
  static constexpr bool enable_dma{Mode == transfer_mode::dma || (Mode == transfer_mode::automatic && EnableDma)};
  static constexpr bool enable_interrupts{Mode == transfer_mode::interrupt ||
                                          (Mode == transfer_mode::automatic && EnableInterrupts)};
  static constexpr std::uint32_t timeout_ms{TimeoutMs};
  static constexpr i2c_clock_stretch_policy clock_stretch_policy{StretchPolicy};
  static constexpr std::uint32_t clock_stretch_timeout_us{StretchTimeoutUs};
};

namespace smbus
{

/**
 * @brief Computes the SMBus CRC-8 PEC for an address phase and payload.
 * @param[in] address Seven-bit unshifted I2C address.
 * @param[in] read    True for a read address phase; false for a write address phase.
 * @param[in] payload Bytes covered by the PEC.
 * @return The SMBus PEC byte using polynomial 0x07 and initial value zero.
 */
[[nodiscard]] constexpr std::uint8_t pec(const std::uint16_t address, const bool read,
                                         const std::span<const std::uint8_t> payload) noexcept
{
  std::uint8_t crc = 0U;
  const auto update = [&crc](const std::uint8_t byte) constexpr
  {
    crc ^= byte;
    for (std::uint8_t bit = 0U; bit < 8U; ++bit)
    {
      crc =
          ((crc & 0x80U) != 0U) ? static_cast<std::uint8_t>((crc << 1U) ^ 0x07U) : static_cast<std::uint8_t>(crc << 1U);
    }
  };
  update(static_cast<std::uint8_t>((address << 1U) | (read ? 1U : 0U)));
  for (const std::uint8_t byte : payload)
  {
    update(byte);
  }
  return crc;
}

} // namespace smbus

/**
 * @brief Zero-overhead I2C master facade with compile-time capability validation.
 */
template <typename Backend, typename Config>
  requires I2cMasterConcept<Backend, Config>
struct master final
{
  using backend_tag = Backend;
  using config_type = Config;

  static constexpr peripheral_kind kind{peripheral_kind::i2c};
  static constexpr bool is_dma_capable{serial_dma_enabled<Config>()};
  static constexpr bool is_isr_capable{serial_interrupt_enabled<Config>()};
  static constexpr transfer_mode mode{configured_transfer_mode<Config>()};
  static constexpr bool has_bus_recovery = backend_traits<Backend>::i2c_capability().bus_recovery ||
                                           requires { Config::binding::template recover_i2c_bus<Config>(); };
  static constexpr bool supports_smbus_pec =
      backend_traits<Backend>::i2c_capability().smbus_pec ||
      (backend_traits<Backend>::i2c_capability().blocking && (Config::address_mode == i2c_address_mode::seven_bit));
  using mode_api = serial_mode_api<Backend, peripheral_kind::i2c>;

private:
  [[nodiscard]] static constexpr bool transfer_size_valid(const std::size_t size) noexcept
  {
    constexpr std::uint32_t maximum = backend_traits<Backend>::i2c_capability().max_transfer_size_bytes;
    return (maximum == 0U) || (size <= maximum);
  }

public:
  /**
   * @brief Dispatches the configured I2C DMA completion interrupt.
   * @return The backend's interrupt-dispatch status.
   * @note Call from the DMA interrupt handler when the selected backend requires facade dispatch.
   */
  [[nodiscard]] static status handle_dma_interrupt() noexcept
  {
    return mode_api::template handle_dma_interrupt<Config>();
  }

  /**
   * @brief Initializes the configured I2C controller.
   * @return Initialization status, including backend errors or @ref status::unsupported when the
   *         selected configuration has no initialization path.
   */
  [[nodiscard]] static inline status initialize() noexcept
  {
    if constexpr (mode != transfer_mode::automatic || serial_slave_enabled<Config>())
      return mode_api::template initialize<Config>();
    else
      return detail::as_status([] { return i2c_api_t<Backend>::template initialize<Config>(); });
  }

  /**
   * @brief Writes a payload to a slave using a blocking transfer.
   * @param[in] address  I2C slave address; its valid range is selected by @ref Config::address_mode.
   * @param[in] payload  Bytes to transmit; storage is used only for the duration of this call.
   * @return Transfer status. An out-of-range address returns @ref status::invalid_argument;
   *         backend transfer errors, including timeout where supported, are returned unchanged.
   * @note Blocking calls require completion interrupts and timeout timekeeping to remain serviceable;
   *       do not call from an ISR.
   */
  [[nodiscard]] static inline status write_blocking(const std::uint16_t address,
                                                    const std::span<const std::uint8_t> payload) noexcept
  {
    const std::uint16_t max_address = (Config::address_mode == i2c_address_mode::seven_bit) ? 0x7FU : 0x3FFU;
    if ((address > max_address) || !transfer_size_valid(payload.size()))
    {
      return status::invalid_argument;
    }

    if constexpr (mode != transfer_mode::automatic || serial_slave_enabled<Config>())
      return mode_api::template write<Config, nullptr>(payload, address);
    else
      return detail::as_status([address, payload]
                               { return i2c_api_t<Backend>::template write_blocking<Config>(payload, address); });
  }

  /**
   * @brief Reads bytes from a slave using a blocking transfer.
   * @param[in]  address  I2C slave address; its valid range is selected by @ref Config::address_mode.
   * @param[out] payload  Destination bytes; storage is used only for the duration of this call.
   * @return Transfer status. An out-of-range address returns @ref status::invalid_argument;
   *         backend transfer errors, including timeout where supported, are returned unchanged.
   * @note Blocking calls require completion interrupts and timeout timekeeping to remain serviceable;
   *       do not call from an ISR.
   */
  [[nodiscard]] static inline status read_blocking(const std::uint16_t address,
                                                   const std::span<std::uint8_t> payload) noexcept
  {
    const std::uint16_t max_address = (Config::address_mode == i2c_address_mode::seven_bit) ? 0x7FU : 0x3FFU;
    if ((address > max_address) || !transfer_size_valid(payload.size()))
    {
      return status::invalid_argument;
    }

    if constexpr (mode != transfer_mode::automatic || serial_slave_enabled<Config>())
      return mode_api::template read<Config, nullptr>(payload, address);
    else
      return detail::as_status([address, payload]
                               { return i2c_api_t<Backend>::template read_blocking<Config>(payload, address); });
  }

  /**
   * @brief Checks whether a slave acknowledges its address without transferring payload bytes.
   * @param[in] address  I2C slave address; its valid range is selected by @ref Config::address_mode.
   * @return @ref status::ok if acknowledged, @ref status::io_error if not acknowledged,
   *         @ref status::invalid_argument for an out-of-range address, or
   *         @ref status::unsupported if the backend cannot perform an address-only probe.
   * @note This operation blocks according to the backend's probe implementation; do not call from
   *       an ISR.
   */
  [[nodiscard]] static inline status probe(const std::uint16_t address) noexcept
  {
    const std::uint16_t max_address = (Config::address_mode == i2c_address_mode::seven_bit) ? 0x7FU : 0x3FFU;
    if (address > max_address)
    {
      return status::invalid_argument;
    }

    if constexpr (mode != transfer_mode::automatic || serial_slave_enabled<Config>())
      return mode_api::template probe<Config>(address);
    else
      return detail::as_status([address] { return i2c_api_t<Backend>::template probe<Config>(address); });
  }

  /**
   * @brief Writes a command or register address, then reads data without releasing the bus.
   * @details The backend must issue a repeated START between the write and read phases. A backend
   *          timeout is reported as @ref status::timeout, bus/address failures as @ref status::io_error,
   *          and absent support as @ref status::unsupported. Backends with configurable deadlines use
   *          @c Config::timeout_ms; drivers with only native blocking calls use their native timeout.
   * @param[in]  address     I2C slave address; its valid range is selected by @ref Config::address_mode.
   * @param[in]  tx_payload  Command or register bytes; storage is used only for the duration of this call.
   * @param[out] rx_payload  Destination bytes; storage is used only for the duration of this call.
   * @return Transfer status, or @ref status::invalid_argument for an out-of-range address or an empty buffer.
   * @note Blocking calls require completion interrupts and timeout timekeeping to remain serviceable;
   *       do not call from an ISR.
   */
  [[nodiscard]] static inline status write_read_blocking(const std::uint16_t address,
                                                         const std::span<const std::uint8_t> tx_payload,
                                                         const std::span<std::uint8_t> rx_payload) noexcept
  {
    const std::uint16_t max_address = (Config::address_mode == i2c_address_mode::seven_bit) ? 0x7FU : 0x3FFU;
    if ((address > max_address) || tx_payload.empty() || rx_payload.empty() ||
        !transfer_size_valid(tx_payload.size()) || !transfer_size_valid(rx_payload.size()))
    {
      return status::invalid_argument;
    }

    if constexpr (mode != transfer_mode::automatic || serial_slave_enabled<Config>())
      return mode_api::template write_read<Config, nullptr>(tx_payload, rx_payload, address);
    else
      return detail::as_status(
          [address, tx_payload, rx_payload]
          { return i2c_api_t<Backend>::template write_read_blocking<Config>(tx_payload, rx_payload, address); });
  }

  /**
   * @brief Starts a write followed by a repeated-START read.
   * @details The completion handler is called after the read finishes or an error occurs. Backends
   *          without asynchronous combined-transfer support return @ref status::unsupported. Both
   *          buffers must remain valid until completion; on interrupt-driven backends, the handler
   *          runs in the completion interrupt context. Handlers must be non-blocking and safe for
   *          that context.
   * @tparam CompletionHandler  No-throw callback accepting status or no arguments, as supported by
   *                            the selected serial mode.
   * @param[in]  address     I2C slave address; its valid range is selected by @ref Config::address_mode.
   * @param[in]  tx_payload  Command or register bytes, retained by the backend until completion.
   * @param[out] rx_payload  Destination bytes, retained by the backend until completion.
   * @return Submission status, or @ref status::invalid_argument for an out-of-range address or an
   *         empty buffer. Backend errors such as @ref status::busy or @ref status::unsupported are
   *         returned when applicable.
   */
  template <auto CompletionHandler>
  [[nodiscard]] static inline status write_read_async(const std::uint16_t address,
                                                      const std::span<const std::uint8_t> tx_payload,
                                                      const std::span<std::uint8_t> rx_payload) noexcept
  {
    static_assert(mode == transfer_mode::automatic || valid_serial_completion_handler<CompletionHandler>());
    const std::uint16_t max_address = (Config::address_mode == i2c_address_mode::seven_bit) ? 0x7FU : 0x3FFU;
    if ((address > max_address) || tx_payload.empty() || rx_payload.empty() ||
        !transfer_size_valid(tx_payload.size()) || !transfer_size_valid(rx_payload.size()))
    {
      return status::invalid_argument;
    }

    if constexpr (mode != transfer_mode::automatic || serial_slave_enabled<Config>())
      return mode_api::template write_read<Config, CompletionHandler>(tx_payload, rx_payload, address);
    else if constexpr (!backend_traits<Backend>::i2c_capability().non_blocking)
      return status::unsupported;
    else
      return detail::as_status(
          [address, tx_payload, rx_payload] {
            return i2c_api_t<Backend>::template write_read_async<Config, CompletionHandler>(tx_payload, rx_payload,
                                                                                            address);
          });
  }

  /**
   * @brief Starts transmit using the selected asynchronous engine (legacy interrupt path in automatic mode).
   * @tparam CompletionHandler  No-throw callback accepting status or no arguments, as supported by
   *                            the selected serial mode.
   * @param[in] address  I2C slave address; its valid range is selected by @ref Config::address_mode.
   * @param[in] payload  Bytes to transmit; keep the buffer valid until completion or successful abort.
   * @return Submission status, or @ref status::invalid_argument for an out-of-range address or empty
   *         payload; backend errors are returned unchanged.
   * @note The callback normally runs in interrupt context. It must not block or call blocking facade
   *       operations. Availability is constrained by @c IsrCapableConcept.
   */
  template <auto CompletionHandler>
  [[nodiscard]] static inline status start_transfer(const std::uint16_t address,
                                                    const std::span<std::uint8_t> payload) noexcept
  {
    static_assert(mode == transfer_mode::automatic || valid_serial_completion_handler<CompletionHandler>());
    const std::uint16_t max_address = (Config::address_mode == i2c_address_mode::seven_bit) ? 0x7FU : 0x3FFU;
    if ((address > max_address) || payload.empty() || !transfer_size_valid(payload.size()))
    {
      return status::invalid_argument;
    }

    if constexpr (mode != transfer_mode::automatic || serial_slave_enabled<Config>())
      return mode_api::template write<Config, CompletionHandler>(payload, address);
    else if constexpr (!backend_traits<Backend>::i2c_capability().non_blocking)
      return status::unsupported;
    else
      return detail::as_status(
          [address, payload]
          { return i2c_api_t<Backend>::template start_transfer<Config, CompletionHandler>(payload, address); });
  }

  /**
   * @brief Starts an asynchronous I2C write.
   * @tparam CompletionHandler  No-throw callback accepting status or no arguments, as supported by
   *                            the selected serial mode.
   * @param[in] address  I2C slave address; its valid range is selected by @ref Config::address_mode.
   * @param[in] payload  Bytes to transmit; keep the buffer valid until completion or successful abort.
   * @return Submission status, or @ref status::invalid_argument for an out-of-range address or empty
   *         payload; backend errors are returned unchanged.
   * @note The callback normally runs in interrupt context and must not block. Async support depends
   *       on the selected transfer mode and backend capability.
   */
  template <auto CompletionHandler>
  [[nodiscard]] static status write_async(const std::uint16_t address,
                                          const std::span<const std::uint8_t> payload) noexcept
  {
    static_assert(valid_serial_completion_handler<CompletionHandler>());
    if (address > (Config::address_mode == i2c_address_mode::seven_bit ? 0x7FU : 0x3FFU) || payload.empty() ||
        !transfer_size_valid(payload.size()))
      return status::invalid_argument;
    return mode_api::template write<Config, CompletionHandler>(payload, address);
  }

  /**
   * @brief Starts an asynchronous I2C read.
   * @tparam CompletionHandler  No-throw callback accepting status or no arguments, as supported by
   *                            the selected serial mode.
   * @param[in] address  I2C slave address; its valid range is selected by @ref Config::address_mode.
   * @param[out] payload  Destination bytes; keep the buffer valid until completion or successful abort.
   * @return Submission status, or @ref status::invalid_argument for an out-of-range address or empty
   *         payload; backend errors are returned unchanged.
   * @note The callback normally runs in interrupt context and must not block. Async support depends
   *       on the selected transfer mode and backend capability.
   */
  template <auto CompletionHandler>
  [[nodiscard]] static status read_async(const std::uint16_t address, const std::span<std::uint8_t> payload) noexcept
  {
    static_assert(valid_serial_completion_handler<CompletionHandler>());
    if (address > (Config::address_mode == i2c_address_mode::seven_bit ? 0x7FU : 0x3FFU) || payload.empty() ||
        !transfer_size_valid(payload.size()))
      return status::invalid_argument;
    return mode_api::template read<Config, CompletionHandler>(payload, address);
  }

  /**
   * @brief Attempts to cancel the outstanding I2C transfer.
   * @return Backend cancellation status. Retain active buffers unless cancellation succeeds.
   * @note In automatic mode this operation is available only on supported backend paths; otherwise
   *       it returns @ref status::unsupported.
   */
  [[nodiscard]] static status abort() noexcept
  {
    if constexpr (mode == transfer_mode::automatic && !std::same_as<Backend, nordic_nrf_tag>)
      return status::unsupported;
    else
      return mode_api::template abort<Config>();
  }
  /**
   * @brief Queries the current or most recently completed transfer status.
   * @return Backend status, typically @ref status::busy while active and a terminal status afterward;
   *         @ref status::unsupported when automatic mode has no status path.
   */
  [[nodiscard]] static status transfer_status() noexcept
  {
    if constexpr (mode == transfer_mode::automatic && !std::same_as<Backend, nordic_nrf_tag>)
      return status::unsupported;
    else
      return mode_api::template transfer_status<Config>();
  }

  /**
   * @brief Attempts to recover the I2C bus from a stuck-line condition.
   * @return @ref status::unsupported when neither the backend nor the board binding provides recovery.
   */
  [[nodiscard]] static status recover_bus() noexcept
  {
    if constexpr (requires { i2c_api_t<Backend>::template recover_bus<Config>(); })
    {
      return detail::as_status([] { return i2c_api_t<Backend>::template recover_bus<Config>(); });
    }
    else if constexpr (requires { Config::binding::template recover_i2c_bus<Config>(); })
    {
      return detail::as_status([] { return Config::binding::template recover_i2c_bus<Config>(); });
    }
    else
    {
      return status::unsupported;
    }
  }

  /**
   * @brief Writes an SMBus payload followed by its PEC byte.
   * @param[in] address Seven-bit unshifted SMBus address.
   * @param[in] payload Payload bytes covered by the PEC.
   * @param[in,out] workspace Caller-owned, non-overlapping storage with at least payload.size() + 1 bytes.
   * @return Transfer status, or @ref status::invalid_argument for an invalid address or workspace.
   */
  [[nodiscard]] static status write_blocking_with_pec(const std::uint16_t address,
                                                      const std::span<const std::uint8_t> payload,
                                                      const std::span<std::uint8_t> workspace) noexcept
  {
    if constexpr (Config::address_mode != i2c_address_mode::seven_bit)
      return status::unsupported;
    if ((address > 0x7FU) || (payload.size() == std::numeric_limits<std::size_t>::max()) ||
        (workspace.size() < payload.size() + 1U) || !transfer_size_valid(payload.size() + 1U))
      return status::invalid_argument;

    for (std::size_t index = 0U; index < payload.size(); ++index)
    {
      workspace[index] = payload[index];
    }
    workspace[payload.size()] = smbus::pec(address, false, payload);
    return write_blocking(address, std::span<const std::uint8_t>{workspace.data(), payload.size() + 1U});
  }

  /**
   * @brief Reads an SMBus payload and validates its trailing PEC byte.
   * @param[in] address Seven-bit unshifted SMBus address.
   * @param[out] payload Caller-owned destination for data bytes, excluding PEC.
   * @param[in,out] workspace Caller-owned, non-overlapping storage with at least payload.size() + 1 bytes.
   * @return Transfer status, @ref status::io_error for a PEC mismatch, or
   *         @ref status::invalid_argument for an invalid address or workspace.
   */
  [[nodiscard]] static status read_blocking_with_pec(const std::uint16_t address, const std::span<std::uint8_t> payload,
                                                     const std::span<std::uint8_t> workspace) noexcept
  {
    if constexpr (Config::address_mode != i2c_address_mode::seven_bit)
      return status::unsupported;
    if ((address > 0x7FU) || (payload.size() == std::numeric_limits<std::size_t>::max()) ||
        (workspace.size() < payload.size() + 1U) || !transfer_size_valid(payload.size() + 1U))
      return status::invalid_argument;

    const std::span<std::uint8_t> frame{workspace.data(), payload.size() + 1U};
    const status transfer_result = read_blocking(address, frame);
    if (transfer_result != status::ok)
    {
      return transfer_result;
    }
    if (smbus::pec(address, true, std::span<const std::uint8_t>{frame.data(), payload.size()}) != frame.back())
    {
      return status::io_error;
    }
    for (std::size_t index = 0U; index < payload.size(); ++index)
    {
      payload[index] = frame[index];
    }
    return status::ok;
  }
  /**
   * @brief Dispatches the configured I2C peripheral interrupt.
   * @return The backend's interrupt-dispatch status, or @ref status::unsupported when this automatic
   *         backend has no facade-dispatch path.
   * @note Call from the peripheral ISR when required by the backend.
   */
  [[nodiscard]] static status handle_interrupt() noexcept
  {
    if constexpr (mode == transfer_mode::automatic && !std::same_as<Backend, nordic_nrf_tag>)
      return status::unsupported;
    else
      return detail::as_status([] { return mode_api::template handle_interrupt<Config>(); });
  }
};

/** @brief Fixed-length I2C slave configuration. OwnAddress is unshifted. */
template <std::uint8_t InstanceIndex, std::uint16_t OwnAddress,
          i2c_address_mode AddressMode = i2c_address_mode::seven_bit, std::uint32_t ClockHz = 100000U,
          bool EnableDma = false, bool EnableInterrupts = false, std::uint32_t TimeoutMs = 1000U,
          transfer_mode Mode = transfer_mode::interrupt>
struct slave_config final
{
  static_assert(OwnAddress <= (AddressMode == i2c_address_mode::seven_bit ? 0x7FU : 0x3FFU));
  static_assert(TimeoutMs > 0U);
  static constexpr bool is_slave = true;
  static constexpr auto instance_index = InstanceIndex;
  static constexpr auto own_address = OwnAddress;
  static constexpr auto address_mode = AddressMode;
  static constexpr auto clock_hz = ClockHz;
  static constexpr auto transfer_mode_value = Mode;
  static constexpr bool enable_dma = Mode == transfer_mode::dma || (Mode == transfer_mode::automatic && EnableDma);
  static constexpr bool enable_interrupts =
      Mode == transfer_mode::interrupt || (Mode == transfer_mode::automatic && EnableInterrupts);
  static constexpr auto timeout_ms = TimeoutMs;
};

template <typename Backend, typename Config>
  requires I2cMasterConcept<Backend, Config>
struct slave final
{
  // Async buffers remain owned until completion or a successful abort.
  static_assert(serial_slave_enabled<Config>());
  using backend_tag = Backend;
  using config_type = Config;
  using mode_api = serial_mode_api<Backend, peripheral_kind::i2c>;
  static constexpr auto kind = peripheral_kind::i2c;
  static constexpr auto mode = configured_transfer_mode<Config>();
  /**
   * @brief Initializes the configured I2C slave controller.
   * @return Backend initialization status.
   */
  [[nodiscard]] static status initialize() noexcept
  {
    return mode_api::template initialize<Config>();
  }
  /**
   * @brief Starts receiving bytes addressed to this I2C slave.
   * @tparam Handler  No-throw callback accepting status or no arguments, as supported by the mode.
   * @param[out] bytes  Destination buffer; keep it valid until completion or successful abort.
   * @return Submission status from the selected backend mode.
   * @note The callback normally runs in interrupt context and must not block.
   */
  template <auto Handler> [[nodiscard]] static status receive_async(std::span<std::uint8_t> bytes) noexcept
  {
    static_assert(valid_serial_completion_handler<Handler>());
    return mode_api::template read<Config, Handler>(bytes);
  }
  /**
   * @brief Starts transmitting bytes from this I2C slave.
   * @tparam Handler  No-throw callback accepting status or no arguments, as supported by the mode.
   * @param[in] bytes  Data to transmit; keep it valid until completion or successful abort.
   * @return Submission status from the selected backend mode.
   * @note The callback normally runs in interrupt context and must not block.
   */
  template <auto Handler> [[nodiscard]] static status transmit_async(std::span<const std::uint8_t> bytes) noexcept
  {
    static_assert(valid_serial_completion_handler<Handler>());
    return mode_api::template write<Config, Handler>(bytes);
  }
  /**
   * @brief Attempts to cancel the outstanding slave transfer.
   * @return Backend cancellation status. Retain active buffers unless cancellation succeeds.
   */
  [[nodiscard]] static status abort() noexcept
  {
    return mode_api::template abort<Config>();
  }
  /**
   * @brief Queries the current or most recently completed slave transfer status.
   * @return Backend transfer status, typically @ref status::busy while active.
   */
  [[nodiscard]] static status transfer_status() noexcept
  {
    return mode_api::template transfer_status<Config>();
  }
  /**
   * @brief Dispatches the configured I2C peripheral interrupt.
   * @return Backend interrupt-dispatch status.
   * @note Call from the peripheral ISR when required by the backend.
   */
  [[nodiscard]] static status handle_interrupt() noexcept
  {
    return mode_api::template handle_interrupt<Config>();
  }
  /**
   * @brief Dispatches the configured I2C DMA interrupt.
   * @return Backend interrupt-dispatch status.
   * @note Call from the DMA ISR when required by the backend.
   */
  [[nodiscard]] static status handle_dma_interrupt() noexcept
  {
    return mode_api::template handle_dma_interrupt<Config>();
  }
};
} // namespace microhal::i2c
