// SPDX-License-Identifier: Apache-2.0
/**
 * @file concepts.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <array>
#include <concepts>
#include <cstdint>
#include <string_view>
#include <type_traits>

#include "microhal/core/backend_selector.hpp"

namespace microhal
{

#if defined(__cpp_static_assert) && __cpp_static_assert >= 202306L
namespace detail
{

/// @brief Builds a compile-time diagnostic for an out-of-range clock.
template <std::size_t N>
consteval auto clock_diagnostic(const std::uint32_t requested, const std::array<char, N>& prefix,
                                const std::uint32_t maximum) noexcept
{
  constexpr std::string_view middle{" Hz is outside backend range 1.."};
  constexpr std::string_view suffix{" Hz."};
  constexpr std::size_t capacity = N + middle.size() + suffix.size() + 20U;
  struct message
  {
    std::array<char, capacity> text{};
    std::size_t length{};

    /// @brief Returns the size of the associated data.
    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
      return length;
    }
    /// @brief Returns the underlying data pointer.
    [[nodiscard]] constexpr const char* data() const noexcept
    {
      return text.data();
    }
  };
  message output{};
  const auto append = [&output](const std::string_view text)
  {
    for (const char character : text)
    {
      output.text.at(output.length++) = character;
    }
  };
  const auto append_number = [&output](std::uint32_t value)
  {
    std::array<char, 10U> digits{};
    std::size_t count{};
    while (true)
    {
      digits.at(count++) = static_cast<char>('0' + value % 10U);
      value /= 10U;
      if (value == 0U)
      {
        break;
      }
    }
    while (count != 0U)
    {
      output.text.at(output.length++) = digits.at(--count);
    }
  };
  append(std::string_view{prefix.data(), N - 1U});
  append_number(requested);
  append(middle);
  append_number(maximum);
  append(suffix);
  return output;
}

} // namespace detail
#endif

/**
 * @brief Checks whether a capability type exposes the common capability flags.
 * @tparam Capability Type whose `supported`, `blocking`, `non_blocking`, `isr`, and `dma` members are inspected.
 */
template <typename Capability>
concept CapabilityConcept = requires(const Capability capability) {
  { capability.supported } -> std::convertible_to<bool>;
  { capability.blocking } -> std::convertible_to<bool>;
  { capability.non_blocking } -> std::convertible_to<bool>;
  { capability.isr } -> std::convertible_to<bool>;
  { capability.dma } -> std::convertible_to<bool>;
};

/**
 * @brief Checks whether a backend tag is registered with its identity traits.
 * @tparam Backend Backend tag to inspect.
 */
template <typename Backend>
concept BackendConcept = requires {
  typename backend_traits<Backend>::backend_tag;
  { backend_traits<Backend>::name } -> std::convertible_to<std::string_view>;
  { backend_traits<Backend>::vendor } -> std::convertible_to<vendor_kind>;
  { backend_traits<Backend>::kind } -> std::convertible_to<backend_kind>;
};

/**
 * @brief Checks whether a peripheral API type exposes its backend tag and peripheral kind.
 * @tparam Peripheral Peripheral API type to inspect.
 */
template <typename Peripheral>
concept PeripheralConcept = requires {
  typename Peripheral::backend_tag;
  { Peripheral::kind } -> std::convertible_to<peripheral_kind>;
};

/**
 * @name Predicate layer
 * @brief Pure, SFINAE-friendly capability predicates.
 * @details These never hard-error. A caller may legitimately ask "does this backend support DMA
 *          on this peripheral?" and branch on the answer with `if constexpr`, so evaluating one of
 *          these to `false` must stay a normal answer rather than a compilation failure. The
 *          diagnostic layer below is what produces precise messages for genuinely invalid code.
 */
/// @{

/**
 * @brief Tests peripheral availability without producing a diagnostic for an unregistered backend.
 * @tparam Backend Backend tag to query.
 * @tparam Peripheral Peripheral kind to query.
 * @return `true` when @p Backend satisfies @ref BackendConcept and reports @p Peripheral as supported;
 *         otherwise `false`.
 */
template <typename Backend, peripheral_kind Peripheral> consteval bool peripheral_available() noexcept
{
  if constexpr (BackendConcept<Backend>)
  {
    return peripheral_supported<Backend, Peripheral>();
  }
  else
  {
    return false;
  }
}

/**
 * @brief Tests ISR capability without producing a diagnostic for an unregistered backend.
 * @tparam Backend Backend tag to query.
 * @tparam Peripheral Peripheral kind to query.
 * @return `true` when @p Backend satisfies @ref BackendConcept and its @p Peripheral capability advertises ISR support.
 */
template <typename Backend, peripheral_kind Peripheral> consteval bool peripheral_isr_available() noexcept
{
  if constexpr (BackendConcept<Backend>)
  {
    return peripheral_supports_isr<Backend, Peripheral>();
  }
  else
  {
    return false;
  }
}

/**
 * @brief Tests DMA capability without producing a diagnostic for an unregistered backend.
 * @tparam Backend Backend tag to query.
 * @tparam Peripheral Peripheral kind to query.
 * @return `true` when @p Backend satisfies @ref BackendConcept and its @p Peripheral capability advertises DMA support.
 */
template <typename Backend, peripheral_kind Peripheral> consteval bool peripheral_dma_available() noexcept
{
  if constexpr (BackendConcept<Backend>)
  {
    return peripheral_supports_dma<Backend, Peripheral>();
  }
  else
  {
    return false;
  }
}

/// @brief Compile-time value indicating whether @p Backend supports @p Peripheral.
template <typename Backend, peripheral_kind Peripheral>
inline constexpr bool peripheral_supported_v = peripheral_available<Backend, Peripheral>();

/**
 * @brief Requires that a backend support the requested peripheral.
 * @tparam Backend Backend tag to query.
 * @tparam Peripheral Peripheral kind to query.
 */
template <typename Backend, peripheral_kind Peripheral>
concept SupportedPeripheralConcept = peripheral_available<Backend, Peripheral>();

/**
 * @brief Requires a supported peripheral whose capability advertises ISR use.
 * @tparam Backend Backend tag to query.
 * @tparam Peripheral Peripheral kind to query.
 */
template <typename Backend, peripheral_kind Peripheral>
concept IsrCapableConcept =
    peripheral_available<Backend, Peripheral>() && peripheral_isr_available<Backend, Peripheral>();

/**
 * @brief Requires a supported peripheral whose capability advertises DMA.
 * @tparam Backend Backend tag to query.
 * @tparam Peripheral Peripheral kind to query.
 */
template <typename Backend, peripheral_kind Peripheral>
concept DmaCapableConcept =
    peripheral_available<Backend, Peripheral>() && peripheral_dma_available<Backend, Peripheral>();
/// @}

/**
 * @name Diagnostic layer
 * @brief Helpers that deliberately hard-error with a specific message.
 * @details These are invoked from the `validate_*_config` routines. They are intentionally *not*
 *          used to define the predicates above.
 */
/// @{
/**
 * @brief Requires a registered backend that advertises @p Peripheral.
 * @tparam Backend Backend tag to validate.
 * @tparam Peripheral Peripheral kind to require.
 * @return `true` when validation succeeds.
 * @note An unregistered backend or unsupported peripheral triggers a compile-time assertion.
 */
template <typename Backend, peripheral_kind Peripheral> consteval bool validate_capability() noexcept
{
  static_assert(BackendConcept<Backend>, "microHAL: backend tag is not registered in backend_traits.");
  static_assert(peripheral_supported<Backend, Peripheral>(),
                "microHAL: backend does not support the requested peripheral.");
  return true;
}
/// @}

/**
 * @brief Detects whether a configuration supplies a backend binding type.
 * @tparam Config Configuration type to inspect.
 */
template <typename Config>
concept BindingBackedConfig = requires { typename Config::binding; };

/**
 * @brief Tests whether a GPIO pin configuration matches a backend's GPIO contract.
 * @tparam Backend Backend tag to validate against.
 * @tparam PinConfig Pin configuration type.
 * @return `true` when the backend supports GPIO and the configuration has the required binding or pin fields,
 *         mode fields, and (for legacy configs) valid pin and alternate-function selections.
 */
template <typename Backend, typename PinConfig> consteval bool gpio_pin_compatible() noexcept
{
  if constexpr (!BackendConcept<Backend> || !peripheral_available<Backend, peripheral_kind::gpio>())
  {
    return false;
  }
  else if constexpr (!(BindingBackedConfig<PinConfig> || requires {
                       PinConfig::port_index;
                       PinConfig::pin_index;
                     }))
  {
    return false;
  }
  else if constexpr (!(requires {
                       PinConfig::mode;
                       PinConfig::pull;
                       PinConfig::speed;
                       PinConfig::interrupt_trigger;
                     }))
  {
    return false;
  }
  else if constexpr (!BindingBackedConfig<PinConfig>)
  {
    if constexpr (!(requires { PinConfig::alternate_function; }))
    {
      return false;
    }
    else
    {
      return backend_traits<Backend>::template is_valid_gpio_pin<PinConfig>() &&
             backend_traits<Backend>::template is_valid_alternate_function<PinConfig>();
    }
  }
  else
  {
    return true;
  }
}

/**
 * @brief Tests I2C configuration structure and capability compatibility.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config I2C configuration type.
 * @return `true` when the backend supports I2C and the required instance, clock, address, transfer-mode,
 *         DMA, and interrupt constraints are satisfied.
 */
