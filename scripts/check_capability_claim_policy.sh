#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

backend_files=(
  "$root_dir/include/microhal/backends/stm32/hal/traits.hpp"
  "$root_dir/include/microhal/backends/stm32/ll/traits.hpp"
  "$root_dir/include/microhal/backends/kinetis/espresso/traits.hpp"
  "$root_dir/include/microhal/backends/nordic/nrfx/traits.hpp"
  "$root_dir/include/microhal/backends/zephyr/traits.hpp"
  "$root_dir/include/microhal/backends/nuttx/traits.hpp"
  "$root_dir/include/microhal/backends/mock/traits.hpp"
)

peripherals=(
  gpio i2c spi qspi uart rtc flash timer adc dac can usb ethernet sd dma pwm watchdog i2s rcc crc
)

extract_struct_block() {
  local file="$1"
  local name="$2"
  awk -v name="$name" '
    $0 ~ "^struct " name "_api final" { in_block = 1 }
    in_block { print }
    in_block && $0 ~ /^};/ { exit }
  ' "$file"
}

extract_capability_block() {
  local file="$1"
  local name="$2"
  awk -v name="$name" '
    $0 ~ "static consteval struct " name "_capability " name "_capability\(\) noexcept" {
      in_block = 1
      depth = 0
    }
    in_block {
      print
      open_count = gsub(/\{/, "{")
      close_count = gsub(/\}/, "}")
      depth += open_count - close_count
      if (depth <= 0 && $0 ~ /^  }/) {
        exit
      }
    }
  ' "$file"
}

is_supported_capability() {
  local file="$1"
  local name="$2"
  local capability_block
  capability_block="$(extract_capability_block "$file" "$name")"
  if [[ -z "$capability_block" ]]; then
    return 1
  fi

  grep -q "\.supported[[:space:]]*=[[:space:]]*true" <<<"$capability_block"
}

has_placeholder_markers() {
  local block="$1"
  grep -E -q "return false;[[:space:]]*//|TODO|[Pp]laceholder|Requires .*from Config|would require|not commonly available|Configuration typically done in devicetree" <<<"$block"
}

violations=()

for file in "${backend_files[@]}"; do
  if [[ ! -f "$file" ]]; then
    continue
  fi

  rel_file="${file#$root_dir/}"

  for peripheral in "${peripherals[@]}"; do
    if ! is_supported_capability "$file" "$peripheral"; then
      continue
    fi

    struct_block="$(extract_struct_block "$file" "$peripheral")"
    if [[ -z "$struct_block" ]]; then
      violations+=("$rel_file: supported ${peripheral} capability, but ${peripheral}_api struct is missing")
      continue
    fi

    if has_placeholder_markers "$struct_block"; then
      violations+=("$rel_file: ${peripheral}_capability.supported=true but ${peripheral}_api contains placeholder markers")
    fi
  done
done

if [[ ${#violations[@]} -gt 0 ]]; then
  echo "[microHAL policy] Capability claims vs placeholder implementation check failed:"
  for item in "${violations[@]}"; do
    echo "  - $item"
  done
  echo ""
  echo "Policy: if *_capability().supported is true, corresponding *_api should not contain obvious placeholder markers."
  exit 1
fi

echo "[microHAL policy] Capability claim placeholder check passed."
