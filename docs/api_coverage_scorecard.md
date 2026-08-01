# microHAL API Coverage Scorecard

This scorecard is a repository snapshot of API coverage as implemented in the current `include/microhal` facades and backend traits.

## Scoring Method

- **Facade richness (1-5)**: how broad the public facade operations are for one peripheral.
- **Capability depth (1-5)**: how much compile-time metadata is exposed in capability structs.
- **Backend breadth (1-5)**: how widely that peripheral is available across backends in the current capability matrix.

Interpretation:

- `5` = strong for production portability.
- `3` = practical baseline, but not feature-complete.
- `1-2` = minimal surface, more expansion needed for parity-level portability.

## Peripheral Scorecard

| Peripheral | Public facade operations (snapshot) | Facade richness | Capability depth | Backend breadth | Notes |
| --- | --- | --- | --- | --- | --- |
| GPIO | `configure`, `set`, `clear`, `read`, `enable_interrupt` | 3 | 4 | 5 | Solid baseline, explicit board binding model. |
| I2C | `initialize`, `write_blocking`, `read_blocking`, `start_transfer` | 3 | 2 | 5 | Common-mode operations present; limited advanced policy controls in facade. |
| SPI | `initialize`, `transfer`, `transfer_async` | 2 | 2 | 5 | Lean and portable, intentionally small surface. |
| QSPI | `initialize` | 1 | 3 | 4 | Runtime support exists on more backends now, but facade surface is still very thin. |
| UART | `initialize`, `write_blocking`, `read_blocking`, `start_async_rx`, `start_async_tx` (+ autobaud helper) | 4 | 4 | 5 | One of the richer facades. |
| RTC | `initialize`, `set`, `now`, `enable_alarm` | 3 | 3 | 5 | Practical baseline; feature depth varies by backend. |
| Flash | `erase`, `program`, `read` | 2 | 3 | 5 | Core operations only; advanced partition/safety workflows remain board policy. |
| Timer | `initialize`, `counter`, `enable_capture_compare` | 2 | 4 | 5 | Good capability metadata, minimal operation surface. |
| ADC | `initialize`, `read_blocking`, `start_continuous`, `start_async`, `start_dma_scan` | 4 | 5 | 5 | Strong compile-time modeling and useful runtime entry points. |
| DAC | `initialize`, `write_blocking`, `write_dual`, `start_dma_waveform` | 3 | 4 | 5 | Reasonable richness for common use cases. |
| CAN | `initialize`, `transmit`, `receive`, `configure_filter`, `enable_rx_interrupt` | 3 | 4 | 3 | Good API shape, but backend availability is narrower. |
| USB | `initialize`, `connect`, `disconnect`, `endpoint_transmit`, `endpoint_receive` | 3 | 4 | 3 | Useful baseline; backend support remains selective. |
| Ethernet | `initialize`, `start`, `stop`, `transmit_frame`, `receive_frame`, `set_mac_address`, `enable_rx_interrupt` | 4 | 5 | 2 | Rich surface, but currently available on relatively few backends. |
| SD/MMC | `initialize`, `read_blocks`, `write_blocks`, `erase_blocks`, `capacity_blocks` | 4 | 5 | 4 | Good block-I/O baseline; some backend support is conditional by SDK defines. |
| DMA | `initialize`, `start_transfer`, `abort` | 2 | 5 | 5 | Strong capability model, minimal operation layer. |
| PWM | `initialize`, `set_duty_cycle`, `start`, `stop` | 3 | 4 | 5 | Practical baseline for motor/control loops. |
| Watchdog | `initialize`, `start`, `refresh` | 2 | 4 | 5 | Intentionally focused surface. |
| I2S | `initialize`, `transmit_blocking`, `receive_blocking`, `start_dma_tx` | 3 | 4 | 4 | Good baseline; availability depends on backend/SDK coverage. |
| RCC | `configure`, `get_system_clock_hz`, `enable_peripheral_clock`, `disable_peripheral_clock` | 3 | 4 | 4 | Useful control points; deep clock-tree policy remains board-level. |
| CRC | `initialize`, `calculate`, `reset`, `get_random` | 2 | 4 | 4 | CRC is usable; `get_random` is backend-dependent and not universally meaningful. |

## Overall Assessment

- **Strengths**:
  - Compile-time capability model is broad and explicit.
  - Core portability surface is consistent and clean.
  - Major peripherals (UART/ADC/SD/Ethernet) have useful practical APIs.
- **Current limits**:
  - Several facades are intentionally minimal (QSPI, DMA, Timer, Flash).
  - Backend parity is not uniform for CAN/USB/Ethernet.
  - Some support is intentionally conditional on SDK headers/defines.

## High-Value Expansion Priorities

1. Expand `qspi` facade beyond `initialize`:
   - add command transaction, indirect read/write, memory-mapped mode control.
2. Expand `timer` and `dma` operation surfaces:
   - timer start/stop, period update, capture readback; DMA callbacks and transfer status.
3. Expand `flash` policy operations:
   - explicit sector metadata query and safer partial program workflows.
4. Raise backend parity for CAN/USB/Ethernet:
   - prioritize missing runtime paths where capabilities are currently narrow.

## Data Sources

- Facade headers under `include/microhal/*/*.hpp`
- Capability matrix in `docs/capability_matrix.md`
- Backend trait capability declarations under `include/microhal/backends/**/traits.hpp`
