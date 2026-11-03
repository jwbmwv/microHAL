# microHAL Capability Matrix

The code-level source of truth is the set of `consteval` capability accessors in each
`backend_traits` specialization (`include/microhal/backends/**/traits.hpp`). The summary below
mirrors those accessors; if the two disagree, the header wins.

## Core Peripherals

| Backend | GPIO | I2C | SPI | QSPI | UART | RTC | Flash | Timer |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| STM32Cube HAL | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| STM32Cube LL | ✅ | ✅ | ✅ | ✅* | ✅ | ✅ | ✅ | ✅ |
| Kinetis MCUXpresso | ✅ | ✅ | ✅ | ✅* | ✅ | ✅ | ✅ | ✅ |
| Zephyr RTOS | ✅ | ✅ | ✅ | ✅* | ✅ | ✅ | ✅ | ✅ |
| NuttX RTOS | ❌ | ✅* | ✅* | ❌ | ✅* | ✅* | ✅* | ✅* |
| Nordic nrfx (bare metal) | ✅* | ✅* | ✅* | ✅* | ✅* | ✅* | ✅* | ✅* |
| Mock (host) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |

## Extended Peripherals

| Backend | ADC | DAC | CAN | USB | Ethernet | SD/MMC | DMA | PWM | Watchdog | I2S | RCC | CRC |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| STM32Cube HAL | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| STM32Cube LL | ✅ | ✅ | ✅ | ✅* | ❌ | ✅* | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Kinetis MCUXpresso | ✅ | ✅ | ✅ | ✅* | ❌ | ✅* | ✅ | ✅ | ✅ | ✅* | ⚠️ | ✅ |
| Zephyr RTOS | ✅ | ✅ | ✅ | ✅* | ✅* | ✅* | ✅ | ✅ | ✅ | ✅* | ✅ | ✅ |
| NuttX RTOS | ❌ | ✅* | ✅* | ❌ | ✅* | ✅* | ❌ | ✅* | ✅* | ✅* | ❌ | ❌ |
| Nordic nrfx (bare metal) | ✅* | ❌ | ✅* | ✅* | ❌ | ❌ | ✅* | ✅* | ✅* | ✅* | ✅* | ✅* RNG only |
| Mock (host) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |

**Legend:** ✅ Runtime path available (possibly SDK- or binding-gated) | ⚠️ Compile-time API surface present, backend operations still scaffolded | ❌ Unsupported

The mock backend is a host-only test double. It deliberately advertises only the peripherals its
recorder implements, so the capability machinery guards it exactly like real hardware.

NuttX support is conditional on the configured OS drivers and device bindings: I2C, SPI, serial,
RTC, MTD, timer, DAC, CAN, USB device, Ethernet packet sockets, MMC/SD, PWM, watchdog, and audio
paths are gated by their corresponding NuttX configuration. Its GPIO, ADC, QSPI, DMA, RCC, and CRC
paths remain unsupported. NuttX I2C repeated-start requires `I2C_M_NOSTOP`; its ioctl API has no
per-call timeout parameter, so any timeout is supplied by the underlying driver.

