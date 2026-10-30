# microHAL API Coverage Scorecard

This scorecard is a repository snapshot of API coverage as implemented in the current `include/microhal` facades and backend traits.

## Scoring Method

- **Facade richness (1-5)**: how broad the public facade operations are for one peripheral.
- **Capability depth (1-5)**: how much compile-time metadata is exposed in capability structs.
- **Backend breadth (1-5)**: how widely the usable core API is implemented. A `5` requires at least five
  real backend implementations; trait declarations alone do not count.

Interpretation:

- `5` = strong for production portability.
- `3` = practical baseline, but not feature-complete.
- `1-2` = minimal surface, more expansion needed for parity-level portability.

## Peripheral Scorecard

| Peripheral | Public facade operations (snapshot) | Facade richness | Capability depth | Backend breadth | Notes |
| --- | --- | --- | --- | --- | --- |
| GPIO | `configure`, `set`, `clear`, `read`, `toggle`, `enable_interrupt` | 5 | 5 | 5 | Native toggle support is reported per backend; pin/port/alternate-function geometry remains available. |
| I2C | `initialize`, `write_blocking`, `read_blocking`, `probe`, `write_read_blocking`, `start_transfer`, `write_async`, `read_async`, `write_read_async`, `abort`, `transfer_status`, `handle_interrupt` | 5 | 5 | 5 | Explicit mode capabilities distinguish polling, interrupt and DMA engines; combined transactions retain repeated START. NuttX relies on its driver's native ioctl timeout. |
| SPI | `initialize`, `write_blocking`, `read_blocking`, `transfer`, `transfer_async`, `write_async`, `read_async`, `abort`, `transfer_status`, `handle_interrupt` | 5 | 5 | 5 | Mismatched full-duplex buffers are rejected; explicit mode selection preserves legacy defaults. One-direction operations are supported only where implemented. |
| QSPI | `initialize`, `command_read`, `command_write`, `enter_memory_mapped`, `exit_memory_mapped` | 5 | 5 | 5 | Commands validate bus modes and address widths; STM32 HAL and mock are direct paths, LL forwards to HAL, and Kinetis/Zephyr require complete board bindings. |
| UART | `initialize`, `write_blocking`, `read_blocking`, `write_byte`, `read_byte`, `start_async_rx`, `start_async_tx`, `abort`, `transfer_status`, `handle_interrupt` (+ autobaud helper) | 5 | 5 | 5 | Explicit polling/interrupt/DMA selection and async completion are backend-gated. Framing limits remain capability metadata. |
| RTC | `initialize`, `set`, `read`, `now`, `is_valid`, `enable_alarm` | 5 | 5 | 5 | Checked reads preserve backend errors; time fields are validated, and unsupported calendar fields are rejected instead of discarded. |
| Flash | `erase`, `program`, `program_and_verify`, `read`, `erase_region` | 5 | 5 | 5 | Program units and erase ranges are validated; readback verification uses caller-owned storage. Partition policy remains board/application-owned. |
| Timer | `initialize`, `start`, `stop`, `set_period`, `period`, `counter`, `reset_counter`, `enable_capture_compare`, `capture_value` | 5 | 5 | 5 | Lifecycle and period updates are implemented by mock, STM32 HAL/LL, Kinetis, Zephyr, and Nordic. Nordic additionally implements period readback, counter reset, and compare callbacks, but not input capture. |
| ADC | `initialize`, `read_blocking`, `read_sequence_blocking`, `read_millivolts`, `start_continuous`, `start_async`, `start_dma_scan` | 5 | 5 | 5 | Blocking reads return explicit errors; sequential scans and millivolt conversion are facade utilities over implemented backend reads. Nordic adds one-shot and DMA-scan SAADC, with binding-owned analog settings. |
| DAC | `initialize`, `write_blocking`, `write_sequence_blocking`, `write_millivolts`, `write_dual`, `start_dma_waveform` | 5 | 5 | 5 | Sequence and voltage helpers are portable facade utilities; backend write failures are propagated on implemented mock, STM32 HAL/LL, Kinetis, and Zephyr paths. |
| CAN | `initialize`, `is_valid`, `transmit`, `transmit_many`, `receive`, `receive_many`, `configure_filter`, `enable_rx_interrupt` | 5 | 5 | 5 | Identifier/DLC validation is common; ordered batch calls stop on the first failure and may partially complete. STM32 LL uses the legacy HAL CAN driver where Cube provides no LL CAN API. |
| USB | `initialize`, `connect`, `disconnect`, `endpoint_transmit`, `endpoint_receive` | 5 | 5 | 5 | Endpoint indexes and 16-bit transfer lengths are validated; Kinetis and NuttX require complete stack bindings, and LL forwards to HAL when available. |
| Ethernet | `initialize`, `start`, `stop`, `transmit_frame`, `receive_frame`, `set_mac_address`, `enable_rx_interrupt` | 4 | 5 | 3 | Rich surface, available on STM32 HAL, Zephyr, and NuttX packet-socket configurations. |
| SD/MMC | `initialize`, `read_blocks`, `write_blocks`, `erase_blocks`, `capacity_blocks`, `read_block`, `write_block`, `is_valid_range` | 5 | 5 | 5 | Block sizes and overflow are checked; block erase is capability-gated and reports `unsupported` when the media API cannot erase natively. |
| DMA | `initialize`, `is_valid_range`, `start_transfer`, `start_memcpy`, `abort`, `transfer_status`, `transfer_complete` | 5 | 5 | 5 | Zero-length and overflowing address ranges are rejected; STM32 LL polls the hardware enable bit for one-shot completion, while callback support remains backend-specific. |
| PWM | `initialize`, `set_duty_cycle`, `set_duty_cycle_percent`, `start`, `stop`, `set_enabled` | 5 | 5 | 5 | Permille and percentage inputs are range-checked; enable/disable helpers share the backend lifecycle operations. |
| Watchdog | `initialize`, `start`, `refresh`, `feed` | 5 | 5 | 5 | Both common refresh/feed terminology and timeout range metadata are exposed; stoppability remains hardware-specific. |
| I2S | `initialize`, `transmit_blocking`, `receive_blocking`, `start_dma_tx`, `transmit_async`, `receive_async`, `abort`, `transfer_status`, `handle_interrupt` | 5 | 5 | 5 | Finite 16-bit transfers have explicit execution-mode and TX/RX DMA metadata. Legacy Kinetis/NuttX adapter paths still require complete board bindings. |
| RCC | `configure`, `configure_and_verify`, `get_system_clock_hz`, `enable_peripheral_clock`, `disable_peripheral_clock` | 5 | 5 | 4 | Clock readback/configuration is implemented by the mock, STM32 HAL/LL, and Zephyr; per-peripheral gating requires a board binding on STM32/Kinetis, and Kinetis configuration is binding-provided. |
| CRC | `initialize`, `calculate`, `calculate_software`, `reset`, `get_random` | 5 | 5 | 5 | The portable software path supports the configured non-reflected 32-bit polynomial and explicit initial/final XOR values; RNG remains backend-dependent. |

