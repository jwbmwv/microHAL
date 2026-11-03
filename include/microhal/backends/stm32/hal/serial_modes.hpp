// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <atomic>
#include <type_traits>

namespace microhal::backends::stm32::hal
{
inline status serial_status(const HAL_StatusTypeDef value) noexcept
{
  switch (value)
  {
  case HAL_OK:
    return status::ok;
  case HAL_BUSY:
    return status::busy;
  case HAL_TIMEOUT:
    return status::timeout;
  default:
    return status::io_error;
  }
}

inline bool serial_claim(std::atomic<bool>& active) noexcept
{
#if defined(__arm__) || defined(__thumb__)
  // Cortex-M0 has no lock-free atomic exchange.
  const auto mask = __get_PRIMASK();
  __disable_irq();
  const bool was_active = active.load(std::memory_order_relaxed);
  active.store(true, std::memory_order_relaxed);
  __set_PRIMASK(mask);
  return !was_active;
#else
  return !active.exchange(true);
#endif
}

template <peripheral_kind Kind> struct serial_driver;

#if MICROHAL_STM32_HAL_HAS_I2C || MICROHAL_STM32_HAL_HAS_SPI || MICROHAL_STM32_HAL_HAS_UART
template <typename Dma> inline status serial_stop_dma(Dma* dma) noexcept
{
  if (dma == nullptr)
    return status::ok;
#if defined(HAL_DMA_ERROR_TIMEOUT)
  const bool timed_out = (HAL_DMA_GetError(dma) & HAL_DMA_ERROR_TIMEOUT) != 0U;
#else
  const bool timed_out = false;
#endif
  const auto state = HAL_DMA_GetState(dma);
  if ((state != HAL_DMA_STATE_READY && state != HAL_DMA_STATE_RESET) || timed_out)
    return serial_status(HAL_DMA_Abort(dma));
  return status::ok;
}
#endif

#if MICROHAL_STM32_HAL_HAS_I2C
template <> struct serial_driver<peripheral_kind::i2c>
{
  using handle_type = I2C_HandleTypeDef;
  template <typename Config> static auto& handle() noexcept
  {
    return i2c_api::handle<Config>();
  }
  template <typename Config> static status initialize() noexcept
  {
    return i2c_api::initialize<Config>() ? status::ok : status::io_error;
  }
  static bool ready(handle_type& h) noexcept
  {
    return HAL_I2C_GetState(&h) == HAL_I2C_STATE_READY;
  }
  static status error(handle_type& h) noexcept
  {
    return i2c_api::error_status(h);
  }
  static status stop(handle_type& h) noexcept
  {
    __HAL_I2C_DISABLE(&h);
    auto result = HAL_OK;
    if (serial_stop_dma(h.hdmatx) != status::ok)
      result = HAL_ERROR;
    if (serial_stop_dma(h.hdmarx) != status::ok)
      result = HAL_ERROR;
    if (HAL_I2C_DeInit(&h) != HAL_OK)
      result = HAL_ERROR;
    return serial_status(result);
  }
#if defined(USE_HAL_I2C_REGISTER_CALLBACKS) && USE_HAL_I2C_REGISTER_CALLBACKS == 1
  static constexpr bool callbacks = true;
  template <typename Config> static auto& tx_callback(handle_type& h) noexcept
  {
    if constexpr (serial_slave_enabled<Config>())
      return h.SlaveTxCpltCallback;
    else
      return h.MasterTxCpltCallback;
  }
  template <typename Config> static auto& rx_callback(handle_type& h) noexcept
  {
    if constexpr (serial_slave_enabled<Config>())
      return h.SlaveRxCpltCallback;
    else
      return h.MasterRxCpltCallback;
  }
  static auto& error_callback(handle_type& h) noexcept
  {
    return h.ErrorCallback;
  }
#else
  static constexpr bool callbacks = false;
#endif
  template <typename Config>
  static status start(handle_type& h, const std::span<const std::uint8_t> tx, const std::span<std::uint8_t> rx,
                      const std::uint16_t address, const bool first, const bool last) noexcept
  {
    constexpr auto mode = configured_transfer_mode<Config>();
    if constexpr (serial_slave_enabled<Config>())
    {
      if (!first || !last)
        return status::unsupported;
      const auto count = static_cast<std::uint16_t>(rx.empty() ? tx.size() : rx.size());
      if constexpr (mode == transfer_mode::polled)
        return serial_status(rx.empty()
                                 ? HAL_I2C_Slave_Transmit(&h, hal_tx_buffer(tx.data()), count, timeout_for<Config>())
                                 : HAL_I2C_Slave_Receive(&h, rx.data(), count, timeout_for<Config>()));
      else if constexpr (mode == transfer_mode::dma)
      {
        if ((rx.empty() ? h.hdmatx : h.hdmarx) == nullptr)
          return status::unsupported;
        return serial_status(rx.empty() ? HAL_I2C_Slave_Transmit_DMA(&h, hal_tx_buffer(tx.data()), count)
                                        : HAL_I2C_Slave_Receive_DMA(&h, rx.data(), count));
      }
      else
        return serial_status(rx.empty() ? HAL_I2C_Slave_Transmit_IT(&h, hal_tx_buffer(tx.data()), count)
                                        : HAL_I2C_Slave_Receive_IT(&h, rx.data(), count));
    }
    const auto addr =
        Config::address_mode == i2c_address_mode::seven_bit ? static_cast<std::uint16_t>(address << 1U) : address;
    if constexpr (mode == transfer_mode::polled)
    {
      if (!first || !last)
        return status::unsupported;
      return serial_status(rx.empty()
                               ? HAL_I2C_Master_Transmit(&h, addr, hal_tx_buffer(tx.data()),
                                                         static_cast<std::uint16_t>(tx.size()), timeout_for<Config>())
                               : HAL_I2C_Master_Receive(&h, addr, rx.data(), static_cast<std::uint16_t>(rx.size()),
                                                        timeout_for<Config>()));
    }
    if constexpr (mode == transfer_mode::dma)
      if ((rx.empty() ? h.hdmatx : h.hdmarx) == nullptr)
        return status::unsupported;
    if (!first || !last)
    {
#if defined(I2C_FIRST_FRAME) && defined(I2C_LAST_FRAME)
      const auto frame = first ? I2C_FIRST_FRAME : I2C_LAST_FRAME;
      if constexpr (mode == transfer_mode::dma)
        return serial_status(rx.empty() ? HAL_I2C_Master_Seq_Transmit_DMA(&h, addr, hal_tx_buffer(tx.data()),
                                                                          static_cast<std::uint16_t>(tx.size()), frame)
                                        : HAL_I2C_Master_Seq_Receive_DMA(&h, addr, rx.data(),
                                                                         static_cast<std::uint16_t>(rx.size()), frame));
      else
        return serial_status(rx.empty() ? HAL_I2C_Master_Seq_Transmit_IT(&h, addr, hal_tx_buffer(tx.data()),
                                                                         static_cast<std::uint16_t>(tx.size()), frame)
                                        : HAL_I2C_Master_Seq_Receive_IT(&h, addr, rx.data(),
                                                                        static_cast<std::uint16_t>(rx.size()), frame));
#else
      return status::unsupported;
#endif
    }
    if constexpr (mode == transfer_mode::dma)
      return serial_status(
          rx.empty()
              ? HAL_I2C_Master_Transmit_DMA(&h, addr, hal_tx_buffer(tx.data()), static_cast<std::uint16_t>(tx.size()))
              : HAL_I2C_Master_Receive_DMA(&h, addr, rx.data(), static_cast<std::uint16_t>(rx.size())));
    else
      return serial_status(
          rx.empty()
              ? HAL_I2C_Master_Transmit_IT(&h, addr, hal_tx_buffer(tx.data()), static_cast<std::uint16_t>(tx.size()))
              : HAL_I2C_Master_Receive_IT(&h, addr, rx.data(), static_cast<std::uint16_t>(rx.size())));
  }
};
#endif

#if MICROHAL_STM32_HAL_HAS_SPI
template <> struct serial_driver<peripheral_kind::spi>
{
  using handle_type = SPI_HandleTypeDef;
  template <typename Config> static auto& handle() noexcept
  {
    return spi_api::handle<Config>();
  }
  template <typename Config> static status initialize() noexcept
  {
    return spi_api::initialize<Config>() ? status::ok : status::io_error;
  }
  static bool ready(handle_type& h) noexcept
  {
    return HAL_SPI_GetState(&h) == HAL_SPI_STATE_READY;
  }
  static status error(handle_type& h) noexcept
  {
    return HAL_SPI_GetError(&h) == HAL_SPI_ERROR_NONE ? status::ok : status::io_error;
  }
  static status stop(handle_type& h) noexcept
  {
    const auto stopped = serial_status(HAL_SPI_Abort(&h));
    if (stopped != status::ok)
      return stopped;
    const auto tx = serial_stop_dma(h.hdmatx);
    const auto rx = serial_stop_dma(h.hdmarx);
    return tx == status::ok ? rx : tx;
  }
#if defined(USE_HAL_SPI_REGISTER_CALLBACKS) && USE_HAL_SPI_REGISTER_CALLBACKS == 1
  static constexpr bool callbacks = true;
  static auto& tx_callback(handle_type& h) noexcept
  {
    return h.TxCpltCallback;
  }
  static auto& rx_callback(handle_type& h) noexcept
  {
    return h.RxCpltCallback;
  }
  static auto& duplex_callback(handle_type& h) noexcept
  {
    return h.TxRxCpltCallback;
  }
  static auto& error_callback(handle_type& h) noexcept
  {
    return h.ErrorCallback;
  }
#else
  static constexpr bool callbacks = false;
#endif
  template <typename Config>
  static status start(handle_type& h, const std::span<const std::uint8_t> tx, const std::span<std::uint8_t> rx,
                      std::uint16_t, bool, bool) noexcept
  {
    constexpr auto mode = configured_transfer_mode<Config>();
    if constexpr (Config::duplex_mode == spi_duplex_mode::simplex_rx)
      if (!tx.empty())
        return status::unsupported;
    if constexpr (Config::duplex_mode == spi_duplex_mode::simplex_tx)
      if (!rx.empty())
        return status::unsupported;
    if constexpr (Config::duplex_mode != spi_duplex_mode::full)
      if (!tx.empty() && !rx.empty())
        return status::unsupported;
    const auto count = static_cast<std::uint16_t>(tx.empty() ? rx.size() : tx.size());
    if constexpr (mode == transfer_mode::polled)
    {
      if (tx.empty())
        return serial_status(HAL_SPI_Receive(&h, rx.data(), count, timeout_for<Config>()));
      if (rx.empty())
        return serial_status(HAL_SPI_Transmit(&h, hal_tx_buffer(tx.data()), count, timeout_for<Config>()));
      return serial_status(
          HAL_SPI_TransmitReceive(&h, hal_tx_buffer(tx.data()), rx.data(), count, timeout_for<Config>()));
    }
    if constexpr (mode == transfer_mode::dma)
    {
      if ((!tx.empty() && h.hdmatx == nullptr) || (!rx.empty() && h.hdmarx == nullptr))
        return status::unsupported;
      // Master receive on two-line SPI also transmits dummy bytes.
      if (!serial_slave_enabled<Config>() && tx.empty() && h.Init.Direction == SPI_DIRECTION_2LINES &&
          h.hdmatx == nullptr)
        return status::unsupported;
      if (tx.empty())
        return serial_status(HAL_SPI_Receive_DMA(&h, rx.data(), count));
      if (rx.empty())
        return serial_status(HAL_SPI_Transmit_DMA(&h, hal_tx_buffer(tx.data()), count));
      return serial_status(HAL_SPI_TransmitReceive_DMA(&h, hal_tx_buffer(tx.data()), rx.data(), count));
    }
    else
    {
      if (tx.empty())
        return serial_status(HAL_SPI_Receive_IT(&h, rx.data(), count));
      if (rx.empty())
        return serial_status(HAL_SPI_Transmit_IT(&h, hal_tx_buffer(tx.data()), count));
      return serial_status(HAL_SPI_TransmitReceive_IT(&h, hal_tx_buffer(tx.data()), rx.data(), count));
    }
  }
};
#endif

#if MICROHAL_STM32_HAL_HAS_UART
template <> struct serial_driver<peripheral_kind::uart>
{
  using handle_type = UART_HandleTypeDef;
  template <typename Config> static auto& handle() noexcept
  {
    return uart_api::handle<Config>();
  }
  template <typename Config> static status initialize() noexcept
  {
    return uart_api::initialize<Config>() ? status::ok : status::io_error;
  }
  static bool ready(handle_type& h) noexcept
  {
    return HAL_UART_GetState(&h) == HAL_UART_STATE_READY;
  }
  static status error(handle_type& h) noexcept
  {
    return HAL_UART_GetError(&h) == HAL_UART_ERROR_NONE ? status::ok : status::io_error;
  }
  static bool transmit_error(handle_type& h) noexcept
  {
    return (HAL_UART_GetError(&h) & HAL_UART_ERROR_DMA) != 0U && h.hdmatx != nullptr &&
           HAL_DMA_GetError(h.hdmatx) != HAL_DMA_ERROR_NONE;
  }
  static bool receive_error(handle_type& h) noexcept
  {
    return !transmit_error(h) || (HAL_UART_GetError(&h) & ~HAL_UART_ERROR_DMA) != 0U ||
           (h.hdmarx != nullptr && HAL_DMA_GetError(h.hdmarx) != HAL_DMA_ERROR_NONE);
  }
  static status stop(handle_type& h) noexcept
  {
    return serial_status(HAL_UART_Abort(&h));
  }
  static bool direction_ready(handle_type& h, bool tx) noexcept
  {
    return (tx ? h.gState : h.RxState) == HAL_UART_STATE_READY;
  }
  static status stop_direction(handle_type& h, bool tx) noexcept
  {
    const auto stopped = serial_status(tx ? HAL_UART_AbortTransmit(&h) : HAL_UART_AbortReceive(&h));
    if (stopped != status::ok)
      return stopped;
    // HAL clears DMAT/DMAR before aborting DMA. Retry a timed-out DMA abort even
    // when a later UART abort sees the request bit already cleared.
    return serial_stop_dma(tx ? h.hdmatx : h.hdmarx);
  }
  static bool circular(handle_type& h) noexcept
  {
#if defined(DMA_CIRCULAR)
    return h.hdmarx != nullptr && h.hdmarx->Init.Mode == DMA_CIRCULAR;
#elif defined(DMA_LINKEDLIST_CIRCULAR)
    return h.hdmarx != nullptr && h.hdmarx->Mode == DMA_LINKEDLIST_CIRCULAR;
#else
    (void)h;
    return false;
#endif
  }
  static bool tx_circular(handle_type& h) noexcept
  {
#if defined(DMA_CIRCULAR)
    return h.hdmatx != nullptr && h.hdmatx->Init.Mode == DMA_CIRCULAR;
#elif defined(DMA_LINKEDLIST_CIRCULAR)
    return h.hdmatx != nullptr && h.hdmatx->Mode == DMA_LINKEDLIST_CIRCULAR;
#else
    (void)h;
    return false;
#endif
  }
  static void interrupt(handle_type& h) noexcept
  {
    HAL_UART_IRQHandler(&h);
  }
  static void dma_interrupt(handle_type& h) noexcept
  {
    if (h.hdmatx != nullptr)
      HAL_DMA_IRQHandler(h.hdmatx);
    if (h.hdmarx != nullptr && h.hdmarx != h.hdmatx)
      HAL_DMA_IRQHandler(h.hdmarx);
  }
#if defined(USE_HAL_UART_REGISTER_CALLBACKS) && USE_HAL_UART_REGISTER_CALLBACKS == 1
  static constexpr bool callbacks = true;
  static auto& tx_callback(handle_type& h) noexcept
  {
    return h.TxCpltCallback;
  }
  static auto& rx_callback(handle_type& h) noexcept
  {
    return h.RxCpltCallback;
  }
  static auto& error_callback(handle_type& h) noexcept
  {
    return h.ErrorCallback;
  }
#if defined(HAL_UART_RECEPTION_TOIDLE)
  static constexpr bool idle_supported = true;
  static auto& event_callback(handle_type& h) noexcept
  {
    return h.RxEventCallback;
  }
  static bool half_event(handle_type& h) noexcept
  {
    return HAL_UARTEx_GetRxEventType(&h) == HAL_UART_RXEVENT_HT;
  }
  static bool full_event(handle_type& h) noexcept
  {
    return HAL_UARTEx_GetRxEventType(&h) == HAL_UART_RXEVENT_TC;
  }
  template <typename Config> static status start_idle(handle_type& h, std::span<std::uint8_t> rx) noexcept
  {
    if constexpr (configured_transfer_mode<Config>() == transfer_mode::dma)
    {
      if (h.hdmarx == nullptr)
        return status::unsupported;
#if defined(DMA_LINKEDLIST_CIRCULAR)
      if (circular(h) && (h.hdmarx->LinkedListQueue == nullptr || h.hdmarx->LinkedListQueue->Head == nullptr))
        return status::unsupported;
#endif
      return serial_status(HAL_UARTEx_ReceiveToIdle_DMA(&h, rx.data(), static_cast<std::uint16_t>(rx.size())));
    }
    else if constexpr (configured_transfer_mode<Config>() == transfer_mode::interrupt)
      return serial_status(HAL_UARTEx_ReceiveToIdle_IT(&h, rx.data(), static_cast<std::uint16_t>(rx.size())));
    return status::unsupported;
  }
#else
  static constexpr bool idle_supported = false;
#endif
#else
  static constexpr bool callbacks = false;
  static constexpr bool idle_supported = false;
#endif
  template <typename Config>
  static status start(handle_type& h, const std::span<const std::uint8_t> tx, const std::span<std::uint8_t> rx,
                      std::uint16_t, bool, bool) noexcept
  {
    constexpr auto mode = configured_transfer_mode<Config>();
    const auto count = static_cast<std::uint16_t>(tx.empty() ? rx.size() : tx.size());
    if constexpr (mode == transfer_mode::polled)
      return serial_status(rx.empty() ? HAL_UART_Transmit(&h, hal_tx_buffer(tx.data()), count, timeout_for<Config>())
                                      : HAL_UART_Receive(&h, rx.data(), count, timeout_for<Config>()));
    if constexpr (mode == transfer_mode::dma)
    {
      if ((rx.empty() ? h.hdmatx : h.hdmarx) == nullptr)
        return status::unsupported;
      return serial_status(rx.empty() ? HAL_UART_Transmit_DMA(&h, hal_tx_buffer(tx.data()), count)
                                      : HAL_UART_Receive_DMA(&h, rx.data(), count));
    }
    else
      return serial_status(rx.empty() ? HAL_UART_Transmit_IT(&h, hal_tx_buffer(tx.data()), count)
                                      : HAL_UART_Receive_IT(&h, rx.data(), count));
  }
};
#endif

#if MICROHAL_STM32_HAL_HAS_SPI && defined(HAL_I2S_MODULE_ENABLED) && defined(I2S_STANDARD_PHILIPS)
template <> struct serial_driver<peripheral_kind::i2s>
{
  using handle_type = I2S_HandleTypeDef;
  template <typename Config> static auto& handle() noexcept
  {
    return i2s_api::handle<Config>();
  }
  template <typename Config> static status initialize() noexcept
  {
    auto& h = handle<Config>();
    h.Instance = peripheral_instance<SPI_TypeDef, Config>();
    h.Init.Mode = Config::mode == i2s_mode::master_tx   ? I2S_MODE_MASTER_TX
                  : Config::mode == i2s_mode::master_rx ? I2S_MODE_MASTER_RX
                  : Config::mode == i2s_mode::slave_tx  ? I2S_MODE_SLAVE_TX
                                                        : I2S_MODE_SLAVE_RX;
    h.Init.Standard = Config::standard == i2s_standard::philips         ? I2S_STANDARD_PHILIPS
                      : Config::standard == i2s_standard::msb_justified ? I2S_STANDARD_MSB
                      : Config::standard == i2s_standard::lsb_justified ? I2S_STANDARD_LSB
                      : Config::standard == i2s_standard::pcm_short     ? I2S_STANDARD_PCM_SHORT
                                                                        : I2S_STANDARD_PCM_LONG;
    h.Init.DataFormat = I2S_DATAFORMAT_16B;
    h.Init.MCLKOutput = Config::enable_master_clock ? I2S_MCLKOUTPUT_ENABLE : I2S_MCLKOUTPUT_DISABLE;
    h.Init.AudioFreq = Config::sample_rate;
    h.Init.CPOL = I2S_CPOL_LOW;
#if defined(I2S_CLOCK_PLL)
    h.Init.ClockSource = I2S_CLOCK_PLL;
#endif
    return serial_status(HAL_I2S_Init(&h));
  }
  static bool ready(handle_type& h) noexcept
  {
    return HAL_I2S_GetState(&h) == HAL_I2S_STATE_READY;
  }
  static status error(handle_type& h) noexcept
  {
    return h.ErrorCode == HAL_I2S_ERROR_NONE ? status::ok : status::io_error;
  }
  static status stop(handle_type& h) noexcept
  {
#if defined(I2S_IT_TXE) && defined(I2S_IT_RXNE)
    __HAL_I2S_DISABLE_IT(&h, I2S_IT_TXE | I2S_IT_RXNE | I2S_IT_ERR);
#else
    __HAL_I2S_DISABLE_IT(&h, I2S_IT_TXP | I2S_IT_RXP | I2S_IT_ERR);
#endif
    __HAL_I2S_DISABLE(&h);
    auto result = HAL_OK;
    if (serial_stop_dma(h.hdmatx) != status::ok)
      result = HAL_ERROR;
    if (serial_stop_dma(h.hdmarx) != status::ok)
      result = HAL_ERROR;
    if (HAL_I2S_DeInit(&h) != HAL_OK)
      result = HAL_ERROR;
    return serial_status(result);
  }
#if defined(USE_HAL_I2S_REGISTER_CALLBACKS) && USE_HAL_I2S_REGISTER_CALLBACKS == 1
  static constexpr bool callbacks = true;
  static auto& tx_callback(handle_type& h) noexcept
  {
    return h.TxCpltCallback;
  }
  static auto& rx_callback(handle_type& h) noexcept
  {
    return h.RxCpltCallback;
  }
  static auto& error_callback(handle_type& h) noexcept
  {
    return h.ErrorCallback;
  }
#else
  static constexpr bool callbacks = false;
#endif
  template <typename Config>
  static status start(handle_type& h, const std::span<const std::uint16_t> tx, const std::span<std::uint16_t> rx,
                      std::uint16_t, bool, bool) noexcept
  {
    constexpr bool transmit = Config::mode == i2s_mode::master_tx || Config::mode == i2s_mode::slave_tx;
    if (transmit != rx.empty())
      return status::unsupported;
    constexpr auto mode = configured_transfer_mode<Config>();
    const auto count = static_cast<std::uint16_t>(tx.empty() ? rx.size() : tx.size());
    if constexpr (mode == transfer_mode::polled)
      return serial_status(rx.empty() ? HAL_I2S_Transmit(&h, hal_tx_buffer(tx.data()), count, timeout_for<Config>())
                                      : HAL_I2S_Receive(&h, rx.data(), count, timeout_for<Config>()));
    if constexpr (mode == transfer_mode::dma)
    {
      if ((rx.empty() ? h.hdmatx : h.hdmarx) == nullptr)
        return status::unsupported;
      return serial_status(rx.empty() ? HAL_I2S_Transmit_DMA(&h, hal_tx_buffer(tx.data()), count)
                                      : HAL_I2S_Receive_DMA(&h, rx.data(), count));
    }
    else
      return serial_status(rx.empty() ? HAL_I2S_Transmit_IT(&h, hal_tx_buffer(tx.data()), count)
                                      : HAL_I2S_Receive_IT(&h, rx.data(), count));
  }
};
#endif

template <peripheral_kind Kind> struct serial_engine
{
  using driver = serial_driver<Kind>;
  using handle_type = typename driver::handle_type;
  using callback_type = void (*)(handle_type*);
  template <typename Config> static auto& tx_slot(handle_type& h) noexcept
  {
    if constexpr (requires { driver::template tx_callback<Config>(h); })
      return driver::template tx_callback<Config>(h);
    else
      return driver::tx_callback(h);
  }
  template <typename Config> static auto& rx_slot(handle_type& h) noexcept
  {
    if constexpr (requires { driver::template rx_callback<Config>(h); })
      return driver::template rx_callback<Config>(h);
    else
      return driver::rx_callback(h);
  }
  template <typename Config> struct operation
  {
    std::atomic<bool> active{false};
    std::atomic<bool> complete{false};
    std::atomic<status> result{status::unsupported};
    std::atomic<bool> initialized{false};
    std::atomic<bool> stopping{false};
    void (*notify)(status) noexcept {nullptr};
    callback_type tx{}, rx{}, duplex{}, error{};
    std::span<std::uint8_t> next_rx{};
    std::uint16_t address{};
  };
  template <typename Config> static auto& state() noexcept
  {
    static operation<Config> value{};
    return value;
  }
  template <auto Handler> static void notify(const status result) noexcept
  {
    static_assert(
        requires { Handler(result); } || requires { Handler(); },
        "microHAL: completion handler must accept status or no arguments");
    if constexpr (requires { Handler(result); })
      Handler(result);
    else if constexpr (requires { Handler(); })
      Handler();
  }
  template <typename Config> static status initialize() noexcept
  {
    if constexpr (configured_transfer_mode<Config>() == transfer_mode::automatic)
      return status::unsupported;
    if constexpr (!driver::callbacks && configured_transfer_mode<Config>() != transfer_mode::polled)
      return status::unsupported;
    auto& s = state<Config>();
    if (s.active.load())
      return status::busy;
    s.initialized = false;
    const auto result = driver::template initialize<Config>();
    if (result != status::ok)
    {
      s.result = result;
      return result;
    }
    auto& h = driver::template handle<Config>();
    if constexpr (configured_transfer_mode<Config>() == transfer_mode::dma)
    {
      if constexpr (requires { Config::binding::configure_dma(h); })
      {
        const auto configured = microhal::detail::as_status([&] { return Config::binding::configure_dma(h); });
        if (configured != status::ok)
        {
          s.result = configured;
          return configured;
        }
      }
    }
    s.initialized = true;
    s.result = status::ok;
    return status::ok;
  }
  template <typename Config> static void restore() noexcept
  {
    if constexpr (driver::callbacks)
    {
      auto& h = driver::template handle<Config>();
      auto& s = state<Config>();
      tx_slot<Config>(h) = s.tx;
      rx_slot<Config>(h) = s.rx;
      driver::error_callback(h) = s.error;
      if constexpr (Kind == peripheral_kind::spi)
        driver::duplex_callback(h) = s.duplex;
    }
  }
  template <typename Config> static void finish(const status result) noexcept
  {
    auto& s = state<Config>();
    if (!s.active.load())
      return;
    restore<Config>();
    const auto callback = s.notify;
    s.notify = nullptr;
    s.next_rx = {};
    s.result = result;
    s.complete.store(true, std::memory_order_release);
    s.active.store(false);
    if (callback != nullptr)
      callback(result);
  }
  template <typename Config> static void completed(handle_type* h) noexcept
  {
    auto& s = state<Config>();
    if (!s.active.load() || s.stopping || h != &driver::template handle<Config>())
      return;
    if constexpr (Kind == peripheral_kind::i2c)
    {
      if (!s.next_rx.empty())
      {
        const auto rx = s.next_rx;
        s.next_rx = {};
        const auto result = driver::template start<Config>(*h, {}, rx, s.address, false, true);
        if (result != status::ok)
        {
          s.stopping = true;
          const auto stopped = driver::stop(*h);
          s.initialized = false;
          if (stopped != status::ok)
          {
            s.result = stopped;
            return;
          }
          s.stopping = false;
          finish<Config>(result);
        }
        return;
      }
    }
    finish<Config>(driver::error(*h));
  }
  template <typename Config> static void failed(handle_type* h) noexcept
  {
    auto& s = state<Config>();
    if (!s.active.load() || s.stopping || h != &driver::template handle<Config>())
      return;
    const auto error = driver::error(*h);
    s.stopping = true;
    const auto stopped = driver::stop(*h);
    s.initialized = false;
    if (stopped != status::ok)
    {
      s.result = stopped;
      return;
    }
    s.stopping = false;
    finish<Config>(error == status::ok ? status::io_error : error);
  }
  template <typename Config> static status cancel(const status reason) noexcept
  {
    auto& s = state<Config>();
    if (!s.active.load())
      return status::ok;
    s.stopping = true;
    const auto result = driver::stop(driver::template handle<Config>());
    s.initialized = false;
    if (result != status::ok)
    {
      s.result = result;
      return result;
    }
    s.stopping = false;
    finish<Config>(reason);
    return result;
  }
  template <typename Config> static status abort() noexcept
  {
    return cancel<Config>(status::error);
  }
  template <typename Config> static status transfer_status() noexcept
  {
    return state<Config>().result.load();
  }
  template <typename Config> static status probe(const std::uint16_t address) noexcept
  {
    if constexpr (Kind == peripheral_kind::i2c)
    {
      auto& s = state<Config>();
      if (s.active.load())
        return status::busy;
      if (!s.initialized)
        return status::unsupported;
      if (!serial_claim(s.active))
        return status::busy;
      auto& h = driver::template handle<Config>();
      const auto addr =
          Config::address_mode == i2c_address_mode::seven_bit ? static_cast<std::uint16_t>(address << 1U) : address;
      const auto result = serial_status(HAL_I2C_IsDeviceReady(&h, addr, 1U, timeout_for<Config>()));
      s.result = result;
      s.active.store(false);
      return result;
    }
    return status::unsupported;
  }
  template <typename Config, auto Handler, typename Sample>
  static status submit(const std::span<const Sample> tx, const std::span<Sample> rx, const std::uint16_t address = 0U,
                       const bool combined = false) noexcept
  {
    constexpr bool synchronous = std::is_same_v<decltype(Handler), std::nullptr_t>;
    constexpr auto mode = configured_transfer_mode<Config>();
    if constexpr (mode == transfer_mode::automatic || (!synchronous && mode == transfer_mode::polled))
      return status::unsupported;
    if (tx.empty() && rx.empty())
      return status::invalid_argument;
    if (tx.size() > UINT16_MAX || rx.size() > UINT16_MAX)
      return status::invalid_argument;
    auto& s = state<Config>();
    if (s.active.load())
      return status::busy;
    if (!s.initialized)
      return status::unsupported;
    if (!serial_claim(s.active))
      return status::busy;
    auto& h = driver::template handle<Config>();
    if (!driver::ready(h))
    {
      s.active.store(false);
      return status::busy;
    }
    if constexpr (mode == transfer_mode::dma)
    {
#if defined(DMA_CIRCULAR)
      if ((h.hdmatx != nullptr && h.hdmatx->Init.Mode == DMA_CIRCULAR) ||
          (h.hdmarx != nullptr && h.hdmarx->Init.Mode == DMA_CIRCULAR))
      {
        s.active.store(false);
        return status::unsupported;
      }
#elif defined(DMA_LINKEDLIST_CIRCULAR)
      if ((h.hdmatx != nullptr && h.hdmatx->Mode == DMA_LINKEDLIST_CIRCULAR) ||
          (h.hdmarx != nullptr && h.hdmarx->Mode == DMA_LINKEDLIST_CIRCULAR))
      {
        s.active.store(false);
        return status::unsupported;
      }
#endif
    }
    if constexpr (mode == transfer_mode::polled)
    {
      const auto result = driver::template start<Config>(h, tx, rx, address, true, !combined);
      s.result = result;
      s.active.store(false);
      return result;
    }
    else if constexpr (driver::callbacks)
    {
      if constexpr (Kind == peripheral_kind::i2c && mode == transfer_mode::dma)
        if (combined && (h.hdmatx == nullptr || h.hdmarx == nullptr))
        {
          s.active.store(false);
          return status::unsupported;
        }
      s.complete.store(false);
      s.stopping = false;
      s.result = status::busy;
      if constexpr (synchronous)
        s.notify = nullptr;
      else
        s.notify = notify<Handler>;
      s.tx = tx_slot<Config>(h);
      s.rx = rx_slot<Config>(h);
      s.error = driver::error_callback(h);
      tx_slot<Config>(h) = completed<Config>;
      rx_slot<Config>(h) = completed<Config>;
      driver::error_callback(h) = failed<Config>;
      if constexpr (Kind == peripheral_kind::spi)
      {
        s.duplex = driver::duplex_callback(h);
        driver::duplex_callback(h) = completed<Config>;
      }
      if constexpr (Kind == peripheral_kind::i2c)
      {
        s.next_rx = combined ? rx : std::span<Sample>{};
        s.address = address;
      }
      const auto result =
          driver::template start<Config>(h, tx, combined ? std::span<Sample>{} : rx, address, true, !combined);
      if (result != status::ok)
      {
        s.notify = nullptr;
        if (result == status::unsupported || result == status::invalid_argument || result == status::busy)
        {
          finish<Config>(result);
          return result;
        }
        s.stopping = true;
        const auto stopped = driver::stop(h);
        s.initialized = false;
        if (stopped != status::ok)
        {
          s.result = stopped;
          return stopped;
        }
        s.stopping = false;
        finish<Config>(result);
        return result;
      }
      if constexpr (!synchronous)
        return status::ok;
      const auto start = HAL_GetTick();
      while (!s.complete.load(std::memory_order_acquire))
      {
        if (s.result.load() != status::busy)
          return s.result.load();
        if (HAL_GetTick() - start >= timeout_for<Config>())
        {
          const auto stopped = cancel<Config>(status::timeout);
          return stopped == status::ok ? status::timeout : stopped;
        }
      }
      return s.result;
    }
    else
    {
      s.active.store(false);
      return status::unsupported;
    }
  }
  template <typename Config, auto Handler>
  static status write(std::span<const std::uint8_t> tx, std::uint16_t address = 0U) noexcept
  {
    return submit<Config, Handler, std::uint8_t>(tx, {}, address);
  }
  template <typename Config, auto Handler>
  static status read(std::span<std::uint8_t> rx, std::uint16_t address = 0U) noexcept
  {
    return submit<Config, Handler, std::uint8_t>({}, rx, address);
  }
  template <typename Config, auto Handler>
  static status transfer(std::span<const std::uint8_t> tx, std::span<std::uint8_t> rx) noexcept
  {
    if (!tx.empty() && !rx.empty() && tx.size() != rx.size())
      return status::invalid_argument;
    return submit<Config, Handler, std::uint8_t>(tx, rx);
  }
  template <typename Config, auto Handler>
  static status write_read(std::span<const std::uint8_t> tx, std::span<std::uint8_t> rx, std::uint16_t address) noexcept
  {
    if (tx.empty() || rx.empty())
      return status::invalid_argument;
    return submit<Config, Handler, std::uint8_t>(tx, rx, address, true);
  }
  template <typename Config, auto Handler> static status transmit(std::span<const std::uint16_t> tx) noexcept
  {
    return submit<Config, Handler, std::uint16_t>(tx, {});
  }
  template <typename Config, auto Handler> static status receive(std::span<std::uint16_t> rx) noexcept
  {
    return submit<Config, Handler, std::uint16_t>({}, rx);
  }
  template <typename Config> static status handle_interrupt() noexcept
  {
    if (!state<Config>().initialized.load() && !state<Config>().active.load())
      return status::unsupported;
    auto& h = driver::template handle<Config>();
    if constexpr (Kind == peripheral_kind::i2c)
    {
      HAL_I2C_EV_IRQHandler(&h);
      HAL_I2C_ER_IRQHandler(&h);
    }
    if constexpr (Kind == peripheral_kind::spi)
      HAL_SPI_IRQHandler(&h);
    if constexpr (Kind == peripheral_kind::uart)
      HAL_UART_IRQHandler(&h);
    if constexpr (Kind == peripheral_kind::i2s)
      HAL_I2S_IRQHandler(&h);
    return status::ok;
  }
  template <typename Config> static status handle_dma_interrupt() noexcept
  {
    if (!state<Config>().initialized.load() && !state<Config>().active.load())
      return status::unsupported;
    auto& h = driver::template handle<Config>();
    if (h.hdmatx != nullptr)
      HAL_DMA_IRQHandler(h.hdmatx);
    if (h.hdmarx != nullptr && h.hdmarx != h.hdmatx)
      HAL_DMA_IRQHandler(h.hdmarx);
    return status::ok;
  }
};
/** Independent UART direction ownership; error callbacks stop DMA before returning buffers. */
template <typename Driver> struct uart_engine
{
  using driver = Driver;
  using handle_type = typename driver::handle_type;
  using callback_type = void (*)(handle_type*);
  template <typename Config> struct operation
  {
    std::atomic<bool> active{false};
    std::atomic<status> result{status::unsupported};
    bool stopping{};
    void (*notify)(status) noexcept {};
    void (*receive)(status, std::span<std::uint8_t>) noexcept {};
    std::span<std::uint8_t> buffer;
    std::size_t position{};
    std::size_t generation{};
    bool streaming{}, circular{}, pending_full{};
    callback_type previous{};
    void (*event)(handle_type*, std::uint16_t){};
  };
  template <typename Config> struct context
  {
    operation<Config> tx{}, rx{};
    bool initialized{}, error_installed{}, aborting{};
    std::atomic<status> result{status::unsupported};
    callback_type error{};
  };
  template <typename Config> static auto& state() noexcept
  {
    static context<Config> value;
    return value;
  }
  template <typename Config> static auto& direction(bool tx) noexcept
  {
    auto& s = state<Config>();
    return tx ? s.tx : s.rx;
  }
  template <typename Config> static status initialize() noexcept
  {
    auto& s = state<Config>();
    if (s.tx.active || s.rx.active)
      return status::busy;
    s.initialized = false;
    if constexpr (configured_transfer_mode<Config>() == transfer_mode::automatic ||
                  (!driver::callbacks && configured_transfer_mode<Config>() != transfer_mode::polled))
      return status::unsupported;
    auto result = driver::template initialize<Config>();
    if (result != status::ok)
    {
      s.tx.result = s.rx.result = result;
      s.result = result;
      return result;
    }
    auto& h = driver::template handle<Config>();
    if constexpr (configured_transfer_mode<Config>() == transfer_mode::dma &&
                  requires { Config::binding::configure_dma(h); })
      result = microhal::detail::as_status([&] { return Config::binding::configure_dma(h); });
    s.initialized = result == status::ok;
    s.tx.result = s.rx.result = result;
    s.result = result;
    return result;
  }
  template <typename Config> static void finish(bool tx, status result, std::span<std::uint8_t> bytes = {}) noexcept
  {
    auto& s = state<Config>();
    auto& op = direction<Config>(tx);
    if (!op.active)
      return;
    if constexpr (driver::callbacks)
    {
      auto& h = driver::template handle<Config>();
      (tx ? driver::tx_callback(h) : driver::rx_callback(h)) = op.previous;
      if constexpr (driver::idle_supported)
        if (!tx && op.streaming)
          driver::event_callback(h) = op.event;
      if (!(tx ? s.rx.active.load() : s.tx.active.load()))
      {
        driver::error_callback(h) = s.error;
        s.error_installed = false;
      }
    }
    const auto notify = op.notify;
    const auto receive = op.receive;
    op.notify = nullptr;
    op.receive = nullptr;
    op.buffer = {};
    op.result = result;
    s.result = result;
    op.stopping = false;
    op.active = false;
    if (receive != nullptr)
      receive(result, result == status::ok ? bytes : std::span<std::uint8_t>{});
    else if (notify != nullptr)
      notify(result);
  }
  template <typename Config> static status cancel(bool tx, status reason) noexcept
  {
    auto& op = direction<Config>(tx);
    if (!op.active)
      return status::ok;
    op.stopping = true;
    state<Config>().aborting = true;
    const auto stopped = driver::stop_direction(driver::template handle<Config>(), tx);
    state<Config>().aborting = false;
    if (stopped != status::ok)
    {
      op.result = stopped;
      state<Config>().result = stopped;
      return stopped;
    }
    finish<Config>(tx, reason);
    return status::ok;
  }
  template <typename Config> static void tx_completed(handle_type* h) noexcept
  {
    if (h == &driver::template handle<Config>() && !state<Config>().tx.stopping)
      finish<Config>(true, status::ok);
  }
  template <typename Config> static void rx_completed(handle_type* h) noexcept
  {
    if (h == &driver::template handle<Config>() && !state<Config>().rx.stopping && !state<Config>().rx.streaming)
      finish<Config>(false, status::ok);
  }
  template <typename Config> static void failed(handle_type* h) noexcept
  {
    if (h != &driver::template handle<Config>())
      return;
    if (state<Config>().aborting)
      return;
    const auto error = driver::error(*h);
    const auto result = error == status::ok ? status::io_error : error;
    bool tx_error = !state<Config>().rx.active.load();
    bool rx_error = true;
    if constexpr (requires {
                    driver::transmit_error(*h);
                    driver::receive_error(*h);
                  })
    {
      tx_error = driver::transmit_error(*h);
      rx_error = driver::receive_error(*h);
    }
    const auto tx_generation = state<Config>().tx.generation;
    if (rx_error)
      (void)cancel<Config>(false, result);
    if (tx_error && state<Config>().tx.generation == tx_generation)
      (void)cancel<Config>(true, result);
  }
  template <typename Config> static void event(handle_type* h, std::uint16_t position) noexcept
  {
    auto& op = state<Config>().rx;
    if (h != &driver::template handle<Config>() || !op.active || op.stopping || !op.streaming ||
        position > op.buffer.size())
      return;
    if (!op.circular)
    {
      // DMA half-transfer is progress, not the end of a finite receive.
      if constexpr (configured_transfer_mode<Config>() == transfer_mode::dma)
        if (!driver::direction_ready(*h, false))
          return;
      finish<Config>(false, status::ok, op.buffer.first(position));
      return;
    }
    const auto end = static_cast<std::size_t>(position);
    const auto generation = op.generation;
    bool half = false;
    bool full = false;
    if constexpr (requires {
                    driver::half_event(*h);
                    driver::full_event(*h);
                  })
    {
      half = driver::half_event(*h);
      full = driver::full_event(*h);
    }
    // IDLE may run before a pending DMA half/full interrupt from the same cycle.
    if (half && (op.pending_full || (end <= op.position && op.position != op.buffer.size())))
      return;
    if (full && op.pending_full)
    {
      op.pending_full = false;
      return;
    }
    if (end == op.position || (end == 0U && op.position == op.buffer.size()))
      return;
    auto deliver = [&](std::size_t first, std::size_t last)
    {
      if (last > first && op.receive != nullptr)
        op.receive(status::ok, op.buffer.subspan(first, last - first));
    };
    const auto previous = op.position;
    if (!half && !full && end < previous && previous != op.buffer.size())
      op.pending_full = true;
    op.position = end;
    if (end < previous)
    {
      deliver(previous, op.buffer.size());
      if (!op.active || op.stopping || op.generation != generation)
        return;
      deliver(0U, end);
    }
    else
      deliver(previous, end);
  }
  template <typename Config> static void install(bool tx) noexcept
  {
    auto& s = state<Config>();
    auto& op = direction<Config>(tx);
    auto& h = driver::template handle<Config>();
    auto& callback = tx ? driver::tx_callback(h) : driver::rx_callback(h);
    op.previous = callback;
    callback = tx ? tx_completed<Config> : rx_completed<Config>;
    if (!s.error_installed)
    {
      s.error = driver::error_callback(h);
      driver::error_callback(h) = failed<Config>;
      s.error_installed = true;
    }
  }
  template <auto Handler> static void notify(status result) noexcept
  {
    if constexpr (requires { Handler(result); })
      Handler(result);
    else
      Handler();
  }
  template <typename Config, auto Handler>
  static status submit(std::span<const std::uint8_t> tx, std::span<std::uint8_t> rx) noexcept
  {
    constexpr auto mode = configured_transfer_mode<Config>();
    constexpr bool sync = std::is_same_v<decltype(Handler), std::nullptr_t>;
    if constexpr (mode == transfer_mode::automatic || (!sync && mode == transfer_mode::polled))
      return status::unsupported;
    const bool transmit = !tx.empty();
    if ((tx.empty() && rx.empty()) || tx.size() > UINT16_MAX || rx.size() > UINT16_MAX)
      return status::invalid_argument;
    auto& s = state<Config>();
    auto& op = direction<Config>(transmit);
    if (!s.initialized)
      return status::unsupported;
    if (!serial_claim(op.active))
      return status::busy;
    auto& h = driver::template handle<Config>();
    if (!driver::direction_ready(h, transmit))
    {
      op.active = false;
      return status::busy;
    }
    if constexpr (mode == transfer_mode::dma)
    {
      if (!transmit && driver::circular(h))
      {
        op.active = false;
        return status::unsupported;
      }
      if constexpr (requires { driver::tx_circular(h); })
        if (transmit && driver::tx_circular(h))
        {
          op.active = false;
          return status::unsupported;
        }
    }
    if constexpr (mode == transfer_mode::polled)
    {
      const auto result = driver::template start<Config>(h, tx, rx, 0, true, true);
      op.result = result;
      s.result = result;
      op.active = false;
      return result;
    }
    else if constexpr (driver::callbacks)
    {
      op.result = status::busy;
      s.result = status::busy;
      ++op.generation;
      op.stopping = op.streaming = op.circular = false;
      op.receive = nullptr;
      if constexpr (sync)
        op.notify = nullptr;
      else
        op.notify = notify<Handler>;
      install<Config>(transmit);
      const auto result = driver::template start<Config>(h, tx, rx, 0, true, true);
      if (result != status::ok)
      {
        op.notify = nullptr;
        if (result == status::unsupported || result == status::busy || result == status::invalid_argument)
          finish<Config>(transmit, result);
        else
          (void)cancel<Config>(transmit, result);
        return op.active ? op.result.load() : result;
      }
      if constexpr (!sync)
        return status::ok;
      const auto start = HAL_GetTick();
      while (op.active)
      {
        if (op.result != status::busy)
          return op.result;
        if (HAL_GetTick() - start >= timeout_for<Config>())
        {
          const auto stopped = cancel<Config>(transmit, status::timeout);
          return stopped == status::ok ? status::timeout : stopped;
        }
      }
      return op.result;
    }
    op.active = false;
    return status::unsupported;
  }
  template <typename Config, auto Handler, bool Circular>
  static status start_stream(std::span<std::uint8_t> buffer) noexcept
  {
    static_assert(valid_uart_receive_handler<Handler>());
    constexpr auto mode = configured_transfer_mode<Config>();
    if constexpr (!driver::idle_supported || !driver::callbacks ||
                  (mode != transfer_mode::interrupt && mode != transfer_mode::dma) ||
                  (Circular && mode != transfer_mode::dma))
      return status::unsupported;
    else
    {
      if (buffer.empty() || buffer.size() > UINT16_MAX || (Circular && buffer.size() < 2U))
        return status::invalid_argument;
      auto& s = state<Config>();
      auto& op = s.rx;
      if (!s.initialized)
        return status::unsupported;
      if (!serial_claim(op.active))
        return status::busy;
      auto& h = driver::template handle<Config>();
      if (!driver::direction_ready(h, false))
      {
        op.active = false;
        return status::busy;
      }
      if constexpr (mode == transfer_mode::dma)
        if (driver::circular(h) != Circular)
        {
          op.active = false;
          return status::unsupported;
        }
      op.result = status::busy;
      s.result = status::busy;
      ++op.generation;
      op.stopping = false;
      op.streaming = true;
      op.circular = Circular;
      op.pending_full = false;
      op.buffer = buffer;
      op.position = 0;
      op.notify = nullptr;
      op.receive = [](status result, std::span<std::uint8_t> bytes) noexcept { Handler(result, bytes); };
      install<Config>(false);
      op.event = driver::event_callback(h);
      driver::event_callback(h) = event<Config>;
      const auto result = driver::template start_idle<Config>(h, buffer);
      if (result != status::ok)
      {
        op.receive = nullptr;
        if (result == status::unsupported || result == status::busy || result == status::invalid_argument)
          finish<Config>(false, result);
        else
          (void)cancel<Config>(false, result);
        return op.active ? op.result.load() : result;
      }
      return status::ok;
    }
  }
  template <typename Config, auto Handler> static status start_receive_to_idle(std::span<std::uint8_t> bytes) noexcept
  {
    return start_stream<Config, Handler, false>(bytes);
  }
  template <typename Config, auto Handler> static status start_circular_rx(std::span<std::uint8_t> bytes) noexcept
  {
    return start_stream<Config, Handler, true>(bytes);
  }
  template <typename Config, auto Handler> static status write(std::span<const std::uint8_t> bytes) noexcept
  {
    return submit<Config, Handler>(bytes, {});
  }
  template <typename Config, auto Handler> static status read(std::span<std::uint8_t> bytes) noexcept
  {
    return submit<Config, Handler>({}, bytes);
  }
  template <typename Config> static status abort_tx() noexcept
  {
    return cancel<Config>(true, status::error);
  }
  template <typename Config> static status abort_rx() noexcept
  {
    return cancel<Config>(false, status::error);
  }
  template <typename Config> static status abort() noexcept
  {
    const auto tx = abort_tx<Config>();
    const auto rx = abort_rx<Config>();
    return tx == status::ok ? rx : tx;
  }
  template <typename Config> static status tx_status() noexcept
  {
    return state<Config>().tx.result;
  }
  template <typename Config> static status rx_status() noexcept
  {
    return state<Config>().rx.result;
  }
  template <typename Config> static status transfer_status() noexcept
  {
    auto& s = state<Config>();
    if (s.tx.active && s.tx.result != status::busy)
      return s.tx.result;
    if (s.rx.active && s.rx.result != status::busy)
      return s.rx.result;
    if (s.tx.active || s.rx.active)
      return status::busy;
    return s.result;
  }
  template <typename Config> static status handle_interrupt() noexcept
  {
    if (!state<Config>().initialized)
      return status::unsupported;
    driver::interrupt(driver::template handle<Config>());
    return status::ok;
  }
  template <typename Config> static status handle_dma_interrupt() noexcept
  {
    if (!state<Config>().initialized)
      return status::unsupported;
    driver::dma_interrupt(driver::template handle<Config>());
    return status::ok;
  }
};
} // namespace microhal::backends::stm32::hal