Nordic nrfx covers nRF52840, nRF5340-app, nRF54L15-app and nRF54H20-app, gated by actual
hardware and enabled SDK modules. Serial masters support polling/DMA; SPIS/TWIS slaves require
DMA. Completion IRQs are implemented, not a bytewise non-DMA interrupt engine.
UART has independent directions and IDLE/circular RX with reserved timer/event resources;
SAADC supports finite/circular scans, continuous readback and signed differential conversion.
TIMER capture requires real board event routing; PWM outputs share timing without disrupting
neighboring duty updates. I2S supports 16-bit stereo full-duplex/streaming, except on H20.
RTC/GRTC provide calendar, subsecond, alarm and periodic operations. NVMC/RRAM/MRAM use
the public memory facade with target-specific erase/program semantics and ownership.
USBD/QSPI are available on 52/53; H20 adds classical CAN and memory-copy DMA.
Clock configuration/gating is ownership-gated on 52/53/L15 and readback-only on H20.
CRC hardware remains absent; the CRC/RNG facade exposes native or explicitly enabled vendor
entropy providers. DAC, Ethernet and native SD/MMC remain unavailable. These are compile-validated
implementations, not board-tested drivers. See [Nordic integration requirements](backend_sdk_requirements.md#nordic-board-integration).
QSPI/USB and legacy `automatic` I2S adapter paths on Kinetis
require complete board bindings; Zephyr QSPI also requires a binding. SD erase is not available
through generic Zephyr or NuttX block I/O and is reported as unsupported. Kinetis RCC supports
clock readback, but clock configuration and peripheral gating require application-provided bindings,
so the RCC entry is conditional. The remaining ⚠️ entries identify partial or scaffolded paths.

STM32 HAL Ethernet and SD/MMC now route to real HAL calls and should be instantiated through binding-backed configs so board code can provide the concrete MAC and host resources. NuttX Ethernet uses raw packet sockets only when `CONFIG_NET_PKT` and Ethernet support are enabled. NuttX SD/MMC and Nordic flash support are likewise configuration/target-gated.

STM32 LL QSPI and SD/MMC route through Cube HAL module APIs when the corresponding HAL modules are enabled in the Cube configuration (`HAL_QSPI_MODULE_ENABLED` / `HAL_SD_MODULE_ENABLED`) and the MCU family exposes those peripherals.

STM32 LL explicit UART/SPI/I2C modes use native register-level engines, including
IRQ/DMA combined I2C transactions and fixed-length SPI/I2C slave transfers.
Legacy `automatic` adapters and other peripherals may still forward to HAL.
I2S, generic stream-DMA callbacks, and flash erase-geometry queries retain their
existing HAL dependencies. Both STM32 ADC tags share native acquisition with
finite/circular DMA, hardware triggers, callbacks, and lifecycle APIs.

`*` Capability is conditional on the target SDK and/or hardware: STM32 LL forwards to HAL when the
selected family supports the peripheral; Kinetis QSPI, SD, USB, and I2S require the relevant
controller macro; Zephyr QSPI requires flash configuration, while USB, SD, and I2S require their
corresponding device-stack, disk-access, and I2S configurations. NuttX capabilities require the
matching `CONFIG_*` drivers (Ethernet additionally requires packet sockets); Nordic peripherals
are target/driver gated. Binding-backed configurations must provide every operation required by the
facade concept. Generic Zephyr/NuttX SD block APIs do not expose native block erase.

## Compile-Time Constraints Encoded in Traits

I2C, SPI, UART, and I2S additionally expose
`transfer_modes.{polled,interrupt,dma}` and `transfer_modes.supports(mode)`.
These describe explicit selectable execution engines, not merely silicon DMA
availability. `automatic` preserves existing configurations on every backend.
STM32 HAL implements polling, interrupt and DMA engines with matching enabled
HAL modules; interrupt/DMA additionally require callback registration and board
IRQ/DMA resources. LL UART/SPI/I2C explicit modes use independent native engines.
STM32 UART additionally supports independent TX/RX, receive-to-IDLE, and circular
DMA reception. Unsupported combinations,
including asynchronous calls in polled mode and arbitrary polled I2C combined
transactions, return `unsupported` rather than silently changing modes.
See [serial integration](backend_sdk_requirements.md#explicit-serial-transfer-modes)
for callback lifetime, timeout, cancellation and DMA requirements.
Kinetis I2C/LPI2C, DSPI/LPSPI, UART/LPUART and modern SAI also provide all three
engines when matching SDK drivers are available. DMA resources are supplied by
typed board bindings; EDMA and simple DMA availability differ by module.
SAI is Philips stereo16/channel0, polled LPI2C RX is limited to 256 bytes, and
polled combined I2C remains unsupported. These paths are fixture-tested, not
validated against an installed MCUXpresso SDK.

Every capability struct starts with the seven `base_capability` fields — `supported`, `blocking`,
`non_blocking`, `isr`, `dma`, `instance_count`, `max_frequency_hz` — and then adds the
peripheral-specific fields listed below. Fields not listed here do not exist; a constraint that is
not in this table is enforced (if at all) by the `validate_*_config` diagnostics rather than by the
capability record.

| Peripheral | Additional capability fields |
| --- | --- |
| GPIO | `alternate_function`, `alternate_function_count`, `port_count`, `pins_per_port`, `output_toggle` |
| I2C | `ten_bit_addressing`, `repeated_start`, `address_probe`, `transfer_modes` |
| SPI | `transmit_only`, `receive_only`, `async_transfer`, `transfer_modes` |
| QSPI | `quad_mode`, `dual_mode`, `octo_mode`, `ddr_mode`, `command_transfer`, `memory_mapped`, `max_address_bytes` |
| UART | `fifo`, `lpuart`, `autobaud`, `fifo_depth`, `parity`, `two_stop_bits`, `one_and_half_stop_bits`, `hardware_flow_control`, `max_baud_rate`, `transfer_modes` |
| RTC | `periodic`, `alarm`, `calendar`, `subsecond` |
| Flash | `alignment_bytes`, `erase_bytes`, `total_size_kb`, `erase_region_query`, `program_readback` |
| Timer | `capture_compare`, `periodic`, `pwm`, `channel_count`, `period_readback`, `counter_reset` |
| ADC | `resolution_bits`, `channel_count`, `differential`, `oversampling`, `injected`, `sequential_scan`, `voltage_conversion` |
| DAC | `resolution_bits`, `channel_count`, `dual_mode`, `waveform_generation`, `output_buffer`, `voltage_conversion`, `sequence_write` |
| CAN | `can_fd`, `time_triggered`, `mailbox_count`, `filter_banks`, `extended_ids`, `remote_frames`, `rx_interrupt` |
| USB | `device`, `host`, `otg`, `high_speed`, `endpoint_count`, `endpoint_transfer`, `endpoint_stall`, `max_packet_size_bytes` |
| Ethernet | `checksum_offload`, `ptp_timestamp`, `vlan_filter`, `gigabit`, `mtu_bytes` |
| SD/MMC | `high_speed`, `uhs`, `bus_width_1`, `bus_width_4`, `bus_width_8`, `block_size_bytes`, `block_io`, `erase`, `capacity_query` |
| DMA | `stream_count`, `channel_count`, `mem_to_mem`, `fifo`, `double_buffer`, `transfer_status`, `abort`, `completion_callback`, `address_width_bits` |
| PWM | `channel_count`, `complementary_output`, `dead_time`, `runtime_duty_update`, `runtime_start_stop` |
| Watchdog | `independent`, `window`, `min_timeout_ms`, `max_timeout_ms`, `runtime_refresh`, `reset_on_timeout` |
| I2S | `full_duplex`, `master_mode`, `slave_mode`, `master_clock_output`, `max_sample_rate`, `transmit_dma`, `receive_dma`, `sample_bits`, `transfer_modes` |
| RCC | `pll_config`, `hse`, `lse`, `max_system_clock_hz`, `clock_configuration`, `peripheral_clock_gating`, `system_clock_readback` |
| CRC | `hardware_crc32`, `programmable_polynomial`, `random_number_generator`, `seed_configurable`, `output_xor_configurable`, `checksum_width_bits` |

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
