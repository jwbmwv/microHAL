# Backend SDK Requirements

This document defines what each microHAL backend needs from its vendor SDK or environment to compile and what the board integration layer must provide.

## Quick Matrix

| Backend tag | Vendor/environment | Minimum SDK/environment requirement | Required compile-time surface | Board-binding requirements |
| --- | --- | --- | --- | --- |
| `microhal::stm32_hal_tag` | ST STM32Cube HAL + CMSIS device headers | Cube HAL family `Inc/` headers, CMSIS core/device headers, family + part macro (`STM32F4`, `STM32F407xx`, etc.) | HAL symbols for the used peripherals (`HAL_UART_*`, `HAL_I2C_*`, etc.) and `USE_HAL_DRIVER` | Peripheral identity through `binding::base_address` (or legacy instance fields); GPIO EXTI board hook for IRQ wiring |
| `microhal::stm32_ll_tag` | ST STM32Cube LL + CMSIS device headers | Cube LL headers for selected family + CMSIS | LL symbols (`LL_USART_*`, `LL_I2C_*`, etc.) | Same binding model as STM32 HAL |
| `microhal::kinetis_espresso_tag` | NXP MCUXpresso SDK (Kinetis or i.MX RT package) | `fsl_common.h` available in include path (backend auto-detects optional `fsl_*` headers) | Peripheral drivers are enabled by header presence + SoC symbols (`I2C0`, `LPI2C1`, `FTM0`, `PIT`, etc.) | Register-backed bindings (`binding::base_address`) and GPIO routing (`binding::port_index`, `binding::pin_index`, `binding::mux`) |
| `microhal::nordic_nrf_tag` | Nordic bare-metal nrfx SDK | Current stateful nrfx APIs, target CMSIS/system headers, and application glue/configuration; RTC requires API 4.1 | Native serial masters/slaves, acquisition, waveforms, RTC/GRTC, storage, clocks and target-gated USB/CAN/memory DMA/entropy | Typed bindings, linked drivers/support libraries, clock/security-domain ownership, IRQ dispatch, and DMA buffer lifetimes |
| `microhal::zephyr_tag` | Zephyr RTOS driver model | Zephyr include tree + generated headers (`autoconf.h`, devicetree-generated headers) | `CONFIG_*` gates and Zephyr driver APIs (`device`, `gpio`, `i2c`, `spi`, `adc`, etc.) | Device identity via `binding::device` or `Config::device`, initialized with `DEVICE_DT_GET()` in board code; peripheral-specific config (`gpio_dt_spec`, `spi_config`, etc.) |
| `microhal::nuttx_tag` | NuttX RTOS | NuttX headers and board defconfig-selected subsystems | `__NUTTX__`, generated `nuttx/config.h`, `CONFIG_*` feature macros, and character/block device or network socket APIs | Character-device paths (for example `/dev/i2c0`, `/dev/spi0`, `/dev/mmcsd0`) via `binding::device_path` or `Config::device_path`; flash requires a board-supplied `mtd_dev_s*` binding |
| `microhal::mock_tag` | Host test environment | No vendor SDK | C++20 or newer host compiler; full peripheral recorder surface | None |

## Kinetis/MCUXpresso Family Coverage Model

Kinetis backend support is now keyed to **SDK availability** instead of a short hardcoded CPU allowlist:

- If `fsl_common.h` is visible, `MICROHAL_KINETIS_SDK_PRESENT=1`.
- Optional peripheral support is enabled by the presence of the relevant SDK headers (`fsl_i2c.h`, `fsl_lpi2c.h`, `fsl_ftm.h`, `fsl_pit.h`, etc.) and SoC symbols.
- This allows the same backend source to compile across broader Kinetis/i.MX RT families without per-part source forks.

What this means in practice:

- The backend can compile on families that expose either legacy modules (for example `I2C0`, `FTM0`) or newer IP blocks (for example `LPI2C1`, `PIT`).
- Runtime capability still depends on which modules your selected SoC/SDK actually provides.

## Vendor Validation Inputs

### Explicit serial transfer modes

