#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/.." && pwd)"
out_dir="$repo_root/docs/diagrams/generated"

mkdir -p "$out_dir"

find "$out_dir" -maxdepth 1 -type f \( -name '*.svg' -o -name '*.png' -o -name '*.pdf' \) -delete

echo "Cleaned generated diagram artifacts in: $out_dir"
