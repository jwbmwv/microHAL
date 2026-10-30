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
    if (h.hdmatx != nullptr && HAL_DMA_GetState(h.hdmatx) == HAL_DMA_STATE_BUSY && HAL_DMA_Abort(h.hdmatx) != HAL_OK)
      result = HAL_ERROR;
    if (h.hdmarx != nullptr && HAL_DMA_GetState(h.hdmarx) == HAL_DMA_STATE_BUSY && HAL_DMA_Abort(h.hdmarx) != HAL_OK)
      result = HAL_ERROR;
    if (HAL_I2C_DeInit(&h) != HAL_OK)
      result = HAL_ERROR;
    return serial_status(result);
  }
#if defined(USE_HAL_I2C_REGISTER_CALLBACKS) && USE_HAL_I2C_REGISTER_CALLBACKS == 1
  static constexpr bool callbacks = true;
  static auto& tx_callback(handle_type& h) noexcept
  {
    return h.MasterTxCpltCallback;
  }
  static auto& rx_callback(handle_type& h) noexcept
  {
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
    return serial_status(HAL_SPI_Abort(&h));
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
      if (tx.empty() && h.Init.Direction == SPI_DIRECTION_2LINES && h.hdmatx == nullptr)
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
  static status stop(handle_type& h) noexcept
  {
    return serial_status(HAL_UART_Abort(&h));
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
#else
  static constexpr bool callbacks = false;
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

#if MICROHAL_STM32_HAL_HAS_SPI && defined(HAL_I2S_MODULE_ENABLED)
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
    __HAL_I2S_DISABLE_IT(&h, I2S_IT_TXE | I2S_IT_RXNE | I2S_IT_ERR);
    __HAL_I2S_DISABLE(&h);
    auto result = HAL_OK;
    if (h.hdmatx != nullptr && HAL_DMA_GetState(h.hdmatx) == HAL_DMA_STATE_BUSY && HAL_DMA_Abort(h.hdmatx) != HAL_OK)
      result = HAL_ERROR;
    if (h.hdmarx != nullptr && HAL_DMA_GetState(h.hdmarx) == HAL_DMA_STATE_BUSY && HAL_DMA_Abort(h.hdmarx) != HAL_OK)
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
      driver::tx_callback(h) = s.tx;
      driver::rx_callback(h) = s.rx;
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
      s.tx = driver::tx_callback(h);
      s.rx = driver::rx_callback(h);
      s.error = driver::error_callback(h);
      driver::tx_callback(h) = completed<Config>;
      driver::rx_callback(h) = completed<Config>;
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
};
} // namespace microhal::backends::stm32::hal

namespace microhal
{
#if MICROHAL_STM32_HAL_HAS_I2C
template <>
struct serial_mode_api<stm32_hal_tag, peripheral_kind::i2c> : backends::stm32::hal::serial_engine<peripheral_kind::i2c>
{
};
template <>
struct serial_mode_api<stm32_ll_tag, peripheral_kind::i2c> : serial_mode_api<stm32_hal_tag, peripheral_kind::i2c>
{
};
#endif
#if MICROHAL_STM32_HAL_HAS_SPI
template <>
struct serial_mode_api<stm32_hal_tag, peripheral_kind::spi> : backends::stm32::hal::serial_engine<peripheral_kind::spi>
{
};
template <>
struct serial_mode_api<stm32_ll_tag, peripheral_kind::spi> : serial_mode_api<stm32_hal_tag, peripheral_kind::spi>
{
};
#endif
#if MICROHAL_STM32_HAL_HAS_UART
template <>
struct serial_mode_api<stm32_hal_tag, peripheral_kind::uart>
    : backends::stm32::hal::serial_engine<peripheral_kind::uart>
{
};
template <>
struct serial_mode_api<stm32_ll_tag, peripheral_kind::uart> : serial_mode_api<stm32_hal_tag, peripheral_kind::uart>
{
};
#endif
#if MICROHAL_STM32_HAL_HAS_SPI && defined(HAL_I2S_MODULE_ENABLED)
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