namespace microhal
{
#if MICROHAL_STM32_HAL_HAS_I2C
template <>
struct serial_mode_api<stm32_hal_tag, peripheral_kind::i2c> : backends::stm32::hal::serial_engine<peripheral_kind::i2c>
{
};
#endif
#if MICROHAL_STM32_HAL_HAS_SPI
template <>
struct serial_mode_api<stm32_hal_tag, peripheral_kind::spi> : backends::stm32::hal::serial_engine<peripheral_kind::spi>
{
};
#endif
#if MICROHAL_STM32_HAL_HAS_UART
template <>
struct serial_mode_api<stm32_hal_tag, peripheral_kind::uart>
    : backends::stm32::hal::uart_engine<backends::stm32::hal::serial_driver<peripheral_kind::uart>>
{
};
#endif
#if MICROHAL_STM32_HAL_HAS_SPI && defined(HAL_I2S_MODULE_ENABLED) && defined(I2S_STANDARD_PHILIPS)
template <>
struct serial_mode_api<stm32_hal_tag, peripheral_kind::i2s> : backends::stm32::hal::serial_engine<peripheral_kind::i2s>
{
};
template <>
struct serial_mode_api<stm32_ll_tag, peripheral_kind::i2s> : serial_mode_api<stm32_hal_tag, peripheral_kind::i2s>
{
};
#endif
} // namespace microhal
