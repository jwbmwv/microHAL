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

build_system_pdf_classpath() {
  local plantuml_jar=""

  if [[ -n "${PLANTUML_JAR:-}" ]] && [[ -f "${PLANTUML_JAR}" ]]; then
    plantuml_jar="$PLANTUML_JAR"
  elif [[ -f "/usr/share/plantuml/plantuml.jar" ]]; then
    plantuml_jar="/usr/share/plantuml/plantuml.jar"
  else
    local plantuml_bin
    plantuml_bin="$(command -v plantuml || true)"
    if [[ -n "$plantuml_bin" ]] && [[ -f "$plantuml_bin" ]]; then
      plantuml_jar="$(sed -n 's|.*-jar[[:space:]]\([^[:space:]]\+\.jar\).*|\1|p' "$plantuml_bin" | head -n 1)"
      if [[ -n "$plantuml_jar" ]]; then
        plantuml_jar="$(eval echo "$plantuml_jar")"
      fi
    fi
  fi

  shopt -s nullglob
  local jars=(
    "$plantuml_jar"
    /opt/plantuml*/plantuml*.jar
    /usr/share/java/plantuml*.jar
    /usr/share/java/batik*.jar
    /usr/share/java/xml-apis*.jar
    /usr/share/java/xmlgraphics-commons*.jar
  )
  shopt -u nullglob

  if [[ "${#jars[@]}" -eq 0 ]]; then
    return 1
  fi

  local cp
  cp="$(IFS=:; echo "${jars[*]}")"
  printf '%s' "$cp"
}

render_pdf_with_system_classpath() {
  local src="$1"
  local out_pdf="$2"

  if ! command -v java >/dev/null 2>&1; then
    return 1
  fi

  local classpath
  classpath="$(build_system_pdf_classpath)" || return 1

  rm -f "$out_pdf"
  set +e
  java -cp "$classpath" net.sourceforge.plantuml.Run -tpdf -o "$out_dir" "$src"
  local rc=$?
  set -e
  if [[ $rc -ne 0 ]]; then
    return 1
  fi

  assert_fresh_nonempty "$out_pdf"
}

render_plantuml_pdf() {
  local src="$1"
  local out_pdf="$2"

  if render_pdf_with_system_classpath "$src" "$out_pdf"; then
    return 0
  fi

  rm -f "$out_pdf"
  set +e
  "${PLANTUML[@]}" -tpdf -o "$out_dir" "$src"
  local fallback_rc=$?
  set -e

  if [[ $fallback_rc -eq 0 ]] && assert_fresh_nonempty "$out_pdf"; then
    return 0
  fi

  echo "Failed to generate PDF for $src" >&2
  return 1
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
  "${PLANTUML[@]}" -tpng -o "$out_dir" "$src"
  assert_fresh_nonempty "$out_png"

  echo "Generating ${stem}.pdf"
  render_plantuml_pdf "$src" "$out_pdf"
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
