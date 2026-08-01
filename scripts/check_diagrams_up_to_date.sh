#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/.." && pwd)"

cd "$repo_root"

bash ./scripts/clean_diagrams.sh
bash ./scripts/generate_diagrams.sh

if ! git diff --quiet -- docs/diagrams/generated; then
  echo "Generated diagrams are out of date. Commit regenerated files under docs/diagrams/generated." >&2
  git --no-pager diff --name-status -- docs/diagrams/generated >&2
  exit 1
fi

untracked="$(git ls-files --others --exclude-standard docs/diagrams/generated)"
if [[ -n "$untracked" ]]; then
  echo "Found untracked generated diagram artifacts:" >&2
  echo "$untracked" >&2
  exit 1
fi

echo "Diagram artifacts are up to date."
