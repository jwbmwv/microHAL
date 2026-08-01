cmake_minimum_required(VERSION 3.24)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" MICROHAL_VERSION)
string(STRIP "${MICROHAL_VERSION}" MICROHAL_VERSION)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(microhal VERSION ${MICROHAL_VERSION} LANGUAGES C CXX)

include(cmake/microhal-options.cmake)

add_library(microhal INTERFACE)
add_library(microhal::microhal ALIAS microhal)

target_compile_features(microhal INTERFACE cxx_std_20)
target_include_directories(microhal INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

if(MICROHAL_SUPPRESS_MISSING_MCU_WARNINGS)
  target_compile_definitions(microhal INTERFACE MICROHAL_SUPPRESS_MISSING_MCU_WARNINGS=1)
endif()

# Backend selection.
#
# A vendor-validation build compiles against one vendor's real SDK headers. Compiling a foreign
# vendor's backend in that same translation unit is not just wasteful, it is actively unsafe:
# vendor SDKs claim generic macro names (STM32's CMSIS defines GPIOA, for example), so a foreign
# backend's feature-detection can be triggered by the wrong SDK. Vendor-validation builds therefore
# default to only the backends that match the provider under test.
set(_microhal_all_backends STM32_HAL STM32_LL KINETIS_ESPRESSO ZEPHYR NUTTX NORDIC_NRFX MOCK)
set(_microhal_selected_backends "${MICROHAL_BACKENDS}")

if(_microhal_selected_backends STREQUAL "" AND MICROHAL_VENDOR_VALIDATION)
  if(MICROHAL_VENDOR_VALIDATION_PROVIDER STREQUAL "STM32")
    set(_microhal_selected_backends STM32_HAL STM32_LL MOCK)
  elseif(MICROHAL_VENDOR_VALIDATION_PROVIDER STREQUAL "ZEPHYR")
    set(_microhal_selected_backends ZEPHYR MOCK)
  elseif(MICROHAL_VENDOR_VALIDATION_PROVIDER STREQUAL "NORDIC")
    set(_microhal_selected_backends NORDIC_NRFX MOCK)
  endif()
endif()

if(NOT _microhal_selected_backends STREQUAL "")
  foreach(_microhal_backend IN LISTS _microhal_selected_backends)
    list(FIND _microhal_all_backends "${_microhal_backend}" _microhal_backend_index)
    if(_microhal_backend_index EQUAL -1)
      message(FATAL_ERROR
        "MICROHAL_BACKENDS entry '${_microhal_backend}' is not one of: ${_microhal_all_backends}")
    endif()
  endforeach()

  target_compile_definitions(microhal INTERFACE MICROHAL_BACKENDS_EXPLICIT=1)
  foreach(_microhal_backend IN LISTS _microhal_selected_backends)
    target_compile_definitions(microhal INTERFACE MICROHAL_ENABLE_${_microhal_backend}=1)
  endforeach()
endif()

# Number of backends visible to the compiled tests, so they can assert the registry size.
if(_microhal_selected_backends STREQUAL "")
  list(LENGTH _microhal_all_backends _microhal_backend_count)
else()
  list(LENGTH _microhal_selected_backends _microhal_backend_count)
endif()
target_compile_definitions(microhal INTERFACE
  MICROHAL_EXPECTED_BACKEND_COUNT=${_microhal_backend_count})

if(MICROHAL_VENDOR_VALIDATION)
  if(MICROHAL_VENDOR_VALIDATION_PROVIDER STREQUAL "STM32")
    set(_microhal_supported_stm32_families
      STM32C0
      STM32F0
      STM32F1
      STM32F2
      STM32F3
      STM32F4
      STM32F7
      STM32G0
      STM32G4
      STM32H5
      STM32H7
      STM32H7RS
      STM32L0
      STM32L1
      STM32L4
      STM32L5
      STM32N6
      STM32U0
      STM32U3
      STM32U5
      STM32WB)
    list(FIND _microhal_supported_stm32_families "${MICROHAL_VENDOR_VALIDATION_STM32_FAMILY}" _microhal_family_index)
    if(_microhal_family_index EQUAL -1)
      message(FATAL_ERROR
        "MICROHAL_VENDOR_VALIDATION_STM32_FAMILY must be one of: ${_microhal_supported_stm32_families}")
    endif()

    set(_microhal_stm32_family_suffix "")
    if(MICROHAL_VENDOR_VALIDATION_STM32_FAMILY STREQUAL "STM32C0")
      set(_microhal_stm32_family_suffix "c0")
    elseif(MICROHAL_VENDOR_VALIDATION_STM32_FAMILY STREQUAL "STM32F0")
      set(_microhal_stm32_family_suffix "f0")
    elseif(MICROHAL_VENDOR_VALIDATION_STM32_FAMILY STREQUAL "STM32F1")
      set(_microhal_stm32_family_suffix "f1")
    elseif(MICROHAL_VENDOR_VALIDATION_STM32_FAMILY STREQUAL "STM32F2")
      set(_microhal_stm32_family_suffix "f2")
    elseif(MICROHAL_VENDOR_VALIDATION_STM32_FAMILY STREQUAL "STM32F3")
      set(_microhal_stm32_family_suffix "f3")
    elseif(MICROHAL_VENDOR_VALIDATION_STM32_FAMILY STREQUAL "STM32F4")
      set(_microhal_stm32_family_suffix "f4")
    elseif(MICROHAL_VENDOR_VALIDATION_STM32_FAMILY STREQUAL "STM32F7")
      set(_microhal_stm32_family_suffix "f7")
    elseif(MICROHAL_VENDOR_VALIDATION_STM32_FAMILY STREQUAL "STM32G0")
      set(_microhal_stm32_family_suffix "g0")
    elseif(MICROHAL_VENDOR_VALIDATION_STM32_FAMILY STREQUAL "STM32G4")
      set(_microhal_stm32_family_suffix "g4")
    elseif(MICROHAL_VENDOR_VALIDATION_STM32_FAMILY STREQUAL "STM32H5")
      set(_microhal_stm32_family_suffix "h5")
    elseif(MICROHAL_VENDOR_VALIDATION_STM32_FAMILY STREQUAL "STM32H7")
      set(_microhal_stm32_family_suffix "h7")
    elseif(MICROHAL_VENDOR_VALIDATION_STM32_FAMILY STREQUAL "STM32H7RS")
      set(_microhal_stm32_family_suffix "h7rs")
    elseif(MICROHAL_VENDOR_VALIDATION_STM32_FAMILY STREQUAL "STM32L0")
      set(_microhal_stm32_family_suffix "l0")
    elseif(MICROHAL_VENDOR_VALIDATION_STM32_FAMILY STREQUAL "STM32L1")
      set(_microhal_stm32_family_suffix "l1")
    elseif(MICROHAL_VENDOR_VALIDATION_STM32_FAMILY STREQUAL "STM32L4")
      set(_microhal_stm32_family_suffix "l4")
    elseif(MICROHAL_VENDOR_VALIDATION_STM32_FAMILY STREQUAL "STM32L5")
      set(_microhal_stm32_family_suffix "l5")
    elseif(MICROHAL_VENDOR_VALIDATION_STM32_FAMILY STREQUAL "STM32N6")
      set(_microhal_stm32_family_suffix "n6")
    elseif(MICROHAL_VENDOR_VALIDATION_STM32_FAMILY STREQUAL "STM32U0")
      set(_microhal_stm32_family_suffix "u0")
    elseif(MICROHAL_VENDOR_VALIDATION_STM32_FAMILY STREQUAL "STM32U3")
      set(_microhal_stm32_family_suffix "u3")
    elseif(MICROHAL_VENDOR_VALIDATION_STM32_FAMILY STREQUAL "STM32U5")
      set(_microhal_stm32_family_suffix "u5")
    elseif(MICROHAL_VENDOR_VALIDATION_STM32_FAMILY STREQUAL "STM32WB")
      set(_microhal_stm32_family_suffix "wb")
    endif()

    if(MICROHAL_VENDOR_VALIDATION_STM32_DEVICE STREQUAL "")
      message(FATAL_ERROR
        "MICROHAL_VENDOR_VALIDATION=ON with MICROHAL_VENDOR_VALIDATION_PROVIDER=STM32 requires MICROHAL_VENDOR_VALIDATION_STM32_DEVICE to name a concrete STM32 part macro such as STM32F407xx or STM32C011xx")
    endif()

    if(MICROHAL_VENDOR_VALIDATION_STM32_INCLUDE_DIRS STREQUAL "" AND NOT MICROHAL_VENDOR_VALIDATION_STM32_SUPPORT_ROOT STREQUAL "")
      set(_microhal_stm32_header_prefix "stm32${_microhal_stm32_family_suffix}xx")
      set(_microhal_stm32_hal_inc_dir
        "${MICROHAL_VENDOR_VALIDATION_STM32_SUPPORT_ROOT}/${_microhal_stm32_header_prefix}-hal-driver/Inc")
      set(_microhal_stm32_cmsis_inc_dir
        "${MICROHAL_VENDOR_VALIDATION_STM32_SUPPORT_ROOT}/cmsis-device-${_microhal_stm32_family_suffix}/Include")
      set(_microhal_stm32_cmsis_core_inc_dir "")
      # Prefer ST's packaged CMSIS core for STM32 vendor validation, but keep
      # the legacy upstream CMSIS_5 tree as a compatibility fallback.
      if(EXISTS "${MICROHAL_VENDOR_VALIDATION_STM32_SUPPORT_ROOT}/cmsis-core/CMSIS/Core/Include")
        set(_microhal_stm32_cmsis_core_inc_dir
          "${MICROHAL_VENDOR_VALIDATION_STM32_SUPPORT_ROOT}/cmsis-core/CMSIS/Core/Include")
      elseif(EXISTS "${MICROHAL_VENDOR_VALIDATION_STM32_SUPPORT_ROOT}/CMSIS_5/CMSIS/Core/Include")
        set(_microhal_stm32_cmsis_core_inc_dir
          "${MICROHAL_VENDOR_VALIDATION_STM32_SUPPORT_ROOT}/CMSIS_5/CMSIS/Core/Include")
      endif()
      set(_microhal_stm32_hal_conf_template
        "${_microhal_stm32_hal_inc_dir}/${_microhal_stm32_header_prefix}_hal_conf_template.h")
      set(_microhal_stm32_hal_conf_dir
        "${CMAKE_CURRENT_BINARY_DIR}/generated/stm32-hal-conf/${_microhal_stm32_header_prefix}")
      set(_microhal_stm32_hal_conf_header
        "${_microhal_stm32_hal_conf_dir}/${_microhal_stm32_header_prefix}_hal_conf.h")

      foreach(_microhal_required_path
          "${_microhal_stm32_hal_inc_dir}"
          "${_microhal_stm32_cmsis_inc_dir}"
          "${_microhal_stm32_cmsis_core_inc_dir}"
          "${_microhal_stm32_hal_conf_template}")
        if(NOT EXISTS "${_microhal_required_path}")
          message(FATAL_ERROR
            "MICROHAL STM32 vendor-validation auto-setup expected path not found: ${_microhal_required_path}")
        endif()
      endforeach()

      file(MAKE_DIRECTORY "${_microhal_stm32_hal_conf_dir}")
      configure_file("${_microhal_stm32_hal_conf_template}" "${_microhal_stm32_hal_conf_header}" COPYONLY)

      set(MICROHAL_VENDOR_VALIDATION_STM32_INCLUDE_DIRS
        "${_microhal_stm32_hal_conf_dir};${_microhal_stm32_hal_inc_dir};${_microhal_stm32_cmsis_inc_dir};${_microhal_stm32_cmsis_core_inc_dir}")
    endif()

    if(MICROHAL_VENDOR_VALIDATION_STM32_INCLUDE_DIRS STREQUAL "")
      message(FATAL_ERROR
        "MICROHAL_VENDOR_VALIDATION=ON with MICROHAL_VENDOR_VALIDATION_PROVIDER=STM32 requires either MICROHAL_VENDOR_VALIDATION_STM32_INCLUDE_DIRS or MICROHAL_VENDOR_VALIDATION_STM32_SUPPORT_ROOT to locate the STM32 SDK headers")
    endif()

    target_compile_definitions(microhal INTERFACE
      USE_HAL_DRIVER
      USE_FULL_LL_DRIVER
      "${MICROHAL_VENDOR_VALIDATION_STM32_FAMILY}"
      "${MICROHAL_VENDOR_VALIDATION_STM32_DEVICE}")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      target_compile_options(microhal INTERFACE -fpermissive)
    endif()
    foreach(_microhal_stm32_include_dir IN LISTS MICROHAL_VENDOR_VALIDATION_STM32_INCLUDE_DIRS)
      target_include_directories(microhal INTERFACE
        $<BUILD_INTERFACE:${_microhal_stm32_include_dir}>)
    endforeach()
  elseif(MICROHAL_VENDOR_VALIDATION_PROVIDER STREQUAL "ZEPHYR")
    if(MICROHAL_VENDOR_VALIDATION_ZEPHYR_INCLUDE_DIRS STREQUAL "")
      message(FATAL_ERROR
        "MICROHAL_VENDOR_VALIDATION=ON with MICROHAL_VENDOR_VALIDATION_PROVIDER=ZEPHYR requires MICROHAL_VENDOR_VALIDATION_ZEPHYR_INCLUDE_DIRS to point at the Zephyr source and generated headers")
    endif()

    if(NOT MICROHAL_VENDOR_VALIDATION_ZEPHYR_DEFINITIONS STREQUAL "")
      target_compile_definitions(microhal INTERFACE ${MICROHAL_VENDOR_VALIDATION_ZEPHYR_DEFINITIONS})
    endif()
    foreach(_microhal_zephyr_include_dir IN LISTS MICROHAL_VENDOR_VALIDATION_ZEPHYR_INCLUDE_DIRS)
      target_include_directories(microhal INTERFACE
        $<BUILD_INTERFACE:${_microhal_zephyr_include_dir}>)
    endforeach()
  elseif(MICROHAL_VENDOR_VALIDATION_PROVIDER STREQUAL "NORDIC")
    if(MICROHAL_VENDOR_VALIDATION_NORDIC_INCLUDE_DIRS STREQUAL "" AND NOT MICROHAL_VENDOR_VALIDATION_NORDIC_SUPPORT_ROOT STREQUAL "")
      set(_microhal_nordic_candidate_roots
        "${MICROHAL_VENDOR_VALIDATION_NORDIC_SUPPORT_ROOT}"
        "${MICROHAL_VENDOR_VALIDATION_NORDIC_SUPPORT_ROOT}/v2.0.0"
        "${MICROHAL_VENDOR_VALIDATION_NORDIC_SUPPORT_ROOT}/nrf-bm/v2.0.0")
      set(_microhal_nordic_resolved_root "")
      foreach(_microhal_nordic_candidate_root IN LISTS _microhal_nordic_candidate_roots)
        if(EXISTS "${_microhal_nordic_candidate_root}/modules/hal/nordic/nrfx/nrfx.h")
          set(_microhal_nordic_resolved_root "${_microhal_nordic_candidate_root}")
          break()
        endif()
      endforeach()

      if(NOT _microhal_nordic_resolved_root STREQUAL "")
        set(_microhal_nordic_nrfx_root "${_microhal_nordic_resolved_root}/modules/hal/nordic/nrfx")
        set(_microhal_nordic_cmsis_root "")
        if(EXISTS "${_microhal_nordic_resolved_root}/modules/hal/cmsis_6/CMSIS/Core/Include")
          set(_microhal_nordic_cmsis_root "${_microhal_nordic_resolved_root}/modules/hal/cmsis_6/CMSIS/Core/Include")
        elseif(EXISTS "${_microhal_nordic_resolved_root}/modules/hal/cmsis/CMSIS/Core/Include")
          set(_microhal_nordic_cmsis_root "${_microhal_nordic_resolved_root}/modules/hal/cmsis/CMSIS/Core/Include")
        endif()

        set(_microhal_nordic_template_header "nrfx_templates_config.h")
        if(MICROHAL_VENDOR_VALIDATION_NORDIC_DEFINITIONS MATCHES "(^|;)NRF52840_XXAA(;|$)")
          set(_microhal_nordic_template_header "nrfx_config_nrf52840.h")
        elseif(MICROHAL_VENDOR_VALIDATION_NORDIC_DEFINITIONS MATCHES "(^|;)NRF5340_XXAA_APPLICATION(;|$)")
          set(_microhal_nordic_template_header "nrfx_config_nrf5340_application.h")
        elseif(MICROHAL_VENDOR_VALIDATION_NORDIC_DEFINITIONS MATCHES "(^|;)NRF5340_XXAA_NETWORK(;|$)")
          set(_microhal_nordic_template_header "nrfx_config_nrf5340_network.h")
        elseif(MICROHAL_VENDOR_VALIDATION_NORDIC_DEFINITIONS MATCHES "(^|;)NRF54H20_XXAA(;|$)")
          set(_microhal_nordic_template_header "nrfx_config_nrf54h20_application.h")
        endif()

        foreach(_microhal_required_path
            "${_microhal_nordic_nrfx_root}/nrfx.h"
            "${_microhal_nordic_nrfx_root}/drivers/include/nrfx_uarte.h"
            "${_microhal_nordic_nrfx_root}/bsp/stable/mdk/nrf.h"
            "${_microhal_nordic_nrfx_root}/bsp/stable/templates/${_microhal_nordic_template_header}"
            "${_microhal_nordic_cmsis_root}/core_cm4.h")
          if(NOT EXISTS "${_microhal_required_path}")
            message(FATAL_ERROR
              "MICROHAL Nordic vendor-validation auto-setup expected path not found: ${_microhal_required_path}")
          endif()
        endforeach()

        set(_microhal_nordic_generated_config_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/nordic-nrfx")
        file(MAKE_DIRECTORY "${_microhal_nordic_generated_config_dir}")
        file(WRITE "${_microhal_nordic_generated_config_dir}/nrfx_config.h"
          "#pragma once\n#ifndef NRFX_CONFIG_H__\n#define NRFX_CONFIG_H__\n#endif\n#include <${_microhal_nordic_template_header}>\n")

        set(MICROHAL_VENDOR_VALIDATION_NORDIC_INCLUDE_DIRS
          "${_microhal_nordic_generated_config_dir};${_microhal_nordic_nrfx_root};${_microhal_nordic_nrfx_root}/drivers/include;${_microhal_nordic_nrfx_root}/hal;${_microhal_nordic_nrfx_root}/haly;${_microhal_nordic_nrfx_root}/helpers;${_microhal_nordic_nrfx_root}/templates;${_microhal_nordic_nrfx_root}/bsp/stable;${_microhal_nordic_nrfx_root}/bsp/stable/mdk;${_microhal_nordic_nrfx_root}/bsp/stable/templates;${_microhal_nordic_cmsis_root}")
      endif()
    endif()

    if(MICROHAL_VENDOR_VALIDATION_NORDIC_INCLUDE_DIRS STREQUAL "")
      message(FATAL_ERROR
        "MICROHAL_VENDOR_VALIDATION=ON with MICROHAL_VENDOR_VALIDATION_PROVIDER=NORDIC requires MICROHAL_VENDOR_VALIDATION_NORDIC_INCLUDE_DIRS or MICROHAL_VENDOR_VALIDATION_NORDIC_SUPPORT_ROOT")
    endif()

    if(NOT MICROHAL_VENDOR_VALIDATION_NORDIC_DEFINITIONS STREQUAL "")
      target_compile_definitions(microhal INTERFACE ${MICROHAL_VENDOR_VALIDATION_NORDIC_DEFINITIONS})
    endif()
    foreach(_microhal_nordic_include_dir IN LISTS MICROHAL_VENDOR_VALIDATION_NORDIC_INCLUDE_DIRS)
      target_include_directories(microhal INTERFACE
        $<BUILD_INTERFACE:${_microhal_nordic_include_dir}>)
    endforeach()
  else()
    message(FATAL_ERROR
      "MICROHAL_VENDOR_VALIDATION_PROVIDER must be one of: STM32, ZEPHYR, NORDIC")
  endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(microhal INTERFACE
    -fno-exceptions
    -fno-rtti
    -Wall
    -Wextra
    -Wpedantic)
endif()

set(_microhal_build_full_matrix ON)
if(MICROHAL_VENDOR_VALIDATION AND MICROHAL_VENDOR_VALIDATION_PROVIDER STREQUAL "NORDIC")
  set(_microhal_build_full_matrix OFF)
endif()

if(_microhal_build_full_matrix)
  if(MICROHAL_VENDOR_VALIDATION)
    add_library(microhal_compile_time_smoke STATIC tests/compile_time_smoke.cpp)
  else()
    add_executable(microhal_compile_time_smoke tests/compile_time_smoke.cpp)
  endif()
  target_link_libraries(microhal_compile_time_smoke PRIVATE microhal::microhal)

  add_executable(microhal_extended_peripherals tests/extended_peripherals_test.cpp)
  target_link_libraries(microhal_extended_peripherals PRIVATE microhal::microhal)

  add_executable(microhal_edge_cases tests/edge_cases_test.cpp)
  target_link_libraries(microhal_edge_cases PRIVATE microhal::microhal)

  add_executable(microhal_multi_backend tests/multi_backend_validation.cpp)
  target_link_libraries(microhal_multi_backend PRIVATE microhal::microhal)
endif()

# Backend-agnostic: this is the one test target that builds under any MICROHAL_BACKENDS selection.
add_executable(microhal_backend_selection tests/backend_selection_test.cpp)
target_link_libraries(microhal_backend_selection PRIVATE microhal::microhal)

add_executable(microhal_backend_api_contract tests/backend_api_contract_test.cpp)
target_link_libraries(microhal_backend_api_contract PRIVATE microhal::microhal)

add_executable(microhal_preset_capability_expectations tests/preset_capability_expectations_test.cpp)
target_link_libraries(microhal_preset_capability_expectations PRIVATE microhal::microhal)

# Runtime behavioural tests against the host recording backend. These are the only tests that
# observe what the facades actually do at runtime, so they are host-only.
if(NOT MICROHAL_VENDOR_VALIDATION)
  add_executable(microhal_mock_runtime tests/mock_runtime_test.cpp)
  target_link_libraries(microhal_mock_runtime PRIVATE microhal::microhal)
endif()

if(_microhal_build_full_matrix)
  add_executable(example_stm32_adc_dma examples/stm32_adc_dma_sampling.cpp)
  target_link_libraries(example_stm32_adc_dma PRIVATE microhal::microhal)

  add_executable(example_stm32_pwm_motor examples/stm32_pwm_motor_control.cpp)
  target_link_libraries(example_stm32_pwm_motor PRIVATE microhal::microhal)

  add_executable(example_stm32_can_comm examples/stm32_can_communication.cpp)
  target_link_libraries(example_stm32_can_comm PRIVATE microhal::microhal)

  add_executable(example_stm32_flash_log examples/stm32_flash_data_logging.cpp)
  target_link_libraries(example_stm32_flash_log PRIVATE microhal::microhal)

  add_executable(example_stm32_ethernet examples/stm32_ethernet_frame_io.cpp)
  target_link_libraries(example_stm32_ethernet PRIVATE microhal::microhal)

  add_executable(example_stm32_sd examples/stm32_sd_block_io.cpp)
  target_link_libraries(example_stm32_sd PRIVATE microhal::microhal)

  add_executable(example_nuttx_device_paths examples/nuttx_device_path_bindings.cpp)
  target_link_libraries(example_nuttx_device_paths PRIVATE microhal::microhal)
endif()

if(NOT MICROHAL_VENDOR_VALIDATION)
  add_library(microhal_example_stm32_uart_c_shim STATIC examples/stm32_uart_c_shim.cpp)
  target_include_directories(microhal_example_stm32_uart_c_shim PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/examples)
  target_link_libraries(microhal_example_stm32_uart_c_shim PRIVATE microhal::microhal)

  add_executable(example_stm32_uart_c_api examples/stm32_uart_c_consumer.c)
  target_link_libraries(example_stm32_uart_c_api PRIVATE microhal_example_stm32_uart_c_shim)

  add_library(microhal_example_stm32_i2c_spi_c_shim STATIC examples/stm32_i2c_spi_c_shim.cpp)
  target_include_directories(microhal_example_stm32_i2c_spi_c_shim PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/examples)
  target_link_libraries(microhal_example_stm32_i2c_spi_c_shim PRIVATE microhal::microhal)

  add_executable(example_stm32_i2c_spi_c_api examples/stm32_i2c_spi_c_consumer.c)
  target_link_libraries(example_stm32_i2c_spi_c_api PRIVATE microhal_example_stm32_i2c_spi_c_shim)
endif()

if(_microhal_build_full_matrix)
  add_custom_target(microhal_contractual_examples
    DEPENDS
      example_stm32_adc_dma
      example_stm32_pwm_motor
      example_stm32_can_comm
      example_stm32_flash_log
      example_stm32_ethernet
      example_stm32_sd
      example_nuttx_device_paths)
endif()

if(_microhal_build_full_matrix AND NOT MICROHAL_VENDOR_VALIDATION)
  add_dependencies(microhal_contractual_examples example_stm32_uart_c_api example_stm32_i2c_spi_c_api)
endif()

# Warnings are errors for microHAL's own tests and examples, but never for the INTERFACE target,
# which downstream projects consume. This is what keeps discarded [[nodiscard]] statuses from
# creeping back into the examples that are supposed to demonstrate correct error handling.
if(MICROHAL_WERROR AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  get_property(_microhal_own_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
  foreach(_microhal_target IN LISTS _microhal_own_targets)
    get_target_property(_microhal_target_type ${_microhal_target} TYPE)
    if(NOT _microhal_target_type STREQUAL "INTERFACE_LIBRARY" AND NOT _microhal_target_type STREQUAL "UTILITY")
      target_compile_options(${_microhal_target} PRIVATE -Werror)
    endif()
  endforeach()
endif()

enable_testing()
if(TARGET microhal_compile_time_smoke AND NOT MICROHAL_VENDOR_VALIDATION)
  add_test(NAME microhal_compile_time_smoke COMMAND microhal_compile_time_smoke)
endif()
if(TARGET microhal_extended_peripherals)
  add_test(NAME microhal_extended_peripherals COMMAND microhal_extended_peripherals)
endif()
if(TARGET microhal_edge_cases)
  add_test(NAME microhal_edge_cases COMMAND microhal_edge_cases)
endif()
if(TARGET microhal_multi_backend)
  add_test(NAME microhal_multi_backend COMMAND microhal_multi_backend)
endif()
add_test(NAME microhal_backend_selection COMMAND microhal_backend_selection)
add_test(NAME microhal_backend_api_contract COMMAND microhal_backend_api_contract)
add_test(NAME microhal_preset_capability_expectations COMMAND microhal_preset_capability_expectations)
add_test(NAME microhal_capability_claim_policy
  COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_capability_claim_policy.sh)

if(NOT MICROHAL_VENDOR_VALIDATION)
  add_test(NAME microhal_mock_runtime COMMAND microhal_mock_runtime)
  add_test(NAME example_stm32_uart_c_api COMMAND example_stm32_uart_c_api)
  add_test(NAME example_stm32_i2c_spi_c_api COMMAND example_stm32_i2c_spi_c_api)
endif()

include(GNUInstallDirs)

install(DIRECTORY include/microhal
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS microhal
  EXPORT microhal-targets
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT microhal-targets
  FILE microhal-targets.cmake
  NAMESPACE microhal::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/microhal)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/microhal-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/microhal-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/microhal)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/microhal-config-version.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/microhal-config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/microhal-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/microhal)