I2C, SPI, UART, and I2S configurations accept a final `microhal::transfer_mode`
parameter: `automatic` (the backward-compatible default), `polled`, `interrupt`, or
`dma`. Custom binding-backed configurations expose `transfer_mode_value` with the
same enum. Explicit selection takes precedence over the older DMA/interrupt
request flags. Query `backend_traits<Backend>::<peripheral>_capability().transfer_modes`
to check the actual implemented engines; the older `dma`/`isr` booleans alone do
not identify how a blocking call transfers data. Explicit modes are currently
implemented for STM32 HAL/LL and supported MCUXpresso modules. Other backends
retain their existing `automatic` paths, rather than claiming modes they cannot
select.

Blocking calls in interrupt/DMA mode start the selected engine and wait for its
completion, bounded by `Config::timeout_ms` (1000 ms when absent). They need live
interrupts and an advancing timeout clock; do not call them from a masked or
higher-priority interrupt that prevents completion. Nonblocking calls return
after submission: I2C/SPI offer `write_async`/`read_async`, I2C also
`write_read_async`, SPI also `transfer_async`, UART uses
`start_async_tx`/`start_async_rx`, and I2S offers
`transmit_async`/`receive_async`. Callbacks accept `status` or no arguments and
normally run in IRQ context; explicit cancellation may notify from the caller of
`abort()`. Polled mode reports `unsupported` for async requests;
it never runs a blocking operation disguised as async.

Use one configuration/owner per physical peripheral and serialize application
control calls between threads. Buffers must remain DMA-accessible, correctly aligned,
and alive until completion or successful `abort()`. Cache maintenance and memory
placement remain board/application responsibilities. The baseline serial contract
is finite transfers; STM32 UART and ADC additionally expose explicit streaming
operations described below. A failed submission does not generate
a completion callback. `start_dma_tx` remains available for legacy I2S configs;
with an explicit mode it only starts when that mode is `dma`.

#### STM32 integration

Explicit UART, SPI, and I2C modes use Cube HAL on `stm32_hal_tag` and independent
register-level engines on `stm32_ll_tag`. I2S retains its existing HAL integration.
For the HAL tag, enable the matching `HAL_I2C_MODULE_ENABLED`, `HAL_SPI_MODULE_ENABLED`,
`HAL_UART_MODULE_ENABLED`, or `HAL_I2S_MODULE_ENABLED`. Interrupt/DMA modes also
require the respective `USE_HAL_*_REGISTER_CALLBACKS=1` option in every translation
unit, including the linked HAL driver C sources.

Board MSP initialization must enable clocks, configure pins, initialize and link
the appropriate `hdmatx`/`hdmarx` handles, select DMA/DMAMUX requests and enable the
NVIC. Alternatively `Config::binding::configure_dma(handle)` may configure/link
DMA after HAL initialization and return `status`, `bool`, or `void`. Both DMA
handles are required before an I2C combined DMA transaction starts; two-line SPI
master DMA receive also needs TX DMA. Finite serial operations reject circular
DMA handles; UART streaming must be submitted using `start_circular_rx`.

Route peripheral IRQs through the facade's `handle_interrupt()` (I2C handles both
event/error dispatch) and DMA IRQs through `HAL_DMA_IRQHandler` for the board's
linked channels/streams. Clock ticks must continue while a blocking operation
waits. Callback ownership is shared across handler instantiations. SPI/I2C/I2S
allow one outstanding operation per configuration. UART has independent TX and
RX ownership, permitting simultaneous transmission and reception. Finite
completion restores the owned callback pointers before notifying the caller.

`transfer_status()` reports `busy` while active and a terminal status afterward.
Timeout/error cleanup and `abort()` stop the hardware; reinitialize after abort,
an asynchronous hardware error, or failed hardware submission (not a preflight
`unsupported`/`busy` result). If hardware refuses to
abort, ownership stays busy and no terminal callback is issued:
`transfer_status()`/`abort()` report the failure. Keep the buffers alive, recover
the hardware and retry `abort()` before reusing them, including after a blocking
call returns an abort error.

HAL I2C sequential interrupt/DMA APIs preserve repeated START. Arbitrary combined
write/read in explicit **polled** mode currently reports `unsupported` instead
of substituting STOP+START or secretly using interrupts. Individual polled reads
and writes remain supported. An address-only `probe()` uses the HAL polling
probe and shares the same busy guard, irrespective of the selected data engine.
I2S applies configured TX/RX role, framing standard, sample rate and master-clock
selection; the public sample format remains 16-bit. Receive DMA is exposed through
`receive_async`/`receive_blocking` with an RX configuration.

