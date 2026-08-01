cmake_minimum_required(VERSION 3.24)

project(microhal_package_consumer LANGUAGES CXX)

file(READ "${CMAKE_CURRENT_LIST_DIR}/../../VERSION" MICROHAL_EXPECTED_VERSION)
string(STRIP "${MICROHAL_EXPECTED_VERSION}" MICROHAL_EXPECTED_VERSION)

find_package(microhal ${MICROHAL_EXPECTED_VERSION} CONFIG REQUIRED)

if(NOT microhal_VERSION STREQUAL MICROHAL_EXPECTED_VERSION)
  message(FATAL_ERROR "Expected microHAL package version ${MICROHAL_EXPECTED_VERSION}, got ${microhal_VERSION}")
endif()

add_executable(microhal_package_consumer main.cpp)
target_link_libraries(microhal_package_consumer PRIVATE microhal::microhal)