template <typename Backend, typename Config> consteval bool i2c_config_compatible() noexcept
{
  if constexpr (!BackendConcept<Backend> || !peripheral_available<Backend, peripheral_kind::i2c>())
  {
    return false;
  }
  else if constexpr (!(BindingBackedConfig<Config> || requires { Config::instance_index; }))
  {
    return false;
  }
  else if constexpr (!(requires {
                       Config::clock_hz;
                       Config::address_mode;
                       Config::enable_dma;
                       Config::enable_interrupts;
                     }))
  {
    return false;
  }
  else
  {
    const auto capability = backend_traits<Backend>::i2c_capability();
    constexpr bool instance_valid = []
    {
      if constexpr (requires { Config::instance_index; })
      {
        constexpr auto instance_count = backend_traits<Backend>::i2c_capability().instance_count;
        return (instance_count == 0U) || (Config::instance_index <= instance_count);
      }
      return true;
    }();
    constexpr bool clock_stretch_valid = []
    {
      if constexpr (requires {
                      Config::clock_stretch_policy;
                      Config::clock_stretch_timeout_us;
                    })
      {
        constexpr auto stretch_capability = backend_traits<Backend>::i2c_capability();
        constexpr bool policy_supported = (Config::clock_stretch_policy == i2c_clock_stretch_policy::backend_default) ||
                                          ((Config::clock_stretch_policy == i2c_clock_stretch_policy::allow) &&
                                           stretch_capability.clock_stretching) ||
                                          ((Config::clock_stretch_policy == i2c_clock_stretch_policy::disallow) &&
                                           stretch_capability.clock_stretch_control);
        constexpr bool timeout_supported =
            (Config::clock_stretch_timeout_us == 0U) ||
            ((stretch_capability.max_clock_stretch_timeout_us != 0U) &&
             (Config::clock_stretch_timeout_us <= stretch_capability.max_clock_stretch_timeout_us));
        return policy_supported && timeout_supported;
      }
      return true;
    }();
    return instance_valid && capability.transfer_modes.supports(configured_transfer_mode<Config>()) &&
           (Config::clock_hz > 0U) && (Config::clock_hz <= capability.max_frequency_hz) && clock_stretch_valid &&
           ((Config::address_mode != i2c_address_mode::ten_bit) || capability.ten_bit_addressing) &&
           ((!serial_dma_enabled<Config>()) || DmaCapableConcept<Backend, peripheral_kind::i2c>) &&
           ((!serial_interrupt_enabled<Config>()) || IsrCapableConcept<Backend, peripheral_kind::i2c>);
  }
}

/**
 * @brief Tests SPI configuration structure and capability compatibility.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config SPI configuration type.
 * @return `true` when the backend supports SPI and the required instance, clock, transfer-mode, DMA, and
 *         interrupt constraints are satisfied.
 */
template <typename Backend, typename Config> consteval bool spi_config_compatible() noexcept
{
  if constexpr (!BackendConcept<Backend> || !peripheral_available<Backend, peripheral_kind::spi>())
  {
    return false;
  }
  else if constexpr (!(BindingBackedConfig<Config> || requires { Config::instance_index; }))
  {
    return false;
  }
  else if constexpr (!(requires {
                       Config::clock_hz;
                       Config::duplex_mode;
                       Config::clock_polarity;
                       Config::clock_phase;
                       Config::bit_order;
                       Config::enable_dma;
                       Config::enable_interrupts;
                     }))
  {
    return false;
  }
  else
  {
    return backend_traits<Backend>::spi_capability().transfer_modes.supports(configured_transfer_mode<Config>()) &&
           (Config::clock_hz > 0U) &&
           (Config::clock_hz <= backend_traits<Backend>::spi_capability().max_frequency_hz) &&
           ((!serial_dma_enabled<Config>()) || DmaCapableConcept<Backend, peripheral_kind::spi>) &&
           ((!serial_interrupt_enabled<Config>()) || IsrCapableConcept<Backend, peripheral_kind::spi>);
  }
}

/**
 * @brief Tests QSPI configuration and required backend API compatibility.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config QSPI configuration type.
 * @return `true` when required capabilities, fields, DMA support, and any backend-specific binding operations match.
 */
template <typename Backend, typename Config> consteval bool qspi_config_compatible() noexcept
{
  if constexpr (!BackendConcept<Backend> || !peripheral_available<Backend, peripheral_kind::qspi>())
  {
    return false;
  }
  else if constexpr (!backend_traits<Backend>::qspi_capability().quad_mode ||
                     !backend_traits<Backend>::qspi_capability().command_transfer)
  {
    return false;
  }
  else if constexpr (!(BindingBackedConfig<Config> || requires { Config::instance_index; }))
  {
    return false;
  }
  else if constexpr (!(requires {
                       Config::clock_hz;
                       Config::enable_dma;
                       Config::enable_interrupts;
                     }))
  {
    return false;
  }
  else
  {
    constexpr bool dma_compatible = (!Config::enable_dma) || DmaCapableConcept<Backend, peripheral_kind::qspi>;
    if constexpr (std::same_as<Backend, kinetis_espresso_tag> || std::same_as<Backend, zephyr_tag> ||
                  std::same_as<Backend, nuttx_tag>)
    {
      return dma_compatible && requires(const qspi_command& command, const std::span<const std::uint8_t> tx,
                                        const std::span<std::uint8_t> rx) {
        typename Config::binding;
        Config::binding::template initialize<Config>();
        Config::binding::template command_write<Config>(command, tx);
        Config::binding::template command_read<Config>(command, rx);
        Config::binding::template enter_memory_mapped<Config>(command);
        Config::binding::template exit_memory_mapped<Config>();
      };
    }
    return dma_compatible;
  }
}

/**
 * @brief Tests UART configuration structure and capability compatibility.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config UART configuration type.
 * @return `true` when the backend supports the configured baud, mode, transfer, FIFO/LPUART, parity, and stop-bit
 *         requirements.
 */
template <typename Backend, typename Config> consteval bool uart_config_compatible() noexcept
{
  if constexpr (!BackendConcept<Backend> || !peripheral_available<Backend, peripheral_kind::uart>())
  {
    return false;
  }
  else if constexpr (!(BindingBackedConfig<Config> || requires { Config::instance_index; }))
  {
    return false;
  }
  else if constexpr (!(requires {
                       Config::baud_rate;
                       Config::parity;
                       Config::stop_bits;
                       Config::enable_dma;
                       Config::enable_interrupts;
                       Config::require_fifo;
                       Config::require_lpuart;
                     }))
  {
    return false;
  }
  else
  {
    const auto capability = backend_traits<Backend>::uart_capability();
    constexpr std::uint16_t supported_data_bits = backend_traits<Backend>::uart_capability().data_bits_mask == 0U
                                                      ? uart_data_bits_mask(uart_data_bits::eight)
                                                      : backend_traits<Backend>::uart_capability().data_bits_mask;
    constexpr uart_data_bits requested_data_bits = []
    {
      if constexpr (requires { Config::data_bits; })
      {
        return Config::data_bits;
      }
      else
      {
        return uart_data_bits::eight;
      }
    }();
    return capability.transfer_modes.supports(configured_transfer_mode<Config>()) && (Config::baud_rate > 0U) &&
           ((!serial_dma_enabled<Config>()) || DmaCapableConcept<Backend, peripheral_kind::uart>) &&
           ((!serial_interrupt_enabled<Config>()) || IsrCapableConcept<Backend, peripheral_kind::uart>) &&
           ((!Config::require_fifo) || backend_traits<Backend>::uart_capability().fifo) &&
           ((!Config::require_lpuart) || capability.lpuart) &&
           ((capability.max_baud_rate == 0U) || (Config::baud_rate <= capability.max_baud_rate)) &&
           ((Config::parity == uart_parity::none) || capability.parity) &&
           ((supported_data_bits & uart_data_bits_mask(requested_data_bits)) != 0U) &&
           ((requested_data_bits != uart_data_bits::nine) || (Config::parity == uart_parity::none)) &&
           ((Config::stop_bits == uart_stop_bits::one) ||
            ((Config::stop_bits == uart_stop_bits::two) && capability.two_stop_bits) ||
            ((Config::stop_bits == uart_stop_bits::one_and_half) && capability.one_and_half_stop_bits));
  }
}

/**
 * @brief Tests timer configuration structure and DMA/interrupt capability requirements.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config Timer configuration type.
 * @return `true` when the backend supports timers and the required configuration fields and enabled features.
 */
template <typename Backend, typename Config> consteval bool timer_config_compatible() noexcept
{
  if constexpr (!BackendConcept<Backend> || !peripheral_available<Backend, peripheral_kind::timer>())
  {
    return false;
  }
  else if constexpr (!(BindingBackedConfig<Config> || requires { Config::instance_index; }))
  {
    return false;
  }
  else if constexpr (!(requires {
                       Config::clock_hz;
                       Config::counter_mode;
                       Config::clock_source;
                       Config::enable_dma;
                       Config::enable_interrupts;
                     }))
  {
    return false;
  }
  else
  {
    return (Config::clock_hz > 0U) &&
           ((!serial_dma_enabled<Config>()) || DmaCapableConcept<Backend, peripheral_kind::timer>) &&
           ((!Config::enable_interrupts) || IsrCapableConcept<Backend, peripheral_kind::timer>);
  }
}

/**
 * @brief Tests flash geometry and DMA requirements for a configuration.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config Flash configuration type.
 * @return `true` when configured program alignment and erase size are nonzero, meet integral backend granularity
 *         constraints, and any requested DMA is supported.
 */
