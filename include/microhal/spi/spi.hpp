// SPDX-License-Identifier: Apache-2.0
/**
 * @file spi.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <span>

#include "microhal/core/concepts.hpp"

namespace microhal::spi
{

/**
 * @brief Compile-time SPI master configuration.
 */
template <std::uint8_t InstanceIndex, std::uint32_t ClockHz, spi_duplex_mode DuplexMode = spi_duplex_mode::full,
          spi_clock_polarity ClockPolarity = spi_clock_polarity::idle_low,
          spi_clock_phase ClockPhase = spi_clock_phase::first_edge, spi_bit_order BitOrder = spi_bit_order::msb_first,
          bool EnableDma = false, bool EnableInterrupts = false, std::uint32_t TimeoutMs = 1000U,
          transfer_mode Mode = transfer_mode::automatic>
struct master_config final
{
  static constexpr std::uint8_t instance_index{InstanceIndex};
  static constexpr std::uint32_t clock_hz{ClockHz};
  static constexpr spi_duplex_mode duplex_mode{DuplexMode};
  static constexpr spi_clock_polarity clock_polarity{ClockPolarity};
  static constexpr spi_clock_phase clock_phase{ClockPhase};
  static constexpr spi_bit_order bit_order{BitOrder};
  static constexpr transfer_mode transfer_mode_value{Mode};
  static constexpr bool enable_dma{Mode == transfer_mode::dma || (Mode == transfer_mode::automatic && EnableDma)};
  static constexpr bool enable_interrupts{Mode == transfer_mode::interrupt ||
                                          (Mode == transfer_mode::automatic && EnableInterrupts)};
  static constexpr std::uint32_t timeout_ms{TimeoutMs};
};

/**
 * @brief Zero-overhead SPI master facade.
 */
template <typename Backend, typename Config>
  requires SpiMasterConcept<Backend, Config>
struct master
{
  using backend_tag = Backend;
  using config_type = Config;

  static constexpr peripheral_kind kind{peripheral_kind::spi};
  static constexpr transfer_mode mode{configured_transfer_mode<Config>()};
  using mode_api = serial_mode_api<Backend, peripheral_kind::spi>;

  /**
   * @brief Dispatches the configured SPI DMA completion interrupt.
   * @return The backend's interrupt-dispatch status.
   * @note Call from the DMA interrupt handler when the selected backend requires facade dispatch.
   */
  [[nodiscard]] static status handle_dma_interrupt() noexcept
  {
    return mode_api::template handle_dma_interrupt<Config>();
  }

  /**
   * @brief Initializes the configured SPI controller.
   * @return Initialization status, including backend errors or @ref status::unsupported when the
   *         selected configuration has no initialization path.
   */
  [[nodiscard]] static inline status initialize() noexcept
  {
    if constexpr (mode != transfer_mode::automatic || serial_slave_enabled<Config>())
      return mode_api::template initialize<Config>();
    else
      return detail::as_status([] { return spi_api_t<Backend>::template initialize<Config>(); });
  }

  /**
   * @brief Transmits and receives an equal number of SPI bytes.
   * @param[in]  tx_payload  Bytes to transmit; storage is used only for the duration of this call.
   * @param[out] rx_payload  Destination bytes; storage is used only for the duration of this call.
   * @return Transfer status, or @ref status::invalid_argument if the span lengths differ. Equal
   *         empty spans return @ref status::ok without accessing the backend.
   * @note Blocking modes use the configured timeout where supported and may require live interrupt
   *       service. Do not call from an ISR.
   */
  [[nodiscard]] static inline status transfer(const std::span<const std::uint8_t> tx_payload,
                                              const std::span<std::uint8_t> rx_payload) noexcept
  {
    if (tx_payload.size() != rx_payload.size())
    {
      return status::invalid_argument;
    }
    if (tx_payload.empty())
    {
      return status::ok;
    }

    if constexpr (mode != transfer_mode::automatic || serial_slave_enabled<Config>())
      return mode_api::template transfer<Config, nullptr>(tx_payload, rx_payload);
    else
      return detail::as_status([tx_payload, rx_payload]
                               { return spi_api_t<Backend>::template transfer<Config>(tx_payload, rx_payload); });
  }

  /**
   * @brief Transmits bytes without requiring a receive buffer.
   * @details Backends that cannot provide a transmit-only operation report @ref status::unsupported.
   * @param[in] payload  Bytes to transmit; storage is used only for the duration of this call.
   * @return Backend transfer status. An empty payload returns @ref status::ok without accessing the backend.
   * @note Blocking modes use the configured timeout where supported and may require live interrupt
   *       service. Do not call from an ISR.
   */
  [[nodiscard]] static inline status write_blocking(const std::span<const std::uint8_t> payload) noexcept
  {
    if (payload.empty())
    {
      return status::ok;
    }

    if constexpr (mode != transfer_mode::automatic || serial_slave_enabled<Config>())
      return mode_api::template write<Config, nullptr>(payload);
    else
      return detail::as_status([payload] { return spi_api_t<Backend>::template write_blocking<Config>(payload); });
  }