STM32 vendor-validation auto-setup enables serial callback registration in its
generated HAL configuration and instantiates public polled/interrupt/DMA bodies
against the real SDK. Host state-machine tests exercise callbacks, ownership,
repeated-START phases, submission failure, timeout and abort recovery; this is
not physical-board validation.

#### STM32 UART reception and directional lifecycle

`start_receive_to_idle<Handler>(buffer)` receives a finite, variable-length
message in interrupt or DMA mode. `start_circular_rx<Handler>(buffer)` uses
circular DMA and reports completed ranges without releasing the receive buffer.
Range handlers accept `(status, std::span<std::uint8_t>)`; consume/copy a range
before DMA can overwrite it. DMA half/full notifications are not a queue.
On GPDMA, board code must supply the circular linked-list resources as well.

`tx_status()` and `rx_status()` inspect the directions independently.
`abort_tx()` and `abort_rx()` stop only that direction; `abort()` stops both.
Do not release either buffer after a failed cancellation. Configure the HAL RX
DMA handle for circular operation before starting the circular API; finite RX
uses normal DMA. Board IRQ dispatch must remain active during reception.

#### Native STM32 LL serial bindings

Native LL UART/SPI/I2C explicit modes do not require Cube HAL callback
registration or HAL driver objects. Select only `STM32_LL` and do not define
`USE_HAL_DRIVER` when a HAL-free firmware is required. Other, legacy
HAL-forwarded peripherals remain separate optional dependencies.

Provide a configuration binding or specialize
`microhal::backends::stm32::ll::serial_binding<kind, instance>` for an unmodified
public configuration. The binding supplies `source_clock_hz()` and, for blocking
deadlines, a wrapping `std::uint32_t now_ms()`. Peripheral identity uses the
existing base-address binding. Modern I2C also needs a board-calculated
`timing` value matching the peripheral clock, bus speed, and electrical timing.

For DMA, `tx_dma()` and `rx_dma()` return native resources constructed with
`ll::channel_dma`, `ll::stream_dma`, or `ll::gpdma`. Board code selects the
request/channel/DMAMUX mapping and enables the relevant clocks and NVIC entries;
microHAL programs addresses, widths, counts, transfer interrupts, and lifecycle.
Circular GPDMA additionally needs a persistent `serial_gpdma_node`. Dispatch
peripheral IRQs through `handle_interrupt()` and DMA IRQs through
`handle_dma_interrupt()`. No HAL DMA interrupt handler is used for these native
resources.

Native SPI supports full-duplex, half-duplex, and simplex configurations in
master/slave interrupt/DMA modes; wrong-direction submissions are rejected.
Half-duplex/simplex DMA needs only the participating DMA endpoint. Classic SPI
receive-only masters have no hardware transfer counter: stopping after the
requested data can generate extra clocks because of IRQ/DMA service latency.
Use full-duplex master reception with dummy TX when an exact clock count is
required. Classic slave BSY errata can cause a conservative timeout/error rather
than a false successful completion; validate the selected device on hardware.

#### STM32 SPI and I2C slave transfers

`spi::slave_config` / `spi::slave` and `i2c::slave_config` / `i2c::slave`
add fixed-length interrupt/DMA slave transfers. Submit `transmit_async` or
`receive_async` before the external master accesses the device. SPI additionally
supports full-duplex `transfer_async` and uses hardware NSS. I2C's `OwnAddress`
is unshifted and validated for the selected seven- or ten-bit address mode.
Completion handlers accept `status` or no arguments. These APIs do not
implement an application register map, response queue, or automatic variable-length
slave protocol.

#### STM32 ADC acquisition

Both STM32 tags share a native register-level ADC acquisition engine. It does
not borrow HAL ADC callbacks or depend on a HAL ADC handle.
Channel arguments are numeric channel indexes, not encoded HAL/LL channel
constants. The requested sample time and channel sequence are configured before
acquisition. Fixed-order hardware sequencers reject orders they cannot represent
rather than silently reordering the results.

