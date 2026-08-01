# Vendor Validation

For the full per-backend SDK/environment contract (STM32, Kinetis/MCUXpresso, Nordic nrfx, Zephyr, NuttX, mock), see
[docs/backend_sdk_requirements.md](backend_sdk_requirements.md).

microHAL can validate backends against concrete vendor headers instead of the host-side fallback paths. For STM32 builds there are two ways to provide those headers:

## Preset Macro Expectations

The active CMake preset determines which backend-selection and vendor macros are expected at compile time.

| Preset | Vendor Validation | Expected backend-selection macros | Expected config macro behavior |
| --- | --- | --- | --- |
| default host (`build/`) | OFF | none (`MICROHAL_BACKENDS_EXPLICIT` unset) | NuttX/Zephyr `CONFIG_*` macros are usually unset on host, so conditional capabilities (for example NuttX I2C/SPI/SD and Zephyr QSPI) compile as unsupported unless explicitly provided by includes/defines |
| `contractual-examples-ci` | ON (`STM32`) | `MICROHAL_BACKENDS_EXPLICIT`, `MICROHAL_ENABLE_STM32_HAL`, `MICROHAL_ENABLE_STM32_LL`, `MICROHAL_ENABLE_MOCK` | STM32 family/device macros are set from preset inputs; non-selected backends are not compiled |
| `zephyr-backend-ci` | ON (`ZEPHYR`) | `MICROHAL_BACKENDS_EXPLICIT`, `MICROHAL_ENABLE_ZEPHYR`, `MICROHAL_ENABLE_MOCK` | Zephyr capabilities follow provided Zephyr include tree and optional `MICROHAL_VENDOR_VALIDATION_ZEPHYR_DEFINITIONS` (`CONFIG_*`) |
| `nordic-backend-ci` | ON (`NORDIC`) | `MICROHAL_BACKENDS_EXPLICIT`, `MICROHAL_ENABLE_NORDIC_NRFX`, `MICROHAL_ENABLE_MOCK` | Nordic capabilities follow nrfx header discovery and optional SoC defines from `MICROHAL_VENDOR_VALIDATION_NORDIC_DEFINITIONS` |

The compile-time guard target `microhal_preset_capability_expectations` exists specifically to enforce these host-vs-vendor conditional expectations for NuttX/Zephyr capabilities.

## Nordic nrfx Validation

Use the `nordic-backend-ci` preset with the required environment variables:

- `MICROHAL_NORDIC_INCLUDE_DIRS`: semicolon-separated include roots for nrfx headers (optional when support root is set)
- `MICROHAL_NORDIC_SUPPORT_ROOT`: path to Nordic support content (for example `vendor-support/nrf-bm/v2.0.0` or `vendor-support`)
- `MICROHAL_NORDIC_DEFINITIONS`: SoC and feature defines (for example `NRF52840_XXAA`)

Example:

```bash
export MICROHAL_NORDIC_INCLUDE_DIRS="/path/to/nrfx;/path/to/nrfx/mdk;/path/to/cmsis"
export MICROHAL_NORDIC_DEFINITIONS="NRF52840_XXAA"
cmake --preset nordic-backend-ci
cmake --build --preset nordic-backend-ci
ctest --preset nordic-backend-ci --output-on-failure
```

With `MICROHAL_NORDIC_SUPPORT_ROOT` set, CMake auto-derives include directories from the Nordic tree. If neither include dirs nor support root are set, configuration fails intentionally.

### nrf-bm Unified Tree

`vendor-support/nrf-bm/v2.0.0` can be used as a single source for legacy and newer Nordic families in this repository's validation flow.

- nRF52 validation definition: `NRF52840_XXAA`
- nRF53 validation definition: `NRF5340_XXAA_APPLICATION`
- nRF54 validation definition: `NRF54H20_XXAA`

Run the matrix locally:

```bash
bash ./scripts/validate_nordic_vendor_matrix.sh
```

- Set `MICROHAL_VENDOR_VALIDATION_STM32_INCLUDE_DIRS` directly when your CI or local environment already knows the exact include list.
- Set `MICROHAL_VENDOR_VALIDATION_STM32_SUPPORT_ROOT` to a local support tree, typically created by `./scripts/clone_vendor_support.sh`, and let CMake derive the include directories automatically.

## STM32 Support Root Layout

When `MICROHAL_VENDOR_VALIDATION_STM32_SUPPORT_ROOT` is set, the support tree is expected to contain:

- `stm32<family>xx-hal-driver/Inc`
- `cmsis-device-<family>/Include`
- `cmsis-core/CMSIS/Core/Include`
- `CMSIS_5/CMSIS/Core/Include`

The clone script keeps both `cmsis-core` and `CMSIS_5` on purpose.

## CMSIS Core Selection

When CMake auto-derives the STM32 include stack from `MICROHAL_VENDOR_VALIDATION_STM32_SUPPORT_ROOT`, it selects the CMSIS core headers in this order:

1. `cmsis-core/CMSIS/Core/Include`
2. `CMSIS_5/CMSIS/Core/Include`

This preference order is intentional.

- `cmsis-core` is the preferred STM32 path because it matches ST's current package layout and is the closest fit for STM32 device-pack style validation.
- `CMSIS_5` remains in the support tree as a compatibility fallback for older local mirrors and older integrations that still expect the legacy upstream layout.

If `MICROHAL_VENDOR_VALIDATION_STM32_INCLUDE_DIRS` is set explicitly, CMake skips support-root auto-detection and uses the provided include list as-is.

## Recommended Local Workflow

Clone or update the vendor support tree:

```bash
./scripts/clone_vendor_support.sh
```

Configure a concrete STM32 validation build:

```bash
cmake -S . -B build/stm32f4-vendor \
  -DMICROHAL_VENDOR_VALIDATION=ON \
  -DMICROHAL_VENDOR_VALIDATION_PROVIDER=STM32 \
  -DMICROHAL_VENDOR_VALIDATION_STM32_FAMILY=STM32F4 \
  -DMICROHAL_VENDOR_VALIDATION_STM32_DEVICE=STM32F407xx \
  -DMICROHAL_VENDOR_VALIDATION_STM32_SUPPORT_ROOT="$PWD/vendor-support"
```

Use explicit `MICROHAL_VENDOR_VALIDATION_STM32_INCLUDE_DIRS` instead when the headers come from a nonstandard SDK checkout or a CI-provided bundle.

## Representative Matrix

The repository validates three deliberately different STM32 HAL/LL header shapes in CI:

- `STM32C0` / `STM32C011xx` on Cortex-M0+
- `STM32F4` / `STM32F407xx` on Cortex-M4
- `STM32G4` / `STM32G431xx` on Cortex-M4

Run the local matrix after populating `vendor-support`:

```bash
bash ./scripts/validate_stm32_vendor_matrix.sh
```

The runner builds only `microhal_compile_time_smoke` as a static library. It validates microHAL against real Cube headers and public wrapper instantiation, but deliberately does not link Cube driver objects or run hardware. Each family supplies its Cortex CPU flag and sets `CMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY`, so the bare-metal compiler is never asked to link a host-style probe executable.

The matrix is representative rather than a claim that every mirrored STM32 family is already compatible. In particular, STM32F1 currently needs an explicit legacy HAL/LL compatibility pass before it can join the passing matrix.