  /**
   * @brief Receives bytes without requiring a transmit buffer.
   * @details The backend supplies its documented dummy transmit value; unsupported paths report
   *          @ref status::unsupported.
   * @param[out] payload  Destination bytes; storage is used only for the duration of this call.
   * @return Backend transfer status. An empty payload returns @ref status::ok without accessing the backend.
   * @note Blocking modes use the configured timeout where supported and may require live interrupt
   *       service. Do not call from an ISR.
   */
  [[nodiscard]] static inline status read_blocking(const std::span<std::uint8_t> payload) noexcept
  {
    if (payload.empty())
    {
      return status::ok;
    }

    if constexpr (mode != transfer_mode::automatic || serial_slave_enabled<Config>())
      return mode_api::template read<Config, nullptr>(payload);
    else
      return detail::as_status([payload] { return spi_api_t<Backend>::template read_blocking<Config>(payload); });
  }

  /**
   * @brief Starts a nonblocking full-duplex SPI transfer.
   * @tparam CompletionHandler  No-throw callback accepting status or no arguments, as supported by
   *                            the selected serial mode.
   * @param[in]  tx_payload  Bytes to transmit; retain until completion or successful abort.
   * @param[out] rx_payload  Destination bytes; retain until completion or successful abort.
   * @return Submission status, or @ref status::invalid_argument when lengths differ. Equal empty
   *         spans return @ref status::ok in automatic mode and do not submit a transfer.
   * @note Requires interrupt-capable SPI support. The completion callback normally runs in interrupt
   *       context and must not block.
   */
  template <auto CompletionHandler>
  [[nodiscard]] static inline status transfer_async(const std::span<const std::uint8_t> tx_payload,
                                                    const std::span<std::uint8_t> rx_payload) noexcept
    requires IsrCapableConcept<Backend, peripheral_kind::spi>
  {
    static_assert(mode == transfer_mode::automatic || valid_serial_completion_handler<CompletionHandler>());
    if (tx_payload.size() != rx_payload.size())
    {
      return status::invalid_argument;
    }
    if (tx_payload.empty())
    {
      if constexpr (mode != transfer_mode::automatic)
        return status::invalid_argument;
      return status::ok;
    }

    if constexpr (mode != transfer_mode::automatic || serial_slave_enabled<Config>())
      return mode_api::template transfer<Config, CompletionHandler>(tx_payload, rx_payload);
    else
      return detail::as_status(
          [tx_payload, rx_payload]
          { return spi_api_t<Backend>::template transfer_async<Config, CompletionHandler>(tx_payload, rx_payload); });
  }

  /**
   * @brief Starts an asynchronous SPI transmit-only operation.
   * @tparam CompletionHandler  No-throw callback accepting status or no arguments, as supported by
   *                            the selected serial mode.
   * @param[in] payload  Bytes to transmit; retain until completion or successful abort.
   * @return Submission status, or @ref status::invalid_argument for an empty payload; backend errors
   *         are returned unchanged.
   * @note The callback normally runs in interrupt context and must not block. Async support depends
   *       on the configured mode and backend capability.
   */
  template <auto CompletionHandler>
  [[nodiscard]] static status write_async(const std::span<const std::uint8_t> payload) noexcept
  {
    static_assert(valid_serial_completion_handler<CompletionHandler>());
    if (payload.empty())
      return status::invalid_argument;
    return mode_api::template write<Config, CompletionHandler>(payload);
  }
  /**
   * @brief Starts an asynchronous SPI receive-only operation.
   * @tparam CompletionHandler  No-throw callback accepting status or no arguments, as supported by
   *                            the selected serial mode.
   * @param[out] payload  Destination bytes; retain until completion or successful abort.
   * @return Submission status, or @ref status::invalid_argument for an empty payload; backend errors
   *         are returned unchanged.
   * @note The callback normally runs in interrupt context and must not block. Async support depends
   *       on the configured mode and backend capability.
   */
  template <auto CompletionHandler>
  [[nodiscard]] static status read_async(const std::span<std::uint8_t> payload) noexcept
  {
    static_assert(valid_serial_completion_handler<CompletionHandler>());
    if (payload.empty())
      return status::invalid_argument;
    return mode_api::template read<Config, CompletionHandler>(payload);
  }
  /**
   * @brief Attempts to cancel the outstanding SPI transfer.
   * @return Backend cancellation status. Retain active buffers unless cancellation succeeds;
   *         @ref status::unsupported is returned where automatic mode has no abort path.
   */
  [[nodiscard]] static status abort() noexcept
  {
    if constexpr (mode == transfer_mode::automatic && !serial_slave_enabled<Config>() &&
                  !std::same_as<Backend, nordic_nrf_tag>)
      return status::unsupported;
    else
      return mode_api::template abort<Config>();
  }
  /**
   * @brief Queries the current or most recently completed SPI transfer status.
   * @return Backend status, typically @ref status::busy while active; automatic mode may report
   *         @ref status::unsupported where it has no status path.
   */
  [[nodiscard]] static status transfer_status() noexcept
  {
    if constexpr (mode == transfer_mode::automatic && !serial_slave_enabled<Config>() &&
                  !std::same_as<Backend, nordic_nrf_tag>)
      return status::unsupported;
    else
      return mode_api::template transfer_status<Config>();
  }
  /**
   * @brief Dispatches the configured SPI peripheral interrupt.
   * @return Backend interrupt-dispatch status, or @ref status::unsupported where automatic mode has
   *         no facade-dispatch path.
   * @note Call from the peripheral ISR when required by the backend.
   */
  [[nodiscard]] static status handle_interrupt() noexcept
  {
    if constexpr (mode == transfer_mode::automatic && !serial_slave_enabled<Config>() &&
                  !std::same_as<Backend, nordic_nrf_tag>)
      return status::unsupported;
    else
      return detail::as_status([] { return mode_api::template handle_interrupt<Config>(); });
  }
};

