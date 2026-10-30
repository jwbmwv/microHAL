// SPDX-License-Identifier: Apache-2.0
/**
 * @file concepts.hpp
 * @brief microHAL source header for this module.
 * @details Contains compile-time configuration and API surface used by microHAL consumers.
 * @copyright Copyright (c) 2026 James Baldwin
 */

#pragma once

#include <concepts>
#include <type_traits>

#include "microhal/core/backend_selector.hpp"

namespace microhal
{

template <typename Capability>
concept CapabilityConcept = requires(const Capability capability) {
  { capability.supported } -> std::convertible_to<bool>;
  { capability.blocking } -> std::convertible_to<bool>;
  { capability.non_blocking } -> std::convertible_to<bool>;
  { capability.isr } -> std::convertible_to<bool>;
  { capability.dma } -> std::convertible_to<bool>;
};

template <typename Backend>
concept BackendConcept = requires {
  typename backend_traits<Backend>::backend_tag;
  { backend_traits<Backend>::name } -> std::convertible_to<std::string_view>;
  { backend_traits<Backend>::vendor } -> std::convertible_to<vendor_kind>;
  { backend_traits<Backend>::kind } -> std::convertible_to<backend_kind>;
};

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

/// @brief True when @p Backend is registered and implements @p Peripheral.
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

template <typename Backend, peripheral_kind Peripheral>
inline constexpr bool peripheral_supported_v = peripheral_available<Backend, Peripheral>();

template <typename Backend, peripheral_kind Peripheral>
concept SupportedPeripheralConcept = peripheral_available<Backend, Peripheral>();

template <typename Backend, peripheral_kind Peripheral>
concept IsrCapableConcept =
    peripheral_available<Backend, Peripheral>() && peripheral_isr_available<Backend, Peripheral>();

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
template <typename Backend, peripheral_kind Peripheral> consteval bool validate_capability() noexcept
{
  static_assert(BackendConcept<Backend>, "microHAL: backend tag is not registered in backend_traits.");
  static_assert(peripheral_supported<Backend, Peripheral>(),
                "microHAL: backend does not support the requested peripheral.");
  return true;
}
/// @}

template <typename Config>
concept BindingBackedConfig = requires { typename Config::binding; };

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
    return capability.transfer_modes.supports(configured_transfer_mode<Config>()) && (Config::clock_hz > 0U) &&
           (Config::clock_hz <= capability.max_frequency_hz) &&
           ((Config::address_mode != i2c_address_mode::ten_bit) || capability.ten_bit_addressing) &&
           ((!serial_dma_enabled<Config>()) || DmaCapableConcept<Backend, peripheral_kind::i2c>) &&
           ((!serial_interrupt_enabled<Config>()) || IsrCapableConcept<Backend, peripheral_kind::i2c>);
  }
}

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
    constexpr bool dma_compatible =
        (!Config::enable_dma) || DmaCapableConcept<Backend, peripheral_kind::qspi>;
    if constexpr (std::same_as<Backend, kinetis_espresso_tag> || std::same_as<Backend, zephyr_tag> ||
                  std::same_as<Backend, nuttx_tag>)
    {
      return dma_compatible &&
             requires(const qspi_command& command, const std::span<const std::uint8_t> tx,
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
    return capability.transfer_modes.supports(configured_transfer_mode<Config>()) && (Config::baud_rate > 0U) &&
           ((!serial_dma_enabled<Config>()) || DmaCapableConcept<Backend, peripheral_kind::uart>) &&
           ((!serial_interrupt_enabled<Config>()) || IsrCapableConcept<Backend, peripheral_kind::uart>) &&
           ((!Config::require_fifo) || backend_traits<Backend>::uart_capability().fifo) &&
           ((!Config::require_lpuart) || capability.lpuart) &&
           ((capability.max_baud_rate == 0U) || (Config::baud_rate <= capability.max_baud_rate)) &&
           ((Config::parity == uart_parity::none) || capability.parity) &&
           ((Config::stop_bits == uart_stop_bits::one) ||
            ((Config::stop_bits == uart_stop_bits::two) && capability.two_stop_bits) ||
            ((Config::stop_bits == uart_stop_bits::one_and_half) && capability.one_and_half_stop_bits));
  }
}

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
    return (Config::baudrate > 0U) &&
           ((!Config::enable_interrupts) || IsrCapableConcept<Backend, peripheral_kind::can>);
  }
}

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
    constexpr bool speed_valid =
        ((Config::speed == usb_speed::low) || (Config::speed == usb_speed::full) ||
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
        (Config::clock_hz > 0U) &&
        (Config::clock_hz <= backend_traits<Backend>::sd_capability().max_frequency_hz) &&
        ((Config::bus_width != sd_bus_width::one) || backend_traits<Backend>::sd_capability().bus_width_1) &&
        ((Config::bus_width != sd_bus_width::four) || backend_traits<Backend>::sd_capability().bus_width_4) &&
        ((Config::bus_width != sd_bus_width::eight) || backend_traits<Backend>::sd_capability().bus_width_8) &&
        ((Config::timing_mode != sd_timing_mode::high_speed) || backend_traits<Backend>::sd_capability().high_speed) &&
        ((Config::timing_mode != sd_timing_mode::uhs) || backend_traits<Backend>::sd_capability().uhs) &&
        ((!Config::enable_dma) || DmaCapableConcept<Backend, peripheral_kind::sd>) &&
        ((!Config::enable_interrupts) || IsrCapableConcept<Backend, peripheral_kind::sd>);
    if constexpr (std::same_as<Backend, kinetis_espresso_tag>)
    {
      return config_valid &&
             requires(const std::uint32_t block, const std::uint32_t count,
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
      return config_valid &&
             requires(const std::span<const std::uint16_t> tx, const std::span<std::uint16_t> rx) {
               typename Config::binding;
               Config::binding::template initialize<Config>();
               Config::binding::template transmit_blocking<Config>(tx);
               Config::binding::template receive_blocking<Config>(rx);
             } &&
             ((!Config::enable_dma) ||
              requires(const std::span<const std::uint16_t> tx) {
                Config::binding::template start_dma_tx<Config, nullptr>(tx);
              });
    }
    return config_valid;
  }
}

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

