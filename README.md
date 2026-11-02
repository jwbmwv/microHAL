# microHAL

A header-only, zero-overhead hardware abstraction layer for microcontrollers, requiring C++20 or newer.

microHAL lets you write peripheral code once and compile it against STM32Cube HAL, STM32Cube LL,
NXP MCUXpresso, Zephyr, or NuttX — without virtual dispatch, dynamic allocation, exceptions, RTTI,
or any runtime backend indirection. Backend selection and capability checking happen entirely in the
type system, so the generated code is the same as calling the vendor SDK directly.

```cpp
#include "microhal/uart/uart.hpp"

using backend = microhal::stm32_hal_tag;
using console = microhal::uart::port_config<2U, 115'200U>;

if (microhal::is_ok(microhal::uart::port<backend, console>::initialize()))
{
  const std::array<std::uint8_t, 5U> greeting{'h', 'e', 'l', 'l', 'o'};
  (void)microhal::uart::port<backend, console>::write_blocking(greeting);
}
```

Retarget the same code to Zephyr by changing one line: `using backend = microhal::zephyr_tag;`.

## Why

Vendor HALs are C, untyped, and mutually incompatible. The usual C++ wrapper introduces virtual
functions and a runtime driver registry, which costs flash, RAM, and determinism on a Cortex-M0+.

microHAL takes the opposite approach: **configuration is a type**, backends are selected by tag, and
every capability check is `consteval`. Asking a backend for a peripheral it does not have is a
compile error with a readable message, not a runtime `nullptr`.

## Design at a glance

| Property | How |
| --- | --- |
| No runtime dispatch | Backend chosen by tag type; `uart_api_t<Backend>` resolves at compile time |
| No allocation | Facades are stateless; backends use `static inline` handle storage |
| No exceptions | Every fallible call returns `microhal::status`; value-returning calls return `microhal::result<T>` |
| Unsupported = build error | `consteval` capability accessors + concepts reject bad combinations during substitution |
| Pay for what you use | Per-peripheral alias templates instantiate lazily; unused peripherals emit no code |

See [docs/architecture.md](docs/architecture.md) for the full model, including diagrams of the
layering, the tag-dispatch path, and status propagation.

## Supported backends

| Tag | Target |
| --- | --- |
| `microhal::stm32_hal_tag` | STM32Cube HAL |
| `microhal::stm32_ll_tag` | STM32Cube LL |
| `microhal::kinetis_espresso_tag` | NXP Kinetis / i.MX RT MCUXpresso |
| `microhal::nordic_nrf_tag` | Nordic bare-metal nrfx |
| `microhal::zephyr_tag` | Zephyr device driver model |
| `microhal::nuttx_tag` | NuttX driver model |
| `microhal::mock_tag` | Host recording backend for unit tests |

Twenty peripherals are modelled: GPIO, I2C, SPI, QSPI, UART, RTC, Flash, Timer, ADC, DAC, CAN, USB,
Ethernet, SD/MMC, DMA, PWM, Watchdog, I2S, RCC, and CRC. Coverage varies by backend — see
[docs/capability_matrix.md](docs/capability_matrix.md).

## Requirements

- A compiler supporting at least C++20, including concepts and `consteval` (GCC 11+, Clang 14+, `arm-none-eabi-g++` 11+)
- CMake 3.24+
- The vendor SDK for whichever backend you target (not vendored here)

Backend-specific SDK/environment requirements are documented in
[docs/backend_sdk_requirements.md](docs/backend_sdk_requirements.md).

## Using it

microHAL is header-only and exports an INTERFACE target.

C++20 is the minimum, not a fixed language mode. The exported target requests `cxx_std_20`;
both `find_package` and `add_subdirectory` preserve a consuming target's newer standard.
For microHAL's own builds, CMake defaults to C++20 unless `CMAKE_CXX_STANDARD` is already set:

```sh
cmake -S . -B build/cxx23 -DCMAKE_CXX_STANDARD=23
```

Host CI tests C++20 and C++23 with GCC and Clang, plus Clang's experimental C++26 mode.
Newer modes still depend on the selected compiler, standard library, and vendor SDK.
Newer features are used only when they materially improve microHAL without changing the
public API or runtime behavior. When `__cpp_static_assert >= 202306L`, I2C/SPI clock validation
reports the requested frequency and backend limit; older compilers retain fixed-string diagnostics.

```cmake
find_package(microhal REQUIRED)
target_link_libraries(my_firmware PRIVATE microhal::microhal)
```

Or vendor it directly:

```cmake
add_subdirectory(third_party/microHAL)
target_link_libraries(my_firmware PRIVATE microhal::microhal)
```

### Selecting backends

By default every backend is compiled in, which is convenient for host builds and capability queries
across backends. Production firmware normally wants exactly one:

```cmake
set(MICROHAL_BACKENDS "STM32_HAL" CACHE STRING "" FORCE)
```

This shortens compile times and, more importantly, keeps a foreign vendor SDK out of your
translation unit. Vendor-validation builds scope this automatically.

The equivalent preprocessor form:

```cpp
#define MICROHAL_BACKENDS_EXPLICIT 1
#define MICROHAL_ENABLE_STM32_HAL 1
```

## Error handling

There are no exceptions, so results are values:

```cpp
using adc = microhal::adc::converter<backend, sensor_cfg>;

const microhal::result<std::uint16_t> reading = adc::read_blocking(3U);
if (!reading)
{
  log(microhal::to_string(reading.error())); // "timeout", "io_error", ...
  return;
}
use(reading.value());
```

