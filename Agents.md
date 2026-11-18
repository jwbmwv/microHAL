# microHAL Agent Rules

This file is the contributor and agent playbook for this repository. It captures the
constraints that keep microHAL a small, portable, compile-time peripheral abstraction.

## 1) Purpose and Scope

- Keep microHAL a header-only, zero-overhead hardware abstraction layer for microcontrollers.
- Favor correctness and portability across supported vendor and RTOS backends.
- Preserve the public API and the distinction between peripheral abstraction and board integration.

## 2) Core Architecture Invariants

- C++20 is the minimum language standard. Newer standards may be used when they materially
  improve microHAL without changing the C++20 API or runtime behavior.
- Backend tags, traits, concepts, and compile-time capability checks select and validate backend
  behavior. Do not add runtime backend selection, virtual dispatch, RTTI, exceptions, or dynamic
  allocation to the public abstraction.
- microHAL is an INTERFACE CMake target. Keep public facades thin and keep vendor-specific
  behavior in the matching backend headers under `include/microhal/backends/`.
- Facades and configurations are stateless descriptions; backend resources and vendor handles
  must follow the existing backend patterns.
- Report fallible operations with `microhal::status` or `microhal::result<T>`. Preserve
  `[[nodiscard]]` and explicit error propagation.
- Capability-dependent APIs must remain coherent with backend traits and compile-time
  validation. Do not infer support from a vendor or target name alone.

## 3) Backend and Integration Rules

- Backend tags and registration live in `include/microhal/core/traits.hpp` and the backend trait
  specializations. The supported tags and current peripheral coverage are summarized in
  `docs/capability_matrix.md`; the code is authoritative when the summary and implementation differ.
- `MICROHAL_BACKENDS` can select one or several backends. An empty selection enables all backends;
  do not assume exactly one backend is selected in every build.
- Public facades must not expose vendor SDK types. Keep vendor symbols and SDK-specific behavior
  in backend headers, and keep board-owned clocks, pin muxing, IRQ routing, DMA resources, and
  other concrete hardware identity in application bindings.
- Capability entries may be conditional on SDK configuration, target hardware, or bindings.
  Update the capability matrix and relevant integration docs when support changes.
- Keep the mock backend useful for host tests; use it to verify call forwarding and error
  propagation where runtime behavior can be tested without hardware.
- Keep the Zephyr backend usable as a Zephyr module. Follow `docs/zephyr_module.md` and distinguish
  missing west/manifest or generated-header setup from a microHAL source regression.

## 4) API, Configuration, and Resource Rules

- Keep configuration types compile-time friendly and preserve the existing split between
  peripheral policy/configuration and board-specific resource bindings.
- Prefer typed `binding` information for concrete board resources when extending integrations.
  Retain legacy numeric locators only where existing API compatibility requires them.
- Do not add RTOS services or generic resource-management layers to the HAL. Applications own
  synchronization, peripheral sharing, pin/clock setup, and interrupt routing.
- Do not silently change peripheral semantics, capability claims, timeout behavior, ownership,
  callback lifetime, or transfer-mode behavior. Document externally visible contract changes.
- Maintain the C++20 minimum and do not require newer language features from consumers.

## 5) Concurrency and Interrupt Semantics

- microHAL does not provide locks, atomics, or RTOS-aware synchronization. A peripheral
  configuration must not be driven concurrently from multiple threads or contexts unless the
  backend contract explicitly supports it; serialization is application-owned.
- Blocking facade calls must not be made from interrupt context.
- Backend interrupt callbacks execute in interrupt context. Keep handlers `noexcept`, bounded,
  and non-blocking; do not call blocking facade operations from them.
- Respect each backend's advertised ISR and DMA capabilities. Hosted or simulated validation is
  not hardware deployment proof.

## 6) Code Style and Documentation

- Follow repository `.clang-format` (LLVM-based, two-space indentation, 120-column limit) and
  `.editorconfig`.
- Public API declarations need documentation of behavior and applicable constraints, including
  configuration requirements, return/error behavior, ownership, timeout, and callback or
  interrupt-context rules. Avoid duplicating API documentation at definitions.
- Update directly related documentation when API, capability, integration, or user-visible
  behavior changes. Use existing documentation in `docs/` rather than adding overlapping guides.
- Generated diagrams under `docs/diagrams/generated/` must be regenerated with the repository
  scripts when their PlantUML sources change; do not hand-edit generated output.

## 7) Build and Test

Default hosted regression commands:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

For a selected-backend build:

```bash
cmake -S . -B build -DMICROHAL_BACKENDS=MOCK
cmake --build build
ctest --test-dir build --output-on-failure
```

- Tests and examples are enabled by default and use strict warnings; `MICROHAL_WERROR` controls
  warnings-as-errors for project-owned targets, not downstream consumers.
- Choose focused validation for the changed surface, then broaden to hosted regression when
  appropriate. Use the CI workflow (`.github/workflows/ci.yml`) as the source of truth for
  formatting, static analysis, cross-compilation, and vendor-specific validation commands.
- Backend changes should be compiled with that backend selected and, when its SDK is available,
  validated against the real SDK. Preserve host regression coverage as well.
- Compile-time checks prove constraints and instantiation, not runtime behavior. Add or update
  focused mock runtime tests when behavior changes and is testable on the host.
- Cross-compilation and simulation are useful regression evidence, not proof of hardware behavior.

## 8) Source of Truth

When guidance conflicts, use this order:

1. Public declarations and backend trait implementations under `include/microhal/`
2. `docs/architecture.md` and `docs/backend_sdk_requirements.md`
3. `docs/capability_matrix.md` and `docs/vendor_validation.md`
4. `README.md` and the relevant integration guide under `docs/`
5. Build, formatting, and CI configuration (`CMakeLists.txt`, `cmake/`, `.clang-format`,
   `.editorconfig`, and `.github/workflows/`)

Do not treat examples or summaries as more authoritative than the implementation and explicit
integration contracts.

## 9) Change Discipline and Validation

- Make minimal, targeted changes. Do not reformat or refactor unrelated code.
- Read and preserve existing worktree changes. Never overwrite unrelated user changes.
- Add tests for behavior introduced or fixed; avoid tests that only restate constants or
  implementation details.
- For public API or capability changes, update the API, tests, and directly related documentation
  together. State API/ABI impact in a PR summary when applicable.
- For build/configuration changes, reconfigure and validate a hosted build and test pass.
- Report exact validation commands, backend/configuration context, skipped checks, and any
  unrelated failures. Do not hide failures or claim hardware validation that was not performed.

## 10) Generated, External, and Versioned Files

- Treat build directories, downloaded vendor SDKs, and generated artifacts as read-only unless
  explicitly required by the task.
- Do not modify external or vendored SDK code as a workaround for microHAL changes.
- `VERSION` is read by `CMakeLists.txt`; keep release-version changes consistent with the
  repository's packaging and release metadata.
- Preserve license headers, SPDX identifiers, `LICENSE`, and `NOTICE` unless the task explicitly
  requires a legal or compliance update.

## 11) Definition of Done

A change is complete when:

1. It stays within the requested scope and preserves microHAL's compile-time architecture.
2. Relevant formatting, build, static-analysis, and test checks pass, or skipped checks and risks
   are reported.
3. Backend capability, binding, concurrency, and interrupt contracts remain coherent.
4. Directly affected documentation is updated for externally visible changes.
