# Adding A New Vendor Backend

A backend is a header that declares a tag type and specialises `microhal::backend_traits` for it.
Nothing else in the library needs to change. This guide describes the contract that specialisation
must satisfy.

The smallest complete worked example is
[include/microhal/backends/mock/traits.hpp](../include/microhal/backends/mock/traits.hpp), which
implements eight peripherals and is short enough to read end to end. The largest is
[include/microhal/backends/stm32/hal/traits.hpp](../include/microhal/backends/stm32/hal/traits.hpp).

## 1. Declare a tag

The tag is an empty type used only for dispatch. It never has storage or members.

```cpp
namespace microhal
{
struct my_vendor_tag final
{
};
} // namespace microhal
```

The tag does not have to live in this repository. `tests/zero_overhead_probe.cpp` declares its own
tag and traits specialisation locally, which is exactly how an out-of-tree backend works.

If the backend is being contributed upstream, also add: a `vendor_kind` and `backend_kind`
enumerator in [core/traits.hpp](../include/microhal/core/traits.hpp); a `MICROHAL_ENABLE_<NAME>`
guard in [core/backend_config.hpp](../include/microhal/core/backend_config.hpp); a registry entry in
[core/capabilities.hpp](../include/microhal/core/capabilities.hpp); a `backend_tag_from_kind`
specialisation in [core/backend_selector.hpp](../include/microhal/core/backend_selector.hpp); and
the backend name to `_microhal_all_backends` in `CMakeLists.txt`.

## 2. Specialise `backend_traits`

### Required identity members

| Member | Type | Purpose |
| --- | --- | --- |
| `vendor` | `vendor_kind` | Which silicon vendor this targets. |
| `kind` | `backend_kind` | Which SDK or abstraction layer is being wrapped. |
| `name` | `std::string_view` | Unique, human-readable name shown in `backend_registry`. |

`tests/backend_selection_test.cpp` asserts that every registered name is non-empty and unique.

### Required GPIO geometry and validation hooks

| Member | Purpose |
| --- | --- |
| `gpio_port_count` | Number of GPIO ports. |
| `gpio_pins_per_port` | Pins per port. |
| `gpio_alternate_function_count` | Number of selectable alternate functions. |
| `is_valid_gpio_pin<PinConfig>()` | `static consteval bool`. Rejects port/pin combinations the part does not have. |
| `is_valid_alternate_function<PinConfig>()` | `static consteval bool`. Rejects AF numbers the pin cannot select. |

The last two are *not* capability fields. They exist because pin maps are irregular in ways a flat
count cannot express, and they are what turns a wrong pin choice into a compile error.

### Required capability accessors

Provide a `static consteval` accessor for every peripheral, returning the matching capability struct
from `core/traits.hpp`. The function shares its name with the type it returns, so the return type
needs an elaborated type specifier:

```cpp
static consteval struct uart_capability uart_capability() noexcept
{
  return {.supported = true,
          .blocking = true,
          .non_blocking = true,
          .isr = true,
          .dma = false,
          .instance_count = 4U,
          .max_frequency_hz = 10'000'000U,
          .fifo = false,
          .lpuart = false,
          .fifo_depth = 0U};
}
```

The twenty accessors are `gpio_capability()`, `i2c_capability()`, `spi_capability()`,
`qspi_capability()`, `uart_capability()`, `rtc_capability()`, `flash_capability()`,
`timer_capability()`, `adc_capability()`, `dac_capability()`, `can_capability()`,
`usb_capability()`, `ethernet_capability()`, `sd_capability()`, `dma_capability()`,
`pwm_capability()`, `watchdog_capability()`, `i2s_capability()`, `rcc_capability()`, and
`crc_capability()`.

**All twenty must exist.** For peripherals the backend does not implement, return a
value-initialised struct. `supported` then defaults to `false`, and the facade for that peripheral
fails to compile with a capability diagnostic rather than a confusing missing-member error:

```cpp
static consteval struct ethernet_capability ethernet_capability() noexcept
{
  return {};
}
```

The exact fields of each capability struct are listed in
[capability_matrix.md](capability_matrix.md).

### Required API type aliases

For each peripheral the backend *does* implement, add a nested alias naming the struct that performs
the work:

```cpp
using gpio_api = backends::my_vendor::gpio_api;
using uart_api = backends::my_vendor::uart_api;
```

Only implemented peripherals need an alias. The `*_api_t<Backend>` alias templates in
`core/backend_selector.hpp` are instantiated lazily, so a missing alias only becomes an error when
something actually instantiates that peripheral's facade for this backend.

## 3. Implement the API structs

Each API struct is a stateless collection of `static` functions under
`microhal::backends::<vendor>`. They may return `void`, `bool`, or `microhal::status`; the facades
normalise all three through `detail::as_status`, so use whatever is natural for the underlying SDK.

```cpp
struct uart_api final
{
  template <typename Config> static status initialize() noexcept;
  template <typename Config> static status write_blocking(std::span<const std::uint8_t> data) noexcept;
};
```

Configuration arrives as a template parameter, not a runtime argument. Read whatever members the
config exposes (`Config::instance_index`, `Config::baud_rate`, `Config::timeout_ms`, and so on).
Use a `requires { Config::member; }` check when reading an optional member, as
`stm32::hal::timeout_for<Config>()` does for `timeout_ms`.

Because a backend header may be compiled alongside another vendor's SDK, **do not gate code on
generic macro names alone.** `#if defined(GPIOA)` is not a reliable test for "the NXP SDK is
present" — ST's CMSIS defines `GPIOA` as well, and that exact collision was a real bug in this
repository. Gate on a part-number macro and derive a single backend-specific
`MICROHAL_<VENDOR>_SDK_PRESENT` flag from it.

## 4. Add validation rules

Encode vendor-specific constraints as `consteval` predicates so violations are compile errors:
alternate-function maps, flash program and erase alignment, timer capture channel limits, minimum
and maximum watchdog timeouts, and valid clock ranges.

## 5. Add tests

- Extend `tests/multi_backend_validation.cpp` and `tests/extended_peripherals_test.cpp` with static
  assertions covering the new backend's capabilities.
- Add negative cases to `tests/edge_cases_test.cpp` proving invalid configurations are rejected.
- If the backend can run on the host, add runtime tests in the style of
  `tests/mock_runtime_test.cpp`.
- Confirm the build still works with `-DMICROHAL_BACKENDS=<NEW_BACKEND>` on its own; the CI matrix
  builds `microhal_backend_selection` for each backend in isolation.
- Run `./scripts/check_zero_overhead.sh` if the backend targets a compiler the check supports.

## Validation Checklist

- No exceptions, RTTI, dynamic allocation, or virtual dispatch.
- All twenty capability accessors present, even for unimplemented peripherals.
- Backend name unique within `backend_registry`.
- SDK detection gated on part-number macros, not generic peripheral macros.
- Optional features compile out completely when not requested.
- DMA and ISR paths gated on the corresponding capability flags.
- Any RTOS coupling stays outside the core HAL types; follow the micrOSAL `MigrateToCpp20`
  conventions rather than adding allocator-backed service objects.
