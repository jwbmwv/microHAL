#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/.." && pwd)"
support_root="${MICROHAL_NORDIC_SUPPORT_ROOT:-$repo_root/vendor-support/nrf-bm/v2.0.1}"
build_root="${MICROHAL_NORDIC_MATRIX_BUILD_ROOT:-$repo_root/build/vendor-nordic-matrix}"

validate_soc() {
  local label="$1"
  local soc_define="$2"
  local processor_define="${3:-}"
  local build_dir="$build_root/$label"
  local definitions="$soc_define"
  local cpu="cortex-m33"
  if [[ "$label" == "nrf52" ]]; then cpu="cortex-m4"; fi

  if [[ -n "$processor_define" ]]; then
    definitions="$definitions;$processor_define"
  fi

  cmake -S "$repo_root" -B "$build_dir" \
    -DCMAKE_SYSTEM_NAME=Generic \
    -DCMAKE_C_COMPILER=arm-none-eabi-gcc \
    -DCMAKE_CXX_COMPILER=arm-none-eabi-g++ \
    -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
    -DCMAKE_C_FLAGS="-mcpu=$cpu -mthumb" \
    -DCMAKE_CXX_FLAGS="-mcpu=$cpu -mthumb" \
    -DCMAKE_EXE_LINKER_FLAGS="-mcpu=$cpu -mthumb -specs=nosys.specs" \
    -DMICROHAL_WERROR=OFF \
    -DMICROHAL_VENDOR_VALIDATION=ON \
    -DMICROHAL_VENDOR_VALIDATION_PROVIDER=NORDIC \
    -DMICROHAL_VENDOR_VALIDATION_NORDIC_SUPPORT_ROOT="$support_root" \
    -DMICROHAL_VENDOR_VALIDATION_NORDIC_DEFINITIONS="$definitions"

  cmake --build "$build_dir" \
    --target microhal_backend_selection microhal_backend_api_contract microhal_preset_capability_expectations

  # ARM objects and executables are compile/link checks, not host-runnable tests.
  ctest --test-dir "$build_dir" --output-on-failure -R "^microhal_capability_claim_policy$"
}

if [[ ! -d "$support_root" ]]; then
  printf 'Nordic support root does not exist: %s\n' "$support_root" >&2
  exit 1
fi

if ! command -v arm-none-eabi-gcc >/dev/null || ! command -v arm-none-eabi-g++ >/dev/null; then
  printf 'Nordic matrix validation requires the ARM GCC bare-metal toolchain.\n' >&2
  exit 1
fi

case "${1:-all}" in
  all)
    validate_soc nrf52 NRF52840_XXAA
    validate_soc nrf53 NRF5340_XXAA_APPLICATION
    validate_soc nrf54 NRF54H20_XXAA NRF_APPLICATION
    validate_soc nrf54l15 NRF54L15_XXAA NRF_APPLICATION
    ;;
  nrf52)
    validate_soc nrf52 NRF52840_XXAA
    ;;
  nrf53)
    validate_soc nrf53 NRF5340_XXAA_APPLICATION
    ;;
  nrf54)
    validate_soc nrf54 NRF54H20_XXAA NRF_APPLICATION
    ;;
  nrf54l15)
    validate_soc nrf54l15 NRF54L15_XXAA NRF_APPLICATION
    ;;
  *)
    printf 'Unknown Nordic matrix target: %s\n' "$1" >&2
    printf 'Supported targets: nrf52, nrf53, nrf54, nrf54l15\n' >&2
    exit 2
    ;;
esac
