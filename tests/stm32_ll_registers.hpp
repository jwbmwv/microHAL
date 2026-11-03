// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>

// Register-shaped host models. The engine is unchanged; tests drive peripheral status bits.
struct USART_TypeDef
{
  volatile std::uint32_t CR1{}, CR2{}, CR3{}, BRR{}, ISR{}, ICR{}, RDR{}, TDR{}, PRESC{};
};
inline USART_TypeDef ll_test_lpuart_registers{};
#define LPUART1_BASE reinterpret_cast<std::uintptr_t>(&ll_test_lpuart_registers)
#define USART_PRESC_PRESCALER 0xFU
#define USART_CR1_UE (1U << 0)
#define USART_ISR_RXNE (1U << 5)
#define USART_ICR_IDLECF (1U << 4)
#define USART_ICR_TCCF (1U << 6)
inline constexpr auto USART_CR1_TE = 1U << 3, USART_CR1_RE = 1U << 2, USART_CR1_TCIE = 1U << 6,
                      USART_CR1_IDLEIE = 1U << 4, USART_CR1_PEIE = 1U << 8, USART_CR1_PCE = 1U << 10,
                      USART_CR1_M = 1U << 12, USART_CR1_PS = 1U << 9, USART_CR2_STOP_1 = 1U << 13, USART_CR3_EIE = 1U,
                      USART_CR3_DMAR = 1U << 6, USART_CR3_DMAT = 1U << 7;

#if defined(MICROHAL_TEST_SPI_MODERN)
struct SPI_TypeDef
{
  volatile std::uint32_t CR1{}, CR2{}, SR{}, CFG1{}, CFG2{}, IER{}, IFCR{}, TXDR{}, RXDR{};
};
#define SPI_CFG1_DSIZE 0x1FU
#define SPI_SR_TXP (1U << 1)
#define SPI_SR_RXP 1U
#define SPI_CR1_SPE 1U
inline constexpr auto SPI_CFG1_RXDMAEN = 1U << 14, SPI_CFG1_TXDMAEN = 1U << 15, SPI_CFG2_COMM = 3U << 17,
                      SPI_CFG2_COMM_0 = 1U << 17, SPI_CFG2_COMM_1 = 1U << 18, SPI_CFG2_MASTER = 1U << 22,
                      SPI_CFG2_SSM = 1U << 26, SPI_CFG2_CPOL = 1U << 25, SPI_CFG2_CPHA = 1U << 24,
                      SPI_CFG2_LSBFRST = 1U << 23, SPI_CR1_SSI = 1U << 12, SPI_CR1_HDDIR = 1U << 11,
                      SPI_CR1_CSTART = 1U << 9, SPI_CR1_CSUSP = 1U << 10, SPI_SR_EOT = 1U << 3, SPI_SR_SUSP = 1U << 11,
                      SPI_SR_OVR = 1U << 6, SPI_SR_MODF = 1U << 9, SPI_SR_UDR = 1U << 5, SPI_IER_EOTIE = 1U << 3,
                      SPI_IER_RXPIE = 1U, SPI_IER_TXPIE = 1U << 1, SPI_IER_OVRIE = 1U << 6, SPI_IER_MODFIE = 1U << 9,
                      SPI_IER_UDRIE = 1U << 5;
#else
struct SPI_TypeDef
{
  volatile std::uint32_t CR1{}, CR2{}, SR{}, DR{};
};
#define SPI_CR1_SPE (1U << 6)
inline constexpr auto SPI_CR1_MSTR = 1U << 2, SPI_CR1_SSM = 1U << 9, SPI_CR1_SSI = 1U << 8, SPI_CR1_CPOL = 1U << 1,
                      SPI_CR1_CPHA = 1U, SPI_CR1_LSBFIRST = 1U << 7, SPI_CR2_TXEIE = 1U << 7, SPI_CR2_RXNEIE = 1U << 6,
                      SPI_CR2_ERRIE = 1U << 5, SPI_CR2_TXDMAEN = 1U << 1, SPI_CR2_RXDMAEN = 1U, SPI_SR_RXNE = 1U,
                      SPI_SR_TXE = 1U << 1, SPI_SR_OVR = 1U << 6, SPI_SR_MODF = 1U << 5, SPI_SR_BSY = 1U << 7;
