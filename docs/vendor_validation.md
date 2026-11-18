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

The default support root is `vendor-support/nrf-bm`. Auto-discovery accepts a
direct SDK root, or its `v2.0.1` / `v2.0.0` subdirectory; set
`MICROHAL_NORDIC_SUPPORT_ROOT` to pin another location. The `all` selection attempts
every target before reporting failures. `nrf54` and `nrf54h20` select the H20
application-core entry.

CI can run the four-target matrix when `MICROHAL_NORDIC_SUPPORT_ROOT` is configured
as a repository variable and the SDK is available at that path on the runner.
The existing include-only validation remains available when only
`MICROHAL_NORDIC_INCLUDE_DIRS` is configured. Neither job downloads the Nordic SDK.
Set `MICROHAL_NORDIC_RUNNER` to an Ubuntu-compatible self-hosted runner label when
the SDK is preinstalled there; otherwise the native job uses `ubuntu-latest`.
The support-root path must exist on the selected runner.

The runner requires `arm-none-eabi-gcc`/`arm-none-eabi-g++` with newlib and its C++ headers/libraries.
Set `MICROHAL_NORDIC_SUPPORT_ROOT` for a non-default SDK path and
`MICROHAL_NORDIC_MATRIX_BUILD_ROOT` for a separate build directory. It cross-compiles/links the
selection, API-contract, and preset-expectation targets, then runs only the host capability-policy
script. Existing build directories are reused rather than deleted.

On nRF52840, nRF5340 application core, nRF54L15 application core, and nRF54H20 application core,
`microhal_backend_api_contract` additionally depends on the object target
`microhal_nordic_driver_instantiation`. This instantiates actual enabled peripheral template bodies,
public facade calls, capability assertions, and interrupt dispatchers, rather than only testing
`requires` expressions. It enables the implemented nrfx drivers and API 4.1 (including RTC on
nRF52/53). When the support root is auto-discovered it also compiles the corresponding nrfx C driver
sources and GPPI dependencies where applicable. H20 uses its native HAL paths rather than
the generic CLOCK/MRAMC C drivers, whose single-controller aliases do not match that SoC.
Explicit include-only configurations compile the C++ instantiations but must validate/link
their driver sources separately.

The object target uses test-only glue from `tests/nordic/`; applications need their own real nrfx
integration. These checks do not link a complete firmware image, start clocks, execute peripheral
operations, or validate electrical behavior. The host suite additionally exercises deterministic
serial/acquisition/system driver fixtures and public frontends: cancellation/ownership, DMA
streaming, differential conversion, capture/PWM, RTC/GRTC calendar wrap, CAN, memory DMA,
entropy failures and USB RX staging. Optional crypto-provider variants require their vendor
headers/libraries and security-domain integration; the default matrix does not certify those ports.

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

## STM32 Family Matrix

CI instantiates the drivers for all 21 STM32 families accepted by CMake. The
default local command retains a quick, representative subset:

- `STM32C0` / `STM32C011xx` on Cortex-M0+
- `STM32F4` / `STM32F407xx` on Cortex-M4
- `STM32G4` / `STM32G431xx` on Cortex-M4

Run the local matrix after populating `vendor-support`:

```bash
bash ./scripts/validate_stm32_vendor_matrix.sh
```

Use `full` to attempt every family accepted by the STM32 CMake configuration,
or pass a specific family such as `STM32F7`. The full sweep reports all failing
families rather than stopping at the first one:

```bash
bash ./scripts/validate_stm32_vendor_matrix.sh full
```

The runner builds `microhal_compile_time_smoke` and its driver-instantiation
dependencies, including ADC, HAL serial extensions, and native LL serial.
`microhal_stm32_ll_only_instantiation` selects only the LL backend for serial and
ADC without `USE_HAL_DRIVER` to catch accidental HAL dependencies. Generated HAL validation
configurations enable the exercised modules and callback registration; SDK
include directories are system includes so microHAL's warnings remain errors
without promoting third-party SDK warnings.

These checks deliberately do not link Cube driver objects or run hardware.
Each family supplies its Cortex CPU flag and sets
`CMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY`, so the bare-metal compiler is
never asked to link a host-style probe executable.

Each entry covers one concrete device, not every part or peripheral variant in
that family. The full matrix includes the legacy STM32F1 register layout and the
newer GPDMA families. Passing header instantiation does not certify board-level
electrical behavior, interrupt timing, or DMA/cache coherency.

The host suite separately exercises HAL callback ownership and native acquisition/
transfer state machines, with legacy/modern ADC, I2C, and SPI register fixtures.
Those fixtures cover completion, cancellation, error recovery, and streaming
notifications, but do not replace physical-board validation.