template <typename Backend, typename Config> consteval bool flash_config_compatible() noexcept
{
  if constexpr (!BackendConcept<Backend> || !peripheral_available<Backend, peripheral_kind::flash>())
  {
    return false;
  }
  else if constexpr (!(requires {
                       Config::program_alignment_bytes;
                       Config::erase_size_bytes;
                       Config::enable_dma;
                       Config::enable_interrupts;
                     }))
  {
    return false;
  }
  else
  {
    return (Config::program_alignment_bytes > 0U) && (Config::erase_size_bytes > 0U) &&
           (backend_traits<Backend>::flash_capability().alignment_bytes > 0U) &&
           (backend_traits<Backend>::flash_capability().erase_bytes > 0U) &&
           (Config::program_alignment_bytes >= backend_traits<Backend>::flash_capability().alignment_bytes) &&
           ((Config::program_alignment_bytes % backend_traits<Backend>::flash_capability().alignment_bytes) == 0U) &&
           (Config::erase_size_bytes >= backend_traits<Backend>::flash_capability().erase_bytes) &&
           ((Config::erase_size_bytes % backend_traits<Backend>::flash_capability().erase_bytes) == 0U) &&
           ((!Config::enable_dma) || DmaCapableConcept<Backend, peripheral_kind::flash>);
  }
}

/**
 * @brief Tests RTC configuration structure and interrupt requirements.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config RTC configuration type.
 * @return `true` when the backend supports RTC and any configured periodic or alarm interrupt is supported.
 */
template <typename Backend, typename Config> consteval bool rtc_config_compatible() noexcept
{
  if constexpr (!BackendConcept<Backend> || !peripheral_available<Backend, peripheral_kind::rtc>())
  {
    return false;
  }
  else if constexpr (!(requires {
                       Config::enable_periodic_irq;
                       Config::enable_alarm_irq;
                     }))
  {
    return false;
  }
  else
  {
    return (!Config::enable_periodic_irq && !Config::enable_alarm_irq) ||
           IsrCapableConcept<Backend, peripheral_kind::rtc>;
  }
}

template <typename Backend, typename UartConfig, typename AutobaudConfig>
/**
 * @brief Tests UART autobaud configuration composition and backend support.
 * @tparam Backend Backend tag to validate against.
 * @tparam UartConfig UART configuration type.
 * @tparam AutobaudConfig Type providing `timer_config`, `gpio_pin`, `min_baud`, and `max_baud`.
 * @return `true` when the UART, timer, and GPIO configurations are compatible, the backend advertises autobaud,
 *         and the minimum baud is below the maximum.
 */
consteval bool autobaud_config_compatible() noexcept
{
  if constexpr (!uart_config_compatible<Backend, UartConfig>())
  {
    return false;
  }
  else if constexpr (!(requires {
                       typename AutobaudConfig::timer_config;
                       typename AutobaudConfig::gpio_pin;
                       AutobaudConfig::min_baud;
                       AutobaudConfig::max_baud;
                     }))
  {
    return false;
  }
  else
  {
    return timer_config_compatible<Backend, typename AutobaudConfig::timer_config>() &&
           gpio_pin_compatible<Backend, typename AutobaudConfig::gpio_pin>() &&
           backend_traits<Backend>::uart_capability().autobaud && (AutobaudConfig::min_baud < AutobaudConfig::max_baud);
  }
}

/**
 * @brief Tests ADC configuration structure and enabled DMA/interrupt support.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config ADC configuration type.
 * @return `true` when the backend supports ADC, the required fields and instance binding are present, and enabled
 *         DMA or interrupt operation is supported.
 */
template <typename Backend, typename Config> consteval bool adc_config_compatible() noexcept
{
  if constexpr (!BackendConcept<Backend> || !peripheral_available<Backend, peripheral_kind::adc>())
  {
    return false;
  }
  else if constexpr (!(BindingBackedConfig<Config> || requires { Config::instance_index; }))
  {
    return false;
  }
  else if constexpr (!(requires {
                       Config::resolution;
                       Config::sample_time;
                       Config::enable_dma;
                       Config::enable_interrupts;
                       Config::differential_mode;
                     }))
  {
    return false;
  }
  else
  {
    return ((!Config::enable_dma) || DmaCapableConcept<Backend, peripheral_kind::adc>) &&
           ((!Config::enable_interrupts) || IsrCapableConcept<Backend, peripheral_kind::adc>);
  }
}

/**
 * @brief Tests DAC configuration structure and enabled DMA support.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config DAC configuration type.
 * @return `true` when the backend supports DAC, the required channel/configuration members are present, and requested
 *         DMA is supported.
 */
template <typename Backend, typename Config> consteval bool dac_config_compatible() noexcept
{
  if constexpr (!BackendConcept<Backend> || !peripheral_available<Backend, peripheral_kind::dac>())
  {
    return false;
  }
  else if constexpr (!(BindingBackedConfig<Config> || requires {
                       Config::instance_index;
                       Config::channel;
                     }))
  {
    return false;
  }
  else if constexpr (!(requires {
                       Config::resolution;
                       Config::enable_dma;
                       Config::enable_waveform;
                     }))
  {
    return false;
  }
  else
  {
    return (!Config::enable_dma) || DmaCapableConcept<Backend, peripheral_kind::dac>;
  }
}

/**
 * @brief Tests CAN configuration structure and interrupt requirements.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config CAN configuration type.
 * @return `true` when the backend supports CAN, required configuration members exist, baudrate is nonzero, and any
 *         enabled interrupts are supported.
 */
template <typename Backend, typename Config> consteval bool can_config_compatible() noexcept
{
  if constexpr (!BackendConcept<Backend> || !peripheral_available<Backend, peripheral_kind::can>())
  {
    return false;
  }
  else if constexpr (!(BindingBackedConfig<Config> || requires { Config::instance_index; }))
  {
    return false;
  }
  else if constexpr (!(requires {
                       Config::baudrate;
                       Config::mode;
                       Config::enable_can_fd;
                       Config::enable_interrupts;
                     }))
  {
    return false;
  }
  else
  {
    constexpr auto capability = backend_traits<Backend>::can_capability();
    constexpr bool instance_valid = []
    {
      if constexpr (BindingBackedConfig<Config>)
      {
        return true;
      }
      else
      {
        return Config::instance_index <= backend_traits<Backend>::can_capability().instance_count;
      }
    }();
    return instance_valid && (Config::baudrate > 0U) &&
           ((capability.max_bitrate == 0U) || (Config::baudrate <= capability.max_bitrate)) &&
           ((!Config::enable_can_fd) || capability.can_fd) &&
           ((!Config::enable_interrupts) || IsrCapableConcept<Backend, peripheral_kind::can>);
  }
}

/**
 * @brief Tests USB configuration mode, speed, and backend API compatibility.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config USB configuration type.
 * @return `true` when endpoint transfer is supported, at least one role is enabled, the requested roles and speed
 *         are available, and any required backend binding operations are present.
 */
template <typename Backend, typename Config> consteval bool usb_config_compatible() noexcept
{
  if constexpr (!BackendConcept<Backend> || !peripheral_available<Backend, peripheral_kind::usb>())
  {
    return false;
  }
  else if constexpr (!backend_traits<Backend>::usb_capability().endpoint_transfer)
  {
    return false;
  }
  else if constexpr (!(BindingBackedConfig<Config> || requires { Config::instance_index; }))
  {
    return false;
  }
  else if constexpr (!(requires {
                       Config::speed;
                       Config::device_mode;
                       Config::host_mode;
                       Config::otg_mode;
                     }))
  {
    return false;
  }
  else
  {
    constexpr bool mode_valid = Config::device_mode || Config::host_mode || Config::otg_mode;
    constexpr auto capability = backend_traits<Backend>::usb_capability();
    constexpr bool speed_valid = ((Config::speed == usb_speed::low) || (Config::speed == usb_speed::full) ||
                                  (Config::speed == usb_speed::high)) &&
                                 ((Config::speed != usb_speed::high) || capability.high_speed);
    constexpr bool role_valid = (!Config::device_mode || capability.device) &&
                                (!Config::host_mode || capability.host) && (!Config::otg_mode || capability.otg);
    if constexpr (std::same_as<Backend, kinetis_espresso_tag> || std::same_as<Backend, nuttx_tag>)
    {
      return mode_valid && speed_valid && role_valid &&
             requires(const std::uint8_t endpoint, const std::span<const std::uint8_t> tx,
                      const std::span<std::uint8_t> rx) {
               typename Config::binding;
               Config::binding::template initialize<Config>();
               Config::binding::template connect<Config>();
               Config::binding::template disconnect<Config>();
               Config::binding::template endpoint_transmit<Config>(endpoint, tx);
               Config::binding::template endpoint_receive<Config>(endpoint, rx);
             };
    }
    return mode_valid && speed_valid && role_valid;
  }
}

/**
 * @brief Tests Ethernet configuration structure and enabled-feature requirements.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config Ethernet configuration type.
 * @return `true` when the backend supports Ethernet and any selected DMA, interrupts, gigabit speed, or checksum
 *         offload are available.
 */
