set(MICROHAL_ENABLE_TESTS ON CACHE BOOL "Enable microHAL compile-time smoke tests")
set(MICROHAL_ENABLE_EXAMPLES ON CACHE BOOL "Enable microHAL examples")
set(MICROHAL_ENABLE_STRICT ON CACHE BOOL "Enable strict warnings and zero-overhead guardrails")
set(MICROHAL_WERROR ON CACHE BOOL
	"Treat warnings as errors for microHAL's own tests and examples (never for consumers of the INTERFACE target)")
set(MICROHAL_SUPPRESS_MISSING_MCU_WARNINGS ON CACHE BOOL
	"Suppress backend fallback #warning diagnostics when host builds intentionally omit a concrete MCU family")
set(MICROHAL_BACKENDS "" CACHE STRING
	"Semicolon-separated list of backends to compile in (STM32_HAL, STM32_LL, KINETIS_ESPRESSO, ZEPHYR, NUTTX, NORDIC_NRFX, MOCK). Empty means all of them.")
set(MICROHAL_VENDOR_VALIDATION OFF CACHE BOOL
	"Build against concrete vendor headers instead of the generic fallback paths")
set(MICROHAL_VENDOR_VALIDATION_PROVIDER "STM32" CACHE STRING
	"Concrete backend family used for vendor-validation builds")
set(MICROHAL_VENDOR_VALIDATION_STM32_FAMILY "STM32F4" CACHE STRING
	"Concrete STM32 family macro used for vendor-validation builds")
set(MICROHAL_VENDOR_VALIDATION_STM32_DEVICE "" CACHE STRING
	"Concrete STM32 part-number macro used for vendor-validation builds, e.g. STM32F407xx")
set(MICROHAL_VENDOR_VALIDATION_STM32_SUPPORT_ROOT "" CACHE PATH
	"Path to local vendor-support clones used to derive STM32 HAL/CMSIS include directories automatically, preferring ST cmsis-core and falling back to ARM CMSIS_5")
set(MICROHAL_VENDOR_VALIDATION_STM32_INCLUDE_DIRS "" CACHE STRING
	"Semicolon-separated STM32 SDK include directories for vendor-validation builds")
set(MICROHAL_VENDOR_VALIDATION_ZEPHYR_INCLUDE_DIRS "" CACHE STRING
	"Semicolon-separated Zephyr include directories, including generated headers, for vendor-validation builds")
set(MICROHAL_VENDOR_VALIDATION_ZEPHYR_DEFINITIONS
	"CONFIG_ZEPHYR_KERNEL;CONFIG_GPIO;CONFIG_I2C;CONFIG_SPI;CONFIG_FLASH;CONFIG_UART;CONFIG_COUNTER;CONFIG_ADC;CONFIG_DAC;CONFIG_CAN;CONFIG_USB_DEVICE_STACK;CONFIG_DMA;CONFIG_PWM;CONFIG_WATCHDOG;CONFIG_I2S;CONFIG_CLOCK_CONTROL;CONFIG_CRC;CONFIG_ENTROPY_GENERATOR"
	CACHE STRING
	"Semicolon-separated CONFIG_* compile definitions used for Zephyr vendor-validation builds")
set(MICROHAL_VENDOR_VALIDATION_NORDIC_INCLUDE_DIRS "" CACHE STRING
	"Semicolon-separated Nordic nrfx include directories for vendor-validation builds")
set(MICROHAL_VENDOR_VALIDATION_NORDIC_SUPPORT_ROOT "" CACHE PATH
	"Path to a Nordic support tree (for example vendor-support/nrf-bm/v2.0.1 or vendor-support) used to derive nrfx include directories automatically")
set(MICROHAL_VENDOR_VALIDATION_NORDIC_DEFINITIONS "NRF52840_XXAA"
	CACHE STRING
	"Semicolon-separated compile definitions used for Nordic nrfx vendor-validation builds")
set(MICROHAL_VENDOR_VALIDATION_NUTTX_INCLUDE_DIRS "" CACHE STRING
	"Semicolon-separated NuttX include directories, including generated config and board headers")
set_property(CACHE MICROHAL_VENDOR_VALIDATION_PROVIDER PROPERTY STRINGS STM32 ZEPHYR NORDIC NUTTX)
set_property(CACHE MICROHAL_VENDOR_VALIDATION_STM32_FAMILY PROPERTY STRINGS
			 STM32C0 STM32F0 STM32F1 STM32F2 STM32F3 STM32F4 STM32F7 STM32G0 STM32G4
			 STM32H5 STM32H7 STM32H7RS STM32L0 STM32L1 STM32L4 STM32L5 STM32N6
			 STM32U0 STM32U3 STM32U5 STM32WB)
