cmake_minimum_required(VERSION 3.24)
project(microhal_clock_diagnostics LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

try_compile(_generated_diagnostics
  SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/clock.cpp"
  COMPILE_DEFINITIONS -DMICROHAL_TEST_PROBE_FEATURE=1)

foreach(_peripheral I2C SPI)
  if(_peripheral STREQUAL "I2C")
    set(_maximum 1000000)
  else()
    set(_maximum 50000000)
  endif()
  math(EXPR _excessive "${_maximum} + 1")
  foreach(_clock 0 1 ${_maximum} ${_excessive} 4294967295)
    try_compile(_compiled
      SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/clock.cpp"
      CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${CMAKE_CURRENT_SOURCE_DIR}/../../include"
      COMPILE_DEFINITIONS
        -DMICROHAL_BACKENDS_EXPLICIT=1 -DMICROHAL_ENABLE_MOCK=1
        -DMICROHAL_TEST_${_peripheral}=1 -DMICROHAL_TEST_CLOCK_HZ=${_clock}U
      OUTPUT_VARIABLE _output)
    if(_clock EQUAL 1 OR _clock EQUAL _maximum)
      if(NOT _compiled)
        message(FATAL_ERROR "Valid ${_peripheral} clock ${_clock} failed:\n${_output}")
      endif()
    else()
      if(_compiled)
        message(FATAL_ERROR "Invalid ${_peripheral} clock ${_clock} was accepted")
      endif()
      if(_generated_diagnostics)
        set(_expected
          "microHAL: requested ${_peripheral} clock ${_clock} Hz is outside backend range 1..${_maximum} Hz.")
      else()
        set(_expected "microHAL: requested ${_peripheral} clock exceeds backend capability.")
      endif()
      string(FIND "${_output}" "${_expected}" _found)
      if(_found EQUAL -1)
        message(FATAL_ERROR "Expected '${_expected}' in compiler output:\n${_output}")
      endif()
    endif()
  endforeach()
endforeach()

message(STATUS "Clock validation passed (generated diagnostics: ${_generated_diagnostics})")
