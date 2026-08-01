# microHAL Zephyr Module

microHAL can now be consumed as a Zephyr module in the same style as micrOSAL.
The module entry points are:

- `zephyr/module.yml`
- `zephyr/Kconfig`
- `zephyr/CMakeLists.txt`
- `zephyr/module_anchor.cpp`

Applications opt in by setting `CONFIG_MICRO_HAL=y` in `prj.conf` and adding
microHAL to `EXTRA_ZEPHYR_MODULES` when building outside a west workspace.

## Contractual Board Binding Example

`examples/zephyr_board_bindings.hpp` is the contractual example header for
board-owned Zephyr bindings. It demonstrates the intended boundary:

- `binding::spec` for GPIO resources resolved from devicetree aliases such as `led0`
- `binding::node_id` for device-backed peripherals resolved from chosen nodes such as `zephyr_console`

The header is consumed by the Zephyr Twister test application under
`tests/zephyr/` so drift between the example binding pattern and the backend is
caught during Zephyr builds.

## Twister And Renode

The Zephyr test application lives under `tests/zephyr/` and is meant to be run
through Twister. The current Renode-targeted suites are:

- `microhal.ztest.renode_stm32f4_disco`
- `microhal.ztest.renode_frdm_k64f`

Run them with:

```bash
source /path/to/zephyr-env.sh

west twister -T tests/zephyr -p stm32f4_disco --runner renode \
  --extra-args="EXTRA_ZEPHYR_MODULES=/path/to/microHAL"

west twister -T tests/zephyr -p frdm_k64f --runner renode \
  --extra-args="EXTRA_ZEPHYR_MODULES=/path/to/microHAL"
```

## Runtime Coverage

The current Renode suites intentionally validate the binding seam on peripherals
that are present and dependable on both reference boards without extra external
devices.

| Platform | Runner | Peripherals exercised | What the test does |
| --- | --- | --- | --- |
| `stm32f4_disco` | Renode via Twister | GPIO, UART | Binds `led0` through `gpio_dt_spec`, toggles it through the microHAL GPIO facade, then binds `zephyr_console` and performs a blocking UART transmit through the Zephyr backend. |
| `frdm_k64f` | Renode via Twister | GPIO, UART | Runs the same `led0` GPIO toggle/readback path and `zephyr_console` blocking UART transmit path against a Kinetis target. |

Peripherals such as I2C, SPI, CAN, flash, DMA, PWM,
watchdog, and I2S are now binding-aware in the Zephyr backend,
but they are not yet runtime-verified in the Renode suites because those flows
need board-specific external-device models or more elaborate fixture setup.
