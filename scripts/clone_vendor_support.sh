#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/.." && pwd)"
support_root="${1:-$repo_root/vendor-support}"

readonly st_github_org="https://github.com/STMicroelectronics"
readonly arm_github_org="https://github.com/ARM-software"
readonly hal_driver_repos=(
  stm32c0xx-hal-driver
  stm32f0xx-hal-driver
  stm32f1xx-hal-driver
  stm32f2xx-hal-driver
  stm32f3xx-hal-driver
  stm32f4xx-hal-driver
  stm32f7xx-hal-driver
  stm32g0xx-hal-driver
  stm32g4xx-hal-driver
  stm32h5xx-hal-driver
  stm32h7xx-hal-driver
  stm32h7rsxx-hal-driver
  stm32l0xx-hal-driver
  stm32l1xx-hal-driver
  stm32l4xx-hal-driver
  stm32l5xx-hal-driver
  stm32n6xx-hal-driver
  stm32u0xx-hal-driver
  stm32u3xx-hal-driver
  stm32u5xx-hal-driver
  stm32wbxx-hal-driver
)
readonly cmsis_device_repos=(
  cmsis-device-c0
  cmsis-device-f0
  cmsis-device-f1
  cmsis-device-f2
  cmsis-device-f3
  cmsis-device-f4
  cmsis-device-f7
  cmsis-device-g0
  cmsis-device-g4
  cmsis-device-h5
  cmsis-device-h7
  cmsis-device-h7rs
  cmsis-device-l0
  cmsis-device-l1
  cmsis-device-l4
  cmsis-device-l5
  cmsis-device-n6
  cmsis-device-u0
  cmsis-device-u3
  cmsis-device-u5
  cmsis-device-wb
)
readonly cmsis_core_repos=(
  cmsis-core
  CMSIS_5
)

mkdir -p "$support_root"

clone_or_update() {
  local repo="$1"
  local remote_url="$st_github_org/$repo.git"
  local target_dir="$support_root/$repo"

  if [[ "$repo" == "CMSIS_5" ]]; then
    remote_url="$arm_github_org/$repo.git"
  fi

  if [[ -d "$target_dir/.git" ]]; then
    echo "Updating $repo"
    git -C "$target_dir" fetch --tags --prune
    git -C "$target_dir" pull --ff-only
    return 0
  fi

  if [[ -e "$target_dir" ]]; then
    echo "Refusing to overwrite existing non-git path: $target_dir" >&2
    exit 1
  fi

  echo "Cloning $repo"
  git clone "$remote_url" "$target_dir"
}

for repo in "${hal_driver_repos[@]}"; do
  clone_or_update "$repo"
done

for repo in "${cmsis_device_repos[@]}"; do
  clone_or_update "$repo"
done

for repo in "${cmsis_core_repos[@]}"; do
  clone_or_update "$repo"
done

echo "Vendor support repositories are available under: $support_root"
