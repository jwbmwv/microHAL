// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "microhal/core/status.hpp"
#include <cstddef>
#include <cstdint>

namespace microhal::backends::stm32::detail
{
enum class serial_dma_layout : std::uint8_t
{
  channel,
  stream,
  gpdma
};
struct alignas(32) serial_gpdma_node
{
  std::uint32_t transfer1{}, transfer2{}, count{}, source{}, destination{}, link{};
};
struct serial_dma_resource
{
  serial_dma_layout layout{serial_dma_layout::channel};
  volatile std::uint32_t* control{};
  volatile std::uint32_t* count{};
  volatile std::uint32_t* peripheral{};
  volatile std::uint32_t* memory{};
  volatile std::uint32_t* flags{};
  volatile std::uint32_t* clear{};
  std::uint32_t complete_mask{}, half_mask{}, error_mask{};
  volatile std::uint32_t* transfer_control{};
  volatile std::uint32_t* request_control{};
  volatile std::uint32_t* link_base{};
  volatile std::uint32_t* link_address{};
  serial_gpdma_node* circular_node{};
  unsigned sample_bytes{1U};
  mutable std::uint32_t stopped_count{};
  mutable bool stopped_count_valid{};

  explicit operator bool() const noexcept
  {
    return control && count && peripheral && memory && flags && clear;
  }
  bool stop() const noexcept
  {
    if (!*this)
      return true;
    if (layout == serial_dma_layout::gpdma && (*control & 1U))
    {
      *control = *control | 4U;
      unsigned budget = 1024U;
      while (!(*flags & (1U << 13U)) && --budget)
      {
      }
      if (!budget)
        return false;
      stopped_count = *count & 0xFFFFU;
      stopped_count_valid = true;
      *control = *control | 2U;
    }
    else if (layout != serial_dma_layout::gpdma)
      *control = *control & ~1U;
    unsigned budget = 1024U;
    while ((*control & 1U) && --budget)
    {
    }
    if ((*control & 1U) == 0U && !stopped_count_valid)
    {
      stopped_count = *count & 0xFFFFU;
      stopped_count_valid = true;
    }
    return (*control & 1U) == 0U;
  }
  status start(std::uintptr_t peripheral_address, std::uintptr_t memory_address, std::size_t length, bool transmit,
               bool circular = false, bool increment = true, unsigned width = 1U) noexcept
  {
    if (!*this)
      return status::unsupported;
    // Keep validation and programming on the same binding across stop/MMIO writes.
    const auto dma_layout = layout;
    const auto transfer = transfer_control;
    const auto request = request_control;
    const auto node = circular_node;
    const auto base = link_base;
    const auto link_register = link_address;
    if ((width != 1U && width != 2U && width != 4U) || !length || length > 65535U || peripheral_address % width ||
        memory_address % width || (dma_layout == serial_dma_layout::gpdma && length * width > 65535U))
      return status::invalid_argument;
    if (dma_layout == serial_dma_layout::gpdma &&
        (!transfer || (circular && (!node || !base || !link_register || !request))))
      return status::unsupported;
    if (!stop())
      return status::busy;
    stopped_count_valid = false;
    sample_bytes = width;
    const unsigned size_bits = width == 4U ? 2U : width == 2U ? 1U : 0U;
    *clear = complete_mask | half_mask | error_mask;
    *count = static_cast<std::uint32_t>(dma_layout == serial_dma_layout::gpdma ? length * width : length);
    if (dma_layout == serial_dma_layout::gpdma)
    {
      *peripheral = static_cast<std::uint32_t>(transmit ? memory_address : peripheral_address);
      *memory = static_cast<std::uint32_t>(transmit ? peripheral_address : memory_address);
      *transfer = (*transfer & ~0x03FF03FFU) | size_bits | (size_bits << 16U) |
                  (increment ? (transmit ? (1U << 3U) : (1U << 19U)) : 0U);
      if (request)
        *request = (*request & ~((3U << 30U) | (1U << 9U) | (1U << 10U))) | (transmit ? 1U << 10U : 0U);
      if (circular)
      {
        const auto address = reinterpret_cast<std::uintptr_t>(node);
        const auto link = 0xF8010000U | (static_cast<std::uint32_t>(address) & 0xFFFCU);
        *node = {*transfer, *request, *count, *peripheral, *memory, link};
        *base = static_cast<std::uint32_t>(address) & 0xFFFF0000U;
        *link_register = link;
      }
      else if (link_register)
        *link_register = 0U;
#if defined(__arm__) || defined(__thumb__)
      __asm volatile("dmb" ::: "memory");
#endif
      *control = (*control & ~0xFF00U) | (1U << 8U) | (1U << 10U) | (1U << 11U) | (1U << 12U) | (1U << 14U) |
                 (circular ? 1U << 9U : 0U) | 1U;
    }
    else
    {
      *peripheral = static_cast<std::uint32_t>(peripheral_address);
      *memory = static_cast<std::uint32_t>(memory_address);
      if (dma_layout == serial_dma_layout::channel)
        *control = (*control & ~0x4FFFU) | 0xAU | (size_bits << 8U) | (size_bits << 10U) | (transmit ? 0x10U : 0U) |
                   (increment ? 0x80U : 0U) | (circular ? 0x24U : 0U) | 1U;
      else
        *control = (*control & ~0x01FCFFFFU) | 0x16U | (size_bits << 11U) | (size_bits << 13U) |
                   (transmit ? 0x40U : 0U) | (increment ? 0x400U : 0U) | (circular ? 0x108U : 0U) | 1U;
    }
    return status::ok;
  }
  std::size_t remaining() const noexcept
  {
    return *this ? (stopped_count_valid ? stopped_count : (*count & 0xFFFFU)) /
                       (layout == serial_dma_layout::gpdma ? sample_bytes : 1U)
                 : 0U;
  }
};

template <typename Channel, typename Controller>
serial_dma_resource channel_dma(Channel& channel, Controller& controller, unsigned index) noexcept
{
  if (!index || index > 8U)
    return {};
  const auto shift = (index - 1U) * 4U;
  auto* memory = [&]()
  {
    if constexpr (requires { channel.CMAR; })
      return &channel.CMAR;
    else
      return &channel.CM0AR;
  }();
  return {serial_dma_layout::channel, &channel.CCR, &channel.CNDTR, &channel.CPAR, memory, &controller.ISR,
          &controller.IFCR,           2U << shift,  4U << shift,    8U << shift};
}
template <typename Stream>
serial_dma_resource stream_dma(Stream& stream, volatile std::uint32_t& flags, volatile std::uint32_t& clear,
                               unsigned shift) noexcept
{
  if (shift != 0U && shift != 6U && shift != 16U && shift != 22U)
    return {};
  return {serial_dma_layout::stream,
          &stream.CR,
          &stream.NDTR,
          &stream.PAR,
          &stream.M0AR,
          &flags,
          &clear,
          0x20U << shift,
          0x10U << shift,
          0xDU << shift};
}
template <typename Channel> serial_dma_resource gpdma(Channel& channel, serial_gpdma_node* node = nullptr) noexcept
{
  return {serial_dma_layout::gpdma,
          &channel.CCR,
          &channel.CBR1,
          &channel.CSAR,
          &channel.CDAR,
          &channel.CSR,
          &channel.CFCR,
          1U << 8U,
          1U << 9U,
          (1U << 10U) | (1U << 11U) | (1U << 12U) | (1U << 14U),
          &channel.CTR1,
          &channel.CTR2,
          &channel.CLBAR,
          &channel.CLLR,
          node};
}
} // namespace microhal::backends::stm32::detail