## Overall Assessment

- **Strengths**:
  - Compile-time capability model is broad and explicit.
  - Core portability surface is consistent and clean.
  - Major peripherals (UART/ADC/SD/Ethernet) have useful practical APIs.
- **Current limits**:
  - Explicit serial execution modes are backend/module-specific, not universally
    available wherever silicon has DMA. STM32 LL explicit modes forward to HAL.
    Arbitrary polled I2C combined transactions remain unsupported; interrupt/DMA
    combined paths preserve repeated START. See the [integration contract](backend_sdk_requirements.md#explicit-serial-transfer-modes).
  - Backend parity is not uniform for Ethernet and other peripherals.
  - Nordic nrfx now implements native GPIO/GPIOTE, TWIM, SPIM, UARTE, TIMER, SAADC, PWM, watchdog,
    I2S, and clock readback on nRF52840/nRF5340-app/nRF54L15-app, plus target-gated NVMC, QSPI,
    and RTC. Support is compile-validated, not hardware-certified; NuttX features require
    corresponding OS configuration and registered devices.
  - Binding-backed QSPI, USB, and legacy I2S adapters require board/application operations.
    Explicit Kinetis I2S instead uses native SAI polling/IRQ/EDMA with typed board resources,
    currently Philips stereo16/channel0 only.

  The new operation-level support is intentionally narrower than the peripheral capability matrix:

  | Operation | Mock | STM32 HAL | STM32 LL | Kinetis | Zephyr | NuttX | Nordic nrfx |
  | --- | --- | --- | --- | --- | --- | --- | --- |
  | QSPI command read/write and memory-mapped mode | ✅ | ✅ | ✅ (HAL forwarding) | ✅ (FLEXSPI + binding) | ✅ (CONFIG_FLASH + binding) | ⚪ | ✅ commands only (nRF52/53 QSPI) |
  | USB endpoint transfer | ✅ | ✅ | ✅ (HAL forwarding) | ✅ (USB hardware + binding) | ✅ (CONFIG_USB_DEVICE_STACK) | ✅ (CONFIG_USBDEV + binding) | ⚪ |
  | SD block I/O and capacity query | ✅ | ✅ | ✅ | ✅ (SDHC + binding) | ✅ (SDHC/DISK_ACCESS) | ✅ (CONFIG_MMCSD) | ⚪ |
  | SD native block erase | ✅ | ✅ | ✅ | ✅ (binding) | ⚪ | ⚪ | ⚪ |
  | I2S blocking transmit/receive | ✅ | ✅ | ✅ (HAL forwarding) | ✅ (I2S0 + binding) | ✅ (CONFIG_I2S) | ✅ (CONFIG_AUDIO + binding) | ✅ finite 16-bit stereo |
  | I2S DMA transmit start | ✅ | ✅* | ✅* (HAL forwarding) | ✅ (I2S0 + binding) | ⚪ | ⚪ | ✅ finite TX |
  | I2C combined write/read (repeated START) | ✅ (simulated) | ✅* | ✅* (HAL forwarding) | ✅ (I2C/LPI2C) | ✅ | ✅ (with `I2C_M_NOSTOP`) | ✅ (TWIM) |
  | I2C combined async write/read | ✅ (simulated) | ✅* | ✅* (HAL forwarding) | ⚪ | ⚪ | ⚪ | ✅ (TWIM callback) |
  | I2C address probe | ✅ (simulated) | ✅ | ✅* (HAL forwarding) | ⚪ | ✅ | ✅ | ✅ zero-length TWIM TX |
  | SPI transmit-only / receive-only | ✅ (simulated) | ✅ | ✅* (HAL forwarding) | ⚪ | ✅ | ✅ | ✅ SPIM |
  | SPI interrupt-driven transfer | ✅ (simulated) | ✅* | ✅* (HAL forwarding) | ⚪ | ⚪ | ⚪ | ✅ SPIM |
  | GPIO native toggle | ✅ | ✅ | ✅ | ⚪ | ✅ | ⚪ | ✅ |
  | Flash erase-region query | ✅ (uniform mock geometry) | ✅ (page or supported sector geometry) | ✅ (HAL forwarding) | ✅ (MCUXpresso geometry) | ✅ | ✅ (MTD geometry) | ✅ (NVMC targets) |
  | Flash program/erase alignment validation | ✅ facade | ✅ facade + HAL programming-unit checks | ✅ facade | ✅ facade + driver geometry | ✅ facade | ✅ (MTD geometry) | ✅ (NVMC constraints) |
  | Flash program/readback verification | ✅ facade | ✅ facade | ✅ facade | ✅ facade | ✅ facade | ✅ (MTD readback) | ✅ (NVMC targets) |
  | CAN validation and ordered batch calls | ✅ facade | ✅ facade | ✅ facade | ✅ facade | ✅ facade | ⚪ | ⚪ |
  | Timer start/stop and period update | ✅ | ✅ | ✅ | ✅ | ✅ | ✅* | ✅ TIMER |
  | Timer period readback | ✅ | ✅ | ✅ | ⚪ | ✅ | ✅* | ✅ |
  | Timer counter reset | ✅ | ✅ | ✅ | ✅ (FTM) | ⚪ | ✅* | ✅ |
  | Timer capture readback | ✅ | ✅ | ✅ | ⚪ | ⚪ | ⚪ | ⚪ |
  | ADC blocking read, sequential scan, and voltage conversion | ✅ | ✅ | ✅ | ✅ | ✅ | ⚪ | ✅ SAADC (binding sets analog reference/gain) |
  | DAC blocking write, sequence write, and voltage conversion | ✅ | ✅ | ✅ | ✅ | ✅ | ⚪ | ⚪ |
  | DMA transfer status and callback | ✅ (simulated) | ✅ | ✅* (HAL stream-DMA forwarding) | ✅ | ✅ | ⚪ | ⚪ |
  | RCC system-clock readback/verification | ✅ (simulated) | ✅ | ✅ | ⚠️ (readback only; configuration needs binding) | ✅ | ⚪ | ✅ readback only |
  | CRC software fallback | ✅ facade | ✅ facade | ✅ facade | ✅ facade | ✅ facade | ⚪ | ⚪ |

  `*` STM32 HAL sequential I2C is available only on Cube families exposing the sequential-transfer
  APIs; the blocking facade uses interrupt-driven phases and requires the I2C IRQs and system tick to
  run. The async I2C, SPI, and I2S DMA completion paths require `USE_HAL_*_REGISTER_CALLBACKS=1`.
  STM32 LL uses HAL forwarding for these operations, GPIO toggle, flash geometry, and stream-DMA
  completion; those paths also require the corresponding HAL modules.
  `capture_value` reads a one-based timer channel already configured for input capture by the board or
  vendor setup. Timer `period` is expressed in counter ticks. Binding entries in this table describe
  required board/OS-provided implementations, not built-in device drivers; the concepts reject configs
  that omit those operations. Nordic SD/MMC and USB remain unsupported; QSPI memory mapping,
  continuous ADC, timer input capture, RTC calendar/date, and RCC configuration are not implemented.
  Nordic blocking I2C
  uses the configured timeout and CMSIS core clock; NuttX I2C can only report a timeout supplied by its
  driver because `I2CIOC_TRANSFER` has no timeout argument. Zephyr and Kinetis DMA completion callbacks
  retain their native callback signatures; STM32 LL exposes polling status but not a callback. Calls
  not implemented by a backend report `status::unsupported`.

## High-Value Expansion Priorities

1. Add target-SDK build coverage for conditional QSPI, USB, SD/MMC, and I2S paths, including binding
   examples for Kinetis and NuttX.
2. Expand flash error reporting and hardware-specific operation coverage while keeping partition
   definitions and ownership in board/application code.
3. Raise backend parity for CAN/Ethernet:
   - prioritize missing runtime paths where capabilities are currently narrow.

## Data Sources

- Facade headers under `include/microhal/*/*.hpp`
- Capability matrix in `docs/capability_matrix.md`
- Backend trait capability declarations under `include/microhal/backends/**/traits.hpp`
