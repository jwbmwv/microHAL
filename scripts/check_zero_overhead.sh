#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
#
# Verifies microHAL's zero-overhead claim by compiling tests/zero_overhead_probe.cpp and comparing
# the machine code of a call made through the microHAL facades against the same call made directly
# against the backend struct. If the abstraction costs anything, the two disassemblies differ.
#
# Usage: scripts/check_zero_overhead.sh [compiler] [extra flags...]

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
compiler="${1:-g++}"
shift || true

if ! command -v "${compiler}" > /dev/null 2>&1; then
  echo "skip: ${compiler} not available" >&2
  exit 0
fi

objdump_tool="objdump"
case "${compiler}" in
  *arm-none-eabi-g++) objdump_tool="arm-none-eabi-objdump" ;;
esac

if ! command -v "${objdump_tool}" > /dev/null 2>&1; then
  echo "skip: ${objdump_tool} not available" >&2
  exit 0
fi

flags_label="-O2 $*"
work_dir="$(mktemp -d)"
trap 'rm -rf "${work_dir}"' EXIT

object="${work_dir}/probe.o"

"${compiler}" \
  -std=c++20 -O2 -fno-exceptions -fno-rtti \
  -I "${repo_root}/include" \
  -DMICROHAL_SUPPRESS_MISSING_MCU_WARNINGS=1 \
  -c "${repo_root}/tests/zero_overhead_probe.cpp" \
  -o "${object}" \
  "$@"

# Extract one function's instructions, normalising away everything that is not the instruction
# stream itself: addresses, raw opcode bytes, objdump's "# <symbol+offset>" annotations, and branch
# target addresses (which necessarily differ because the two functions sit at different offsets).
extract() {
  "${objdump_tool}" --disassemble="$1" "${object}" \
    | sed -n '/>:$/,$p' \
    | tail -n +2 \
    | sed -E 's/^[[:space:]]*[0-9a-f]+:[[:space:]]*//' \
    | sed -E 's/^(([0-9a-f]{2}|[0-9a-f]{4})[[:space:]]+)+//' \
    | sed -E 's/#.*$//' \
    | sed -E 's/<[^>]*>//g' \
    | sed -E 's/[[:space:]]+/ /g' \
    | sed -E 's/^ //; s/ $//' \
    | sed -E 's/^([a-z]{1,4}(\.[nw])?) [0-9a-f]+$/\1 TARGET/' \
    | grep -v '^$'
}

extract via_microhal > "${work_dir}/facade.txt"
extract via_backend_directly > "${work_dir}/direct.txt"

if [[ ! -s "${work_dir}/facade.txt" || ! -s "${work_dir}/direct.txt" ]]; then
  echo "error: could not disassemble the probe functions" >&2
  exit 1
fi

if diff -u "${work_dir}/direct.txt" "${work_dir}/facade.txt" > "${work_dir}/diff.txt"; then
  instructions="$(wc -l < "${work_dir}/facade.txt" | tr -d ' ')"
  echo "zero-overhead check passed (${compiler} ${flags_label}): both paths compile to the same ${instructions} instructions"
  exit 0
fi

echo "zero-overhead check FAILED (${compiler} ${flags_label}): going through microHAL changed the generated code" >&2
echo "--- direct backend call" >&2
echo "+++ through microHAL facade" >&2
cat "${work_dir}/diff.txt" >&2
exit 1