template <typename Backend, typename PinConfig>
concept GpioPinConcept = gpio_pin_compatible<Backend, PinConfig>();

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
  static_assert((Config::clock_hz > 0U) &&
                    (Config::clock_hz <= backend_traits<Backend>::i2c_capability().max_frequency_hz),
                "microHAL: requested I2C clock exceeds backend capability.");
  static_assert((!Config::enable_dma) || DmaCapableConcept<Backend, peripheral_kind::i2c>,
                "microHAL: selected backend does not provide DMA-capable I2C.");
  static_assert((!Config::enable_interrupts) || IsrCapableConcept<Backend, peripheral_kind::i2c>,
                "microHAL: selected backend does not provide interrupt-capable I2C.");
  return true;
}

template <typename Backend, typename Config>
concept I2cMasterConcept = i2c_config_compatible<Backend, Config>();

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
                "microHAL: requested SPI clock exceeds backend capability.");
  static_assert((!Config::enable_dma) || DmaCapableConcept<Backend, peripheral_kind::spi>,
                "microHAL: selected backend does not provide DMA-capable SPI.");
  static_assert((!Config::enable_interrupts) || IsrCapableConcept<Backend, peripheral_kind::spi>,
                "microHAL: selected backend does not provide interrupt-capable SPI.");
  return true;
}

template <typename Backend, typename Config>
concept SpiMasterConcept = spi_config_compatible<Backend, Config>();

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

template <typename Backend, typename Config>
concept QspiConcept = qspi_config_compatible<Backend, Config>();

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

template <typename Backend, typename Config>
concept UartConcept = uart_config_compatible<Backend, Config>();

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

template <typename Backend, typename Config>
concept TimerConcept = timer_config_compatible<Backend, Config>();

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

template <typename Backend, typename Config>
concept FlashConcept = flash_config_compatible<Backend, Config>();

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

template <typename Backend, typename Config>
concept RtcConcept = rtc_config_compatible<Backend, Config>();

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

template <typename Backend, typename UartConfig, typename AutobaudConfig>
concept AutobaudCapableConcept = autobaud_config_compatible<Backend, UartConfig, AutobaudConfig>();

// New peripheral concepts

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

template <typename Backend, typename Config>
concept AdcConcept = adc_config_compatible<Backend, Config>();

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

template <typename Backend, typename Config>
concept DacConcept = dac_config_compatible<Backend, Config>();

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
  static_assert((!Config::enable_interrupts) || IsrCapableConcept<Backend, peripheral_kind::can>,
                "microHAL: selected backend does not provide interrupt-capable CAN.");
  return true;
}

template <typename Backend, typename Config>
concept CanConcept = can_config_compatible<Backend, Config>();

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

template <typename Backend, typename Config>
concept UsbConcept = usb_config_compatible<Backend, Config>();

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

template <typename Backend, typename Config>
concept EthernetConcept = ethernet_config_compatible<Backend, Config>();

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

template <typename Backend, typename Config>
concept SdConcept = sd_config_compatible<Backend, Config>();

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

template <typename Backend, typename Config>
concept DmaConcept = dma_config_compatible<Backend, Config>();

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

template <typename Backend, typename Config>
concept PwmConcept = pwm_config_compatible<Backend, Config>();

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

template <typename Backend, typename Config>
concept WatchdogConcept = watchdog_config_compatible<Backend, Config>();

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

template <typename Backend, typename Config>
concept I2sConcept = i2s_config_compatible<Backend, Config>();

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

template <typename Backend, typename Config>
concept RccConcept = rcc_config_compatible<Backend, Config>();

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

template <typename Backend, typename Config>
concept CrcConcept = crc_config_compatible<Backend, Config>();

} // namespace microhal
