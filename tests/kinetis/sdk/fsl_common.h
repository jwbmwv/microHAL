// SPDX-License-Identifier: Apache-2.0
// Deterministic API fixture, not an NXP SDK or hardware emulation.
#pragma once
#include <cstddef>
#include <cstdint>
#define MAKE_VERSION(a, b, c) (((a) << 16) | ((b) << 8) | (c))
using status_t = int;
#if defined(__arm__) || defined(__thumb__)
inline std::uint32_t __get_PRIMASK()
{
  std::uint32_t v;
  asm volatile("mrs %0, primask" : "=r"(v));
  return v;
}
inline void __disable_irq()
{
  asm volatile("cpsid i" ::: "memory");
}
inline void __set_PRIMASK(std::uint32_t v)
{
  asm volatile("msr primask, %0" ::"r"(v) : "memory");
}
#endif
inline constexpr status_t kStatus_Success = 0, kStatus_InvalidArgument = 1;
enum
{
  kCLOCK_CoreSysClk
};
inline std::uint32_t CLOCK_GetFreq(int)
{
  return 48'000'000U;
}
struct edma_handle_t
{
  unsigned channel{};
};
struct dma_handle_t
{
  unsigned channel{};
};
namespace fixture
{
inline unsigned starts{}, aborts{}, creates{}, dma_creates{};
inline status_t next_status{}, completion_status{};
inline void (*pending)(void*){};
inline void* pending_context{};
inline bool auto_complete = true;
inline bool poll_progress = true;
inline std::uint32_t ticks{};
inline void complete()
{
  auto fn = pending;
  auto context = pending_context;
  pending = nullptr;
  if (fn)
    fn(context);
}
template <typename Handle> status_t start(Handle* h)
{
  ++starts;
  if (next_status)
    return next_status;
  pending_context = h;
  pending = +[](void* context)
  {
    auto* handle = static_cast<Handle*>(context);
    handle->callback(handle->base, handle, completion_status, handle->context);
  };
  return kStatus_Success;
}
inline void abort()
{
  ++aborts;
  pending = nullptr;
}
} // namespace fixture

#define I2C_S_BUSY_MASK 0x20U
#define I2C_S_IAL_MASK 0x10U
#define I2C_S_IICIF_MASK 0x02U
#define I2C_S_RXAK_MASK 0x01U
#define I2C_C1_IICIE_MASK 0x40U
#define I2C_C1_MST_MASK 0x20U
#define I2C_C1_TX_MASK 0x10U
#define I2C_C1_TXAK_MASK 0x08U
struct fixture_i2c_status
{
  std::uint8_t flags{};
  operator std::uint8_t() const
  {
    return flags | (fixture::poll_progress ? I2C_S_IICIF_MASK : 0U);
  }
  void operator=(std::uint8_t mask)
  {
    flags &= static_cast<std::uint8_t>(~mask);
  }
};
struct I2C_Type
{
  volatile std::uint8_t C1{}, D{}, S2{1U};
  fixture_i2c_status S{};
};
inline void I2C_EnableDMA(I2C_Type*, bool) {}
inline void I2C_Enable(I2C_Type*, bool enabled)
{
  if (!enabled)
    fixture::abort();
}
#define LPI2C_MCR_MEN_MASK 1U
#define LPI2C_MCR_RTF_MASK 0x100U
#define LPI2C_MCR_RRF_MASK 0x200U
#define LPI2C_MTDR_CMD(value) ((value) << 8U)
enum
{
  kLPI2C_MasterTxReadyFlag = 1U,
  kLPI2C_MasterRxReadyFlag = 2U,
  kLPI2C_MasterStopDetectFlag = 4U,
  kLPI2C_MasterPinLowTimeoutFlag = 8U,
  kLPI2C_MasterBusBusyFlag = 16U,
  kLPI2C_MasterErrorFlags = 0x68U,
  kLPI2C_MasterClearFlags = 0xFF00U
};
struct LPI2C_Type
{
  volatile std::uint32_t MCR{1U}, MIER{}, MDER{}, MTDR{}, MRDR{0xA5U};
  std::uint32_t flags{};
};
inline std::uint32_t LPI2C_MasterGetStatusFlags(LPI2C_Type* b)
{
  return b->flags | (fixture::poll_progress ? 7U : 0U);
}
inline void LPI2C_MasterClearStatusFlags(LPI2C_Type*, std::uint32_t) {}