`start_async` provides a one-shot result, `start_dma_scan` a finite scan, and
`start_dma_stream` circular acquisition. A streaming callback accepts
`(status, std::span<std::uint16_t>)` for a completed half-buffer; both halves
must contain an integral number of scans. The callback's range is valid only
until hardware reuses that half. Keep storage alive until successful
`stop()` / `abort()`. `transfer_status()` returns `result<dma_transfer_state>`.
Route the ADC IRQ to `handle_interrupt()` and its DMA IRQ to
`handle_dma_interrupt()`.
Passing `circular=false` to `start_dma_stream` delivers two half-block
notifications followed by automatic stop. If both half/full events accumulate
in circular mode before service, acquisition stops with `io_error` rather than
returning a half-buffer that hardware might already be overwriting.

Use `Config::binding` or specialize `backends::stm32::adc_binding<instance>`.
DMA bindings supply typed `dma_channel()` and `dma_controller()` pointers and
`dma_index` (one-based for channel DMA, zero-based for stream DMA).
Board code owns request routing, clocks, pins, NVIC, and cache maintenance.
Optional `enable_clock()` and `configure_pins()` hooks support initialization.
The trailing ADC configuration parameters select an unshifted, device-specific
EXTSEL value and `adc_trigger_edge`; software triggering remains the default.
Timer setup and trigger routing remain board responsibilities.

These drivers have host state/register fixtures and real-header instantiation
checks. Those checks do not replace validation on physical boards, especially
for bus timing, DMA/cache coherency, linked-list DMA, and IRQ priorities.

### MCUXpresso explicit serial integration

Native I2C/LPI2C, DSPI/LPSPI, UART/LPUART, and modern SAI (SDK API >= 2.4)
implement polling, interrupt, and EDMA modes when their SDK headers are present.
I2C/UART/LPUART also support the simple-DMA driver; EDMA takes precedence when
both variants are available. SDK absence never enables a mode based only on
silicon capability.

`Config::binding` supplies a correctly typed peripheral `instance`, its enabled
`source_clock_hz`, and a wrapping monotonic `now_ms()` returning `uint32_t` for
synchronous calls. Optional `idle()` must be bounded. Board code owns pinmux,
clocks, DMA/DMAMUX routing, IRQ priorities and cache maintenance. Provide reserved,
initialized `edma_handle_t*` (or `dma_handle_t*`) resources as follows:

| Peripheral | Binding DMA handles |
| --- | --- |
| I2C/LPI2C/SAI | `dma_handle` |
| UART/LPUART | `tx_dma_handle`, `rx_dma_handle` |
| DSPI | `rx_dma_handle`, `intermediary_dma_handle`, `tx_dma_handle` |
| LPSPI | `rx_dma_handle`, `tx_dma_handle` |

SPI/UART may additionally supply a typed `sdk_config`; SPI accepts native
`transfer_flags`. Route peripheral IRQs through `handle_interrupt()` and DMA IRQs
through the SDK handler for each reserved DMA channel. Blocking interrupt/DMA
operations require live completion IRQs; polling uses register/FIFO flags, not
an interrupt-driven operation in disguise.

Polled combined I2C transactions remain unsupported, polled LPI2C receives are
limited to 256 bytes, and LPI2C polling errors require reinitialization. SPI is
eight-bit; polled DSPI uses CTAR0. SAI supports Philips stereo16/channel0 in
master/slave TX/RX roles. Other SAI framing configurations are rejected. Legacy
`automatic` I2S retains its complete-board-binding adapter contract.

Kinetis `abort()` suppresses completion callbacks; inspect its return value and
`transfer_status()` instead. Successful cancellation returns `ok` and leaves
terminal transfer status `error`. Reinitialize classic I2C after cancellation or
an error; cancellation disables local access without relying on a potentially
unbounded SDK bus-abort loop, and a stuck bus reports `timeout`. DMA buffer
quiescence relies on the selected SDK's DMA-abort guarantees. `now_ms()` is not
required for initialization or asynchronous submission, only synchronous calls.

The host suite builds no-SDK, common-header-only, IRQ, EDMA and simple-DMA fixture
variants. These are deterministic SDK fixtures, **not real MCUXpresso vendor
validation or physical-board testing**. Boundedness inside vendor start/IRQ
routines has not been certified against a real SDK.

### STM32 vendor validation

Use either:

- Explicit include list via `MICROHAL_VENDOR_VALIDATION_STM32_INCLUDE_DIRS`, or
- Support-root auto-discovery via `MICROHAL_VENDOR_VALIDATION_STM32_SUPPORT_ROOT`.

