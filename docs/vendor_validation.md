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
- `MICROHAL_NORDIC_SUPPORT_ROOT`: path to Nordic support content (for example `vendor-support/nrf-bm/v2.0.1` or `vendor-support`)
- `MICROHAL_NORDIC_DEFINITIONS`: SoC and feature defines (for example `NRF52840_XXAA`)

Example:

```bash
export MICROHAL_NORDIC_INCLUDE_DIRS="/path/to/nrfx;/path/to/nrfx/mdk;/path/to/cmsis"
export MICROHAL_NORDIC_DEFINITIONS="NRF52840_XXAA"
cmake --preset nordic-backend-ci \
  -DCMAKE_SYSTEM_NAME=Generic \
  -DCMAKE_C_COMPILER=arm-none-eabi-gcc \
  -DCMAKE_CXX_COMPILER=arm-none-eabi-g++ \
  -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
  -DCMAKE_C_FLAGS="-mcpu=cortex-m4 -mthumb" \
  -DCMAKE_CXX_FLAGS="-mcpu=cortex-m4 -mthumb" \
  -DCMAKE_EXE_LINKER_FLAGS="-mcpu=cortex-m4 -mthumb -specs=nosys.specs" \
  -DMICROHAL_WERROR=OFF
cmake --build --preset nordic-backend-ci
ctest --preset nordic-backend-ci --output-on-failure -R "^microhal_capability_claim_policy$"
```

With `MICROHAL_NORDIC_SUPPORT_ROOT` set, CMake auto-derives include directories from the Nordic tree. If neither include dirs nor support root are set, configuration fails intentionally.
For the enabled-driver validation below, configure this preset with an ARM bare-metal toolchain
and CPU flags; do not use the host compiler or run ARM executables through the host CTest runner.
The matrix script supplies the toolchain flags automatically.

### nrf-bm Unified Tree

`vendor-support/nrf-bm/v2.0.1` can be used as a single source for legacy and newer Nordic families in this repository's validation flow.

- nRF52 validation definition: `NRF52840_XXAA`
- nRF53 validation definition: `NRF5340_XXAA_APPLICATION`
- nRF54H20 application-core definitions: `NRF54H20_XXAA;NRF_APPLICATION`
- nRF54L15 application-core definitions: `NRF54L15_XXAA;NRF_APPLICATION`

Run the matrix locally:

```bash
bash ./scripts/validate_nordic_vendor_matrix.sh
```

The runner requires `arm-none-eabi-gcc`/`arm-none-eabi-g++` with newlib and its C++ headers/libraries.
Set `MICROHAL_NORDIC_SUPPORT_ROOT` for a non-default SDK path and
`MICROHAL_NORDIC_MATRIX_BUILD_ROOT` for a separate build directory. It cross-compiles/links the
selection, API-contract, and preset-expectation targets, then runs only the host capability-policy
script. Existing build directories are reused rather than deleted.

On nRF52840, nRF5340 application core, and nRF54L15 application core,
`microhal_backend_api_contract` additionally depends on the object target
`microhal_nordic_driver_instantiation`. This instantiates actual enabled peripheral template bodies,
public facade calls, capability assertions, and interrupt dispatchers, rather than only testing
`requires` expressions. It enables the implemented nrfx drivers and API 4.1 (including RTC on
nRF52/53). When the support root is auto-discovered it also compiles the corresponding nrfx C driver
sources. Explicit include-only configurations compile the C++ instantiations but must validate/link
their driver sources separately.

The object target uses test-only glue from `tests/nordic/`; applications need their own real nrfx
integration. These checks do not link a complete firmware image, start clocks, execute peripheral
operations, or validate electrical behavior. nRF54H20 retains only the earlier header/API-contract
checks; it does not gain the expanded native-driver coverage. The host suite separately runs
`microhal_nordic_logic` for status mapping, transfer lifecycle, PWM timing, channel masks, transfer
count/address boundaries, and unsupported-without-SDK behavior.

### NuttX validation

Configure the NuttX tree for the board and configuration being validated so its generated
`nuttx/config.h` is present. Pass the NuttX public include tree, architecture include directory,
generated configuration include directory, and board include directory through
`MICROHAL_VENDOR_VALIDATION_NUTTX_INCLUDE_DIRS`:

```bash
cmake -S . -B build/nuttx-h743-vendor \
  -DMICROHAL_VENDOR_VALIDATION=ON \
  -DMICROHAL_VENDOR_VALIDATION_PROVIDER=NUTTX \
  -DMICROHAL_VENDOR_VALIDATION_NUTTX_INCLUDE_DIRS="/path/to/nuttx/include;/path/to/nuttx/arch/arm/include;/path/to/nuttx/boards/arm/stm32h7/nucleo-h743zi/include"
cmake --build build/nuttx-h743-vendor \
  --target microhal_backend_selection microhal_backend_api_contract microhal_preset_capability_expectations
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