template <typename Backend, typename Config> consteval bool ethernet_config_compatible() noexcept
{
  if constexpr (!BackendConcept<Backend> || !peripheral_available<Backend, peripheral_kind::ethernet>())
  {
    return false;
  }
  else if constexpr (!(BindingBackedConfig<Config> || requires { Config::instance_index; }))
  {
    return false;
  }
  else if constexpr (!(requires {
                       Config::link_speed;
                       Config::duplex;
                       Config::enable_checksum_offload;
                       Config::enable_dma;
                       Config::enable_interrupts;
                     }))
  {
    return false;
  }
  else
  {
    return ((!Config::enable_dma) || DmaCapableConcept<Backend, peripheral_kind::ethernet>) &&
           ((!Config::enable_interrupts) || IsrCapableConcept<Backend, peripheral_kind::ethernet>) &&
           ((Config::link_speed != ethernet_link_speed::mbps1000) ||
            backend_traits<Backend>::ethernet_capability().gigabit) &&
           ((!Config::enable_checksum_offload) || backend_traits<Backend>::ethernet_capability().checksum_offload);
  }
}

/**
 * @brief Tests SD configuration geometry, timing, transfer, and backend API compatibility.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config SD configuration type.
 * @return `true` when required block operations and capacity query are supported, clock and bus settings are within
 *         advertised capabilities, enabled DMA/interrupts are supported, and required binding operations exist.
 */
template <typename Backend, typename Config> consteval bool sd_config_compatible() noexcept
{
  if constexpr (!BackendConcept<Backend> || !peripheral_available<Backend, peripheral_kind::sd>())
  {
    return false;
  }
  else if constexpr (!(BindingBackedConfig<Config> || requires { Config::instance_index; }))
  {
    return false;
  }
  else if constexpr (!(requires {
                       Config::clock_hz;
                       Config::bus_width;
                       Config::timing_mode;
                       Config::enable_dma;
                       Config::enable_interrupts;
                     }))
  {
    return false;
  }
  else
  {
    constexpr bool config_valid =
        backend_traits<Backend>::sd_capability().block_io && backend_traits<Backend>::sd_capability().capacity_query &&
        (Config::clock_hz > 0U) && (Config::clock_hz <= backend_traits<Backend>::sd_capability().max_frequency_hz) &&
        ((Config::bus_width != sd_bus_width::one) || backend_traits<Backend>::sd_capability().bus_width_1) &&
        ((Config::bus_width != sd_bus_width::four) || backend_traits<Backend>::sd_capability().bus_width_4) &&
        ((Config::bus_width != sd_bus_width::eight) || backend_traits<Backend>::sd_capability().bus_width_8) &&
        ((Config::timing_mode != sd_timing_mode::high_speed) || backend_traits<Backend>::sd_capability().high_speed) &&
        ((Config::timing_mode != sd_timing_mode::uhs) || backend_traits<Backend>::sd_capability().uhs) &&
        ((!Config::enable_dma) || DmaCapableConcept<Backend, peripheral_kind::sd>) &&
        ((!Config::enable_interrupts) || IsrCapableConcept<Backend, peripheral_kind::sd>);
    if constexpr (std::same_as<Backend, kinetis_espresso_tag>)
    {
      return config_valid && requires(const std::uint32_t block, const std::uint32_t count,
                                      const std::span<std::uint8_t> rx, const std::span<const std::uint8_t> tx) {
        typename Config::binding;
        Config::binding::template initialize<Config>();
        Config::binding::read_blocks(block, rx);
        Config::binding::write_blocks(block, tx);
        Config::binding::erase_blocks(block, count);
        Config::binding::capacity_blocks();
      };
    }
    return config_valid;
  }
}

/**
 * @brief Tests DMA configuration structure and backend availability.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config DMA configuration type.
 * @return `true` when the backend supports DMA and the configuration provides an instance/stream/channel binding and
 *         the required transfer attributes.
 */
template <typename Backend, typename Config> consteval bool dma_config_compatible() noexcept
{
  if constexpr (!BackendConcept<Backend> || !peripheral_available<Backend, peripheral_kind::dma>())
  {
    return false;
  }
  else if constexpr (!(BindingBackedConfig<Config> || requires {
                       Config::instance_index;
                       Config::stream;
                       Config::channel;
                     }))
  {
    return false;
  }
  else
  {
    return requires {
      Config::direction;
      Config::priority;
      Config::circular_mode;
      Config::double_buffer;
    };
  }
}

/**
 * @brief Tests PWM configuration structure and frequency.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config PWM configuration type.
 * @return `true` when the backend supports PWM, required timer/channel and output fields exist, and frequency is
 *         nonzero.
 */
template <typename Backend, typename Config> consteval bool pwm_config_compatible() noexcept
{
  if constexpr (!BackendConcept<Backend> || !peripheral_available<Backend, peripheral_kind::pwm>())
  {
    return false;
  }
  else if constexpr (!(BindingBackedConfig<Config> || requires {
                       Config::timer_index;
                       Config::channel;
                     }))
  {
    return false;
  }
  else if constexpr (!(requires {
                       Config::frequency_hz;
                       Config::polarity;
                       Config::enable_dead_time;
                     }))
  {
    return false;
  }
  else
  {
    return Config::frequency_hz > 0U;
  }
}

/**
 * @brief Tests watchdog configuration structure and timeout.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config Watchdog configuration type.
 * @return `true` when the backend supports watchdog, required fields and instance binding exist, and timeout is
 *         nonzero.
 */
template <typename Backend, typename Config> consteval bool watchdog_config_compatible() noexcept
{
  if constexpr (!BackendConcept<Backend> || !peripheral_available<Backend, peripheral_kind::watchdog>())
  {
    return false;
  }
  else if constexpr (!(BindingBackedConfig<Config> || requires { Config::instance_index; }))
  {
    return false;
  }
  else if constexpr (!(requires {
                       Config::timeout_ms;
                       Config::independent_mode;
                       Config::window_mode;
                     }))
  {
    return false;
  }
  else
  {
    return Config::timeout_ms > 0U;
  }
}

/**
 * @brief Tests I2S mode, sample rate, transfer mode, and backend API requirements.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config I2S configuration type.
 * @return `true` when requested mode, clocking, sample width/rate, transfer mode, and optional DMA features are
 *         supported, with required binding operations present where applicable.
 */
template <typename Backend, typename Config> consteval bool i2s_config_compatible() noexcept
{
  if constexpr (!BackendConcept<Backend> || !peripheral_available<Backend, peripheral_kind::i2s>())
  {
    return false;
  }
  else if constexpr (!(BindingBackedConfig<Config> || requires { Config::instance_index; }))
  {
    return false;
  }
  else if constexpr (!(requires {
                       Config::mode;
                       Config::standard;
                       Config::sample_rate;
                       Config::enable_master_clock;
                       Config::enable_dma;
                     }))
  {
    return false;
  }
  else
  {
    constexpr auto capability = backend_traits<Backend>::i2s_capability();
    constexpr bool mode_valid =
        ((Config::mode == i2s_mode::master_tx) || (Config::mode == i2s_mode::master_rx) ||
         (Config::mode == i2s_mode::slave_tx) || (Config::mode == i2s_mode::slave_rx)) &&
        (((Config::mode == i2s_mode::master_tx) || (Config::mode == i2s_mode::master_rx)) ? capability.master_mode
                                                                                          : capability.slave_mode);
    constexpr bool config_valid =
        mode_valid && capability.transfer_modes.supports(configured_transfer_mode<Config>()) &&
        (Config::sample_rate > 0U) && (Config::sample_rate <= capability.max_sample_rate) &&
        (capability.sample_bits >= 16U) && (!Config::enable_master_clock || capability.master_clock_output) &&
        ((!Config::enable_dma) || (capability.dma &&
                                   ((configured_transfer_mode<Config>() == transfer_mode::automatic ||
                                     Config::mode == i2s_mode::master_tx || Config::mode == i2s_mode::slave_tx)
                                        ? capability.transmit_dma
                                        : capability.receive_dma) &&
                                   DmaCapableConcept<Backend, peripheral_kind::i2s>));
    if constexpr ((std::same_as<Backend, kinetis_espresso_tag> || std::same_as<Backend, nuttx_tag>) &&
                  configured_transfer_mode<Config>() == transfer_mode::automatic)
    {
      return config_valid && requires(const std::span<const std::uint16_t> tx, const std::span<std::uint16_t> rx) {
        typename Config::binding;
        Config::binding::template initialize<Config>();
        Config::binding::template transmit_blocking<Config>(tx);
        Config::binding::template receive_blocking<Config>(rx);
      } && ((!Config::enable_dma) || requires(const std::span<const std::uint16_t> tx) {
               Config::binding::template start_dma_tx<Config, nullptr>(tx);
             });
    }
    return config_valid;
  }
}

/**
 * @brief Tests RCC configuration structure and system-clock value.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config RCC configuration type.
 * @return `true` when the backend supports RCC, the required members exist, and the system clock is nonzero.
 */
template <typename Backend, typename Config> consteval bool rcc_config_compatible() noexcept
{
  if constexpr (!BackendConcept<Backend> || !peripheral_available<Backend, peripheral_kind::rcc>())
  {
    return false;
  }
  else if constexpr (!(requires {
                       Config::source;
                       Config::system_clock_hz;
                       Config::enable_pll;
                     }))
  {
    return false;
  }
  else
  {
    return Config::system_clock_hz > 0U;
  }
}

/**
 * @brief Tests CRC configuration structure and backend availability.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config CRC configuration type.
 * @return `true` when the backend supports CRC and the configuration has the required instance binding and members.
 */
