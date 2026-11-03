cmake_minimum_required(VERSION 3.24)

project(microhal_package_consumer LANGUAGES CXX)

option(MICROHAL_TEST_ADD_SUBDIRECTORY "Test source-tree consumption instead of find_package" OFF)
set(_consumer_cxx_standard "${CMAKE_CXX_STANDARD}")

file(READ "${CMAKE_CURRENT_LIST_DIR}/../../VERSION" MICROHAL_EXPECTED_VERSION)
string(STRIP "${MICROHAL_EXPECTED_VERSION}" MICROHAL_EXPECTED_VERSION)

if(MICROHAL_TEST_ADD_SUBDIRECTORY)
  add_subdirectory(../.. microhal EXCLUDE_FROM_ALL)
  if(_consumer_cxx_standard MATCHES "^(20|23|26)$")
    get_target_property(_microhal_cxx_standard microhal_backend_selection CXX_STANDARD)
    if(NOT _microhal_cxx_standard STREQUAL _consumer_cxx_standard)
      message(FATAL_ERROR
        "microHAL changed the requested C++${_consumer_cxx_standard} to C++${_microhal_cxx_standard}")
    endif()
  endif()
else()
  find_package(microhal ${MICROHAL_EXPECTED_VERSION} CONFIG REQUIRED)
  if(NOT microhal_VERSION STREQUAL MICROHAL_EXPECTED_VERSION)
    message(FATAL_ERROR "Expected microHAL package version ${MICROHAL_EXPECTED_VERSION}, got ${microhal_VERSION}")
  endif()
endif()

add_executable(microhal_package_consumer main.cpp)
target_link_libraries(microhal_package_consumer PRIVATE microhal::microhal)
if(NOT _consumer_cxx_standard STREQUAL "")
  target_compile_definitions(microhal_package_consumer PRIVATE
    MICROHAL_TEST_CXX_STANDARD=${_consumer_cxx_standard})
endif()

enable_testing()
add_test(NAME microhal_package_consumer COMMAND microhal_package_consumer)