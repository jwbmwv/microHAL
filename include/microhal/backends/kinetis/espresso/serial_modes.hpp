// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "microhal/core/traits.hpp"
#include <array>
#include <atomic>
#include <span>
#include <type_traits>

// Native resource contract (Config::binding):
// - instance: a non-null I2C_Type*, LPI2C_Type*, SPI_Type*, LPSPI_Type*,
//   UART_Type*, LPUART_Type*, or I2S_Type* matching the facade.
// - source_clock_hz: the actual enabled peripheral input clock.
// - now_ms(): wrapping uint32_t monotonic milliseconds, required for synchronous
//   calls, even when the selected engine is interrupt/DMA.
// - idle(): optional bounded wait/yield hook; it must not mask completion IRQs.
// - I2C/SAI DMA: dma_handle; UART/LPUART DMA: tx_dma_handle and rx_dma_handle;
//   DSPI EDMA: rx_dma_handle, intermediary_dma_handle, tx_dma_handle;
//   LPSPI EDMA: rx_dma_handle and tx_dma_handle.
// DMA resources are actual initialized edma_handle_t*/dma_handle_t*, never
// channel numbers. EDMA is preferred if both drivers are present. The board owns
// clock gates, pin mux, DMA/DMAMUX initialization, channel/request routing, IRQ
// priorities, and DMA cache maintenance/alignment. Reserve these resources for
// this engine: do not replace the SDK's callbacks/handles from legacy code.
// SPI optionally accepts sdk_config (typed SDK master config) and transfer_flags
// (native CTAR/PCS/continuous flags). UART optionally accepts a typed sdk_config.
// SAI uses modern >=2.4 transceiver APIs: Philips stereo 16-bit, channel 0,
// asynchronous clocking, master/slave TX/RX. Master clock output is board-owned
// and enable_master_clock=true is explicitly rejected here.
// Buffers must remain valid until completion/abort; async errors are also
// observable with transfer_status(). Abort suppresses the user callback.
// Polled paths are direct register/FIFO engines, never SDK blocking calls or
// IRQ-state-machine pumping. They use timeout_ms (default 1000ms) for the entire
// operation plus a stagnant-clock watchdog. Polled I2C combined transactions are
// unsupported; LPI2C polled RX is limited to one receive command (256 bytes).
// Polled SPI accepts CTAR0 (8-bit), PCS, and within-transfer continuous PCS only;
// every operation releases PCS. LPI2C polling errors disable the master:
// initialize() is required before reuse. SAI TX completion, like the SDK's TX
// callback, denotes buffer consumption, not final on-wire bit completion.
// Classic I2C cancellation/errors disable the controller without invoking the
// SDK's potentially unbounded abort; initialize() is required before reuse.
// A still-busy bus reports timeout even though local buffer access is stopped.

// These engines deliberately do not call the SDK's blocking transfer routines:
// several MCUXpresso drivers default to an infinite retry count.
#if MICROHAL_KINETIS_SDK_PRESENT
#if __has_include("fsl_dspi.h")
#include "fsl_dspi.h"
#define MICROHAL_KINETIS_SERIAL_DSPI 1
#else
#define MICROHAL_KINETIS_SERIAL_DSPI 0
#endif
#if __has_include("fsl_lpspi.h")
#include "fsl_lpspi.h"
#define MICROHAL_KINETIS_SERIAL_LPSPI 1
#else
#define MICROHAL_KINETIS_SERIAL_LPSPI 0
#endif
#if __has_include("fsl_sai.h")
#include "fsl_sai.h"
#define MICROHAL_KINETIS_SERIAL_SAI 1
#else
#define MICROHAL_KINETIS_SERIAL_SAI 0
#endif
#if __has_include("fsl_i2c_edma.h")
#include "fsl_i2c_edma.h"
#define MICROHAL_KINETIS_SERIAL_I2C_EDMA 1
#else
#define MICROHAL_KINETIS_SERIAL_I2C_EDMA 0
#endif
#if __has_include("fsl_lpi2c_edma.h")
#include "fsl_lpi2c_edma.h"
#define MICROHAL_KINETIS_SERIAL_LPI2C_EDMA 1
#else
#define MICROHAL_KINETIS_SERIAL_LPI2C_EDMA 0
#endif
#if __has_include("fsl_dspi_edma.h")
#include "fsl_dspi_edma.h"
#define MICROHAL_KINETIS_SERIAL_DSPI_EDMA 1
#else
#define MICROHAL_KINETIS_SERIAL_DSPI_EDMA 0
#endif
#if __has_include("fsl_lpspi_edma.h")
#include "fsl_lpspi_edma.h"
#define MICROHAL_KINETIS_SERIAL_LPSPI_EDMA 1
#else
#define MICROHAL_KINETIS_SERIAL_LPSPI_EDMA 0
#endif
#if __has_include("fsl_uart_edma.h")
#include "fsl_uart_edma.h"
#define MICROHAL_KINETIS_SERIAL_UART_EDMA 1
#else
#define MICROHAL_KINETIS_SERIAL_UART_EDMA 0
#endif
#if __has_include("fsl_lpuart_edma.h")
#include "fsl_lpuart_edma.h"
#define MICROHAL_KINETIS_SERIAL_LPUART_EDMA 1
#else
#define MICROHAL_KINETIS_SERIAL_LPUART_EDMA 0
#endif
#if __has_include("fsl_sai_edma.h")
#include "fsl_sai_edma.h"
#define MICROHAL_KINETIS_SERIAL_SAI_EDMA 1
#else
#define MICROHAL_KINETIS_SERIAL_SAI_EDMA 0
#endif
#if __has_include("fsl_i2c_dma.h")
#include "fsl_i2c_dma.h"
#define MICROHAL_KINETIS_SERIAL_I2C_DMA 1
#else
#define MICROHAL_KINETIS_SERIAL_I2C_DMA 0
#endif
#if __has_include("fsl_uart_dma.h")
#include "fsl_uart_dma.h"
#define MICROHAL_KINETIS_SERIAL_UART_DMA 1
#else
#define MICROHAL_KINETIS_SERIAL_UART_DMA 0
#endif
#if __has_include("fsl_lpuart_dma.h")
#include "fsl_lpuart_dma.h"
#define MICROHAL_KINETIS_SERIAL_LPUART_DMA 1
#else
#define MICROHAL_KINETIS_SERIAL_LPUART_DMA 0
#endif
#else
#define MICROHAL_KINETIS_SERIAL_DSPI 0
#define MICROHAL_KINETIS_SERIAL_LPSPI 0
#define MICROHAL_KINETIS_SERIAL_SAI 0
#define MICROHAL_KINETIS_SERIAL_I2C_EDMA 0
#define MICROHAL_KINETIS_SERIAL_LPI2C_EDMA 0
#define MICROHAL_KINETIS_SERIAL_DSPI_EDMA 0
#define MICROHAL_KINETIS_SERIAL_LPSPI_EDMA 0
#define MICROHAL_KINETIS_SERIAL_UART_EDMA 0
#define MICROHAL_KINETIS_SERIAL_LPUART_EDMA 0
#define MICROHAL_KINETIS_SERIAL_SAI_EDMA 0
#define MICROHAL_KINETIS_SERIAL_I2C_DMA 0
#define MICROHAL_KINETIS_SERIAL_UART_DMA 0
#define MICROHAL_KINETIS_SERIAL_LPUART_DMA 0
#endif

