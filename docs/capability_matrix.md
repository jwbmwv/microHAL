# microHAL Capability Matrix

The code-level source of truth is the set of `consteval` capability accessors in each
`backend_traits` specialization (`include/microhal/backends/**/traits.hpp`). The summary below
mirrors those accessors; if the two disagree, the header wins.

## Core Peripherals

| Backend | GPIO | I2C | SPI | QSPI | UART | RTC | Flash | Timer |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| STM32Cube HAL | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| STM32Cube LL | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Kinetis MCUXpresso | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Zephyr RTOS | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| NuttX RTOS | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Nordic nrfx (bare metal) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Mock (host) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |

## Extended Peripherals

| Backend | ADC | DAC | CAN | USB | Ethernet | SD/MMC | DMA | PWM | Watchdog | I2S | RCC | CRC |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| STM32Cube HAL | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| STM32Cube LL | ✅ | ✅ | ❌ | ❌ | ❌ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Kinetis MCUXpresso | ✅ | ✅ | ✅ | ❌ | ❌ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Zephyr RTOS | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| NuttX RTOS | ✅ | ✅ | ✅ | ✅ | ❌ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Nordic nrfx (bare metal) | ✅ | ✅ | ❌ | ❌ | ❌ | ✅ | ✅ | ✅ | ✅ | ❌ | ❌ | ❌ |
| Mock (host) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |

**Legend:** ✅ Full implementation | ⚠️ Compile-time API surface present, backend operations still scaffolded | ❌ Unsupported

The mock backend is a host-only test double. It deliberately advertises only the peripherals its
recorder implements, so the capability machinery guards it exactly like real hardware.

STM32 HAL Ethernet and SD/MMC now route to real HAL calls and should be instantiated through binding-backed configs so board code can provide the concrete MAC and host resources. The other backends still expose the header-level API types for future work, but now report unsupported until their runtime paths exist.

STM32 LL QSPI and SD/MMC route through Cube HAL module APIs when the corresponding HAL modules are enabled in the Cube configuration (`HAL_QSPI_MODULE_ENABLED` / `HAL_SD_MODULE_ENABLED`) and the MCU family exposes those peripherals.

## Compile-Time Constraints Encoded in Traits

Every capability struct starts with the seven `base_capability` fields — `supported`, `blocking`,
`non_blocking`, `isr`, `dma`, `instance_count`, `max_frequency_hz` — and then adds the
peripheral-specific fields listed below. Fields not listed here do not exist; a constraint that is
not in this table is enforced (if at all) by the `validate_*_config` diagnostics rather than by the
capability record.

| Peripheral | Additional capability fields |
| --- | --- |
| GPIO | `alternate_function`, `alternate_function_count`, `port_count`, `pins_per_port` |
| I2C | *(none beyond the base fields)* |
| SPI | *(none beyond the base fields)* |
| QSPI | `quad_mode`, `octo_mode`, `ddr_mode` |
| UART | `fifo`, `lpuart`, `autobaud`, `fifo_depth` |
| RTC | `periodic`, `alarm` |
| Flash | `alignment_bytes`, `erase_bytes`, `total_size_kb` |
| Timer | `capture_compare`, `periodic`, `pwm`, `channel_count` |
| ADC | `resolution_bits`, `channel_count`, `differential`, `oversampling`, `injected` |
| DAC | `resolution_bits`, `channel_count`, `dual_mode`, `waveform_generation` |
| CAN | `can_fd`, `time_triggered`, `mailbox_count`, `filter_banks` |
| USB | `device`, `host`, `otg`, `high_speed`, `endpoint_count` |
| Ethernet | `checksum_offload`, `ptp_timestamp`, `vlan_filter`, `gigabit`, `mtu_bytes` |
| SD/MMC | `high_speed`, `uhs`, `bus_width_4`, `bus_width_8`, `block_size_bytes` |
| DMA | `stream_count`, `channel_count`, `mem_to_mem`, `fifo`, `double_buffer` |
| PWM | `channel_count`, `complementary_output`, `dead_time` |
| Watchdog | `independent`, `window`, `min_timeout_ms`, `max_timeout_ms` |
| I2S | `full_duplex`, `master_clock_output`, `max_sample_rate` |
| RCC | `pll_config`, `hse`, `lse`, `max_system_clock_hz` |
| CRC | `hardware_crc32`, `programmable_polynomial`, `random_number_generator` |

Backends additionally publish two GPIO validation hooks that are not capability fields:
`is_valid_gpio_pin<PinConfig>()` and `is_valid_alternate_function<PinConfig>()`.

## Concept-Level Validation Coverage

### Core Concepts

- `PeripheralConcept`
- `GpioPinConcept` / `GpioConcept`
- `SpiMasterConcept` / `SpiConcept`
- `I2cMasterConcept` / `I2cConcept`
- `UartConcept`
- `TimerConcept`
- `FlashConcept`
- `RtcConcept`
- `QspiConcept`

### Extended Concepts

- `AdcConcept`
- `DacConcept`
- `CanConcept`
- `UsbConcept`
- `EthernetConcept`
- `SdConcept`
- `DmaConcept`
- `PwmConcept`
- `WatchdogConcept`
- `I2sConcept`
- `RccConcept`
- `CrcConcept`

### Cross-Cutting Concerns

- `BackendConcept`
- `CapabilityConcept`
- `AutobaudCapableConcept`
- `IsrCapableConcept`
- `DmaCapableConcept`

`SupportedPeripheralConcept`, `IsrCapableConcept`, and `DmaCapableConcept` are **pure predicates**:
they evaluate to `false` for an unsupported combination, so they are safe to use in `if constexpr`
and `requires` clauses. The per-peripheral configuration concepts above (`UartConcept`,
`I2cMasterConcept`, and the rest) are **diagnostic**: they are built on `static_assert`-ing
`validate_*_config` functions, so a bad configuration produces a targeted error message rather than
a quietly-false constraint. Do not branch on them.

No runtime fallback paths are provided in either case.
