if(CONFIG_MICRO_HAL)
  zephyr_library()
  zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
  zephyr_library_sources(${CMAKE_CURRENT_SOURCE_DIR}/module_anchor.cpp)

  if(CONFIG_MICRO_HAL_SUPPRESS_MISSING_MCU_WARNINGS)
    zephyr_library_compile_definitions(MICROHAL_SUPPRESS_MISSING_MCU_WARNINGS=1)
  endif()

  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    zephyr_library_compile_options(-fno-exceptions -fno-rtti)
  endif()
endif()