See [docs/vendor_validation.md](docs/vendor_validation.md) for the concrete command lines and matrix flow.

### Zephyr vendor validation

Set:

- `MICROHAL_VENDOR_VALIDATION=ON`
- `MICROHAL_VENDOR_VALIDATION_PROVIDER=ZEPHYR`
- `MICROHAL_VENDOR_VALIDATION_ZEPHYR_INCLUDE_DIRS=<zephyr source + generated includes>`

### Kinetis local validation guidance

Kinetis validation is SDK-discovery based:

- Ensure MCUXpresso SDK include roots containing `fsl_common.h` and your target peripheral driver headers are on the compile include path.
- Define your part macro as your SDK expects (for example `CPU_MK...` or `CPU_MIMXRT...`) so device headers expose the right SoC symbols.

### Nordic local validation guidance

Nordic validation is nrfx-header-discovery based:

- Ensure the include path exposes `nrfx.h` and the driver headers available for the selected SoC.
- Define the exact target SoC symbol (`NRF52840_XXAA`, `NRF5340_XXAA_APPLICATION`, or `NRF54L15_XXAA`) so peripheral instances and configuration match the selected device. nRF54L15 validation defaults to its application core (`NRF_APPLICATION`).
- Prefer `MICROHAL_VENDOR_VALIDATION_NORDIC_SUPPORT_ROOT` for auto-setup when using the local unified tree under `vendor-support/nrf-bm/v2.0.1`; explicit `MICROHAL_VENDOR_VALIDATION_NORDIC_INCLUDE_DIRS` remains supported for custom layouts.

### Nordic board integration

The expanded native paths target **nRF52840**, **nRF5340 application core**, and **nRF54L15
application core**. Header discovery alone does not enable them: enable the corresponding
`NRFX_SPIM_ENABLED`, `NRFX_UARTE_ENABLED`, `NRFX_TIMER_ENABLED`, `NRFX_SAADC_ENABLED`,
`NRFX_PWM_ENABLED`, `NRFX_WDT_ENABLED`, `NRFX_GPIOTE_ENABLED`, `NRFX_I2S_ENABLED`,
`NRFX_QSPI_ENABLED`, and/or `NRFX_RTC_ENABLED` in your application's nrfx configuration.
TWIM uses `NRFX_TWIM_ENABLED`. The implementation uses the current stateful nrfx instance API,
not the older index-only driver ABI. Enable API 4.1 (`NRFX_CONFIG_API_VER_MINOR=1` with API
major 4) for RTC.

Link the selected nrfx C driver sources, normal nrfx support dependencies, CMSIS/system code,
and your startup/vector implementation. Provide real nrfx IRQ/critical-section/atomic/cache
glue; the SDK glue template is not a complete bare-metal integration. The glue under
`tests/nordic/` is compile-validation infrastructure, not an application port.

| API | Required `Config::binding` members |
| --- | --- |
| TWIM | `scl_pin`, `sda_pin` |
| SPIM | `sck_pin`, used `mosi_pin`/`miso_pin`; optional `ss_pin`; half-duplex uses MOSI bidirectionally |
| SPIS | `sck_pin`, `ss_pin`, and pins for enabled directions; explicitly select DMA mode |
| TWIS | `scl_pin`, `sda_pin`; explicitly select DMA mode, seven-bit own address |
| UARTE | `tx_pin`, `rx_pin` |
| PWM | `output_pin`; `Config::timer_index` selects the PWM instance, `channel` is 1-4 |
| SAADC | `channels`, an array of `nrfx_saadc_channel_t` with channel indexes, analog pins, gain, reference, and acquisition settings |
| QSPI | `configuration`, a complete `nrfx_qspi_config_t` for the connected memory |
| I2S | `sck_pin`, `lrck_pin`, `sdout_pin`, `sdin_pin`; `mck_pin` when master-clock output is requested |

Use Nordic encoded pin numbers (`NRF_GPIO_PIN_MAP(port, pin)`); pin ownership and actual board
routing remain application decisions. SAADC binding settings, not the generic `sample_time`,
control acquisition timing; callers supplying millivolt-conversion reference values must account
for the configured gain. Differential channel bindings must match `Config::differential_mode`.
H20 GPIO ports are sparse: P0/P1/P2/P6/P7/P9 expose 12/12/12/14/8/6 pins respectively.
The reported six physical ports do not imply that the largest valid port index is five.