Facade entry points are `[[nodiscard]]`, so a discarded failure is a warning during normal builds
and an error under the vendor-validation build, which compiles with `-Werror`.

## Testing your own drivers

The `mock_tag` backend records every call and can inject failures, so driver logic is testable on a
build machine with no hardware and no emulator:

```cpp
using backend = microhal::mock_tag;
using recorder = microhal::backends::mock::recorder;

recorder::reset();
recorder::fail_after_calls(1U); // first call succeeds, the rest fail

assert(microhal::uart::port<backend, console>::initialize() == microhal::status::ok);
assert(microhal::uart::port<backend, console>::write_blocking(tx) == microhal::status::error);
assert(recorder::log()[1].length == tx.size());
```

`tests/mock_runtime_test.cpp` uses this to verify call sequencing, forwarded arguments, and error
propagation across the facades.

## Building and testing

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

To cross-compile against real ST headers for STM32C0, STM32F4, and STM32G4:

```bash
./scripts/clone_vendor_support.sh          # fetches ST HAL/CMSIS mirrors into vendor-support/
./scripts/validate_stm32_vendor_matrix.sh  # builds each family with arm-none-eabi-g++
```

microHAL's own tests and examples build with `-Werror` (`MICROHAL_WERROR`, on by default). The
`INTERFACE` target you link against never forces that on your project.

### Proving the abstraction is free

The zero-overhead claim is checked, not asserted. `scripts/check_zero_overhead.sh` compiles
`tests/zero_overhead_probe.cpp`, which performs the same UART write twice — once through the
microHAL facade and once by calling the backend struct directly — then disassembles both and
requires the instruction streams to be identical.

```bash
./scripts/check_zero_overhead.sh
./scripts/check_zero_overhead.sh arm-none-eabi-g++ -mcpu=cortex-m0plus -mthumb -Os
./scripts/check_zero_overhead.sh arm-none-eabi-g++ -mcpu=cortex-m4 -mthumb
./scripts/check_zero_overhead.sh arm-none-eabi-g++ -mcpu=cortex-m7 -mthumb -O3
```

All four configurations currently pass. CI runs them on every push.

### What CI checks

| Job | What it proves |
| --- | --- |
| Host build and test | GCC and Clang build cleanly and all `ctest` targets pass |
| Single-backend build | Each backend compiles on its own, with nothing else included first |
| Freestanding compile | The header surface needs no hosted C++ runtime |
| Zero-overhead | The facades cost nothing, on four target configurations |
| clang-format / clang-tidy | Style and static analysis |
| STM32 vendor validation | Real ST HAL/CMSIS headers for STM32C0, F4, and G4 |

## Repository layout

| Path | Contents |
| --- | --- |
| `include/microhal/core/` | Tags, enums, capability structs, concepts, `status`/`result`, backend selection |
| `include/microhal/backends/` | One header per backend; the only code that names vendor symbols |
| `include/microhal/<peripheral>/` | Public facades and configuration types |
| `examples/` | Contractual usage examples, compiled by the default build |
| `tests/` | Compile-time and runtime test suites |
| `docs/` | Architecture, capability matrix, vendor migration and validation, Zephyr module |
| `scripts/` | Vendor mirror fetch, cross-compile validation, zero-overhead check |
| `.github/workflows/` | CI: host build, single-backend matrix, freestanding, overhead, lint |

## Documentation

- [Architecture](docs/architecture.md) — layering, tag dispatch, error model, concurrency contract
- [Capability matrix](docs/capability_matrix.md) — what each backend actually implements
- [Vendor migration](docs/vendor_migration.md) — moving existing vendor-HAL code onto microHAL
- [Vendor validation](docs/vendor_validation.md) — building against real SDK headers
- [Backend SDK requirements](docs/backend_sdk_requirements.md) — per-backend SDK, include, and binding contract
- [Zephyr module](docs/zephyr_module.md) — using microHAL inside a Zephyr workspace
- [Small app guide](docs/small_app_guide.md) — practical step-by-step API usage to build a tiny app
- [Diagrams](docs/diagrams/README.md) — PlantUML sources and generated architecture/integration visuals
- [API coverage scorecard](docs/api_coverage_scorecard.md) — per-peripheral facade richness, capability depth, and backend breadth snapshot

## Status and limitations

Worth knowing before you adopt it:

- **Backend coverage is uneven.** STM32Cube HAL is still the most complete backend. Kinetis now uses
  SDK-discovery-based compilation across broader Kinetis/i.MX RT families, but runtime coverage still
  depends on which MCUXpresso peripheral modules are available for your selected part. Nordic nrfx
  coverage is also header-driven: UART/RTC/flash/timer/ADC/DAC/DMA/PWM/watchdog support appears
  when the corresponding `nrfx_*` drivers are present for the selected SoC.
- **Backend API signatures are not yet uniform.** Because facades instantiate lazily, a signature
  mismatch in a backend only surfaces when that specific peripheral is instantiated for that
  backend. The multi-backend test does not cover every peripheral on every backend.
- **No concurrency support.** Backend handle storage is `static inline` and unsynchronised; see the
  concurrency contract in the architecture document.
- **The C API is example-level.** `examples/*_c_shim.*` show the pattern — including a
  `microhal_status_t` enum kept in lockstep with `microhal::status` by `static_assert` — but they
  are not an installed, supported C library.
- **`renesas_ra_fsp` and `ti_simplelink_driverlib`** appear in `backend_kind` but have no
  implementation; naming one is a compile error, not a silent no-op.

## License

Apache-2.0. See [Licence.md](Licence.md), [LICENSE](LICENSE), and SPDX headers in the source files.
