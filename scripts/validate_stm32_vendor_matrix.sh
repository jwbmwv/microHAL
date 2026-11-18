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
    -DMICROHAL_VENDOR_VALIDATION_STM32_SUPPORT_ROOT="$support_root" || return
  cmake --build "$build_dir" --target microhal_compile_time_smoke
}

if [[ ! -d "$support_root" ]]; then
  printf 'STM32 support root does not exist: %s\n' "$support_root" >&2
  exit 1
fi

readonly matrix=(
  "STM32C0 STM32C011xx cortex-m0plus"
  "STM32F0 STM32F072xB cortex-m0"
  "STM32F1 STM32F103xB cortex-m3"
  "STM32F2 STM32F207xx cortex-m3"
  "STM32F3 STM32F303xC cortex-m4"
  "STM32F4 STM32F407xx cortex-m4"
  "STM32F7 STM32F767xx cortex-m7"
  "STM32G0 STM32G071xx cortex-m0plus"
  "STM32G4 STM32G431xx cortex-m4"
  "STM32H5 STM32H563xx cortex-m33"
  "STM32H7 STM32H743xx cortex-m7"
  "STM32H7RS STM32H7S3xx cortex-m7"
  "STM32L0 STM32L072xx cortex-m0plus"
  "STM32L1 STM32L152xE cortex-m3"
  "STM32L4 STM32L476xx cortex-m4"
  "STM32L5 STM32L552xx cortex-m33"
  "STM32N6 STM32N657xx cortex-m55"
  "STM32U0 STM32U083xx cortex-m0plus"
  "STM32U3 STM32U385xx cortex-m33"
  "STM32U5 STM32U575xx cortex-m33"
  "STM32WB STM32WB55xx cortex-m4"
)

selection="${1:-all}"
matched=false
failures=()
for entry in "${matrix[@]}"; do
  read -r family device cpu <<<"$entry"
  if [[ "$selection" == "all" ]]; then
    case "$family" in
      STM32C0|STM32F4|STM32G4) ;;
      *) continue ;;
    esac
  elif [[ "$selection" != "full" && "$selection" != "$family" ]]; then
    continue
  fi
  matched=true
  if ! validate_family "$family" "$device" "$cpu"; then
    failures+=("$family")
  fi
done

if [[ "$matched" == false ]]; then
  printf 'Unknown matrix selection: %s\n' "$selection" >&2
  printf 'Use all (C0/F4/G4), full (all supported families), or an STM32 family name.\n' >&2
  exit 2
fi
if [[ ${#failures[@]} -gt 0 ]]; then
  printf 'STM32 vendor validation failed: %s\n' "${failures[*]}" >&2
  exit 1
fi