Logical instance indexes map as follows; do not assume that a logical index equals the register
suffix on nRF54:

| Peripheral | nRF52840 | nRF5340 app | nRF54L15 app | nRF54H20 app |
| --- | --- | --- | --- | --- |
| TWIM/TWIS | 0,1 | 0,1,2,3 | 20,21,22,30 | 130-137 |
| SPIM | 0-3 | 0-4 | 00,20,21,22,30 | 130-137,120,121 |
| SPIS | 0-2 | 0-3 | 00,20,21,22,30 | 130-137,120 |
| UARTE | 0,1 | 0-3 | 00,20,21,22,30 | 130-137,120 |
| TIMER | 0-4 | 0-2 | 00,10,20,21,22,23,24 | 120,121,130-137 |
| PWM | 0-3 | 0-3 | 20,21,22 | 120,130-133 |
| WDT | WDT | 0,1 | 30 when exposed, then 31 | 010,011 |
| RTC | 0-2 | 0,1 | GRTC | GRTC |
| I2S | I2S | I2S0 | I2S20 | absent |
| QSPI | QSPI | QSPI | absent | absent |

Route the appropriate vectors to `microhal::backends::nordic::nrfx::*_api::handle_interrupt`.
TWIM/I2C, SPIM/SPI, UARTE/UART, TIMER, and RTC dispatchers take the logical instance index;
GPIO/GPIOTE, ADC/SAADC, and I2S dispatchers take no arguments. For example, an nRF52840
application using SPIM3 routes its dedicated vector like this:

```cpp
extern "C" void SPIM3_IRQHandler()
{
  microhal::backends::nordic::nrfx::spi_api::handle_interrupt(3U);
}
```

Clock configuration requires `Binding::owns_clock=true`: nRF52840 supports 64 MHz and
nRF5340/L15 support 64/128 MHz. Native peripheral-clock gating covers only the shared RTC
LF domain at instance zero. H20 clock configuration/gating remain system-controller-owned;
its RCC path provides readback only (up to 320 MHz). Board code owns clock coordination,
pinmux, startup, security permissions, and requests needed by other firmware.

Blocking I2C, SPI, UART, ADC, and I2S operations need a nonzero, accurate `SystemCoreClock` and
working IRQ dispatch even though they return synchronously. `Config::timeout_ms` defaults to
1000 when omitted. Timeouts bound polling delays, not interrupt/scheduling overhead. Do not block
from an interrupt that prevents completion IRQs from running. Cancellation/timeout must finish
hardware shutdown before reusing DMA buffers. Some terminal failures uninitialize the peripheral;
reinitialize when required (including SPIS cancellation).
QSPI uses the nrfx driver's millisecond timeout (zero selects its vendor default).

Callbacks run in interrupt context. Keep asynchronous buffers alive and DMA-accessible until
completion, including any cache maintenance required by the target. Serialize all access to an
instance; the transfer guards are not a multi-threaded peripheral-ownership framework. Do not run
competing TWIM/SPIM/UARTE drivers on shared SERIAL registers: configure nrfx PRS where applicable
or enforce ownership in board code.

Current operation limits:

- Serial masters support automatic, explicit polled, and explicit DMA modes. Polling still uses
  EasyDMA but services completion synchronously; bytewise non-DMA interrupt mode is unavailable.
  SPIS/TWIS slaves require explicitly selected DMA mode. SPIM supports full/simplex and
  half-duplex on MOSI; SPIS supports full/simplex only. Maximum clock is instance-specific.
  UART supports independent TX/RX ownership/cancellation, one/two stop bits, and no/even parity;
  odd parity is unavailable only on nRF52840. Default instances support up to 1 Mbaud;
  L15 UARTE00 supports 8 Mbaud at 128 MHz (4 Mbaud at 64 MHz), and H20 UARTE120 up to 20 Mbaud.
  Hardware RTS/CTS is opt-in via `Binding::hardware_flow_control=true` and valid, distinct
  `rts_pin`/`cts_pin` without TX/RX overlap; omission keeps it disabled. Hardware autobaud is absent.
  Each TWIM/SPIM/UARTE transfer phase is limited to 65535 bytes to fit these targets' DMA counters.
  IDLE/circular UART RX additionally reserves `rx_timeout_timer` and `rx_timeout_us` (at least
  one UART frame). On 52/53/L15 provide an initialized `rx_timeout_gppi` topology resource;
  link the matching nrfx GPPI implementation and flag allocator (L15 also needs the D2PPI
  topology source). H20 uses reserved `rx_timeout_dppic`/`rx_timeout_channel` resources in the
  same local domain as UART/TIMER. Route the timer IRQ to `port::handle_rx_timeout_interrupt()`.
  Circular RX uses two halves and requires timely completion IRQ servicing; callbacks do not
  extend a completed half's lifetime beyond the next handoff.
