# Backend SDK Requirements

This document defines what each microHAL backend needs from its vendor SDK or environment to compile and what the board integration layer must provide.

## Quick Matrix

| Backend tag | Vendor/environment | Minimum SDK/environment requirement | Required compile-time surface | Board-binding requirements |
| --- | --- | --- | --- | --- |
| `microhal::stm32_hal_tag` | ST STM32Cube HAL + CMSIS device headers | Cube HAL family `Inc/` headers, CMSIS core/device headers, family + part macro (`STM32F4`, `STM32F407xx`, etc.) | HAL symbols for the used peripherals (`HAL_UART_*`, `HAL_I2C_*`, etc.) and `USE_HAL_DRIVER` | Peripheral identity through `binding::base_address` (or legacy instance fields); GPIO EXTI board hook for IRQ wiring |
| `microhal::stm32_ll_tag` | ST STM32Cube LL + CMSIS device headers | Cube LL headers for selected family + CMSIS | LL symbols (`LL_USART_*`, `LL_I2C_*`, etc.) | Same binding model as STM32 HAL |
| `microhal::kinetis_espresso_tag` | NXP MCUXpresso SDK (Kinetis or i.MX RT package) | `fsl_common.h` available in include path (backend auto-detects optional `fsl_*` headers) | Peripheral drivers are enabled by header presence + SoC symbols (`I2C0`, `LPI2C1`, `FTM0`, `PIT`, etc.) | Register-backed bindings (`binding::base_address`) and GPIO routing (`binding::port_index`, `binding::pin_index`, `binding::mux`) |
| `microhal::nordic_nrf_tag` | Nordic bare-metal nrfx SDK | `nrfx.h` plus optional drivers (`nrfx_twim.h`, `nrfx_spim.h`, `nrfx_qspi.h`, `nrfx_sdhc.h`, `nrfx_uarte.h`, `nrfx_rtc.h`, `nrfx_timer.h`, `nrfx_saadc.h`, `nrfx_dac.h`, `nrfx_nvmc.h`, `nrfx_pwm.h`, `nrfx_wdt.h`) | Support is enabled per-driver by header discovery; no Zephyr dependency | Standard config fields and board-owned pin/clock routing for each peripheral instance |
| `microhal::zephyr_tag` | Zephyr RTOS driver model | Zephyr include tree + generated headers (`autoconf.h`, devicetree-generated headers) | `CONFIG_*` gates and Zephyr driver APIs (`device`, `gpio`, `i2c`, `spi`, `adc`, etc.) | Device identity via `binding::device` or `binding::node_id`; peripheral-specific config (`gpio_dt_spec`, `spi_config`, etc.) |
| `microhal::nuttx_tag` | NuttX RTOS | NuttX headers and board defconfig-selected subsystems | `__NUTTX__`, `CONFIG_*` feature macros, character/block device APIs (`open/read/write/ioctl`) | Device paths (for example `/dev/i2c0`, `/dev/spi0`, `/dev/mmcsd0`) via `binding::device_path` or `Config::device_path` |
| `microhal::mock_tag` | Host test environment | No vendor SDK | C++20 host compiler only; full peripheral recorder surface | None |

## Kinetis/MCUXpresso Family Coverage Model

Kinetis backend support is now keyed to **SDK availability** instead of a short hardcoded CPU allowlist:

- If `fsl_common.h` is visible, `MICROHAL_KINETIS_SDK_PRESENT=1`.
- Optional peripheral support is enabled by the presence of the relevant SDK headers (`fsl_i2c.h`, `fsl_lpi2c.h`, `fsl_ftm.h`, `fsl_pit.h`, etc.) and SoC symbols.
- This allows the same backend source to compile across broader Kinetis/i.MX RT families without per-part source forks.

What this means in practice:

- The backend can compile on families that expose either legacy modules (for example `I2C0`, `FTM0`) or newer IP blocks (for example `LPI2C1`, `PIT`).
- Runtime capability still depends on which modules your selected SoC/SDK actually provides.

## Vendor Validation Inputs

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

- Ensure the include path exposes `nrfx.h` and the driver headers you intend to use (`nrfx_twim.h`, `nrfx_spim.h`, `nrfx_qspi.h`, `nrfx_sdhc.h`).
- Define the target SoC symbols expected by your nrfx configuration (for example `NRF52840_XXAA`) so peripheral instance tables match the selected device.
- Prefer `MICROHAL_VENDOR_VALIDATION_NORDIC_SUPPORT_ROOT` for auto-setup when using the local unified tree under `vendor-support/nrf-bm/v2.0.0`; explicit `MICROHAL_VENDOR_VALIDATION_NORDIC_INCLUDE_DIRS` remains supported for custom layouts.

## Board Integration Contract (all backends)

Regardless of vendor SDK, board code is responsible for:

- pin muxing and pin ownership
- clock tree and gate setup
- DMA/DMAMUX routing
- IRQ priority and vector hookup
- peripheral instance identity bindings

microHAL validates interface shape and capability claims at compile time, but it does not infer board topology from instance numbers.
