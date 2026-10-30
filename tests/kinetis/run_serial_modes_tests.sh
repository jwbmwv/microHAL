#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail
cd "$(dirname "$0")/../.."
output="${1:-.build-kinetis-serial}"
mkdir -p "$output"
flags=(-std=c++20 -Wall -Wextra -Werror -DMICROHAL_SUPPRESS_MISSING_MCU_WARNINGS=1 -Iinclude)
"${CXX:-c++}" "${flags[@]}" tests/kinetis/serial_modes_no_sdk_test.cpp -o "$output/no-sdk"
"$output/no-sdk"
echo "no-sdk: PASS"
"${CXX:-c++}" "${flags[@]}" -Itests/kinetis/sdk_common \
  tests/kinetis/serial_modes_no_sdk_test.cpp -o "$output/no-drivers"
"$output/no-drivers"
echo "SDK common without optional drivers: PASS"
for mode in irq edma dma; do
  extras=()
  if [[ "$mode" != irq ]]; then extras+=("-Itests/kinetis/sdk_$mode"); fi
  "${CXX:-c++}" "${flags[@]}" -Itests/kinetis/sdk "${extras[@]}" \
    tests/kinetis/serial_modes_test.cpp -o "$output/$mode"
  "$output/$mode"
  echo "$mode: PASS"
done