- TIMER uses 32-bit up-counting/internal clock. `clock_hz / (prescaler + 1)` must be exact and
  supported by the selected instance. CC0 is reserved for period clear and the last physical CC
  for counter snapshots. Public channels use the intervening CCs: two on nRF52 TIMER0-2,
  four on TIMER3-4/nRF53/H20, and six on nRF54L TIMER10 (four on its other timers).
  Period changes reset the counter. Capture requires binding hooks
  `capture_connect(channel, capture_task_address, edge_callback)` and
  `capture_disconnect(channel)` to allocate/release a real edge-to-CAPTURE route and arrange
  edge notification; CAPTURE alone does not generate a compare interrupt. Optional
  `Channel::compare_ticks` selects a compare point; otherwise callbacks occur at the period.
  `disable_capture_compare` releases that channel's resources.
- PWM supports four independently owned outputs per peripheral, sharing one period.
  Duty updates modify looping EasyDMA values without stopping neighboring outputs.
  `set_frequency` requires all outputs to be stopped and an exactly realizable frequency.
  Complementary/dead-time outputs remain unsupported.
- SAADC supports 8/10/12/14-bit one-shot conversion, finite scans, continuous readback,
  and finite/circular double-buffered DMA via `start_dma_stream`.
  Scan channel indexes must be strictly ascending, unique, and configured in `Binding::channels`;
  each buffer half must contain whole scans. The block callback receives
  `(status, span<uint16_t>)`; completed halves are reusable until the next callback.
  Continuous/streamed acquisition requires either `Binding::saadc_internal_timer_cc`
  (one active channel, within the SDK's samplerate limits), or board-owned
  `saadc_trigger_connect(sample_task_address, selector, edge)`, `saadc_trigger_start()`,
  `saadc_trigger_stop()`, and `saadc_trigger_disconnect()` hooks for a genuine timer/D(P)PI route.
  Failed connection must release partially allocated resources. Trigger startup follows READY;
  completion/cancellation releases the route. Generic sample-time settings do not replace the
  channel binding's gain, reference, acquisition time, or differential selection.
  Negative single-ended noise clamps to zero. Differential unsigned readings/scans use offset binary:
  signed counts plus `1 << (resolution_bits - 1)`. Use `read_differential_blocking` for signed
  counts and `read_differential_millivolts(channel, positive_full_scale_mv)` for signed voltage.
  The caller supplies the gain-adjusted positive full-scale reference. Unsigned `read_millivolts`
  rejects differential configurations rather than reporting a biased voltage.
- QSPI supports matching FASTREAD/READ2O/READ4O and PP/PP2O/PP4O configurations, plus
  single-line custom instructions with at most eight data bytes. Native transfers require
  word-aligned, DMA-accessible buffers and must fit the hardware count and 24/32-bit address
  range. Read commands need eight dummy cycles; writes need none.
  Native XIP requires `Binding::owns_xip_window=true` in addition to the normal configuration.
  Enter/exit transitions serialize ownership and native activation/deactivation; command/DMA
  operations are blocked while mapped, and failed exit retains ownership. The caller must
  quiesce mapped readers/DMA/ISRs and execute transitions from internal memory. nRF53 XIPEN/cache
  controls are handled natively; memory-mapped access remains a board/application lifetime contract.
  The external memory's quad-enable/write-enable/busy policy remains application-owned.
- I2S supports finite TX/RX, full-duplex transfers, and finite/circular DMA streaming of
  word-aligned 16-bit stereo samples, not PCM-short/long or other sample widths.
  Full-duplex spans have equal lengths; each finite buffer or streaming half requires an even
  sample count, at least four samples, within the hardware DMA count.
  Streaming callbacks receive `(status, completed_tx, completed_rx)`; absent directions have empty
  spans. Completion/stop waits for actual hardware quiescence. Explicit polling still uses
  EasyDMA but services completion synchronously; bytewise interrupt mode is unsupported.
  Requested rates of 8-48 kHz use the nearest realizable prescaler. The default source is
  PCLK32M/32 MHz; optional `Binding::i2s_clock_source` and `i2s_source_clock_hz` describe an
  actually configured alternative clock (ACLK requires an explicit frequency).
  nRF54H20 has no I2S instance in the validated peripheral headers.
- RTC/GRTC provide calendar/time, microsecond subsecond reads, alarms, and periodic IRQs.
  Calendar state is software-maintained, not power-loss persistence. GRTC periodic frequencies
  must divide 1 MHz exactly; this programming resolution is not a throughput guarantee.
  WDT is independent/non-windowed and cannot be stopped after start.
- NVMC, L15 RRAM, and H20 MRAM use the public memory facade. RRAM erase means writing FF
  (four-byte erase units; byte-granular programming); H20 MRAM uses 16-byte aligned programming/FF
  erase and requires authorized `mram_start_address`/`mram_size_bytes` plus
  `mram_acquire()`/`mram_release()` ownership hooks. Never assume an application owns the whole NVM.
- USBD on nRF52840/nRF5340 supports full-speed device endpoints 0-7, 64-byte packets,
  internal EasyDMA, endpoint status/abort/stall, and IRQ dispatch. Use a binding with
  `handle_setup(nrfx_usbd_setup_t const&)` for descriptor/control-request policy; optional
  `usb_event(...)`, `request_usb_clock()`, and `release_usb_clock()` integrate the application.
  RX uses driver-owned staging: receive calls never retain the caller's span after returning.
  TX spans remain alive until completion/successful abort/reset. Failed abort retains ownership.
  H20 USBHS is different hardware and is not exposed by this USBD implementation.
- H20 classical CAN supports extended/remote frames, silent/loopback, filters and RX IRQs.
  Bindings provide CAN clock/message-RAM metadata and `can_prepare()` for board ownership.
  RX uses FIFO0; FIFO1 selection explicitly returns unsupported. Sixteen standard and sixteen
  extended filter rules are retained; an existing identifier can be updated even when its table is full.
  CAN-FD is outside the current public eight-byte frame contract. H20 MVDMA supports one
  memory-copy channel with completion/status/abort, not circular/double-buffer operation.
- Hardware CRC remains absent. nRF52840 uses native RNG; optional SDK crypto-provider entropy on
  nRF5340 (`MICROHAL_NORDIC_ENABLE_CC3XX_ENTROPY=1`) and L15/H20
  (`MICROHAL_NORDIC_ENABLE_CRACEN_ENTROPY=1`) requires matching libraries/platform ports and
  authorized security-domain access. `crc::accelerator::read_random()` preserves entropy failures;
  legacy `get_random()` retains its value-only contract. DAC, Ethernet and native SD/MMC
  remain unavailable on the validated hardware/SDK paths.

All four target configurations have been compile-validated against the local nrf-bm SDK,
including actual enabled nrfx C drivers and public facade calls. This is not physical-board testing.

### NuttX validation guidance

- Configure NuttX for the target board and enabled drivers before compiling microHAL; generated `nuttx/config.h` is required.
- Include the NuttX public headers, architecture headers, and board headers in `MICROHAL_VENDOR_VALIDATION_NUTTX_INCLUDE_DIRS`.
- Nucleo-H743ZI targets can expose Ethernet through the NuttX network stack, but raw-frame access additionally requires packet sockets (`CONFIG_NET_PKT`). The `nucleo-h743zi:netnsh` configuration used for validation does not enable `CONFIG_I2C`, MTD, or packet sockets; enable and register the corresponding drivers before claiming those runtime paths on that configuration.

## Board Integration Contract (all backends)

Regardless of vendor SDK, board code is responsible for:

- pin muxing and pin ownership
- clock tree and gate setup
- DMA/DMAMUX routing
- IRQ priority and vector hookup
- peripheral instance identity bindings

microHAL validates interface shape and capability claims at compile time, but it does not infer board topology from instance numbers.