template <typename Backend, typename Config> consteval bool crc_config_compatible() noexcept
{
  if constexpr (!BackendConcept<Backend> || !peripheral_available<Backend, peripheral_kind::crc>())
  {
    return false;
  }
  else if constexpr (!(BindingBackedConfig<Config> || requires { Config::instance_index; }))
  {
    return false;
  }
  else
  {
    return requires {
      Config::enable_rng;
      Config::polynomial;
    };
  }
}

/**
 * @brief Validates a GPIO pin configuration and emits compile-time diagnostics on failure.
 * @tparam Backend Backend tag to validate against.
 * @tparam PinConfig Pin configuration type.
 * @return `true` when the backend and pin configuration pass the static assertions.
 * @note Invalid backend capability, missing fields, or invalid legacy pin/alternate-function selections cause a
 *       compile-time error.
 */
template <typename Backend, typename PinConfig> consteval bool validate_gpio_pin() noexcept
{
  static_assert(validate_capability<Backend, peripheral_kind::gpio>(), "microHAL: GPIO backend validation failed.");
  static_assert(
      BindingBackedConfig<PinConfig> || requires {
        PinConfig::port_index;
        PinConfig::pin_index;
      }, "microHAL: GPIO pin configuration must expose port_index/pin_index or a nested binding type.");
  static_assert(
      requires {
        PinConfig::mode;
        PinConfig::pull;
        PinConfig::speed;
        PinConfig::interrupt_trigger;
      }, "microHAL: GPIO pin configuration must expose mode, pull, speed, and interrupt_trigger.");
  if constexpr (!BindingBackedConfig<PinConfig>)
  {
    static_assert(
        requires { PinConfig::alternate_function; },
        "microHAL: legacy GPIO pin configuration must expose alternate_function.");
    static_assert(backend_traits<Backend>::template is_valid_gpio_pin<PinConfig>(),
                  "microHAL: GPIO port/pin combination is invalid for the selected backend.");
    static_assert(backend_traits<Backend>::template is_valid_alternate_function<PinConfig>(),
                  "microHAL: alternate function selection is invalid for the selected backend.");
  }
  return true;
}

/**
 * @brief Requires a GPIO pin configuration compatible with @p Backend.
 * @tparam Backend Backend tag.
 * @tparam PinConfig Pin configuration type.
 */
template <typename Backend, typename PinConfig>
concept GpioPinConcept = gpio_pin_compatible<Backend, PinConfig>();

/**
 * @brief Validates I2C configuration and emits compile-time diagnostics on failure.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config I2C configuration type.
 * @return `true` when backend capability and required I2C configuration constraints pass.
 * @note Invalid capability, missing fields, unsupported enabled transfer mode, or an out-of-range clock causes a
 *       compile-time error.
 */
template <typename Backend, typename Config> consteval bool validate_i2c_config() noexcept
{
  static_assert(validate_capability<Backend, peripheral_kind::i2c>(), "microHAL: I2C backend validation failed.");
  static_assert(
      BindingBackedConfig<Config> || requires { Config::instance_index; },
      "microHAL: I2C configuration must expose instance_index or a nested binding type.");
  static_assert(
      requires {
        Config::clock_hz;
        Config::address_mode;
        Config::enable_dma;
        Config::enable_interrupts;
      }, "microHAL: I2C configuration must expose clock_hz, address_mode, enable_dma, and enable_interrupts.");
  constexpr auto capability = backend_traits<Backend>::i2c_capability();
  constexpr bool instance_valid = []
  {
    if constexpr (requires { Config::instance_index; })
    {
      constexpr auto instance_count = backend_traits<Backend>::i2c_capability().instance_count;
      return (instance_count == 0U) || (Config::instance_index <= instance_count);
    }
    return true;
  }();
  static_assert(instance_valid, "microHAL: I2C instance index exceeds backend capability.");
  constexpr bool clock_stretch_valid = []
  {
    if constexpr (requires {
                    Config::clock_stretch_policy;
                    Config::clock_stretch_timeout_us;
                  })
    {
      constexpr auto stretch_capability = backend_traits<Backend>::i2c_capability();
      constexpr bool policy_supported =
          (Config::clock_stretch_policy == i2c_clock_stretch_policy::backend_default) ||
          ((Config::clock_stretch_policy == i2c_clock_stretch_policy::allow) && stretch_capability.clock_stretching) ||
          ((Config::clock_stretch_policy == i2c_clock_stretch_policy::disallow) &&
           stretch_capability.clock_stretch_control);
      constexpr bool timeout_supported =
          (Config::clock_stretch_timeout_us == 0U) ||
          ((stretch_capability.max_clock_stretch_timeout_us != 0U) &&
           (Config::clock_stretch_timeout_us <= stretch_capability.max_clock_stretch_timeout_us));
      return policy_supported && timeout_supported;
    }
    return true;
  }();
  static_assert(clock_stretch_valid, "microHAL: selected backend does not support the requested clock-stretch policy.");
  static_assert(capability.transfer_modes.supports(configured_transfer_mode<Config>()),
                "microHAL: selected backend does not provide the requested I2C transfer mode.");
  static_assert((Config::address_mode != i2c_address_mode::ten_bit) || capability.ten_bit_addressing,
                "microHAL: selected backend does not support ten-bit I2C addressing.");
  static_assert((Config::clock_hz > 0U) && (Config::clock_hz <= capability.max_frequency_hz),
#if defined(__cpp_static_assert) && __cpp_static_assert >= 202306L
                detail::clock_diagnostic(Config::clock_hz, std::to_array("microHAL: requested I2C clock "),
                                         capability.max_frequency_hz));
#else
                "microHAL: requested I2C clock exceeds backend capability.");
#endif
  static_assert((!Config::enable_dma) || DmaCapableConcept<Backend, peripheral_kind::i2c>,
                "microHAL: selected backend does not provide DMA-capable I2C.");
  static_assert((!Config::enable_interrupts) || IsrCapableConcept<Backend, peripheral_kind::i2c>,
                "microHAL: selected backend does not provide interrupt-capable I2C.");
  return true;
}

/**
 * @brief Requires an I2C master configuration compatible with @p Backend.
 * @tparam Backend Backend tag.
 * @tparam Config I2C configuration type.
 */
template <typename Backend, typename Config>
concept I2cMasterConcept = i2c_config_compatible<Backend, Config>();

/**
 * @brief Validates SPI configuration and emits compile-time diagnostics on failure.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config SPI configuration type.
 * @return `true` when backend capability and required SPI configuration constraints pass.
 * @note Invalid capability, missing fields, unsupported enabled transfer mode, or an out-of-range clock causes a
 *       compile-time error.
 */
template <typename Backend, typename Config> consteval bool validate_spi_config() noexcept
{
  static_assert(validate_capability<Backend, peripheral_kind::spi>(), "microHAL: SPI backend validation failed.");
  static_assert(
      BindingBackedConfig<Config> || requires { Config::instance_index; },
      "microHAL: SPI configuration must expose instance_index or a nested binding type.");
  static_assert(
      requires {
        Config::clock_hz;
        Config::duplex_mode;
        Config::clock_polarity;
        Config::clock_phase;
        Config::bit_order;
        Config::enable_dma;
        Config::enable_interrupts;
      }, "microHAL: SPI configuration must expose clock_hz, duplex_mode, clock_polarity, clock_phase, bit_order, "
         "enable_dma, and enable_interrupts.");
  static_assert((Config::clock_hz > 0U) &&
                    (Config::clock_hz <= backend_traits<Backend>::spi_capability().max_frequency_hz),
#if defined(__cpp_static_assert) && __cpp_static_assert >= 202306L
                detail::clock_diagnostic(Config::clock_hz, std::to_array("microHAL: requested SPI clock "),
                                         backend_traits<Backend>::spi_capability().max_frequency_hz));
#else
                "microHAL: requested SPI clock exceeds backend capability.");
#endif
  static_assert((!Config::enable_dma) || DmaCapableConcept<Backend, peripheral_kind::spi>,
                "microHAL: selected backend does not provide DMA-capable SPI.");
  static_assert((!Config::enable_interrupts) || IsrCapableConcept<Backend, peripheral_kind::spi>,
                "microHAL: selected backend does not provide interrupt-capable SPI.");
  return true;
}

/**
 * @brief Requires an SPI master configuration compatible with @p Backend.
 * @tparam Backend Backend tag.
 * @tparam Config SPI configuration type.
 */
template <typename Backend, typename Config>
concept SpiMasterConcept = spi_config_compatible<Backend, Config>();

/**
 * @brief Validates QSPI configuration and emits compile-time diagnostics on failure.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config QSPI configuration type.
 * @return `true` when backend capability, quad mode, required fields, and requested DMA pass validation.
 * @note Invalid capability or missing requirements causes a compile-time error.
 */
template <typename Backend, typename Config> consteval bool validate_qspi_config() noexcept
{
  static_assert(validate_capability<Backend, peripheral_kind::qspi>(), "microHAL: QSPI backend validation failed.");
  static_assert(backend_traits<Backend>::qspi_capability().quad_mode,
                "microHAL: selected backend does not provide QSPI quad-mode support.");
  static_assert(
      BindingBackedConfig<Config> || requires { Config::instance_index; },
      "microHAL: QSPI configuration must expose instance_index or a nested binding type.");
  static_assert(
      requires {
        Config::clock_hz;
        Config::enable_dma;
        Config::enable_interrupts;
      }, "microHAL: QSPI configuration must expose clock_hz, enable_dma, and enable_interrupts.");
  static_assert((!Config::enable_dma) || DmaCapableConcept<Backend, peripheral_kind::qspi>,
                "microHAL: selected backend does not provide DMA-capable QSPI.");
  return true;
}