inline constexpr auto SPI_CR1_BIDIMODE = 1U << 15, SPI_CR1_BIDIOE = 1U << 14, SPI_CR1_RXONLY = 1U << 10;
#endif

#if defined(MICROHAL_TEST_I2C_LEGACY)
struct I2C_TypeDef
{
  volatile std::uint32_t CR1{}, CR2{}, OAR1{}, SR1{}, SR2{}, DR{}, CCR{}, TRISE{};
};
#define I2C_CR1_PE 1U
inline constexpr auto I2C_CR1_ACK = 1U << 10, I2C_CR1_POS = 1U << 11, I2C_CR1_STOP = 1U << 9, I2C_CR1_START = 1U << 8,
                      I2C_CCR_FS = 1U << 15, I2C_CR2_DMAEN = 1U << 11, I2C_CR2_LAST = 1U << 12,
                      I2C_CR2_ITEVTEN = 1U << 9, I2C_CR2_ITBUFEN = 1U << 10, I2C_CR2_ITERREN = 1U << 8,
                      I2C_SR1_BERR = 1U << 8, I2C_SR1_ARLO = 1U << 9, I2C_SR1_OVR = 1U << 11, I2C_SR1_AF = 1U << 10,
                      I2C_SR1_SB = 1U, I2C_SR1_ADDR = 1U << 1, I2C_SR1_TXE = 1U << 7, I2C_SR1_BTF = 1U << 2,
                      I2C_SR1_ADD10 = 1U << 3, I2C_SR1_RXNE = 1U << 6, I2C_SR1_STOPF = 1U << 4, I2C_SR2_MSL = 1U,
                      I2C_SR2_BUSY = 1U << 1, I2C_SR2_TRA = 1U << 2;
#else
struct I2C_TypeDef
{
  volatile std::uint32_t CR1{}, CR2{}, OAR1{}, TIMINGR{}, ISR{}, ICR{}, RXDR{}, TXDR{};
};
#define I2C_CR1_PE 1U
#define I2C_CR2_NBYTES (255U << 16)
inline constexpr auto I2C_CR1_TXIE = 1U << 1, I2C_CR1_RXIE = 1U << 2, I2C_CR1_ADDRIE = 1U << 3,
                      I2C_CR1_NACKIE = 1U << 4, I2C_CR1_STOPIE = 1U << 5, I2C_CR1_TCIE = 1U << 6,
                      I2C_CR1_ERRIE = 1U << 7, I2C_CR1_TXDMAEN = 1U << 14, I2C_CR1_RXDMAEN = 1U << 15,
                      I2C_CR2_STOP = 1U << 14, I2C_CR2_START = 1U << 13, I2C_CR2_RD_WRN = 1U << 10,
                      I2C_CR2_ADD10 = 1U << 11, I2C_CR2_RELOAD = 1U << 24, I2C_CR2_AUTOEND = 1U << 25,
                      I2C_CR2_NACK = 1U << 15, I2C_OAR1_OA1EN = 1U << 15, I2C_OAR1_OA1MODE = 1U << 10,
                      I2C_ISR_BERR = 1U << 8, I2C_ISR_ARLO = 1U << 9, I2C_ISR_OVR = 1U << 10, I2C_ISR_BUSY = 1U << 15,
                      I2C_ISR_DIR = 1U << 16, I2C_ISR_ADDR = 1U << 3, I2C_ISR_NACKF = 1U << 4, I2C_ISR_RXNE = 1U << 2,
                      I2C_ISR_TXIS = 1U << 1, I2C_ISR_TCR = 1U << 7, I2C_ISR_TC = 1U << 6, I2C_ISR_STOPF = 1U << 5,
                      I2C_ICR_ADDRCF = 1U << 3;
#endif