namespace microhal::backends::kinetis::espresso::serial
{
struct unavailable
{
};
template <typename T> unavailable select(T*) noexcept;
inline unavailable select(...) noexcept;
template <typename Binding> inline constexpr unsigned owner_token = 0;
template <typename Config> constexpr std::uint32_t timeout_ms() noexcept
{
  if constexpr (requires { Config::timeout_ms; })
    return Config::timeout_ms;
  return 1000U;
}

template <auto Handler>
inline constexpr bool synchronous_handler = []
{
  if constexpr (requires { Handler == nullptr; })
    return Handler == nullptr;
  else
    return false;
}();

template <auto Handler> void notify(status value) noexcept
{
  if constexpr (!synchronous_handler<Handler>)
  {
    if constexpr (requires { Handler(value); })
      Handler(value);
    else if constexpr (requires { Handler(); })
      Handler();
    else
      static_assert(dependent_false_v<decltype(Handler)>, "Serial handler must accept status or no arguments");
  }
}

template <typename Config> struct poll_deadline
{
  using B = typename Config::binding;
  std::uint32_t start = B::now_ms(), last = start, spins{};
  bool expired() noexcept
  {
    const std::uint32_t now = B::now_ms();
    if (now != last)
    {
      last = now;
      spins = 0U;
    }
    return static_cast<std::uint32_t>(now - start) >= timeout_ms<Config>() || ++spins == 1'000'000U;
  }
  void idle() noexcept
  {
    if constexpr (requires { B::idle(); })
      B::idle();
  }
};

template <typename Config, typename Predicate>
status poll_until(poll_deadline<Config>& deadline, Predicate predicate) noexcept
{
  for (;;)
  {
    if (deadline.expired())
      return status::timeout;
    const auto result = predicate();
    if (result != status::busy)
      return result;
    deadline.idle();
  }
}

#if MICROHAL_KINETIS_SDK_PRESENT && MICROHAL_KINETIS_HAS_I2C_DRIVER
template <typename Config, typename S> status poll_i2c(S& s) noexcept
{
  auto* base = s.base;
  auto& x = s.transfer;
  if (x.subaddressSize != 0U)
    return status::unsupported;
  if (base->S & I2C_S_BUSY_MASK)
    return status::busy;
  poll_deadline<Config> deadline;
  I2C_EnableDMA(base, false);
  const auto saved = base->C1;
  base->C1 = static_cast<std::uint8_t>((base->C1 & ~I2C_C1_IICIE_MASK) | I2C_C1_MST_MASK | I2C_C1_TX_MASK);
  const auto wait_byte = [&](bool ack)
  {
    return poll_until(deadline,
                      [&]
                      {
                        const auto flags = base->S;
                        if (flags & I2C_S_IAL_MASK)
                          return status::io_error;
                        if (!(flags & I2C_S_IICIF_MASK))
                          return status::busy;
                        if (ack && (flags & I2C_S_RXAK_MASK))
                          return status::io_error;
                        return status::ok;
                      });
  };
  const auto clear = [&] { base->S = I2C_S_IICIF_MASK | I2C_S_IAL_MASK; };
  const auto write_byte = [&](std::uint8_t value)
  {
#if defined(FSL_FEATURE_I2C_HAS_DOUBLE_BUFFERING) && FSL_FEATURE_I2C_HAS_DOUBLE_BUFFERING
    const auto ready = poll_until(deadline, [&] { return (base->S2 & I2C_S2_EMPTY_MASK) ? status::ok : status::busy; });
    if (ready != status::ok)
      return ready;
#endif
    clear();
    base->D = value;
    return wait_byte(true);
  };
  auto result = write_byte(static_cast<std::uint8_t>((x.slaveAddress << 1U) | (x.direction == kI2C_Read)));
  if (result == status::ok && x.direction == kI2C_Write)
    for (std::size_t n = 0U; n != x.dataSize && result == status::ok; ++n)
      result = write_byte(x.data[n]);
  if (result == status::ok && x.direction == kI2C_Read)
  {
    base->C1 = base->C1 & static_cast<std::uint8_t>(~I2C_C1_TX_MASK);
    if (x.dataSize == 1U)
      base->C1 = base->C1 | I2C_C1_TXAK_MASK;
    else
      base->C1 = base->C1 & static_cast<std::uint8_t>(~I2C_C1_TXAK_MASK);
    clear();
    (void)base->D;
    for (std::size_t n = 0U; n != x.dataSize && result == status::ok; ++n)
    {
      result = wait_byte(false);
      if (result != status::ok)
        break;
      if (n + 1U == x.dataSize)
        base->C1 = base->C1 & static_cast<std::uint8_t>(~I2C_C1_MST_MASK);
      else if (n + 2U == x.dataSize)
        base->C1 = base->C1 | I2C_C1_TXAK_MASK;
      clear();
      x.data[n] = base->D;
    }
  }
  // Avoid I2C_MasterStop: some SDKs implement it as an infinite wait.
  base->C1 = base->C1 & static_cast<std::uint8_t>(~I2C_C1_MST_MASK);
  const auto stopped = poll_until(deadline, [&] { return (base->S & I2C_S_BUSY_MASK) ? status::busy : status::ok; });
  base->C1 = static_cast<std::uint8_t>(saved & ~(I2C_C1_MST_MASK | I2C_C1_TXAK_MASK | I2C_C1_IICIE_MASK));
  clear();
  return stopped != status::ok ? stopped : result;
}
#endif

#if MICROHAL_KINETIS_SDK_PRESENT && MICROHAL_KINETIS_HAS_LPI2C_DRIVER
template <typename Config, typename S> status poll_lpi2c(S& s) noexcept
{
  auto* base = s.base;
  auto& x = s.transfer;
  if (x.subaddressSize != 0U)
    return status::unsupported;
  // More than one receive command must be queued before data reception, or
  // the controller can NACK at a command boundary. Keep this single-command
  // engine explicit rather than silently splitting a bus transaction.
  if (x.direction == kLPI2C_Read && x.dataSize > 256U)
    return status::unsupported;
  if (LPI2C_MasterGetStatusFlags(base) & kLPI2C_MasterBusBusyFlag)
    return status::busy;
  poll_deadline<Config> deadline;
  base->MIER = 0U;
  base->MDER = 0U;
  base->MCR = base->MCR | LPI2C_MCR_RTF_MASK | LPI2C_MCR_RRF_MASK;
  LPI2C_MasterClearStatusFlags(base, kLPI2C_MasterClearFlags);
  const auto wait_flag = [&](std::uint32_t mask)
  {
    return poll_until(deadline,
                      [&]
                      {
                        const auto flags = LPI2C_MasterGetStatusFlags(base);
                        if (flags & kLPI2C_MasterPinLowTimeoutFlag)
                          return status::timeout;
                        if (flags & kLPI2C_MasterErrorFlags)
                          return status::io_error;
                        return (flags & mask) ? status::ok : status::busy;
                      });
  };
  const auto command = [&](std::uint32_t value)
  {
    const auto result = wait_flag(kLPI2C_MasterTxReadyFlag);
    if (result == status::ok)
      base->MTDR = value;
    return result;
  };
  auto result = command(LPI2C_MTDR_CMD(4U) | (x.slaveAddress << 1U) | (x.direction == kLPI2C_Read));
  for (std::size_t n = 0U; n != x.dataSize && result == status::ok;)
  {
    if (x.direction == kLPI2C_Write)
      result = command(x.data[n++]);
    else
    {
      const std::size_t count = (x.dataSize - n) > 256U ? 256U : x.dataSize - n;
      result = command(LPI2C_MTDR_CMD(1U) | (count - 1U));
      for (std::size_t i = 0U; i != count && result == status::ok; ++i)
      {
        result = wait_flag(kLPI2C_MasterRxReadyFlag);
        if (result == status::ok)
          x.data[n++] = static_cast<std::uint8_t>(base->MRDR);
      }
    }
  }
  if (result == status::ok)
    result = command(LPI2C_MTDR_CMD(2U));
  if (result == status::ok)
    result = wait_flag(kLPI2C_MasterStopDetectFlag);
  if (result != status::ok)
  {
    base->MCR = base->MCR & ~LPI2C_MCR_MEN_MASK;
    base->MCR = base->MCR | LPI2C_MCR_RTF_MASK | LPI2C_MCR_RRF_MASK;
    s.faulted = true;
  }
  LPI2C_MasterClearStatusFlags(base, kLPI2C_MasterClearFlags);
  return result;
}
#endif

#if MICROHAL_KINETIS_SERIAL_DSPI
template <typename Config, typename S> status poll_dspi(S& s) noexcept
{
  auto* base = s.base;
  auto& x = s.transfer;
  constexpr std::uint32_t allowed =
      (7U << DSPI_MASTER_CTAR_SHIFT) | (7U << DSPI_MASTER_PCS_SHIFT) | kDSPI_MasterPcsContinuous;
  if (x.configFlags & ~allowed)
    return status::unsupported;
  poll_deadline<Config> deadline;
  base->RSER = 0U;
  DSPI_StopTransfer(base);
  DSPI_FlushFifo(base, true, true);
  DSPI_ClearStatusFlags(base, kDSPI_TxCompleteFlag | kDSPI_EndOfQueueFlag | kDSPI_RxFifoOverflowFlag);
  dspi_command_data_config_t command{};
  DSPI_GetDefaultDataCommandConfig(&command);
  command.whichCtar = (x.configFlags >> DSPI_MASTER_CTAR_SHIFT) & 7U;
  command.whichPcs = 1U << ((x.configFlags >> DSPI_MASTER_PCS_SHIFT) & 7U);
  if (command.whichCtar != 0U)
    return status::unsupported;
  const auto wait_flag = [&](std::uint32_t mask)
  {
    return poll_until(deadline,
                      [&]
                      {
                        const auto flags = DSPI_GetStatusFlags(base);
                        if (flags & kDSPI_RxFifoOverflowFlag)
                          return status::io_error;
                        return (flags & mask) ? status::ok : status::busy;
                      });
  };
  DSPI_StartTransfer(base);
  status result = status::ok;
  for (std::size_t n = 0U; n != x.dataSize && result == status::ok; ++n)
  {
    result = wait_flag(kDSPI_TxFifoFillRequestFlag);
    if (result != status::ok)
      break;
    command.isPcsContinuous = (n + 1U != x.dataSize) && (x.configFlags & kDSPI_MasterPcsContinuous);
    command.isEndOfQueue = n + 1U == x.dataSize;
    DSPI_MasterWriteData(base, &command, x.txData == nullptr ? 0xFFU : x.txData[n]);
    DSPI_ClearStatusFlags(base, kDSPI_TxFifoFillRequestFlag);
    result = wait_flag(kDSPI_RxFifoDrainRequestFlag);
    if (result != status::ok)
      break;
    const auto value = DSPI_ReadData(base);
    if (x.rxData != nullptr)
      x.rxData[n] = static_cast<std::uint8_t>(value);
    DSPI_ClearStatusFlags(base, kDSPI_RxFifoDrainRequestFlag);
  }
  if (result == status::ok)
    result = wait_flag(kDSPI_EndOfQueueFlag);
  DSPI_StopTransfer(base);
  DSPI_FlushFifo(base, true, true);
  return result;
}
#endif

#if MICROHAL_KINETIS_SERIAL_LPSPI
template <typename Config, typename S> status poll_lpspi(S& s) noexcept
{
  auto* base = s.base;
  auto& x = s.transfer;
  constexpr std::uint32_t allowed = (3U << LPSPI_MASTER_PCS_SHIFT) | kLPSPI_MasterPcsContinuous;
  if (x.configFlags & ~allowed)
    return status::unsupported;
  poll_deadline<Config> deadline;
  if (LPSPI_GetStatusFlags(base) & kLPSPI_ModuleBusyFlag)
    return status::busy;
  base->IER = 0U;
  base->DER = 0U;
  LPSPI_FlushFifo(base, true, true);
  LPSPI_ClearStatusFlags(base, kLPSPI_TransferCompleteFlag | kLPSPI_TransmitErrorFlag | kLPSPI_ReceiveErrorFlag);
  const auto original = base->TCR;
  base->TCR = (original & ~(LPSPI_TCR_PCS_MASK | LPSPI_TCR_CONT_MASK | LPSPI_TCR_CONTC_MASK | LPSPI_TCR_TXMSK_MASK |
                            LPSPI_TCR_RXMSK_MASK)) |
              LPSPI_TCR_PCS((x.configFlags >> LPSPI_MASTER_PCS_SHIFT) & 3U) |
              ((x.configFlags & kLPSPI_MasterPcsContinuous) ? LPSPI_TCR_CONT_MASK : 0U);
  const auto wait_flag = [&](std::uint32_t mask)
  {
    return poll_until(deadline,
                      [&]
                      {
                        const auto flags = LPSPI_GetStatusFlags(base);
                        if (flags & (kLPSPI_TransmitErrorFlag | kLPSPI_ReceiveErrorFlag))
                          return status::io_error;
                        return (flags & mask) ? status::ok : status::busy;
                      });
  };
  status result = status::ok;
  for (std::size_t n = 0U; n != x.dataSize && result == status::ok; ++n)
  {
    result = wait_flag(kLPSPI_TxDataRequestFlag);
    if (result != status::ok)
      break;
    if (n + 1U == x.dataSize && (x.configFlags & kLPSPI_MasterPcsContinuous))
      base->TCR = (base->TCR & ~LPSPI_TCR_CONT_MASK) | (n != 0U ? LPSPI_TCR_CONTC_MASK : 0U);
    LPSPI_WriteData(base, x.txData == nullptr ? 0xFFU : x.txData[n]);
    result = wait_flag(kLPSPI_RxDataReadyFlag);
    if (result != status::ok)
      break;
    const auto value = LPSPI_ReadData(base);
    if (x.rxData != nullptr)
      x.rxData[n] = static_cast<std::uint8_t>(value);
  }
  if (result == status::ok)
    result = wait_flag(kLPSPI_TransferCompleteFlag);
  if (result == status::ok)
    result = poll_until(deadline, [&]
                        { return (LPSPI_GetStatusFlags(base) & kLPSPI_ModuleBusyFlag) ? status::busy : status::ok; });
  if (result != status::ok)
  {
    LPSPI_Enable(base, false);
    LPSPI_FlushFifo(base, true, true);
    LPSPI_Enable(base, true);
  }
  base->TCR = original;
  return result;
}
#endif

struct no_dma
{
};

template <typename S> void quiesce_i2c(S&) noexcept {}
template <typename Base> inline constexpr bool classic_i2c_base = false;
#if MICROHAL_KINETIS_SDK_PRESENT && MICROHAL_KINETIS_HAS_I2C_DRIVER
template <> inline constexpr bool classic_i2c_base<I2C_Type> = true;
template <typename S> void quiesce_classic_i2c(S& s) noexcept
{
  // SDK I2C_MasterTransferAbort may wait forever for IICIF. Disable local
  // access directly; DMA cancellation must already have stopped the channel.
  I2C_EnableDMA(s.base, false);
  s.base->C1 = static_cast<std::uint8_t>(s.base->C1 &
                                         ~(I2C_C1_IICIE_MASK | I2C_C1_MST_MASK | I2C_C1_TX_MASK | I2C_C1_TXAK_MASK));
  I2C_Enable(s.base, false);
  s.faulted = true;
  s.cancel_result = (s.base->S & I2C_S_BUSY_MASK) ? status::timeout : status::ok;
}
template <typename S>
void quiesce_i2c(S& s) noexcept
  requires std::is_same_v<decltype(s.base), I2C_Type*>
{
  quiesce_classic_i2c(s);
}
#endif

// ARMv6-M does not provide exclusive load/store instructions. Use the CMSIS
// interrupt mask instead of atomic RMWs that would require libatomic there.
struct interrupt_guard
{
#if MICROHAL_KINETIS_SDK_PRESENT && (defined(__arm__) || defined(__thumb__))
  std::uint32_t mask = __get_PRIMASK();
  bool acquired = true;
  interrupt_guard() noexcept
  {
    __disable_irq();
  }
  ~interrupt_guard()
  {
    __set_PRIMASK(mask);
  }
#else
  inline static std::atomic_flag lock = ATOMIC_FLAG_INIT;
  bool acquired = !lock.test_and_set(std::memory_order_acquire);
  ~interrupt_guard()
  {
    if (acquired)
      lock.clear(std::memory_order_release);
  }
#endif
};

// One slot per physical peripheral, not per Config or callback instantiation.
// The board must reserve this peripheral and its DMA channels for this engine;
// legacy SDK users must not replace its SDK handle while it is initialized.
template <typename Driver> struct engine
{
  using base_type = typename Driver::base_type;
  struct state
  {
    base_type* base{};
    const void* owner{};
    typename Driver::handle_type handle{};
    typename Driver::dma_handle_type dma_handle{};
    typename Driver::transfer_type transfer{};
    std::atomic<unsigned> phase{0U}; // idle, running, completed, cancelling
    status result{status::error};
    status cancel_result{status::ok};
    void (*callback)(status) noexcept {};
    transfer_mode mode{transfer_mode::automatic};
    bool initialized{};
    bool faulted{};
    bool reading{};
  };
  inline static std::array<state, 8> states{};
  static bool change_phase(state& s, unsigned& expected, unsigned desired) noexcept
  {
    interrupt_guard guard;
    if (!guard.acquired)
      return false;
    const auto phase = s.phase.load(std::memory_order_acquire);
    if (phase != expected)
    {
      expected = phase;
      return false;
    }
    s.phase.store(desired, std::memory_order_release);
    return true;
  }

  static state* find(base_type* base) noexcept
  {
    interrupt_guard guard;
    if (base == nullptr || !guard.acquired)
      return nullptr;
    state* result = nullptr;
    for (auto& s : states)
      if (s.base == base)
      {
        result = &s;
        break;
      }
    if (result == nullptr)
      for (auto& s : states)
        if (s.base == nullptr)
        {
          s.base = base;
          result = &s;
          break;
        }
    return result;
  }

  static void complete(state& s, status result) noexcept
  {
    if (s.phase.load(std::memory_order_acquire) != 1U)
      return;
    s.result = result;
    unsigned running = 1U;
    if (!change_phase(s, running, result == status::ok ? 2U : 3U))
      return;
    if (result != status::ok)
    {
      // UART error callbacks need not mean that the SDK has stopped its RX
      // engine. Quiesce IRQ/DMA access before releasing the caller's buffers.
      Driver::abort(s);
      if (s.cancel_result != status::ok)
        s.result = result = s.cancel_result;
      s.phase.store(2U, std::memory_order_release);
    }
    if (s.callback != nullptr)
    {
      const auto callback = s.callback;
      s.phase.store(0U, std::memory_order_release);
      callback(result);
    }
  }

  template <typename Binding> static state* owned() noexcept
  {
    auto* s = find(Binding::instance);
    return s != nullptr && s->owner == &owner_token<Binding> && s->initialized ? s : nullptr;
  }

  template <typename Config> static status initialize() noexcept
  {
    using B = typename Config::binding;
    constexpr auto mode = configured_transfer_mode<Config>();
    if constexpr (mode != transfer_mode::interrupt && mode != transfer_mode::dma &&
                  !(mode == transfer_mode::polled && Driver::polled))
      return status::unsupported;
    else
    {
      if constexpr (mode == transfer_mode::dma && !Driver::dma)
        return status::unsupported;
      auto* s = find(B::instance);
      if (s == nullptr)
        return B::instance == nullptr ? status::invalid_argument : status::busy;
      unsigned idle = 0U;
      if (!change_phase(*s, idle, 3U))
        return status::busy;
      s->initialized = false;
      const auto result = Driver::template initialize<Config>(*s);
      if (result == status::ok)
      {
        s->owner = &owner_token<B>;
        s->mode = mode;
        s->initialized = true;
        s->faulted = false;
        s->result = status::ok;
        s->cancel_result = status::ok;
      }
      s->phase.store(0U, std::memory_order_release);
      return result;
    }
  }

  template <typename Config, auto Handler, typename Prepare> static status run(Prepare prepare) noexcept
  {
    using B = typename Config::binding;
    constexpr bool synchronous = synchronous_handler<Handler>;
    constexpr auto mode = configured_transfer_mode<Config>();
    if constexpr (mode == transfer_mode::automatic ||
                  (mode == transfer_mode::polled && (!Driver::polled || !synchronous)) ||
                  (mode == transfer_mode::dma && !Driver::dma))
      return status::unsupported;
    else if constexpr (synchronous && !requires { B::now_ms(); })
      return status::unsupported;
    else
    {
      auto* s = owned<B>();
      if (s == nullptr || s->mode != mode || s->faulted)
        return status::invalid_argument;
      unsigned idle = 0U;
      if (!change_phase(*s, idle, 1U))
        return status::busy;
      s->callback = synchronous ? nullptr : &notify<Handler>;
      s->result = status::error;
      prepare(*s);
      if constexpr (mode == transfer_mode::polled)
      {
        const auto result = Driver::template poll<Config>(*s);
        s->result = result;
        s->phase.store(0U, std::memory_order_release);
        return result;
      }
      else
      {
        const auto started = Driver::start(*s);
        if (started != status::ok)
        {
          s->result = started;
          s->phase.store(0U, std::memory_order_release);
          return started;
        }
        if constexpr (!synchronous)
          return status::ok;
        else
        {
          const std::uint32_t start = B::now_ms();
          // A broken/stopped board clock must not turn a bounded operation into
          // an infinite loop. The fallback is a finite CPU iteration budget.
          std::uint32_t spins = 0U, last_tick = start;
          while (s->phase.load(std::memory_order_acquire) != 2U)
          {
            const std::uint32_t now = B::now_ms();
            if (now != last_tick)
            {
              last_tick = now;
              spins = 0U;
            }
            if (static_cast<std::uint32_t>(now - start) >= timeout_ms<Config>() || ++spins == 1'000'000U)
            {
              s->phase.store(3U, std::memory_order_release);
              Driver::abort(*s);
              s->result = status::timeout;
              s->phase.store(0U, std::memory_order_release);
              return status::timeout;
            }
            if constexpr (requires { B::idle(); })
              B::idle();
          }
          const auto result = s->result;
          s->phase.store(0U, std::memory_order_release);
          return result;
        }
      }
    }
  }

  template <typename Config> static status abort() noexcept
  {
    auto* s = owned<typename Config::binding>();
    if (s == nullptr)
      return status::invalid_argument;
    unsigned phase = s->phase.load(std::memory_order_acquire);
    if (phase == 0U)
      return status::ok;
    if (phase == 3U || !change_phase(*s, phase, 3U))
      return status::busy;
    Driver::abort(*s);
    s->result = s->cancel_result == status::ok ? status::error : s->cancel_result;
    s->phase.store(0U, std::memory_order_release);
    return s->cancel_result;
  }

  template <typename Config> static status transfer_status() noexcept
  {
    auto* s = owned<typename Config::binding>();
    if (s == nullptr || s->mode != configured_transfer_mode<Config>())
      return status::invalid_argument;
    const auto phase = s->phase.load(std::memory_order_acquire);
    return phase == 1U || phase == 3U ? status::busy : s->result;
  }

  template <typename Config> static status irq() noexcept
  {
    auto* s = owned<typename Config::binding>();
    if (s == nullptr || s->mode != transfer_mode::interrupt)
      return status::unsupported;
    Driver::irq(*s);
    return status::ok;
  }
};

#if MICROHAL_KINETIS_SDK_PRESENT
inline status map(status_t value) noexcept
{
  if (value == kStatus_Success)
    return status::ok;
  if (value == kStatus_InvalidArgument)
    return status::invalid_argument;
#if MICROHAL_KINETIS_HAS_LPI2C_DRIVER
  if (value == kStatus_LPI2C_PinLowTimeout)
    return status::timeout;
#endif
  return status::io_error;
}

template <typename SdkConfig> bool byte_spi_config(const SdkConfig& config) noexcept
{
  if constexpr (requires { config.ctarConfig.bitsPerFrame; })
    return config.ctarConfig.bitsPerFrame == 8U;
  else
    return config.bitsPerFrame == 8U;
}

template <typename C, typename SdkConfig> status stop_bits(SdkConfig& config) noexcept
{
  if constexpr (requires { C::stop_bits; })
  {
    if constexpr (C::stop_bits == uart_stop_bits::one_and_half)
      return status::unsupported;
    if constexpr (requires { config.stopBitCount; })
      config.stopBitCount = static_cast<decltype(config.stopBitCount)>(C::stop_bits == uart_stop_bits::two ? 1U : 0U);
    else if constexpr (C::stop_bits != uart_stop_bits::one)
      return status::unsupported;
  }
  return status::ok;
}

// Driver descriptors bind actual SDK handles and transfer records. They never
// delegate a transfer to a board callback.
#define MICROHAL_KINETIS_I2C_DRIVER(Name, Prefix, Base, Handle, Transfer, ConfigType, Baud, Read, Write, Busy, Poll)   \
  struct Name                                                                                                          \
  {                                                                                                                    \
    static constexpr auto kind = peripheral_kind::i2c;                                                                 \
    using base_type = Base;                                                                                            \
    using handle_type = Handle;                                                                                        \
    using transfer_type = Transfer;                                                                                    \
    static constexpr bool polled = true;                                                                               \
    template <typename C, typename S> static status poll(S& s) noexcept                                                \
    {                                                                                                                  \
      return Poll<C>(s);                                                                                               \
    }                                                                                                                  \
    MICROHAL_KINETIS_##Prefix##_DMA_TYPES template <typename C, typename S> static status initialize(S& s) noexcept    \
    {                                                                                                                  \
      using B = typename C::binding;                                                                                   \
      if constexpr (!requires { B::source_clock_hz; })                                                                 \
        return status::unsupported;                                                                                    \
      else                                                                                                             \
      {                                                                                                                \
        if (B::source_clock_hz == 0U || C::clock_hz == 0U)                                                             \
          return status::invalid_argument;                                                                             \
        ConfigType config{};                                                                                           \
        Prefix##_MasterGetDefaultConfig(&config);                                                                      \
        config.Baud = C::clock_hz;                                                                                     \
        Prefix##_MasterInit(s.base, &config, B::source_clock_hz);                                                      \
        if constexpr (configured_transfer_mode<C>() == transfer_mode::dma)                                             \
          return create_dma<B>(s);                                                                                     \
        else if constexpr (configured_transfer_mode<C>() == transfer_mode::interrupt)                                  \
          Prefix##_MasterTransferCreateHandle(                                                                         \
              s.base, &s.handle, [](Base*, Handle*, status_t result, void* context)                                    \
              { engine<Name>::complete(*static_cast<typename engine<Name>::state*>(context), convert(result)); }, &s); \
        return status::ok;                                                                                             \
      }                                                                                                                \
    }                                                                                                                  \
    static status convert(status_t value) noexcept                                                                     \
    {                                                                                                                  \
      if (value == Busy)                                                                                               \
        return status::busy;                                                                                           \
      if (value == kStatus_##Prefix##_Timeout)                                                                         \
        return status::timeout;                                                                                        \
      return map(value);                                                                                               \
    }                                                                                                                  \
    template <typename S> static status start(S& s) noexcept                                                           \
    {                                                                                                                  \
      if (s.mode == transfer_mode::dma)                                                                                \
        return start_dma(s);                                                                                           \
      return convert(Prefix##_MasterTransferNonBlocking(s.base, &s.handle, &s.transfer));                              \
    }                                                                                                                  \
    template <typename S> static void abort(S& s) noexcept                                                             \
    {                                                                                                                  \
      s.cancel_result = status::ok;                                                                                    \
      if (s.mode == transfer_mode::dma)                                                                                \
        abort_dma(s);                                                                                                  \
      else if constexpr (!classic_i2c_base<Base>)                                                                      \
        Prefix##_MasterTransferAbort(s.base, &s.handle);                                                               \
      quiesce_i2c(s);                                                                                                  \
    }                                                                                                                  \
    template <typename S> static void irq(S& s) noexcept                                                               \
    {                                                                                                                  \
      Prefix##_MasterTransferHandleIRQ(s.base, &s.handle);                                                             \
    }                                                                                                                  \
    template <typename S>                                                                                              \
    static void prepare(S& s, std::uint8_t* data, std::size_t count, std::uint16_t address, bool reading) noexcept     \
    {                                                                                                                  \
      s.transfer = {};                                                                                                 \
      s.transfer.slaveAddress = address;                                                                               \
      s.transfer.direction = reading ? Read : Write;                                                                   \
      s.transfer.data = data;                                                                                          \
      s.transfer.dataSize = count;                                                                                     \
      s.transfer.flags = 0U;                                                                                           \
    }                                                                                                                  \
  };

#define MICROHAL_KINETIS_NO_DMA                                                                                        \
  using dma_handle_type = no_dma;                                                                                      \
  static constexpr bool dma = false;                                                                                   \
  template <typename B, typename S> static status create_dma(S&) noexcept                                              \
  {                                                                                                                    \
    return status::unsupported;                                                                                        \
  }                                                                                                                    \
  template <typename S> static status start_dma(S&) noexcept                                                           \
  {                                                                                                                    \
    return status::unsupported;                                                                                        \
  }                                                                                                                    \
  template <typename S> static void abort_dma(S&) noexcept {}

#define MICROHAL_KINETIS_I2C_EDMA(Name, Prefix, Base, DmaHandle, Resource, Suffix, Create)                             \
  using dma_handle_type = DmaHandle;                                                                                   \
  static constexpr bool dma = true;                                                                                    \
  template <typename B, typename S> static status create_dma(S& s) noexcept                                            \
  {                                                                                                                    \
    if constexpr (requires { static_cast<Resource*>(B::dma_handle); })                                                 \
    {                                                                                                                  \
      if (B::dma_handle == nullptr)                                                                                    \
        return status::invalid_argument;                                                                               \
      Prefix##_##Create(                                                                                               \
          s.base, &s.dma_handle, [](Base*, DmaHandle*, status_t result, void* context)                                 \
          { engine<Name>::complete(*static_cast<typename engine<Name>::state*>(context), convert(result)); }, &s,      \
          B::dma_handle);                                                                                              \
      return status::ok;                                                                                               \
    }                                                                                                                  \
    return status::unsupported;                                                                                        \
  }                                                                                                                    \
  template <typename S> static status start_dma(S& s) noexcept                                                         \
  {                                                                                                                    \
    return convert(Prefix##_MasterTransfer##Suffix(s.base, &s.dma_handle, &s.transfer));                               \
  }                                                                                                                    \
  template <typename S> static void abort_dma(S& s) noexcept                                                           \
  {                                                                                                                    \
    Prefix##_MasterTransferAbort##Suffix(s.base, &s.dma_handle);                                                       \
  }

#if MICROHAL_KINETIS_SERIAL_I2C_EDMA
#define MICROHAL_KINETIS_I2C_DMA_TYPES                                                                                 \
  MICROHAL_KINETIS_I2C_EDMA(i2c_driver, I2C, I2C_Type, i2c_master_edma_handle_t, edma_handle_t, EDMA,                  \
                            MasterCreateEDMAHandle)
#elif MICROHAL_KINETIS_SERIAL_I2C_DMA
#define MICROHAL_KINETIS_I2C_DMA_TYPES                                                                                 \
  MICROHAL_KINETIS_I2C_EDMA(i2c_driver, I2C, I2C_Type, i2c_master_dma_handle_t, dma_handle_t, DMA,                     \
                            MasterTransferCreateHandleDMA)
#else
#define MICROHAL_KINETIS_I2C_DMA_TYPES MICROHAL_KINETIS_NO_DMA
#endif
#if MICROHAL_KINETIS_SERIAL_LPI2C_EDMA
#define MICROHAL_KINETIS_LPI2C_DMA_TYPES                                                                               \
  MICROHAL_KINETIS_I2C_EDMA(lpi2c_driver, LPI2C, LPI2C_Type, lpi2c_master_edma_handle_t, edma_handle_t, EDMA,          \
                            MasterCreateEDMAHandle)
#else
#define MICROHAL_KINETIS_LPI2C_DMA_TYPES MICROHAL_KINETIS_NO_DMA
#endif

#if MICROHAL_KINETIS_HAS_I2C_DRIVER
MICROHAL_KINETIS_I2C_DRIVER(i2c_driver, I2C, I2C_Type, i2c_master_handle_t, i2c_master_transfer_t, i2c_master_config_t,
                            baudRate_Bps, kI2C_Read, kI2C_Write, kStatus_I2C_Busy, poll_i2c)
inline i2c_driver select(I2C_Type*) noexcept
{
  return {};
}
#endif
#if MICROHAL_KINETIS_HAS_LPI2C_DRIVER
MICROHAL_KINETIS_I2C_DRIVER(lpi2c_driver, LPI2C, LPI2C_Type, lpi2c_master_handle_t, lpi2c_master_transfer_t,
                            lpi2c_master_config_t, baudRate_Hz, kLPI2C_Read, kLPI2C_Write, kStatus_LPI2C_Busy,
                            poll_lpi2c)
inline lpi2c_driver select(LPI2C_Type*) noexcept
{
  return {};
}
#endif
#undef MICROHAL_KINETIS_I2C_DRIVER
#undef MICROHAL_KINETIS_I2C_EDMA
#undef MICROHAL_KINETIS_I2C_DMA_TYPES
#undef MICROHAL_KINETIS_LPI2C_DMA_TYPES

#define MICROHAL_KINETIS_SPI_EDMA_BODY(Name, Prefix, Base, DmaHandle, Create)                                          \
  using dma_handle_type = DmaHandle;                                                                                   \
  static constexpr bool dma = true;                                                                                    \
  template <typename B, typename S> static status create_dma(S& s) noexcept                                            \
  {                                                                                                                    \
    if constexpr (requires {                                                                                           \
                    static_cast<edma_handle_t*>(B::rx_dma_handle);                                                     \
                    static_cast<edma_handle_t*>(B::tx_dma_handle);                                                     \
                  })                                                                                                   \
    {                                                                                                                  \
      if (B::rx_dma_handle == nullptr || B::tx_dma_handle == nullptr)                                                  \
        return status::invalid_argument;                                                                               \
      if (B::rx_dma_handle == B::tx_dma_handle)                                                                        \
        return status::invalid_argument;                                                                               \
      auto callback = +[](Base*, DmaHandle*, status_t result, void* context)                                           \
      { engine<Name>::complete(*static_cast<typename engine<Name>::state*>(context), convert(result)); };              \
      Create                                                                                                           \
    }                                                                                                                  \
    return status::unsupported;                                                                                        \
  }                                                                                                                    \
  template <typename S> static status start_dma(S& s) noexcept                                                         \
  {                                                                                                                    \
    return convert(Prefix##_MasterTransferEDMA(s.base, &s.dma_handle, &s.transfer));                                   \
  }                                                                                                                    \
  template <typename S> static void abort_dma(S& s) noexcept                                                           \
  {                                                                                                                    \
    Prefix##_MasterTransferAbortEDMA(s.base, &s.dma_handle);                                                           \
  }

#if MICROHAL_KINETIS_SERIAL_DSPI_EDMA
#define MICROHAL_KINETIS_DSPI_DMA_TYPES                                                                                \
  MICROHAL_KINETIS_SPI_EDMA_BODY(                                                                                      \
      dspi_driver, DSPI, SPI_Type, dspi_master_edma_handle_t,                                                          \
      if constexpr (requires { static_cast<edma_handle_t*>(B::intermediary_dma_handle); }) {                           \
        if (B::intermediary_dma_handle == nullptr)                                                                     \
          return status::invalid_argument;                                                                             \
        if (B::intermediary_dma_handle == B::rx_dma_handle || B::intermediary_dma_handle == B::tx_dma_handle)          \
          return status::invalid_argument;                                                                             \
        DSPI_MasterTransferCreateHandleEDMA(s.base, &s.dma_handle, callback, &s, B::rx_dma_handle,                     \
                                            B::intermediary_dma_handle, B::tx_dma_handle);                             \
        return status::ok;                                                                                             \
      })
#else
#define MICROHAL_KINETIS_DSPI_DMA_TYPES MICROHAL_KINETIS_NO_DMA
#endif
#if MICROHAL_KINETIS_SERIAL_LPSPI_EDMA
#define MICROHAL_KINETIS_LPSPI_DMA_TYPES                                                                               \
  MICROHAL_KINETIS_SPI_EDMA_BODY(                                                                                      \
      lpspi_driver, LPSPI, LPSPI_Type, lpspi_master_edma_handle_t,                                                     \
      LPSPI_MasterTransferCreateHandleEDMA(s.base, &s.dma_handle, callback, &s, B::rx_dma_handle, B::tx_dma_handle);   \
      return status::ok;)
#else
#define MICROHAL_KINETIS_LPSPI_DMA_TYPES MICROHAL_KINETIS_NO_DMA
#endif

#define MICROHAL_KINETIS_SPI_DRIVER(Name, Prefix, Base, Handle, Transfer, ConfigType, Busy, Poll, Setup)               \
  struct Name                                                                                                          \
  {                                                                                                                    \
    static constexpr auto kind = peripheral_kind::spi;                                                                 \
    using base_type = Base;                                                                                            \
    using handle_type = Handle;                                                                                        \
    using transfer_type = Transfer;                                                                                    \
    static constexpr bool polled = true;                                                                               \
    template <typename C, typename S> static status poll(S& s) noexcept                                                \
    {                                                                                                                  \
      return Poll<C>(s);                                                                                               \
    }                                                                                                                  \
    MICROHAL_KINETIS_##Prefix##_DMA_TYPES template <typename C, typename S> static status initialize(S& s) noexcept    \
    {                                                                                                                  \
      using B = typename C::binding;                                                                                   \
      if constexpr (!requires { B::source_clock_hz; })                                                                 \
        return status::unsupported;                                                                                    \
      else                                                                                                             \
      {                                                                                                                \
        if (B::source_clock_hz == 0U || C::clock_hz == 0U)                                                             \
          return status::invalid_argument;                                                                             \
        ConfigType config{};                                                                                           \
        Prefix##_MasterGetDefaultConfig(&config);                                                                      \
        Setup if (!byte_spi_config(config)) return status::unsupported;                                                \
        Prefix##_MasterInit(s.base, &config, B::source_clock_hz);                                                      \
        if constexpr (configured_transfer_mode<C>() == transfer_mode::dma)                                             \
          return create_dma<B>(s);                                                                                     \
        else if constexpr (configured_transfer_mode<C>() == transfer_mode::interrupt)                                  \
          Prefix##_MasterTransferCreateHandle(                                                                         \
              s.base, &s.handle, [](Base*, Handle*, status_t result, void* context)                                    \
              { engine<Name>::complete(*static_cast<typename engine<Name>::state*>(context), convert(result)); }, &s); \
        return status::ok;                                                                                             \
      }                                                                                                                \
    }                                                                                                                  \
    static status convert(status_t value) noexcept                                                                     \
    {                                                                                                                  \
      if (value == Busy)                                                                                               \
        return status::busy;                                                                                           \
      return map(value);                                                                                               \
    }                                                                                                                  \
    template <typename S> static status start(S& s) noexcept                                                           \
    {                                                                                                                  \
      if (s.mode == transfer_mode::dma)                                                                                \
        return start_dma(s);                                                                                           \
      return convert(Prefix##_MasterTransferNonBlocking(s.base, &s.handle, &s.transfer));                              \
    }                                                                                                                  \
    template <typename S> static void abort(S& s) noexcept                                                             \
    {                                                                                                                  \
      if (s.mode == transfer_mode::dma)                                                                                \
        abort_dma(s);                                                                                                  \
      else                                                                                                             \
        Prefix##_MasterTransferAbort(s.base, &s.handle);                                                               \
    }                                                                                                                  \
    template <typename S> static void irq(S& s) noexcept                                                               \
    {                                                                                                                  \
      Prefix##_MasterTransferHandleIRQ(s.base, &s.handle);                                                             \
    }                                                                                                                  \
  };

#if MICROHAL_KINETIS_SERIAL_DSPI
MICROHAL_KINETIS_SPI_DRIVER(
    dspi_driver, DSPI, SPI_Type, dspi_master_handle_t, dspi_transfer_t, dspi_master_config_t, kStatus_DSPI_Busy,
    poll_dspi, config.ctarConfig.baudRate = C::clock_hz;
    config.ctarConfig.bitsPerFrame = 8U;
    if constexpr (requires { C::clock_polarity; })
        config.ctarConfig.cpol = C::clock_polarity == spi_clock_polarity::idle_high ? kDSPI_ClockPolarityActiveLow
                                                                                    : kDSPI_ClockPolarityActiveHigh;
    if constexpr (requires { C::clock_phase; }) config.ctarConfig.cpha = C::clock_phase == spi_clock_phase::second_edge
                                                                             ? kDSPI_ClockPhaseSecondEdge
                                                                             : kDSPI_ClockPhaseFirstEdge;
    if constexpr (requires { C::bit_order; })
        config.ctarConfig.direction = C::bit_order == spi_bit_order::lsb_first ? kDSPI_LsbFirst : kDSPI_MsbFirst;
    if constexpr (requires { B::sdk_config; }) config = B::sdk_config;)
inline dspi_driver select(SPI_Type*) noexcept
{
  return {};
}
#endif
#if MICROHAL_KINETIS_SERIAL_LPSPI
MICROHAL_KINETIS_SPI_DRIVER(
    lpspi_driver, LPSPI, LPSPI_Type, lpspi_master_handle_t, lpspi_transfer_t, lpspi_master_config_t, kStatus_LPSPI_Busy,
    poll_lpspi, config.baudRate = C::clock_hz;
    config.bitsPerFrame = 8U;
    if constexpr (requires { C::clock_polarity; }) config.cpol = C::clock_polarity == spi_clock_polarity::idle_high
                                                                     ? kLPSPI_ClockPolarityActiveLow
                                                                     : kLPSPI_ClockPolarityActiveHigh;
    if constexpr (requires { C::clock_phase; }) config.cpha = C::clock_phase == spi_clock_phase::second_edge
                                                                  ? kLPSPI_ClockPhaseSecondEdge
                                                                  : kLPSPI_ClockPhaseFirstEdge;
    if constexpr (requires { C::bit_order; })
        config.direction = C::bit_order == spi_bit_order::lsb_first ? kLPSPI_LsbFirst : kLPSPI_MsbFirst;
    if constexpr (requires { B::sdk_config; }) config = B::sdk_config;)
inline lpspi_driver select(LPSPI_Type*) noexcept
{
  return {};
}
#endif
#undef MICROHAL_KINETIS_SPI_DRIVER
#undef MICROHAL_KINETIS_SPI_EDMA_BODY
#undef MICROHAL_KINETIS_DSPI_DMA_TYPES
#undef MICROHAL_KINETIS_LPSPI_DMA_TYPES

#define MICROHAL_KINETIS_UART_EDMA(Name, Prefix, Base, DmaHandle, Resource, Suffix, Send, Receive)                     \
  using dma_handle_type = DmaHandle;                                                                                   \
  static constexpr bool dma = true;                                                                                    \
  template <typename B, typename S> static status create_dma(S& s) noexcept                                            \
  {                                                                                                                    \
    if constexpr (requires {                                                                                           \
                    static_cast<Resource*>(B::tx_dma_handle);                                                          \
                    static_cast<Resource*>(B::rx_dma_handle);                                                          \
                  })                                                                                                   \
    {                                                                                                                  \
      if (B::tx_dma_handle == nullptr || B::rx_dma_handle == nullptr)                                                  \
        return status::invalid_argument;                                                                               \
      if (B::tx_dma_handle == B::rx_dma_handle)                                                                        \
        return status::invalid_argument;                                                                               \
      Prefix##_TransferCreateHandle##Suffix(                                                                           \
          s.base, &s.dma_handle,                                                                                       \
          [](Base*, DmaHandle*, status_t result, void* context)                                                        \
          {                                                                                                            \
            if (result == kStatus_##Prefix##_TxIdle || result == kStatus_##Prefix##_RxIdle ||                          \
                result == kStatus_Success)                                                                             \
              engine<Name>::complete(*static_cast<typename engine<Name>::state*>(context), status::ok);                \
            else                                                                                                       \
              engine<Name>::complete(*static_cast<typename engine<Name>::state*>(context), convert(result));           \
          },                                                                                                           \
          &s, B::tx_dma_handle, B::rx_dma_handle);                                                                     \
      return status::ok;                                                                                               \
    }                                                                                                                  \
    return status::unsupported;                                                                                        \
  }                                                                                                                    \
  template <typename S> static status start_dma(S& s) noexcept                                                         \
  {                                                                                                                    \
    return convert(s.reading ? Prefix##_##Receive(s.base, &s.dma_handle, &s.transfer)                                  \
                             : Prefix##_##Send(s.base, &s.dma_handle, &s.transfer));                                   \
  }                                                                                                                    \
  template <typename S> static void abort_dma(S& s) noexcept                                                           \
  {                                                                                                                    \
    if (s.reading)                                                                                                     \
      Prefix##_TransferAbortReceive##Suffix(s.base, &s.dma_handle);                                                    \
    else                                                                                                               \
      Prefix##_TransferAbortSend##Suffix(s.base, &s.dma_handle);                                                       \
  }
#if MICROHAL_KINETIS_SERIAL_UART_EDMA
#define MICROHAL_KINETIS_UART_DMA_TYPES                                                                                \
  MICROHAL_KINETIS_UART_EDMA(uart_driver, UART, UART_Type, uart_edma_handle_t, edma_handle_t, EDMA, SendEDMA,          \
                             ReceiveEDMA)
#elif MICROHAL_KINETIS_SERIAL_UART_DMA
#define MICROHAL_KINETIS_UART_DMA_TYPES                                                                                \
  MICROHAL_KINETIS_UART_EDMA(uart_driver, UART, UART_Type, uart_dma_handle_t, dma_handle_t, DMA, TransferSendDMA,      \
                             TransferReceiveDMA)
#else
#define MICROHAL_KINETIS_UART_DMA_TYPES MICROHAL_KINETIS_NO_DMA
#endif
#if MICROHAL_KINETIS_SERIAL_LPUART_EDMA
#define MICROHAL_KINETIS_LPUART_DMA_TYPES                                                                              \
  MICROHAL_KINETIS_UART_EDMA(lpuart_driver, LPUART, LPUART_Type, lpuart_edma_handle_t, edma_handle_t, EDMA, SendEDMA,  \
                             ReceiveEDMA)
#elif MICROHAL_KINETIS_SERIAL_LPUART_DMA
#define MICROHAL_KINETIS_LPUART_DMA_TYPES                                                                              \
  MICROHAL_KINETIS_UART_EDMA(lpuart_driver, LPUART, LPUART_Type, lpuart_dma_handle_t, dma_handle_t, DMA,               \
                             TransferSendDMA, TransferReceiveDMA)
#else
#define MICROHAL_KINETIS_LPUART_DMA_TYPES MICROHAL_KINETIS_NO_DMA
#endif

#define MICROHAL_KINETIS_UART_DRIVER(Name, Prefix, Base, Handle, Transfer, ConfigType)                                 \
  struct Name                                                                                                          \
  {                                                                                                                    \
    static constexpr auto kind = peripheral_kind::uart;                                                                \
    using base_type = Base;                                                                                            \
    using handle_type = Handle;                                                                                        \
    using transfer_type = Transfer;                                                                                    \
    static constexpr bool polled = true;                                                                               \
    MICROHAL_KINETIS_##Prefix##_DMA_TYPES static status convert(status_t value) noexcept                               \
    {                                                                                                                  \
      if (value == kStatus_##Prefix##_TxBusy || value == kStatus_##Prefix##_RxBusy)                                    \
        return status::busy;                                                                                           \
      return map(value);                                                                                               \
    }                                                                                                                  \
    template <typename C, typename S> static status initialize(S& s) noexcept                                          \
    {                                                                                                                  \
      using B = typename C::binding;                                                                                   \
      if constexpr (!requires { B::source_clock_hz; })                                                                 \
        return status::unsupported;                                                                                    \
      else                                                                                                             \
      {                                                                                                                \
        if (B::source_clock_hz == 0U || C::baud_rate == 0U)                                                            \
          return status::invalid_argument;                                                                             \
        ConfigType config{};                                                                                           \
        Prefix##_GetDefaultConfig(&config);                                                                            \
        config.baudRate_Bps = C::baud_rate;                                                                            \
        config.enableTx = true;                                                                                        \
        config.enableRx = true;                                                                                        \
        if constexpr (requires { C::parity; })                                                                         \
          config.parityMode = C::parity == uart_parity::even  ? k##Prefix##_ParityEven                                 \
                              : C::parity == uart_parity::odd ? k##Prefix##_ParityOdd                                  \
                                                              : k##Prefix##_ParityDisabled;                            \
        const auto framing = stop_bits<C>(config);                                                                     \
        if (framing != status::ok)                                                                                     \
          return framing;                                                                                              \
        if constexpr (requires { B::sdk_config; })                                                                     \
          config = B::sdk_config;                                                                                      \
        const auto result = convert(Prefix##_Init(s.base, &config, B::source_clock_hz));                               \
        if (result != status::ok)                                                                                      \
          return result;                                                                                               \
        if constexpr (configured_transfer_mode<C>() == transfer_mode::dma)                                             \
          return create_dma<B>(s);                                                                                     \
        else if constexpr (configured_transfer_mode<C>() == transfer_mode::interrupt)                                  \
          Prefix##_TransferCreateHandle(                                                                               \
              s.base, &s.handle,                                                                                       \
              [](Base*, Handle*, status_t result, void* context)                                                       \
              {                                                                                                        \
                if (result == kStatus_##Prefix##_TxIdle || result == kStatus_##Prefix##_RxIdle)                        \
                  engine<Name>::complete(*static_cast<typename engine<Name>::state*>(context), status::ok);            \
                else                                                                                                   \
                  engine<Name>::complete(*static_cast<typename engine<Name>::state*>(context), convert(result));       \
              },                                                                                                       \
              &s);                                                                                                     \
        return status::ok;                                                                                             \
      }                                                                                                                \
    }                                                                                                                  \
    template <typename S> static status start(S& s) noexcept                                                           \
    {                                                                                                                  \
      if (s.mode == transfer_mode::dma)                                                                                \
        return start_dma(s);                                                                                           \
      return convert(s.reading ? Prefix##_TransferReceiveNonBlocking(s.base, &s.handle, &s.transfer, nullptr)          \
                               : Prefix##_TransferSendNonBlocking(s.base, &s.handle, &s.transfer));                    \
    }                                                                                                                  \
    template <typename S> static void abort(S& s) noexcept                                                             \
    {                                                                                                                  \
      if (s.mode == transfer_mode::dma)                                                                                \
        abort_dma(s);                                                                                                  \
      else if (s.reading)                                                                                              \
        Prefix##_TransferAbortReceive(s.base, &s.handle);                                                              \
      else                                                                                                             \
        Prefix##_TransferAbortSend(s.base, &s.handle);                                                                 \
    }                                                                                                                  \
    template <typename S> static void irq(S& s) noexcept                                                               \
    {                                                                                                                  \
      Prefix##_TransferHandleIRQ(s.base, &s.handle);                                                                   \
    }                                                                                                                  \
    template <typename C, typename S> static status poll(S& s) noexcept                                                \
    {                                                                                                                  \
      using B = typename C::binding;                                                                                   \
      const std::uint32_t start = B::now_ms();                                                                         \
      std::size_t offset = 0U;                                                                                         \
      std::uint32_t spins = 0U, last_tick = start;                                                                     \
      while (offset != s.transfer.dataSize ||                                                                          \
             (!s.reading && !(Prefix##_GetStatusFlags(s.base) & k##Prefix##_TransmissionCompleteFlag)))                \
      {                                                                                                                \
        const auto flags = Prefix##_GetStatusFlags(s.base);                                                            \
        if (flags & (k##Prefix##_RxOverrunFlag | k##Prefix##_NoiseErrorFlag | k##Prefix##_FramingErrorFlag |           \
                     k##Prefix##_ParityErrorFlag))                                                                     \
          return status::io_error;                                                                                     \
        const std::uint32_t now = B::now_ms();                                                                         \
        if (now != last_tick)                                                                                          \
        {                                                                                                              \
          last_tick = now;                                                                                             \
          spins = 0U;                                                                                                  \
        }                                                                                                              \
        if (static_cast<std::uint32_t>(now - start) >= timeout_ms<C>() || ++spins == 1'000'000U)                       \
          return status::timeout;                                                                                      \
        if (offset != s.transfer.dataSize)                                                                             \
        {                                                                                                              \
          if (s.reading && (flags & k##Prefix##_RxDataRegFullFlag))                                                    \
            s.transfer.data[offset++] = Prefix##_ReadByte(s.base);                                                     \
          else if (!s.reading && (flags & k##Prefix##_TxDataRegEmptyFlag))                                             \
            Prefix##_WriteByte(s.base, s.transfer.data[offset++]);                                                     \
        }                                                                                                              \
        if constexpr (requires { B::idle(); })                                                                         \
          B::idle();                                                                                                   \
      }                                                                                                                \
      return status::ok;                                                                                               \
    }                                                                                                                  \
  };
#if MICROHAL_KINETIS_HAS_UART_DRIVER
MICROHAL_KINETIS_UART_DRIVER(uart_driver, UART, UART_Type, uart_handle_t, uart_transfer_t, uart_config_t)
inline uart_driver select(UART_Type*) noexcept
{
  return {};
}
#endif
#if MICROHAL_KINETIS_HAS_LPUART_DRIVER
MICROHAL_KINETIS_UART_DRIVER(lpuart_driver, LPUART, LPUART_Type, lpuart_handle_t, lpuart_transfer_t, lpuart_config_t)
inline lpuart_driver select(LPUART_Type*) noexcept
{
  return {};
}
#endif
#undef MICROHAL_KINETIS_UART_DRIVER
#undef MICROHAL_KINETIS_UART_EDMA
#undef MICROHAL_KINETIS_UART_DMA_TYPES
#undef MICROHAL_KINETIS_LPUART_DMA_TYPES
#undef MICROHAL_KINETIS_NO_DMA

#if MICROHAL_KINETIS_SERIAL_SAI
// The modern transceiver API is required, rather than the obsolete
// SAI_Transfer*SetFormat API. Older SDKs must use a board adapter.
#if defined(FSL_SAI_DRIVER_VERSION) && (FSL_SAI_DRIVER_VERSION >= MAKE_VERSION(2, 4, 0))
#define MICROHAL_KINETIS_SERIAL_SAI_ENGINE 1
struct sai_driver
{
  static constexpr auto kind = peripheral_kind::i2s;
  using base_type = I2S_Type;
  using handle_type = sai_handle_t;
  using transfer_type = sai_transfer_t;
  static constexpr bool polled = true;
#if MICROHAL_KINETIS_SERIAL_SAI_EDMA
  using dma_handle_type = sai_edma_handle_t;
  static constexpr bool dma = true;
  template <typename B, typename S> static status create_dma(S& s) noexcept
  {
    if constexpr (requires { static_cast<edma_handle_t*>(B::dma_handle); })
    {
      if (B::dma_handle == nullptr)
        return status::invalid_argument;
      auto callback = +[](I2S_Type*, sai_edma_handle_t*, status_t result, void* context)
      { engine<sai_driver>::complete(*static_cast<engine<sai_driver>::state*>(context), convert(result)); };
      if (s.reading)
        SAI_TransferRxCreateHandleEDMA(s.base, &s.dma_handle, callback, &s, B::dma_handle);
      else
        SAI_TransferTxCreateHandleEDMA(s.base, &s.dma_handle, callback, &s, B::dma_handle);
      return status::ok;
    }
    return status::unsupported;
  }
  template <typename S> static status start_dma(S& s) noexcept
  {
    return convert(s.reading ? SAI_TransferReceiveEDMA(s.base, &s.dma_handle, &s.transfer)
                             : SAI_TransferSendEDMA(s.base, &s.dma_handle, &s.transfer));
  }
  template <typename S> static void abort_dma(S& s) noexcept
  {
    if (s.reading)
      SAI_TransferTerminateReceiveEDMA(s.base, &s.dma_handle);
    else
      SAI_TransferTerminateSendEDMA(s.base, &s.dma_handle);
  }
#else
  using dma_handle_type = no_dma;
  static constexpr bool dma = false;
  template <typename B, typename S> static status create_dma(S&) noexcept
  {
    return status::unsupported;
  }
  template <typename S> static status start_dma(S&) noexcept
  {
    return status::unsupported;
  }
  template <typename S> static void abort_dma(S&) noexcept {}
#endif
  static status convert(status_t value) noexcept
  {
    if (value == kStatus_SAI_TxIdle || value == kStatus_SAI_RxIdle)
      return status::ok;
    if (value == kStatus_SAI_TxBusy || value == kStatus_SAI_RxBusy || value == kStatus_SAI_QueueFull)
      return status::busy;
    return map(value);
  }
  template <typename C, typename S> static status initialize(S& s) noexcept
  {
    using B = typename C::binding;
    if constexpr (C::standard != i2s_standard::philips || C::enable_master_clock)
      return status::unsupported;
    else if constexpr (!requires { B::source_clock_hz; })
      return status::unsupported;
    else
    {
      if (B::source_clock_hz == 0U || C::sample_rate == 0U)
        return status::invalid_argument;
      s.reading = C::mode == i2s_mode::master_rx || C::mode == i2s_mode::slave_rx;
      SAI_Init(s.base);
      sai_transceiver_t config{};
      SAI_GetClassicI2SConfig(&config, kSAI_WordWidth16bits, kSAI_Stereo, kSAI_Channel0Mask);
      config.masterSlave =
          (C::mode == i2s_mode::master_tx || C::mode == i2s_mode::master_rx) ? kSAI_Master : kSAI_Slave;
      config.syncMode = kSAI_ModeAsync;
      if constexpr (configured_transfer_mode<C>() == transfer_mode::dma)
      {
        const auto result = create_dma<B>(s);
        if (result != status::ok)
          return result;
#if MICROHAL_KINETIS_SERIAL_SAI_EDMA
        if (s.reading)
          SAI_TransferRxSetConfigEDMA(s.base, &s.dma_handle, &config);
        else
          SAI_TransferTxSetConfigEDMA(s.base, &s.dma_handle, &config);
#endif
      }
      else if constexpr (configured_transfer_mode<C>() == transfer_mode::polled)
      {
        if (s.reading)
          SAI_RxSetConfig(s.base, &config);
        else
          SAI_TxSetConfig(s.base, &config);
      }
      else
      {
        auto callback = +[](I2S_Type*, sai_handle_t*, status_t result, void* context)
        { engine<sai_driver>::complete(*static_cast<engine<sai_driver>::state*>(context), convert(result)); };
        if (s.reading)
        {
          SAI_TransferRxCreateHandle(s.base, &s.handle, callback, &s);
          SAI_TransferRxSetConfig(s.base, &s.handle, &config);
        }
        else
        {
          SAI_TransferTxCreateHandle(s.base, &s.handle, callback, &s);
          SAI_TransferTxSetConfig(s.base, &s.handle, &config);
        }
      }
      if constexpr (C::mode == i2s_mode::master_tx)
        SAI_TxSetBitClockRate(s.base, B::source_clock_hz, C::sample_rate, 16U, 2U);
      if constexpr (C::mode == i2s_mode::master_rx)
        SAI_RxSetBitClockRate(s.base, B::source_clock_hz, C::sample_rate, 16U, 2U);
      return status::ok;
    }
  }
  template <typename S> static status start(S& s) noexcept
  {
    if (s.mode == transfer_mode::dma)
      return start_dma(s);
    return convert(s.reading ? SAI_TransferReceiveNonBlocking(s.base, &s.handle, &s.transfer)
                             : SAI_TransferSendNonBlocking(s.base, &s.handle, &s.transfer));
  }
  template <typename C, typename S> static status poll(S& s) noexcept
  {
    poll_deadline<C> deadline;
    // Configured channel 0, asynchronous TX/RX: neither request source is
    // enabled. FWF means TX empty / RX nonempty, safe for one word even on
    // a single-register implementation without a FIFO.
    constexpr std::uint32_t requests = I2S_TCSR_WSIE_MASK | I2S_TCSR_SEIE_MASK | I2S_TCSR_FWIE_MASK |
                                       I2S_TCSR_FEIE_MASK | I2S_TCSR_FRIE_MASK | I2S_TCSR_FWDE_MASK |
                                       I2S_TCSR_FRDE_MASK;
    if (s.reading)
    {
      s.base->RCSR = s.base->RCSR & ~requests;
      SAI_RxEnable(s.base, true);
    }
    else
    {
      s.base->TCSR = s.base->TCSR & ~requests;
      SAI_TxEnable(s.base, true);
    }
    auto* words = reinterpret_cast<std::uint16_t*>(s.transfer.data);
    status result = status::ok;
    for (std::size_t n = 0U; n != s.transfer.dataSize / 2U && result == status::ok; ++n)
    {
      result = poll_until(deadline,
                          [&]
                          {
                            const auto flags = s.reading ? SAI_RxGetStatusFlag(s.base) : SAI_TxGetStatusFlag(s.base);
                            if (flags & kSAI_FIFOErrorFlag)
                              return status::io_error;
                            return (flags & kSAI_FIFOWarningFlag) ? status::ok : status::busy;
                          });
      if (result != status::ok)
        break;
      if (s.reading)
        words[n] = static_cast<std::uint16_t>(s.base->RDR[0]);
      else
        s.base->TDR[0] = words[n];
    }
    // Match the native transactional contract: successful TX means the
    // buffer has been copied to hardware, not that the last bit left the pin.
    if (s.reading)
      SAI_RxEnable(s.base, false);
    else if (result != status::ok)
      SAI_TxEnable(s.base, false);
    if (result != status::ok)
    {
      if (s.reading)
        SAI_RxSoftwareReset(s.base, kSAI_ResetTypeFIFO);
      else
        SAI_TxSoftwareReset(s.base, kSAI_ResetTypeFIFO);
    }
    return result;
  }
  template <typename S> static void abort(S& s) noexcept
  {
    if (s.mode == transfer_mode::dma)
      abort_dma(s);
    else if (s.reading)
      SAI_TransferTerminateReceive(s.base, &s.handle);
    else
      SAI_TransferTerminateSend(s.base, &s.handle);
  }
  template <typename S> static void irq(S& s) noexcept
  {
    if (s.reading)
      SAI_TransferRxHandleIRQ(s.base, &s.handle);
    else
      SAI_TransferTxHandleIRQ(s.base, &s.handle);
  }
};
inline sai_driver select(I2S_Type*) noexcept
{
  return {};
}
#else
#define MICROHAL_KINETIS_SERIAL_SAI_ENGINE 0
#endif
#else
#define MICROHAL_KINETIS_SERIAL_SAI_ENGINE 0
#endif
#else
#define MICROHAL_KINETIS_SERIAL_SAI_ENGINE 0
#endif
} // namespace microhal::backends::kinetis::espresso::serial

namespace microhal::backends::kinetis::espresso::serial
{
template <typename Config, typename = void> struct driver_for
{
  using type = unavailable;
};
template <typename Config> struct driver_for<Config, std::void_t<decltype(Config::binding::instance)>>
{
  using type = decltype(select(Config::binding::instance));
};
template <typename Config> using driver_t = typename driver_for<Config>::type;
template <typename Config, peripheral_kind Kind>
inline constexpr bool matches = []
{
  if constexpr (std::is_same_v<driver_t<Config>, unavailable>)
    return false;
  else
    return driver_t<Config>::kind == Kind;
}();

template <peripheral_kind Kind> struct api
{
  template <typename Config, auto Handler> static status start_receive_to_idle(std::span<std::uint8_t>) noexcept
  {
    return status::unsupported;
  }
  template <typename Config, auto Handler> static status start_circular_rx(std::span<std::uint8_t>) noexcept
  {
    return status::unsupported;
  }
  template <typename Config> static status abort_tx() noexcept
  {
    return status::unsupported;
  }
  template <typename Config> static status abort_rx() noexcept
  {
    return status::unsupported;
  }
  template <typename Config> static status tx_status() noexcept
  {
    return status::unsupported;
  }
  template <typename Config> static status rx_status() noexcept
  {
    return status::unsupported;
  }
  template <typename Config> static status handle_dma_interrupt() noexcept
  {
    return status::unsupported;
  }
  template <typename Config> static status initialize() noexcept
  {
    if constexpr (!matches<Config, Kind> || serial_slave_enabled<Config>())
      return status::unsupported;
    else
      return engine<driver_t<Config>>::template initialize<Config>();
  }
  template <typename Config> static status abort() noexcept
  {
    if constexpr (!matches<Config, Kind> || serial_slave_enabled<Config>())
      return status::unsupported;
    else
      return engine<driver_t<Config>>::template abort<Config>();
  }
  template <typename Config> static status transfer_status() noexcept
  {
    if constexpr (!matches<Config, Kind> || serial_slave_enabled<Config>())
      return status::unsupported;
    else
      return engine<driver_t<Config>>::template transfer_status<Config>();
  }
  template <typename Config> static status irq() noexcept
  {
    if constexpr (!matches<Config, Kind> || serial_slave_enabled<Config>())
      return status::unsupported;
    else
      return engine<driver_t<Config>>::template irq<Config>();
  }
  template <typename Config> static status handle_interrupt() noexcept
  {
    return irq<Config>();
  }
  template <typename Config> static status probe(std::uint16_t) noexcept
  {
    return status::unsupported;
  }
  template <typename Config, auto Handler>
  static status write(std::span<const std::uint8_t> data, std::uint16_t address = 0U) noexcept
  {
    if constexpr (Kind == peripheral_kind::spi)
      return transfer<Config, Handler>(data, {});
    else
      return bytes<Config, Handler>(const_cast<std::uint8_t*>(data.data()), data.size(), address, false);
  }
  template <typename Config, auto Handler>
  static status read(std::span<std::uint8_t> data, std::uint16_t address = 0U) noexcept
  {
    if constexpr (Kind == peripheral_kind::spi)
      return transfer<Config, Handler>({}, data);
    else
      return bytes<Config, Handler>(data.data(), data.size(), address, true);
  }
  template <typename Config, auto Handler>
  static status bytes(std::uint8_t* data, std::size_t count, std::uint16_t address, bool reading) noexcept
  {
    if constexpr (!matches<Config, Kind> || serial_slave_enabled<Config>() ||
                  (Kind != peripheral_kind::uart && Kind != peripheral_kind::i2c))
      return status::unsupported;
    else
    {
      if (count == 0U || data == nullptr)
        return status::invalid_argument;
      if constexpr (Kind == peripheral_kind::i2c)
      {
        if constexpr (requires { Config::address_mode; })
          if constexpr (Config::address_mode != i2c_address_mode::seven_bit)
            return status::unsupported;
        if (address > 0x7FU)
          return status::invalid_argument;
      }
      return engine<driver_t<Config>>::template run<Config, Handler>(
          [&](auto& s)
          {
            if constexpr (Kind == peripheral_kind::i2c)
              driver_t<Config>::prepare(s, data, count, address, reading);
            else
            {
              s.reading = reading;
              s.transfer = {};
              s.transfer.data = data;
              s.transfer.dataSize = count;
            }
          });
    }
  }
  template <typename Config, auto Handler>
  static status transfer(std::span<const std::uint8_t> tx, std::span<std::uint8_t> rx) noexcept
  {
    if constexpr (Kind != peripheral_kind::spi || !matches<Config, Kind> || serial_slave_enabled<Config>())
      return status::unsupported;
    else
    {
      if ((tx.empty() && rx.empty()) || (!tx.empty() && !rx.empty() && tx.size() != rx.size()))
        return status::invalid_argument;
      return engine<driver_t<Config>>::template run<Config, Handler>(
          [&](auto& s)
          {
            s.transfer = {};
            s.transfer.txData = tx.empty() ? nullptr : const_cast<std::uint8_t*>(tx.data());
            s.transfer.rxData = rx.empty() ? nullptr : rx.data();
            s.transfer.dataSize = tx.empty() ? rx.size() : tx.size();
            if constexpr (requires { Config::binding::transfer_flags; })
              s.transfer.configFlags = Config::binding::transfer_flags;
          });
    }
  }
  template <typename Config, auto Handler>
  static status write_read(std::span<const std::uint8_t> tx, std::span<std::uint8_t> rx, std::uint16_t address) noexcept
  {
    // The SDK's subaddress field gives an atomic repeated-start transaction
    // without an IRQ/DMA gap or an unsafe caller-owned descriptor chain.
    if constexpr (Kind != peripheral_kind::i2c || !matches<Config, Kind> || serial_slave_enabled<Config>())
      return status::unsupported;
    else
    {
      if constexpr (requires { Config::address_mode; })
        if constexpr (Config::address_mode != i2c_address_mode::seven_bit)
          return status::unsupported;
      if (tx.empty() || rx.empty() || address > 0x7FU)
        return status::invalid_argument;
      if (tx.size() > 4U)
        return status::unsupported;
      return engine<driver_t<Config>>::template run<Config, Handler>(
          [&](auto& s)
          {
            driver_t<Config>::prepare(s, rx.data(), rx.size(), address, true);
            s.transfer.subaddressSize = tx.size();
            s.transfer.subaddress = 0U;
            for (auto byte : tx)
              s.transfer.subaddress = (s.transfer.subaddress << 8U) | byte;
          });
    }
  }
  template <typename Config, auto Handler> static status transmit(std::span<const std::uint16_t> samples) noexcept
  {
    return audio<Config, Handler>(reinterpret_cast<std::uint8_t*>(const_cast<std::uint16_t*>(samples.data())),
                                  samples.size_bytes(), false);
  }
  template <typename Config, auto Handler> static status receive(std::span<std::uint16_t> samples) noexcept
  {
    return audio<Config, Handler>(reinterpret_cast<std::uint8_t*>(samples.data()), samples.size_bytes(), true);
  }
  template <typename Config, auto Handler>
  static status audio(std::uint8_t* data, std::size_t count, bool reading) noexcept
  {
    if constexpr (Kind != peripheral_kind::i2s || !matches<Config, Kind>)
      return status::unsupported;
    else
    {
      if (count == 0U || data == nullptr || count % 4U != 0U)
        return status::invalid_argument;
      constexpr bool receive_mode = Config::mode == i2s_mode::master_rx || Config::mode == i2s_mode::slave_rx;
      if (receive_mode != reading)
        return status::unsupported;
      return engine<driver_t<Config>>::template run<Config, Handler>(
          [&](auto& s)
          {
            s.transfer = {};
            s.transfer.data = data;
            s.transfer.dataSize = count;
          });
    }
  }
};
} // namespace microhal::backends::kinetis::espresso::serial

namespace microhal
{
template <>
struct serial_mode_api<kinetis_espresso_tag, peripheral_kind::i2c>
    : backends::kinetis::espresso::serial::api<peripheral_kind::i2c>
{
};
template <>
struct serial_mode_api<kinetis_espresso_tag, peripheral_kind::spi>
    : backends::kinetis::espresso::serial::api<peripheral_kind::spi>
{
};
template <>
struct serial_mode_api<kinetis_espresso_tag, peripheral_kind::uart>
    : backends::kinetis::espresso::serial::api<peripheral_kind::uart>
{
};
template <>
struct serial_mode_api<kinetis_espresso_tag, peripheral_kind::i2s>
    : backends::kinetis::espresso::serial::api<peripheral_kind::i2s>
{
};
} // namespace microhal