/** @brief Hardware-NSS SPI slave; the external master supplies the clock. */
template <std::uint8_t InstanceIndex, spi_duplex_mode DuplexMode = spi_duplex_mode::full,
          spi_clock_polarity ClockPolarity = spi_clock_polarity::idle_low,
          spi_clock_phase ClockPhase = spi_clock_phase::first_edge, spi_bit_order BitOrder = spi_bit_order::msb_first,
          bool EnableDma = false, bool EnableInterrupts = false, std::uint32_t TimeoutMs = 1000U,
          transfer_mode Mode = transfer_mode::interrupt>
struct slave_config final
{
  static constexpr bool is_slave = true;
  static constexpr std::uint8_t instance_index = InstanceIndex;
  // Satisfies shared facade validation; slave hardware never generates this clock.
  static constexpr std::uint32_t clock_hz = 1U;
  static constexpr auto duplex_mode = DuplexMode;
  static constexpr auto clock_polarity = ClockPolarity;
  static constexpr auto clock_phase = ClockPhase;
  static constexpr auto bit_order = BitOrder;
  static constexpr auto transfer_mode_value = Mode;
  static constexpr bool enable_dma = Mode == transfer_mode::dma || (Mode == transfer_mode::automatic && EnableDma);
  static constexpr bool enable_interrupts =
      Mode == transfer_mode::interrupt || (Mode == transfer_mode::automatic && EnableInterrupts);
  static constexpr auto timeout_ms = TimeoutMs;
};

/**
 * @brief SPI slave facade; the external master supplies the transfer clock.
 * @tparam Backend Registered backend tag.
 * @tparam Config Slave configuration accepted by the inherited master facade.
 * @note The application must serialize access and retain asynchronous buffers until completion or successful abort.
 */
template <typename Backend, typename Config> struct slave final : master<Backend, Config>
{
  static_assert(serial_slave_enabled<Config>());
  /**
   * @brief Starts an asynchronous SPI slave transmit.
   * @tparam Handler  No-throw callback accepting status or no arguments, as supported by the mode.
   * @param[in] bytes  Data to transmit; retain until completion or successful abort.
   * @return Submission status from the inherited SPI write operation.
   * @note The external SPI master supplies the clock. The callback normally runs in interrupt context
   *       and must not block.
   */
  template <auto Handler> [[nodiscard]] static status transmit_async(std::span<const std::uint8_t> bytes) noexcept
  {
    return master<Backend, Config>::template write_async<Handler>(bytes);
  }
  /**
   * @brief Starts an asynchronous SPI slave receive.
   * @tparam Handler  No-throw callback accepting status or no arguments, as supported by the mode.
   * @param[out] bytes  Destination buffer; retain until completion or successful abort.
   * @return Submission status from the inherited SPI read operation.
   * @note The external SPI master supplies the clock. The callback normally runs in interrupt context
   *       and must not block.
   */
  template <auto Handler> [[nodiscard]] static status receive_async(std::span<std::uint8_t> bytes) noexcept
  {
    return master<Backend, Config>::template read_async<Handler>(bytes);
  }
};
} // namespace microhal::spi