/**
 * @brief Requires a QSPI configuration compatible with @p Backend.
 * @tparam Backend Backend tag.
 * @tparam Config QSPI configuration type.
 */
template <typename Backend, typename Config>
concept QspiConcept = qspi_config_compatible<Backend, Config>();

/**
 * @brief Validates UART configuration and emits compile-time diagnostics on failure.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config UART configuration type.
 * @return `true` when backend capability and required UART configuration constraints pass.
 * @note Invalid capability, missing fields, zero baud, unsupported requested DMA/interrupts, or unavailable required
 *       FIFO/LPUART support causes a compile-time error.
 */
template <typename Backend, typename Config> consteval bool validate_uart_config() noexcept
{
  static_assert(validate_capability<Backend, peripheral_kind::uart>(), "microHAL: UART backend validation failed.");
  static_assert(
      BindingBackedConfig<Config> || requires { Config::instance_index; },
      "microHAL: UART configuration must expose instance_index or a nested binding type.");
  static_assert(
      requires {
        Config::baud_rate;
        Config::parity;
        Config::stop_bits;
        Config::enable_dma;
        Config::enable_interrupts;
        Config::require_fifo;
        Config::require_lpuart;
      }, "microHAL: UART configuration must expose baud_rate, parity, stop_bits, enable_dma, enable_interrupts, "
         "require_fifo, and require_lpuart.");
  constexpr auto requested_data_bits = []
  {
    if constexpr (requires { Config::data_bits; })
    {
      return Config::data_bits;
    }
    return uart_data_bits::eight;
  }();
  constexpr std::uint16_t supported_data_bits = backend_traits<Backend>::uart_capability().data_bits_mask == 0U
                                                    ? uart_data_bits_mask(uart_data_bits::eight)
                                                    : backend_traits<Backend>::uart_capability().data_bits_mask;
  static_assert((supported_data_bits & uart_data_bits_mask(requested_data_bits)) != 0U,
                "microHAL: selected backend does not support the requested UART data width.");
  static_assert((requested_data_bits != uart_data_bits::nine) || (Config::parity == uart_parity::none),
                "microHAL: nine UART data bits cannot be combined with parity.");
  static_assert(Config::baud_rate > 0U, "microHAL: UART baud rate must be non-zero.");
  static_assert((!Config::enable_dma) || DmaCapableConcept<Backend, peripheral_kind::uart>,
                "microHAL: selected backend does not provide DMA-capable UART.");
  static_assert((!Config::enable_interrupts) || IsrCapableConcept<Backend, peripheral_kind::uart>,
                "microHAL: selected backend does not provide interrupt-capable UART.");
  static_assert((!Config::require_fifo) || backend_traits<Backend>::uart_capability().fifo,
                "microHAL: selected backend does not provide UART FIFO support.");
  static_assert((!Config::require_lpuart) || backend_traits<Backend>::uart_capability().lpuart,
                "microHAL: selected backend does not provide LPUART support.");
  return true;
}

/**
 * @brief Requires a UART configuration compatible with @p Backend.
 * @tparam Backend Backend tag.
 * @tparam Config UART configuration type.
 */
template <typename Backend, typename Config>
concept UartConcept = uart_config_compatible<Backend, Config>();

/**
 * @brief Validates timer configuration and emits compile-time diagnostics on failure.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config Timer configuration type.
 * @return `true` when backend capability, required fields, and enabled features pass validation.
 * @note Invalid capability, missing fields, zero clock, or unsupported requested DMA/interrupts causes a compile-time
 *       error.
 */
template <typename Backend, typename Config> consteval bool validate_timer_config() noexcept
{
  static_assert(validate_capability<Backend, peripheral_kind::timer>(), "microHAL: timer backend validation failed.");
  static_assert(
      BindingBackedConfig<Config> || requires { Config::instance_index; },
      "microHAL: timer configuration must expose instance_index or a nested binding type.");
  static_assert(
      requires {
        Config::clock_hz;
        Config::counter_mode;
        Config::clock_source;
        Config::enable_dma;
        Config::enable_interrupts;
      }, "microHAL: timer configuration must expose clock_hz, counter_mode, clock_source, enable_dma, and "
         "enable_interrupts.");
  static_assert(Config::clock_hz > 0U, "microHAL: timer clock must be non-zero.");
  static_assert((!Config::enable_dma) || DmaCapableConcept<Backend, peripheral_kind::timer>,
                "microHAL: selected backend does not provide DMA-capable timers.");
  static_assert((!Config::enable_interrupts) || IsrCapableConcept<Backend, peripheral_kind::timer>,
                "microHAL: selected backend does not provide interrupt-capable timers.");
  return true;
}

/**
 * @brief Requires a timer configuration compatible with @p Backend.
 * @tparam Backend Backend tag.
 * @tparam Config Timer configuration type.
 */
template <typename Backend, typename Config>
concept TimerConcept = timer_config_compatible<Backend, Config>();

/**
 * @brief Validates flash configuration geometry and DMA requirements.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config Flash configuration type.
 * @return `true` when backend capability and flash alignment/erase constraints pass.
 * @note Invalid granularity, missing fields, or unsupported requested DMA causes a compile-time error.
 */
template <typename Backend, typename Config> consteval bool validate_flash_config() noexcept
{
  static_assert(validate_capability<Backend, peripheral_kind::flash>(), "microHAL: flash backend validation failed.");
  static_assert(
      requires {
        Config::program_alignment_bytes;
        Config::erase_size_bytes;
        Config::enable_dma;
        Config::enable_interrupts;
      }, "microHAL: flash configuration must expose program_alignment_bytes, erase_size_bytes, enable_dma, and "
         "enable_interrupts.");
  static_assert(Config::program_alignment_bytes >= backend_traits<Backend>::flash_capability().alignment_bytes,
                "microHAL: flash program alignment is smaller than backend minimum alignment.");
  static_assert(Config::program_alignment_bytes > 0U, "microHAL: flash program alignment must be non-zero.");
  static_assert((Config::program_alignment_bytes % backend_traits<Backend>::flash_capability().alignment_bytes) == 0U,
                "microHAL: flash program alignment must be a multiple of the backend minimum alignment.");
  static_assert(Config::erase_size_bytes > 0U, "microHAL: flash erase size must be non-zero.");
  static_assert(Config::erase_size_bytes >= backend_traits<Backend>::flash_capability().erase_bytes,
                "microHAL: flash erase size is smaller than backend minimum erase granularity.");
  static_assert((Config::erase_size_bytes % backend_traits<Backend>::flash_capability().erase_bytes) == 0U,
                "microHAL: flash erase size must be an integer multiple of the backend erase granularity.");
  static_assert((!Config::enable_dma) || DmaCapableConcept<Backend, peripheral_kind::flash>,
                "microHAL: selected backend does not provide DMA-capable flash accesses.");
  return true;
}

/**
 * @brief Requires a flash configuration compatible with @p Backend.
 * @tparam Backend Backend tag.
 * @tparam Config Flash configuration type.
 */
template <typename Backend, typename Config>
concept FlashConcept = flash_config_compatible<Backend, Config>();

/**
 * @brief Validates RTC configuration and its interrupt capability requirements.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config RTC configuration type.
 * @return `true` when backend capability and required RTC configuration members pass validation.
 * @note Invalid capability, missing fields, or unsupported requested interrupts causes a compile-time error.
 */
template <typename Backend, typename Config> consteval bool validate_rtc_config() noexcept
{
  static_assert(validate_capability<Backend, peripheral_kind::rtc>(), "microHAL: RTC backend validation failed.");
  static_assert(
      requires {
        Config::enable_periodic_irq;
        Config::enable_alarm_irq;
      }, "microHAL: RTC configuration must expose enable_periodic_irq and enable_alarm_irq.");
  static_assert((!Config::enable_periodic_irq && !Config::enable_alarm_irq) ||
                    IsrCapableConcept<Backend, peripheral_kind::rtc>,
                "microHAL: selected backend does not provide interrupt-capable RTC support.");
  return true;
}

/**
 * @brief Requires an RTC configuration compatible with @p Backend.
 * @tparam Backend Backend tag.
 * @tparam Config RTC configuration type.
 */
template <typename Backend, typename Config>
concept RtcConcept = rtc_config_compatible<Backend, Config>();

/**
 * @brief Validates UART autobaud configuration and emits compile-time diagnostics on failure.
 * @tparam Backend Backend tag to validate against.
 * @tparam UartConfig UART configuration type.
 * @tparam AutobaudConfig Type providing timer, GPIO, and baud-range settings.
 * @return `true` when the composed configurations and backend autobaud capability pass validation.
 * @note Invalid UART/timer/GPIO configuration, unavailable autobaud, or an invalid baud range causes a compile-time
 *       error.
 */
template <typename Backend, typename UartConfig, typename AutobaudConfig>
consteval bool validate_autobaud_config() noexcept
{
  static_assert(validate_uart_config<Backend, UartConfig>(), "microHAL: autobaud requires a valid UART configuration.");
  static_assert(validate_timer_config<Backend, typename AutobaudConfig::timer_config>(),
                "microHAL: autobaud requires a valid timer configuration.");
  static_assert(validate_gpio_pin<Backend, typename AutobaudConfig::gpio_pin>(),
                "microHAL: autobaud requires a valid GPIO configuration for the RX sense input.");
  static_assert(backend_traits<Backend>::uart_capability().autobaud,
                "microHAL: selected backend does not advertise generic autobaud support.");
  static_assert(AutobaudConfig::min_baud < AutobaudConfig::max_baud,
                "microHAL: autobaud minimum baud must be lower than the maximum baud.");
  return true;
}

