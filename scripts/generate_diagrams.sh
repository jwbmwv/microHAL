#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/.." && pwd)"
src_dir="$repo_root/docs/diagrams"
out_dir="$src_dir/generated"

PLANTUML=()

if command -v plantuml >/dev/null 2>&1; then
  PLANTUML=(plantuml)
elif [[ -n "${PLANTUML_JAR:-}" ]] && [[ -f "${PLANTUML_JAR}" ]] && command -v java >/dev/null 2>&1; then
  PLANTUML=(java -jar "$PLANTUML_JAR")
fi

if [[ "${#PLANTUML[@]}" -eq 0 ]]; then
  cat <<'EOF'
PlantUML is required but not found.

Options:
  install plantuml command
  or set PLANTUML_JAR and ensure java is installed

Then re-run:
  bash ./scripts/generate_diagrams.sh
EOF
  exit 1
fi

if ! command -v rsvg-convert >/dev/null 2>&1; then
  cat <<'EOF'
rsvg-convert is required to render PDF diagram artifacts.

Install the librsvg2-bin package, then re-run:
  bash ./scripts/generate_diagrams.sh
EOF
  exit 1
fi

mkdir -p "$out_dir"
run_epoch="$(date +%s)"

assert_fresh_nonempty() {
  local output="$1"

  if [[ ! -f "$output" ]]; then
    echo "Missing expected output: $output" >&2
    return 1
  fi

  if [[ ! -s "$output" ]]; then
    echo "Output is empty: $output" >&2
    return 1
  fi

  local mtime
  mtime="$(stat -c %Y "$output")"
  if [[ "$mtime" -lt "$run_epoch" ]]; then
    echo "Output is stale (older than this run): $output" >&2
    return 1
  fi
}

render_svg_pdf() {
  local svg="$1"
  local out_pdf="$2"

  rm -f "$out_pdf"
  SOURCE_DATE_EPOCH="${SOURCE_DATE_EPOCH:-0}" rsvg-convert --format=pdf --output "$out_pdf" "$svg"
  assert_fresh_nonempty "$out_pdf"
}

render_svg_png() {
  local svg="$1"
  local out_png="$2"

  rm -f "$out_png"
  SOURCE_DATE_EPOCH="${SOURCE_DATE_EPOCH:-0}" rsvg-convert --format=png --output "$out_png" "$svg"
  assert_fresh_nonempty "$out_png"
}

render_plantuml() {
  local src="$1"
  local stem
  stem="$(basename "${src%.*}")"
  local out_svg="$out_dir/${stem}.svg"
  local out_png="$out_dir/${stem}.png"
  local out_pdf="$out_dir/${stem}.pdf"

  rm -f "$out_svg" "$out_png" "$out_pdf"

  echo "Generating ${stem}.svg"
  "${PLANTUML[@]}" -tsvg -o "$out_dir" "$src"
  assert_fresh_nonempty "$out_svg"

  echo "Generating ${stem}.png"
  render_svg_png "$out_svg" "$out_png"

  echo "Generating ${stem}.pdf"
  render_svg_pdf "$out_svg" "$out_pdf"
}

found=0
for src in "$src_dir"/*.puml; do
  if [[ -f "$src" ]]; then
    stem="$(basename "${src%.*}")"
    if [[ "$stem" == "theme" ]] || [[ "$stem" == *_theme ]]; then
      continue
    fi
    found=1
    render_plantuml "$src"
  fi
done

if [[ "$found" -eq 0 ]]; then
  echo "No .puml files found under $src_dir"
  exit 2
fi

echo "Done. Artifacts written to: $out_dir"