#define FIXTURE_I2C(Prefix, Lower, Base, Baud)                                                                         \
  enum                                                                                                                 \
  {                                                                                                                    \
    k##Prefix##_Write,                                                                                                 \
    k##Prefix##_Read                                                                                                   \
  };                                                                                                                   \
  enum                                                                                                                 \
  {                                                                                                                    \
    kStatus_##Prefix##_Busy = __COUNTER__ + 20,                                                                        \
    kStatus_##Prefix##_Timeout,                                                                                        \
    kStatus_##Prefix##_Nak,                                                                                            \
    kStatus_##Prefix##_Addr_Nak,                                                                                       \
    kStatus_##Prefix##_ArbitrationLost,                                                                                \
    kStatus_##Prefix##_PinLowTimeout,                                                                                  \
    kStatus_##Prefix##_FifoError,                                                                                      \
    kStatus_##Prefix##_BitError                                                                                        \
  };                                                                                                                   \
  struct Lower##_master_config_t                                                                                       \
  {                                                                                                                    \
    std::uint32_t Baud{};                                                                                              \
  };                                                                                                                   \
  struct Lower##_master_transfer_t                                                                                     \
  {                                                                                                                    \
    std::uint16_t slaveAddress{};                                                                                      \
    int direction{};                                                                                                   \
    std::uint8_t* data{};                                                                                              \
    std::size_t dataSize{};                                                                                            \
    std::uint32_t flags{}, subaddress{};                                                                               \
    std::uint8_t subaddressSize{};                                                                                     \
  };                                                                                                                   \
  struct Lower##_master_handle_t                                                                                       \
  {                                                                                                                    \
    Base* base{};                                                                                                      \
    void* context{};                                                                                                   \
    void (*callback)(Base*, Lower##_master_handle_t*, status_t, void*){};                                              \
  };                                                                                                                   \
  struct Lower##_master_edma_handle_t                                                                                  \
  {                                                                                                                    \
    Base* base{};                                                                                                      \
    void* context{};                                                                                                   \
    edma_handle_t* dma{};                                                                                              \
    void (*callback)(Base*, Lower##_master_edma_handle_t*, status_t, void*){};                                         \
  };                                                                                                                   \
  inline void Prefix##_MasterGetDefaultConfig(Lower##_master_config_t*) {}                                             \
  inline void Prefix##_MasterInit(Base*, const Lower##_master_config_t*, std::uint32_t) {}                             \
  inline void Prefix##_MasterTransferCreateHandle(Base* b, Lower##_master_handle_t* h,                                 \
                                                  void (*cb)(Base*, Lower##_master_handle_t*, status_t, void*),        \
                                                  void* context)                                                       \
  {                                                                                                                    \
    *h = {b, context, cb};                                                                                             \
    ++fixture::creates;                                                                                                \
  }                                                                                                                    \
  inline status_t Prefix##_MasterTransferNonBlocking(Base*, Lower##_master_handle_t* h, Lower##_master_transfer_t*)    \
  {                                                                                                                    \
    return fixture::start(h);                                                                                          \
  }                                                                                                                    \
  inline void Prefix##_MasterTransferAbort(Base*, Lower##_master_handle_t*)                                            \
  {                                                                                                                    \
    fixture::abort();                                                                                                  \
  }                                                                                                                    \
  inline void Prefix##_MasterTransferHandleIRQ(Base*, Lower##_master_handle_t*)                                        \
  {                                                                                                                    \
    fixture::complete();                                                                                               \
  }                                                                                                                    \
  inline void Prefix##_MasterCreateEDMAHandle(Base* b, Lower##_master_edma_handle_t* h,                                \
                                              void (*cb)(Base*, Lower##_master_edma_handle_t*, status_t, void*),       \
                                              void* context, edma_handle_t* dma)                                       \
  {                                                                                                                    \
    *h = {b, context, dma, cb};                                                                                        \
    ++fixture::dma_creates;                                                                                            \
  }                                                                                                                    \
  inline status_t Prefix##_MasterTransferEDMA(Base*, Lower##_master_edma_handle_t* h, Lower##_master_transfer_t*)      \
  {                                                                                                                    \
    return fixture::start(h);                                                                                          \
  }                                                                                                                    \
  inline void Prefix##_MasterTransferAbortEDMA(Base*, Lower##_master_edma_handle_t*)                                   \
  {                                                                                                                    \
    fixture::abort();                                                                                                  \
  }
FIXTURE_I2C(I2C, i2c, I2C_Type, baudRate_Bps)
FIXTURE_I2C(LPI2C, lpi2c, LPI2C_Type, baudRate_Hz)
#undef FIXTURE_I2C

#define DSPI_MASTER_CTAR_SHIFT 0U
#define DSPI_MASTER_PCS_SHIFT 4U
#define LPSPI_MASTER_PCS_SHIFT 4U
#define LPSPI_TCR_PCS_MASK 0x300U
#define LPSPI_TCR_PCS(value) ((value) << 8U)
#define LPSPI_TCR_CONT_MASK 0x10000U
#define LPSPI_TCR_CONTC_MASK 0x20000U
#define LPSPI_TCR_TXMSK_MASK 0x40000U
#define LPSPI_TCR_RXMSK_MASK 0x80000U
enum
{
  kDSPI_TxCompleteFlag = 1U,
  kDSPI_EndOfQueueFlag = 2U,
  kDSPI_RxFifoOverflowFlag = 4U,
  kDSPI_TxFifoFillRequestFlag = 8U,
  kDSPI_RxFifoDrainRequestFlag = 16U,
  kDSPI_MasterPcsContinuous = 1U << 20U,
  kLPSPI_ModuleBusyFlag = 1U,
  kLPSPI_TransferCompleteFlag = 2U,
  kLPSPI_TransmitErrorFlag = 4U,
  kLPSPI_ReceiveErrorFlag = 8U,
  kLPSPI_TxDataRequestFlag = 16U,
  kLPSPI_RxDataReadyFlag = 32U,
  kLPSPI_MasterPcsContinuous = 1U << 20U
};
struct SPI_Type
{
  volatile std::uint32_t RSER{};
  std::uint32_t flags{}, data{}, writes{};
};
struct LPSPI_Type
{
  volatile std::uint32_t IER{}, DER{}, TCR{};
  std::uint32_t flags{}, data{}, writes{};
};
struct dspi_command_data_config_t
{
  bool isPcsContinuous{};
  std::uint8_t whichCtar{}, whichPcs{};
  bool isEndOfQueue{}, clearTransferCount{};
};
inline void DSPI_GetDefaultDataCommandConfig(dspi_command_data_config_t*) {}
inline std::uint32_t DSPI_GetStatusFlags(SPI_Type* b)
{
  return b->flags | (fixture::poll_progress ? 27U : 0U);
}
inline void DSPI_StopTransfer(SPI_Type*) {}
inline void DSPI_StartTransfer(SPI_Type*) {}
inline void DSPI_FlushFifo(SPI_Type*, bool, bool) {}
inline void DSPI_ClearStatusFlags(SPI_Type*, std::uint32_t) {}
inline void DSPI_MasterWriteData(SPI_Type* b, dspi_command_data_config_t*, std::uint16_t value)
{
  b->data = value;
  ++b->writes;
}
inline std::uint32_t DSPI_ReadData(SPI_Type* b)
{
  return b->data;
}
inline std::uint32_t LPSPI_GetStatusFlags(LPSPI_Type* b)
{
  return b->flags | (fixture::poll_progress ? 50U : 0U);
}
inline void LPSPI_FlushFifo(LPSPI_Type*, bool, bool) {}
inline void LPSPI_ClearStatusFlags(LPSPI_Type*, std::uint32_t) {}
inline void LPSPI_Enable(LPSPI_Type*, bool) {}
inline void LPSPI_WriteData(LPSPI_Type* b, std::uint32_t value)
{
  b->data = value;
  ++b->writes;
}
inline std::uint32_t LPSPI_ReadData(LPSPI_Type* b)
{
  return b->data;
}

struct dspi_master_config_t
{
  struct
  {
    std::uint32_t baudRate{}, bitsPerFrame{};
    int cpol{}, cpha{}, direction{};
  } ctarConfig;
};
struct lpspi_master_config_t
{
  std::uint32_t baudRate{}, bitsPerFrame{};
  int cpol{}, cpha{}, direction{};
};
#define FIXTURE_SPI(Prefix, Lower, Base)                                                                               \
  enum                                                                                                                 \
  {                                                                                                                    \
    kStatus_##Prefix##_Busy = __COUNTER__ + 70                                                                         \
  };                                                                                                                   \
  enum                                                                                                                 \
  {                                                                                                                    \
    k##Prefix##_ClockPolarityActiveLow,                                                                                \
    k##Prefix##_ClockPolarityActiveHigh,                                                                               \
    k##Prefix##_ClockPhaseSecondEdge,                                                                                  \
    k##Prefix##_ClockPhaseFirstEdge,                                                                                   \
    k##Prefix##_LsbFirst,                                                                                              \
    k##Prefix##_MsbFirst                                                                                               \
  };                                                                                                                   \
  struct Lower##_transfer_t                                                                                            \
  {                                                                                                                    \
    std::uint8_t* txData{};                                                                                            \
    std::uint8_t* rxData{};                                                                                            \
    std::size_t dataSize{};                                                                                            \
    std::uint32_t configFlags{};                                                                                       \
  };                                                                                                                   \
  struct Lower##_master_handle_t                                                                                       \
  {                                                                                                                    \
    Base* base{};                                                                                                      \
    void* context{};                                                                                                   \
    void (*callback)(Base*, Lower##_master_handle_t*, status_t, void*){};                                              \
  };                                                                                                                   \
  struct Lower##_master_edma_handle_t                                                                                  \
  {                                                                                                                    \
    Base* base{};                                                                                                      \
    void* context{};                                                                                                   \
    void (*callback)(Base*, Lower##_master_edma_handle_t*, status_t, void*){};                                         \
  };                                                                                                                   \
  inline void Prefix##_MasterGetDefaultConfig(Lower##_master_config_t*) {}                                             \
  inline void Prefix##_MasterInit(Base*, const Lower##_master_config_t*, std::uint32_t) {}                             \
  inline void Prefix##_MasterTransferCreateHandle(Base* b, Lower##_master_handle_t* h,                                 \
                                                  void (*cb)(Base*, Lower##_master_handle_t*, status_t, void*),        \
                                                  void* context)                                                       \
  {                                                                                                                    \
    *h = {b, context, cb};                                                                                             \
    ++fixture::creates;                                                                                                \
  }                                                                                                                    \
  inline status_t Prefix##_MasterTransferNonBlocking(Base*, Lower##_master_handle_t* h, Lower##_transfer_t*)           \
  {                                                                                                                    \
    return fixture::start(h);                                                                                          \
  }                                                                                                                    \
  inline void Prefix##_MasterTransferAbort(Base*, Lower##_master_handle_t*)                                            \
  {                                                                                                                    \
    fixture::abort();                                                                                                  \
  }                                                                                                                    \
  inline void Prefix##_MasterTransferHandleIRQ(Base*, Lower##_master_handle_t*)                                        \
  {                                                                                                                    \
    fixture::complete();                                                                                               \
  }                                                                                                                    \
  inline status_t Prefix##_MasterTransferEDMA(Base*, Lower##_master_edma_handle_t* h, Lower##_transfer_t*)             \
  {                                                                                                                    \
    return fixture::start(h);                                                                                          \
  }                                                                                                                    \
  inline void Prefix##_MasterTransferAbortEDMA(Base*, Lower##_master_edma_handle_t*)                                   \
  {                                                                                                                    \
    fixture::abort();                                                                                                  \
  }
FIXTURE_SPI(DSPI, dspi, SPI_Type)
FIXTURE_SPI(LPSPI, lpspi, LPSPI_Type)
#undef FIXTURE_SPI
inline void DSPI_MasterTransferCreateHandleEDMA(SPI_Type* b, dspi_master_edma_handle_t* h,
                                                void (*cb)(SPI_Type*, dspi_master_edma_handle_t*, status_t, void*),
                                                void* context, edma_handle_t*, edma_handle_t*, edma_handle_t*)
{
  *h = {b, context, cb};
  ++fixture::dma_creates;
}
inline void LPSPI_MasterTransferCreateHandleEDMA(LPSPI_Type* b, lpspi_master_edma_handle_t* h,
                                                 void (*cb)(LPSPI_Type*, lpspi_master_edma_handle_t*, status_t, void*),
                                                 void* context, edma_handle_t*, edma_handle_t*)
{
  *h = {b, context, cb};
  ++fixture::dma_creates;
}

#define FIXTURE_UART(Prefix, Lower, Base)                                                                              \
  enum                                                                                                                 \
  {                                                                                                                    \
    kStatus_##Prefix##_TxBusy = __COUNTER__ + 90,                                                                      \
    kStatus_##Prefix##_RxBusy,                                                                                         \
    kStatus_##Prefix##_TxIdle,                                                                                         \
    kStatus_##Prefix##_RxIdle                                                                                          \
  };                                                                                                                   \
  enum                                                                                                                 \
  {                                                                                                                    \
    k##Prefix##_ParityDisabled = 0,                                                                                    \
    k##Prefix##_ParityEven = 2,                                                                                        \
    k##Prefix##_ParityOdd = 3                                                                                          \
  };                                                                                                                   \
  enum                                                                                                                 \
  {                                                                                                                    \
    k##Prefix##_RxOverrunFlag = 1,                                                                                     \
    k##Prefix##_NoiseErrorFlag = 2,                                                                                    \
    k##Prefix##_FramingErrorFlag = 4,                                                                                  \
    k##Prefix##_ParityErrorFlag = 8,                                                                                   \
    k##Prefix##_RxDataRegFullFlag = 16,                                                                                \
    k##Prefix##_TxDataRegEmptyFlag = 32,                                                                               \
    k##Prefix##_TransmissionCompleteFlag = 64                                                                          \
  };                                                                                                                   \
  struct Base                                                                                                          \
  {                                                                                                                    \
    std::uint32_t flags{112U};                                                                                         \
    unsigned writes{};                                                                                                 \
  };                                                                                                                   \
  struct Lower##_config_t                                                                                              \
  {                                                                                                                    \
    std::uint32_t baudRate_Bps{};                                                                                      \
    bool enableTx{}, enableRx{};                                                                                       \
    int parityMode{}, stopBitCount{};                                                                                  \
  };                                                                                                                   \
  struct Lower##_transfer_t                                                                                            \
  {                                                                                                                    \
    std::uint8_t* data{};                                                                                              \
    std::size_t dataSize{};                                                                                            \
  };                                                                                                                   \
  struct Lower##_handle_t                                                                                              \
  {                                                                                                                    \
    Base* base{};                                                                                                      \
    void* context{};                                                                                                   \
    void (*callback)(Base*, Lower##_handle_t*, status_t, void*){};                                                     \
  };                                                                                                                   \
  struct Lower##_edma_handle_t                                                                                         \
  {                                                                                                                    \
    Base* base{};                                                                                                      \
    void* context{};                                                                                                   \
    void (*callback)(Base*, Lower##_edma_handle_t*, status_t, void*){};                                                \
  };                                                                                                                   \
  inline void Prefix##_GetDefaultConfig(Lower##_config_t*) {}                                                          \
  inline status_t Prefix##_Init(Base*, const Lower##_config_t*, std::uint32_t)                                         \
  {                                                                                                                    \
    return fixture::next_status;                                                                                       \
  }                                                                                                                    \
  inline void Prefix##_TransferCreateHandle(Base* b, Lower##_handle_t* h,                                              \
                                            void (*cb)(Base*, Lower##_handle_t*, status_t, void*), void* context)      \
  {                                                                                                                    \
    *h = {b, context, cb};                                                                                             \
    ++fixture::creates;                                                                                                \
  }                                                                                                                    \
  inline status_t Prefix##_TransferSendNonBlocking(Base*, Lower##_handle_t* h, Lower##_transfer_t*)                    \
  {                                                                                                                    \
    return fixture::start(h);                                                                                          \
  }                                                                                                                    \
  inline status_t Prefix##_TransferReceiveNonBlocking(Base*, Lower##_handle_t* h, Lower##_transfer_t*, std::size_t*)   \
  {                                                                                                                    \
    return fixture::start(h);                                                                                          \
  }                                                                                                                    \
  inline void Prefix##_TransferAbortReceive(Base*, Lower##_handle_t*)                                                  \
  {                                                                                                                    \
    fixture::abort();                                                                                                  \
  }                                                                                                                    \
  inline void Prefix##_TransferAbortSend(Base*, Lower##_handle_t*)                                                     \
  {                                                                                                                    \
    fixture::abort();                                                                                                  \
  }                                                                                                                    \
  inline void Prefix##_TransferHandleIRQ(Base*, Lower##_handle_t*)                                                     \
  {                                                                                                                    \
    fixture::complete();                                                                                               \
  }                                                                                                                    \
  inline std::uint32_t Prefix##_GetStatusFlags(Base* b)                                                                \
  {                                                                                                                    \
    return b->flags;                                                                                                   \
  }                                                                                                                    \
  inline std::uint8_t Prefix##_ReadByte(Base*)                                                                         \
  {                                                                                                                    \
    return 0xA5;                                                                                                       \
  }                                                                                                                    \
  inline void Prefix##_WriteByte(Base* b, std::uint8_t)                                                                \
  {                                                                                                                    \
    ++b->writes;                                                                                                       \
  }                                                                                                                    \
  inline status_t Prefix##_WriteBlocking(Base*, const std::uint8_t*, std::size_t)                                      \
  {                                                                                                                    \
    return 1;                                                                                                          \
  }                                                                                                                    \
  inline status_t Prefix##_ReadBlocking(Base*, std::uint8_t*, std::size_t)                                             \
  {                                                                                                                    \
    return 1;                                                                                                          \
  }                                                                                                                    \
  inline void Prefix##_TransferCreateHandleEDMA(Base* b, Lower##_edma_handle_t* h,                                     \
                                                void (*cb)(Base*, Lower##_edma_handle_t*, status_t, void*),            \
                                                void* context, edma_handle_t*, edma_handle_t*)                         \
  {                                                                                                                    \
    *h = {b, context, cb};                                                                                             \
    ++fixture::dma_creates;                                                                                            \
  }                                                                                                                    \
  inline status_t Prefix##_SendEDMA(Base*, Lower##_edma_handle_t* h, Lower##_transfer_t*)                              \
  {                                                                                                                    \
    return fixture::start(h);                                                                                          \
  }                                                                                                                    \
  inline status_t Prefix##_ReceiveEDMA(Base*, Lower##_edma_handle_t* h, Lower##_transfer_t*)                           \
  {                                                                                                                    \
    return fixture::start(h);                                                                                          \
  }                                                                                                                    \
  inline void Prefix##_TransferAbortReceiveEDMA(Base*, Lower##_edma_handle_t*)                                         \
  {                                                                                                                    \
    fixture::abort();                                                                                                  \
  }                                                                                                                    \
  inline void Prefix##_TransferAbortSendEDMA(Base*, Lower##_edma_handle_t*)                                            \
  {                                                                                                                    \
    fixture::abort();                                                                                                  \
  }
FIXTURE_UART(UART, uart, UART_Type)
FIXTURE_UART(LPUART, lpuart, LPUART_Type)
#undef FIXTURE_UART
inline UART_Type fixture_uart{};
inline LPUART_Type fixture_lpuart{};
#define UART_BASE_PTRS {&fixture_uart}
#define LPUART_BASE_PTRS {&fixture_lpuart}

#define FSL_SAI_DRIVER_VERSION MAKE_VERSION(2, 4, 3)
struct I2S_Type
{
  volatile std::uint32_t TCSR{}, RCSR{}, TDR[1]{}, RDR[1]{0x1234U};
  std::uint32_t flags{};
};
#define I2S_TCSR_WSIE_MASK 0x100U
#define I2S_TCSR_SEIE_MASK 0x200U
#define I2S_TCSR_FWIE_MASK 0x400U
#define I2S_TCSR_FEIE_MASK 0x800U
#define I2S_TCSR_FRIE_MASK 0x1000U
#define I2S_TCSR_FWDE_MASK 0x1U
#define I2S_TCSR_FRDE_MASK 0x2U
enum
{
  kSAI_FIFOErrorFlag = 0x10000U,
  kSAI_FIFOWarningFlag = 0x20000U,
  kSAI_ResetTypeFIFO = 1U
};
inline std::uint32_t SAI_TxGetStatusFlag(I2S_Type* b)
{
  return b->flags | (fixture::poll_progress ? static_cast<std::uint32_t>(kSAI_FIFOWarningFlag) : 0U);
}
inline std::uint32_t SAI_RxGetStatusFlag(I2S_Type* b)
{
  return SAI_TxGetStatusFlag(b);
}
inline void SAI_TxEnable(I2S_Type*, bool) {}
inline void SAI_RxEnable(I2S_Type*, bool) {}
inline void SAI_TxSoftwareReset(I2S_Type*, int) {}
inline void SAI_RxSoftwareReset(I2S_Type*, int) {}
enum
{
  kStatus_SAI_TxIdle = 201,
  kStatus_SAI_RxIdle,
  kStatus_SAI_TxBusy,
  kStatus_SAI_RxBusy,
  kStatus_SAI_QueueFull
};
enum
{
  kSAI_WordWidth16bits = 16,
  kSAI_Stereo = 2,
  kSAI_Channel0Mask = 1,
  kSAI_Master = 0,
  kSAI_Slave = 1,
  kSAI_ModeAsync = 0
};
struct sai_transceiver_t
{
  int masterSlave{}, syncMode{};
};
inline void SAI_TxSetConfig(I2S_Type*, sai_transceiver_t*) {}
inline void SAI_RxSetConfig(I2S_Type*, sai_transceiver_t*) {}
struct sai_transfer_t
{
  std::uint8_t* data{};
  std::size_t dataSize{};
};
struct sai_handle_t
{
  I2S_Type* base{};
  void* context{};
  void (*callback)(I2S_Type*, sai_handle_t*, status_t, void*){};
};
struct sai_edma_handle_t
{
  I2S_Type* base{};
  void* context{};
  void (*callback)(I2S_Type*, sai_edma_handle_t*, status_t, void*){};
};
inline void SAI_Init(I2S_Type*) {}
inline void SAI_GetClassicI2SConfig(sai_transceiver_t*, int, int, std::uint32_t) {}
#define FIXTURE_SAI(Direction, Operation)                                                                              \
  inline void SAI_Transfer##Direction##CreateHandle(                                                                   \
      I2S_Type* b, sai_handle_t* h, void (*cb)(I2S_Type*, sai_handle_t*, status_t, void*), void* context)              \
  {                                                                                                                    \
    *h = {b, context, cb};                                                                                             \
    ++fixture::creates;                                                                                                \
  }                                                                                                                    \
  inline void SAI_Transfer##Direction##SetConfig(I2S_Type*, sai_handle_t*, sai_transceiver_t*) {}                      \
  inline void SAI_##Direction##SetBitClockRate(I2S_Type*, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t)  \
  {}                                                                                                                   \
  inline status_t SAI_Transfer##Operation##NonBlocking(I2S_Type*, sai_handle_t* h, sai_transfer_t*)                    \
  {                                                                                                                    \
    return fixture::start(h);                                                                                          \
  }                                                                                                                    \
  inline void SAI_TransferTerminate##Operation(I2S_Type*, sai_handle_t*)                                               \
  {                                                                                                                    \
    fixture::abort();                                                                                                  \
  }                                                                                                                    \
  inline void SAI_Transfer##Direction##HandleIRQ(I2S_Type*, sai_handle_t*)                                             \
  {                                                                                                                    \
    fixture::complete();                                                                                               \
  }                                                                                                                    \
  inline void SAI_Transfer##Direction##CreateHandleEDMA(I2S_Type* b, sai_edma_handle_t* h,                             \
                                                        void (*cb)(I2S_Type*, sai_edma_handle_t*, status_t, void*),    \
                                                        void* context, edma_handle_t*)                                 \
  {                                                                                                                    \
    *h = {b, context, cb};                                                                                             \
    ++fixture::dma_creates;                                                                                            \
  }                                                                                                                    \
  inline void SAI_Transfer##Direction##SetConfigEDMA(I2S_Type*, sai_edma_handle_t*, sai_transceiver_t*) {}             \
  inline status_t SAI_Transfer##Operation##EDMA(I2S_Type*, sai_edma_handle_t* h, sai_transfer_t*)                      \
  {                                                                                                                    \
    return fixture::start(h);                                                                                          \
  }                                                                                                                    \
  inline void SAI_TransferTerminate##Operation##EDMA(I2S_Type*, sai_edma_handle_t*)                                    \
  {                                                                                                                    \
    fixture::abort();                                                                                                  \
  }
FIXTURE_SAI(Tx, Send)
FIXTURE_SAI(Rx, Receive)
#undef FIXTURE_SAI

struct i2c_master_dma_handle_t
{
  I2C_Type* base{};
  void* context{};
  dma_handle_t* dma{};
  void (*callback)(I2C_Type*, i2c_master_dma_handle_t*, status_t, void*){};
};
inline void I2C_MasterTransferCreateHandleDMA(I2C_Type* b, i2c_master_dma_handle_t* h,
                                              void (*cb)(I2C_Type*, i2c_master_dma_handle_t*, status_t, void*),
                                              void* context, dma_handle_t* dma)
{
  *h = {b, context, dma, cb};
  ++fixture::dma_creates;
}
inline status_t I2C_MasterTransferDMA(I2C_Type*, i2c_master_dma_handle_t* h, i2c_master_transfer_t*)
{
  return fixture::start(h);
}
inline void I2C_MasterTransferAbortDMA(I2C_Type*, i2c_master_dma_handle_t*)
{
  fixture::abort();
}

#define FIXTURE_SIMPLE_UART(Prefix, Lower, Base)                                                                       \
  struct Lower##_dma_handle_t                                                                                          \
  {                                                                                                                    \
    Base* base{};                                                                                                      \
    void* context{};                                                                                                   \
    void (*callback)(Base*, Lower##_dma_handle_t*, status_t, void*){};                                                 \
  };                                                                                                                   \
  inline void Prefix##_TransferCreateHandleDMA(Base* b, Lower##_dma_handle_t* h,                                       \
                                               void (*cb)(Base*, Lower##_dma_handle_t*, status_t, void*),              \
                                               void* context, dma_handle_t*, dma_handle_t*)                            \
  {                                                                                                                    \
    *h = {b, context, cb};                                                                                             \
    ++fixture::dma_creates;                                                                                            \
  }                                                                                                                    \
  inline status_t Prefix##_TransferSendDMA(Base*, Lower##_dma_handle_t* h, Lower##_transfer_t*)                        \
  {                                                                                                                    \
    return fixture::start(h);                                                                                          \
  }                                                                                                                    \
  inline status_t Prefix##_TransferReceiveDMA(Base*, Lower##_dma_handle_t* h, Lower##_transfer_t*)                     \
  {                                                                                                                    \
    return fixture::start(h);                                                                                          \
  }                                                                                                                    \
  inline void Prefix##_TransferAbortReceiveDMA(Base*, Lower##_dma_handle_t*)                                           \
  {                                                                                                                    \
    fixture::abort();                                                                                                  \
  }                                                                                                                    \
  inline void Prefix##_TransferAbortSendDMA(Base*, Lower##_dma_handle_t*)                                              \
  {                                                                                                                    \
    fixture::abort();                                                                                                  \
  }
FIXTURE_SIMPLE_UART(UART, uart, UART_Type)
FIXTURE_SIMPLE_UART(LPUART, lpuart, LPUART_Type)
#undef FIXTURE_SIMPLE_UART