/**
 * @brief Requires compatible UART, timer, and GPIO configurations for autobaud.
 * @tparam Backend Backend tag.
 * @tparam UartConfig UART configuration type.
 * @tparam AutobaudConfig Autobaud settings type.
 */
template <typename Backend, typename UartConfig, typename AutobaudConfig>
concept AutobaudCapableConcept = autobaud_config_compatible<Backend, UartConfig, AutobaudConfig>();

// New peripheral concepts

/**
 * @brief Validates ADC configuration and emits compile-time diagnostics on failure.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config ADC configuration type.
 * @return `true` when backend support, required members, and enabled DMA/interrupts pass validation.
 * @note Invalid capability, missing fields, or unsupported requested DMA/interrupts causes a compile-time error.
 */
template <typename Backend, typename Config> consteval bool validate_adc_config() noexcept
{
  static_assert(validate_capability<Backend, peripheral_kind::adc>(), "microHAL: ADC backend validation failed.");
  static_assert(
      BindingBackedConfig<Config> || requires { Config::instance_index; },
      "microHAL: ADC configuration must expose instance_index or a nested binding type.");
  static_assert(
      requires {
        Config::resolution;
        Config::sample_time;
        Config::enable_dma;
        Config::enable_interrupts;
        Config::differential_mode;
      }, "microHAL: ADC configuration must expose resolution, sample_time, enable_dma, enable_interrupts, and "
         "differential_mode.");
  static_assert((!Config::enable_dma) || DmaCapableConcept<Backend, peripheral_kind::adc>,
                "microHAL: selected backend does not provide DMA-capable ADC.");
  static_assert((!Config::enable_interrupts) || IsrCapableConcept<Backend, peripheral_kind::adc>,
                "microHAL: selected backend does not provide interrupt-capable ADC.");
  return true;
}

/**
 * @brief Requires an ADC configuration compatible with @p Backend.
 * @tparam Backend Backend tag.
 * @tparam Config ADC configuration type.
 */
template <typename Backend, typename Config>
concept AdcConcept = adc_config_compatible<Backend, Config>();

/**
 * @brief Validates DAC configuration and emits compile-time diagnostics on failure.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config DAC configuration type.
 * @return `true` when backend support, required members, and requested DMA pass validation.
 * @note Invalid capability, missing fields, or unsupported requested DMA causes a compile-time error.
 */
template <typename Backend, typename Config> consteval bool validate_dac_config() noexcept
{
  static_assert(validate_capability<Backend, peripheral_kind::dac>(), "microHAL: DAC backend validation failed.");
  static_assert(
      BindingBackedConfig<Config> || requires {
        Config::instance_index;
        Config::channel;
      }, "microHAL: DAC configuration must expose instance_index/channel or a nested binding type.");
  static_assert(
      requires {
        Config::resolution;
        Config::enable_dma;
        Config::enable_waveform;
      }, "microHAL: DAC configuration must expose resolution, enable_dma, and enable_waveform.");
  static_assert((!Config::enable_dma) || DmaCapableConcept<Backend, peripheral_kind::dac>,
                "microHAL: selected backend does not provide DMA-capable DAC.");
  return true;
}

/**
 * @brief Requires a DAC configuration compatible with @p Backend.
 * @tparam Backend Backend tag.
 * @tparam Config DAC configuration type.
 */
template <typename Backend, typename Config>
concept DacConcept = dac_config_compatible<Backend, Config>();

/**
 * @brief Validates CAN configuration and emits compile-time diagnostics on failure.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config CAN configuration type.
 * @return `true` when backend support, required members, nonzero baudrate, and requested interrupts pass validation.
 * @note Invalid capability, missing fields, zero baudrate, or unsupported requested interrupts causes a compile-time
 *       error.
 */
template <typename Backend, typename Config> consteval bool validate_can_config() noexcept
{
  static_assert(validate_capability<Backend, peripheral_kind::can>(), "microHAL: CAN backend validation failed.");
  static_assert(
      BindingBackedConfig<Config> || requires { Config::instance_index; },
      "microHAL: CAN configuration must expose instance_index or a nested binding type.");
  static_assert(
      requires {
        Config::baudrate;
        Config::mode;
        Config::enable_can_fd;
        Config::enable_interrupts;
      }, "microHAL: CAN configuration must expose baudrate, mode, enable_can_fd, and enable_interrupts.");
  static_assert(Config::baudrate > 0U, "microHAL: CAN baudrate must be non-zero.");
  static_assert(
      []
      {
        if constexpr (BindingBackedConfig<Config>)
        {
          return true;
        }
        else
        {
          return Config::instance_index <= backend_traits<Backend>::can_capability().instance_count;
        }
      }(),
      "microHAL: CAN instance index exceeds the backend instance count.");
  static_assert(backend_traits<Backend>::can_capability().max_bitrate == 0U ||
                    Config::baudrate <= backend_traits<Backend>::can_capability().max_bitrate,
                "microHAL: CAN baudrate exceeds the backend maximum.");
  static_assert((!Config::enable_can_fd) || backend_traits<Backend>::can_capability().can_fd,
                "microHAL: selected backend does not support CAN FD.");
  static_assert((!Config::enable_interrupts) || IsrCapableConcept<Backend, peripheral_kind::can>,
                "microHAL: selected backend does not provide interrupt-capable CAN.");
  return true;
}

/**
 * @brief Requires a CAN configuration compatible with @p Backend.
 * @tparam Backend Backend tag.
 * @tparam Config CAN configuration type.
 */
template <typename Backend, typename Config>
concept CanConcept = can_config_compatible<Backend, Config>();

/**
 * @brief Validates USB configuration and emits compile-time diagnostics on failure.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config USB configuration type.
 * @return `true` when backend support, required members, and at least one enabled role pass validation.
 * @note Invalid capability, missing fields, or no enabled USB role causes a compile-time error.
 */
template <typename Backend, typename Config> consteval bool validate_usb_config() noexcept
{
  static_assert(validate_capability<Backend, peripheral_kind::usb>(), "microHAL: USB backend validation failed.");
  static_assert(
      BindingBackedConfig<Config> || requires { Config::instance_index; },
      "microHAL: USB configuration must expose instance_index or a nested binding type.");
  static_assert(
      requires {
        Config::speed;
        Config::device_mode;
        Config::host_mode;
        Config::otg_mode;
      }, "microHAL: USB configuration must expose speed, device_mode, host_mode, and otg_mode.");
  static_assert(Config::device_mode || Config::host_mode || Config::otg_mode,
                "microHAL: USB must have at least one mode enabled (device, host, or OTG).");
  return true;
}

/**
 * @brief Requires a USB configuration compatible with @p Backend.
 * @tparam Backend Backend tag.
 * @tparam Config USB configuration type.
 */
template <typename Backend, typename Config>
concept UsbConcept = usb_config_compatible<Backend, Config>();

/**
 * @brief Validates Ethernet configuration and emits compile-time diagnostics on failure.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config Ethernet configuration type.
 * @return `true` when backend support, required members, and requested DMA/interrupt/speed/offload features pass.
 * @note Unsupported requested capabilities or missing required fields cause a compile-time error.
 */
template <typename Backend, typename Config> consteval bool validate_ethernet_config() noexcept
{
  static_assert(validate_capability<Backend, peripheral_kind::ethernet>(),
                "microHAL: Ethernet backend validation failed.");
  static_assert(
      BindingBackedConfig<Config> || requires { Config::instance_index; },
      "microHAL: Ethernet configuration must expose instance_index or a nested binding type.");
  static_assert(
      requires {
        Config::link_speed;
        Config::duplex;
        Config::enable_checksum_offload;
        Config::enable_dma;
        Config::enable_interrupts;
      }, "microHAL: Ethernet configuration must expose link_speed, duplex, enable_checksum_offload, enable_dma, and "
         "enable_interrupts.");
  static_assert((!Config::enable_dma) || DmaCapableConcept<Backend, peripheral_kind::ethernet>,
                "microHAL: selected backend does not provide DMA-capable Ethernet.");
  static_assert((!Config::enable_interrupts) || IsrCapableConcept<Backend, peripheral_kind::ethernet>,
                "microHAL: selected backend does not provide interrupt-capable Ethernet.");
  static_assert((Config::link_speed != ethernet_link_speed::mbps1000) ||
                    backend_traits<Backend>::ethernet_capability().gigabit,
                "microHAL: selected backend does not provide Gigabit Ethernet.");
  static_assert((!Config::enable_checksum_offload) || backend_traits<Backend>::ethernet_capability().checksum_offload,
                "microHAL: selected backend does not provide Ethernet checksum offload.");
  return true;
}

/**
 * @brief Requires an Ethernet configuration compatible with @p Backend.
 * @tparam Backend Backend tag.
 * @tparam Config Ethernet configuration type.
 */
template <typename Backend, typename Config>
concept EthernetConcept = ethernet_config_compatible<Backend, Config>();

/**
 * @brief Validates SD configuration and emits compile-time diagnostics on failure.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config SD configuration type.
 * @return `true` when backend support, required members, clock, bus width, timing, and enabled transfer features pass.
 * @note Unsupported requested capabilities or out-of-range clock causes a compile-time error.
 */
