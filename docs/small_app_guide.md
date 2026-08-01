# microHAL Small App Guide

This guide shows a minimal path from empty project to a working microHAL app.

## Who This Is For

- You want a small, readable starting point.
- You want to understand backend selection quickly.
- You want one workflow for host tests and one for STM32 firmware.

## 1) Minimal Project Layout

```text
my-app/
  CMakeLists.txt
  src/
    main.cpp
```

## 2) CMake Setup

`CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.24)
project(my_microhal_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(microhal REQUIRED)

add_executable(my_app src/main.cpp)
target_link_libraries(my_app PRIVATE microhal::microhal)

# Pick one backend for production firmware builds.
# set(MICROHAL_BACKENDS "STM32_HAL" CACHE STRING "" FORCE)
```

## 3) Small App Example (UART + GPIO)

`src/main.cpp`:

```cpp
#include <array>
#include <cstdint>

#include "microhal/gpio/gpio.hpp"
#include "microhal/uart/uart.hpp"

using backend = microhal::stm32_hal_tag;

using led_pin = microhal::gpio::pin_config<0U, 5U, microhal::gpio_mode::output>;
using console = microhal::uart::port_config<2U, 115200U>;

int main()
{
  if (!microhal::is_ok(microhal::gpio::pin<backend, led_pin>::configure()))
  {
    return 1;
  }

  if (!microhal::is_ok(microhal::uart::port<backend, console>::initialize()))
  {
    return 2;
  }

  const std::array<std::uint8_t, 5U> hello{'h', 'e', 'l', 'l', 'o'};
  (void)microhal::uart::port<backend, console>::write_blocking(hello);

  microhal::gpio::pin<backend, led_pin>::set();
  return 0;
}
```

## 4) Build It

```bash
cmake -S . -B build
cmake --build build
```

## 5) Switch Backend

Backends are selected at compile time.

Change one line in your app:

```cpp
using backend = microhal::zephyr_tag;
```

Or constrain compiled backends in CMake:

```cmake
set(MICROHAL_BACKENDS "ZEPHYR" CACHE STRING "" FORCE)
```

## 6) Host-Only Driver Testing Pattern

For fast host tests, use the mock backend:

```cpp
using backend = microhal::mock_tag;
using console = microhal::uart::port_config<1U, 115200U>;

microhal::backends::mock::recorder::reset();
auto s = microhal::uart::port<backend, console>::initialize();
if (!microhal::is_ok(s))
{
  return 1;
}
```

This lets you test call sequencing and failure handling without hardware.

## 7) Practical Rules That Prevent Most Integration Bugs

1. Keep one owner per peripheral instance.
2. Keep board routing (clock, mux, DMA/IRQ wiring) in board-specific code.
3. Use capability predicates for feature-dependent code paths.
4. Keep app-level timeout policy in config types (`timeout_ms`) instead of backend code.

## 8) Where To Go Next

- For full architecture model: `docs/architecture.md`
- For backend support map: `docs/capability_matrix.md`
- For vendor setup requirements: `docs/backend_sdk_requirements.md`
- For concrete reference usage: `examples/`
