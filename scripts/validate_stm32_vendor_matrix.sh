#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/.." && pwd)"
support_root="${MICROHAL_STM32_SUPPORT_ROOT:-$repo_root/vendor-support}"
compiler="${MICROHAL_ARM_CXX_COMPILER:-arm-none-eabi-g++}"
build_root="${MICROHAL_STM32_MATRIX_BUILD_ROOT:-$repo_root/build/vendor-matrix}"

validate_family() {
  local family="$1"
  local device="$2"
  local cpu="$3"
  local build_dir="$build_root/${family,,}"

  cmake -S "$repo_root" -B "$build_dir" \
    -DCMAKE_CXX_COMPILER="$compiler" \
    -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
    "-DCMAKE_CXX_FLAGS=-mcpu=$cpu -mthumb" \
    -DMICROHAL_VENDOR_VALIDATION=ON \
    -DMICROHAL_VENDOR_VALIDATION_PROVIDER=STM32 \
    -DMICROHAL_VENDOR_VALIDATION_STM32_FAMILY="$family" \
    -DMICROHAL_VENDOR_VALIDATION_STM32_DEVICE="$device" \
    -DMICROHAL_VENDOR_VALIDATION_STM32_SUPPORT_ROOT="$support_root"
  cmake --build "$build_dir" --target microhal_compile_time_smoke
}

if [[ ! -d "$support_root" ]]; then
  printf 'STM32 support root does not exist: %s\n' "$support_root" >&2
  exit 1
fi

case "${1:-all}" in
  all)
    validate_family STM32C0 STM32C011xx cortex-m0plus
    validate_family STM32F4 STM32F407xx cortex-m4
    validate_family STM32G4 STM32G431xx cortex-m4
    ;;
  STM32C0)
    validate_family STM32C0 STM32C011xx cortex-m0plus
    ;;
  STM32F4)
    validate_family STM32F4 STM32F407xx cortex-m4
    ;;
  STM32G4)
    validate_family STM32G4 STM32G431xx cortex-m4
    ;;
  *)
    printf 'Unknown matrix family: %s\n' "$1" >&2
    printf 'Supported families: STM32C0, STM32F4, STM32G4\n' >&2
    exit 2
    ;;
esac