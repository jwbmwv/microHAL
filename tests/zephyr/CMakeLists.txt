# Zephyr ztest application for microHAL.

cmake_minimum_required(VERSION 3.20.0)

list(APPEND EXTRA_ZEPHYR_MODULES ${CMAKE_CURRENT_LIST_DIR}/../..)
list(APPEND EXTRA_CONF_FILE ${CMAKE_CURRENT_LIST_DIR}/cpp20.conf)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(microhal_zephyr_tests LANGUAGES CXX)

target_sources(app PRIVATE
  src/main.cpp)

target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../..)

target_compile_definitions(app PRIVATE MICROHAL_SUPPRESS_MISSING_MCU_WARNINGS=1)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(app PRIVATE -fno-exceptions -fno-rtti)
endif()
