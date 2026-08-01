#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/.." && pwd)"
support_root="${MICROHAL_NORDIC_SUPPORT_ROOT:-$repo_root/vendor-support/nrf-bm/v2.0.0}"
build_root="${MICROHAL_NORDIC_MATRIX_BUILD_ROOT:-$repo_root/build/vendor-nordic-matrix}"

validate_soc() {
  local label="$1"
  local soc_define="$2"
  local build_dir="$build_root/$label"

  rm -rf "$build_dir"

  cmake -S "$repo_root" -B "$build_dir" \
    -DMICROHAL_VENDOR_VALIDATION=ON \
    -DMICROHAL_VENDOR_VALIDATION_PROVIDER=NORDIC \
    -DMICROHAL_VENDOR_VALIDATION_NORDIC_SUPPORT_ROOT="$support_root" \
    -DMICROHAL_VENDOR_VALIDATION_NORDIC_DEFINITIONS="$soc_define"

  cmake --build "$build_dir" \
    --target microhal_backend_selection microhal_backend_api_contract microhal_preset_capability_expectations

  ctest --test-dir "$build_dir" --output-on-failure \
    -R "microhal_backend_selection|microhal_backend_api_contract|microhal_preset_capability_expectations|microhal_capability_claim_policy"
}

if [[ ! -d "$support_root" ]]; then
  printf 'Nordic support root does not exist: %s\n' "$support_root" >&2
  exit 1
fi

case "${1:-all}" in
  all)
    validate_soc nrf52 NRF52840_XXAA
    validate_soc nrf53 NRF5340_XXAA_APPLICATION
    validate_soc nrf54 NRF54H20_XXAA
    ;;
  nrf52)
    validate_soc nrf52 NRF52840_XXAA
    ;;
  nrf53)
    validate_soc nrf53 NRF5340_XXAA_APPLICATION
    ;;
  nrf54)
    validate_soc nrf54 NRF54H20_XXAA
    ;;
  *)
    printf 'Unknown Nordic matrix target: %s\n' "$1" >&2
    printf 'Supported targets: nrf52, nrf53, nrf54\n' >&2
    exit 2
    ;;
esac
