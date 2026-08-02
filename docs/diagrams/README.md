# microHAL Diagrams

This directory contains source diagrams for core architecture and integration workflows.

## Source Files

All diagram sources are PlantUML (`.puml`):

- `config_validation_flow.puml` — how facade/config validation moves through predicate and diagnostic checks.
- `binding_model_comparison.puml` — legacy numeric config fields vs binding-based board integration.
- `backend_selection_preset_map.puml` — CMake preset -> compile definitions -> enabled backends -> test scope.
- `test_coverage_map.puml` — mapping from tests to contract, policy, and runtime concerns.

All source diagrams include `theme.puml` to enforce consistent colors, typography, and line styling.

## Pinned Tool Versions

The CI diagram job uses this rendering toolchain baseline:

- PlantUML `1.2025.10` (downloaded as a pinned jar)
- Graphviz `dot` from `graphviz` package
- Java Runtime from `default-jre-headless`
- `rsvg-convert` from the `librsvg2-bin` package for PDF artifacts

PDF generation defaults `SOURCE_DATE_EPOCH` to `0`, making the committed PDF artifacts reproducible. Set that variable to a different epoch when a consuming build requires it.

## Generated Artifacts

Generated files are emitted to `docs/diagrams/generated/` as:

- `.svg`
- `.png`
- `.pdf`

## Regenerate

From the repository root:

```bash
bash ./scripts/generate_diagrams.sh
# or
make diagrams
```

The script uses PlantUML (`plantuml` command or `PLANTUML_JAR`).

## Clean

Remove generated artifacts before regeneration:

```bash
bash ./scripts/clean_diagrams.sh
# or
make diagrams-clean
```

## Verify Up To Date

Regenerate and fail if tracked artifacts are stale or missing:

```bash
bash ./scripts/check_diagrams_up_to_date.sh
# or
make diagrams-check
```