template <typename Backend, typename Config> consteval bool validate_sd_config() noexcept
{
  static_assert(validate_capability<Backend, peripheral_kind::sd>(), "microHAL: SD backend validation failed.");
  static_assert(
      BindingBackedConfig<Config> || requires { Config::instance_index; },
      "microHAL: SD configuration must expose instance_index or a nested binding type.");
  static_assert(
      requires {
        Config::clock_hz;
        Config::bus_width;
        Config::timing_mode;
        Config::enable_dma;
        Config::enable_interrupts;
      }, "microHAL: SD configuration must expose clock_hz, bus_width, timing_mode, enable_dma, and enable_interrupts.");
  static_assert((Config::clock_hz > 0U) &&
                    (Config::clock_hz <= backend_traits<Backend>::sd_capability().max_frequency_hz),
                "microHAL: requested SD clock exceeds backend capability.");
  static_assert((Config::bus_width != sd_bus_width::four) || backend_traits<Backend>::sd_capability().bus_width_4,
                "microHAL: selected backend does not provide 4-bit SD bus width.");
  static_assert((Config::bus_width != sd_bus_width::eight) || backend_traits<Backend>::sd_capability().bus_width_8,
                "microHAL: selected backend does not provide 8-bit SD bus width.");
  static_assert((Config::timing_mode != sd_timing_mode::high_speed) ||
                    backend_traits<Backend>::sd_capability().high_speed,
                "microHAL: selected backend does not provide high-speed SD mode.");
  static_assert((Config::timing_mode != sd_timing_mode::uhs) || backend_traits<Backend>::sd_capability().uhs,
                "microHAL: selected backend does not provide UHS SD mode.");
  static_assert((!Config::enable_dma) || DmaCapableConcept<Backend, peripheral_kind::sd>,
                "microHAL: selected backend does not provide DMA-capable SD.");
  static_assert((!Config::enable_interrupts) || IsrCapableConcept<Backend, peripheral_kind::sd>,
                "microHAL: selected backend does not provide interrupt-capable SD.");
  return true;
}

/**
 * @brief Requires an SD configuration compatible with @p Backend.
 * @tparam Backend Backend tag.
 * @tparam Config SD configuration type.
 */
template <typename Backend, typename Config>
concept SdConcept = sd_config_compatible<Backend, Config>();

/**
 * @brief Validates DMA configuration and emits compile-time diagnostics on failure.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config DMA configuration type.
 * @return `true` when the backend supports DMA and the required config fields are present.
 * @note Unsupported DMA or missing instance/stream/channel and transfer members causes a compile-time error.
 */
template <typename Backend, typename Config> consteval bool validate_dma_config() noexcept
{
  static_assert(validate_capability<Backend, peripheral_kind::dma>(), "microHAL: DMA backend validation failed.");
  static_assert(
      BindingBackedConfig<Config> || requires {
        Config::instance_index;
        Config::stream;
        Config::channel;
      }, "microHAL: DMA configuration must expose instance_index/stream/channel or a nested binding type.");
  static_assert(
      requires {
        Config::direction;
        Config::priority;
        Config::circular_mode;
        Config::double_buffer;
      }, "microHAL: DMA configuration must expose direction, priority, circular_mode, and double_buffer.");
  return true;
}

/**
 * @brief Requires a DMA configuration compatible with @p Backend.
 * @tparam Backend Backend tag.
 * @tparam Config DMA configuration type.
 */
template <typename Backend, typename Config>
concept DmaConcept = dma_config_compatible<Backend, Config>();

/**
 * @brief Validates PWM configuration and emits compile-time diagnostics on failure.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config PWM configuration type.
 * @return `true` when the backend supports PWM and the required fields and nonzero frequency are present.
 * @note Invalid capability, missing fields, or zero frequency causes a compile-time error.
 */
template <typename Backend, typename Config> consteval bool validate_pwm_config() noexcept
{
  static_assert(validate_capability<Backend, peripheral_kind::pwm>(), "microHAL: PWM backend validation failed.");
  static_assert(
      BindingBackedConfig<Config> || requires {
        Config::timer_index;
        Config::channel;
      }, "microHAL: PWM configuration must expose timer_index/channel or a nested binding type.");
  static_assert(
      requires {
        Config::frequency_hz;
        Config::polarity;
        Config::enable_dead_time;
      }, "microHAL: PWM configuration must expose frequency_hz, polarity, and enable_dead_time.");
  static_assert(Config::frequency_hz > 0U, "microHAL: PWM frequency must be non-zero.");
  return true;
}

/**
 * @brief Requires a PWM configuration compatible with @p Backend.
 * @tparam Backend Backend tag.
 * @tparam Config PWM configuration type.
 */
template <typename Backend, typename Config>
concept PwmConcept = pwm_config_compatible<Backend, Config>();

/**
 * @brief Validates watchdog configuration and emits compile-time diagnostics on failure.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config Watchdog configuration type.
 * @return `true` when backend support, required members, and nonzero timeout pass validation.
 * @note Invalid capability, missing fields, or zero timeout causes a compile-time error.
 */
template <typename Backend, typename Config> consteval bool validate_watchdog_config() noexcept
{
  static_assert(validate_capability<Backend, peripheral_kind::watchdog>(),
                "microHAL: watchdog backend validation failed.");
  static_assert(
      BindingBackedConfig<Config> || requires { Config::instance_index; },
      "microHAL: watchdog configuration must expose instance_index or a nested binding type.");
  static_assert(
      requires {
        Config::timeout_ms;
        Config::independent_mode;
        Config::window_mode;
      }, "microHAL: watchdog configuration must expose timeout_ms, independent_mode, and window_mode.");
  static_assert(Config::timeout_ms > 0U, "microHAL: watchdog timeout must be non-zero.");
  return true;
}

/**
 * @brief Requires a watchdog configuration compatible with @p Backend.
 * @tparam Backend Backend tag.
 * @tparam Config Watchdog configuration type.
 */
template <typename Backend, typename Config>
concept WatchdogConcept = watchdog_config_compatible<Backend, Config>();

/**
 * @brief Validates I2S configuration and emits compile-time diagnostics on failure.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config I2S configuration type.
 * @return `true` when backend support, required members, nonzero sample rate, and requested DMA pass validation.
 * @note Unsupported requested DMA or missing required members causes a compile-time error.
 */
template <typename Backend, typename Config> consteval bool validate_i2s_config() noexcept
{
  static_assert(validate_capability<Backend, peripheral_kind::i2s>(), "microHAL: I2S backend validation failed.");
  static_assert(
      BindingBackedConfig<Config> || requires { Config::instance_index; },
      "microHAL: I2S configuration must expose instance_index or a nested binding type.");
  static_assert(
      requires {
        Config::mode;
        Config::standard;
        Config::sample_rate;
        Config::enable_master_clock;
        Config::enable_dma;
      }, "microHAL: I2S configuration must expose mode, standard, sample_rate, enable_master_clock, and enable_dma.");
  static_assert(Config::sample_rate > 0U, "microHAL: I2S sample rate must be non-zero.");
  static_assert((!Config::enable_dma) || DmaCapableConcept<Backend, peripheral_kind::i2s>,
                "microHAL: selected backend does not provide DMA-capable I2S.");
  return true;
}

/**
 * @brief Requires an I2S configuration compatible with @p Backend.
 * @tparam Backend Backend tag.
 * @tparam Config I2S configuration type.
 */
template <typename Backend, typename Config>
concept I2sConcept = i2s_config_compatible<Backend, Config>();

/**
 * @brief Validates RCC configuration and emits compile-time diagnostics on failure.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config RCC configuration type.
 * @return `true` when backend support, required members, and nonzero system clock pass validation.
 * @note Invalid capability, missing members, or zero system clock causes a compile-time error.
 */
template <typename Backend, typename Config> consteval bool validate_rcc_config() noexcept
{
  static_assert(validate_capability<Backend, peripheral_kind::rcc>(), "microHAL: RCC backend validation failed.");
  static_assert(
      requires {
        Config::source;
        Config::system_clock_hz;
        Config::enable_pll;
      }, "microHAL: RCC configuration must expose source, system_clock_hz, and enable_pll.");
  static_assert(Config::system_clock_hz > 0U, "microHAL: system clock must be non-zero.");
  return true;
}

/**
 * @brief Requires an RCC configuration compatible with @p Backend.
 * @tparam Backend Backend tag.
 * @tparam Config RCC configuration type.
 */
template <typename Backend, typename Config>
concept RccConcept = rcc_config_compatible<Backend, Config>();

/**
 * @brief Validates CRC configuration and emits compile-time diagnostics on failure.
 * @tparam Backend Backend tag to validate against.
 * @tparam Config CRC configuration type.
 * @return `true` when backend support and required config members are present.
 * @note Invalid capability or missing members causes a compile-time error.
 */
template <typename Backend, typename Config> consteval bool validate_crc_config() noexcept
{
  static_assert(validate_capability<Backend, peripheral_kind::crc>(), "microHAL: CRC backend validation failed.");
  static_assert(
      BindingBackedConfig<Config> || requires { Config::instance_index; },
      "microHAL: CRC configuration must expose instance_index or a nested binding type.");
  static_assert(
      requires {
        Config::enable_rng;
        Config::polynomial;
      }, "microHAL: CRC configuration must expose enable_rng and polynomial.");
  return true;
}

/**
 * @brief Requires a CRC configuration compatible with @p Backend.
 * @tparam Backend Backend tag.
 * @tparam Config CRC configuration type.
 */
template <typename Backend, typename Config>
concept CrcConcept = crc_config_compatible<Backend, Config>();

} // namespace microhal
