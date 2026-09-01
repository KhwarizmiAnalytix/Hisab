# Code Coverage

Coverage is configured per library module. The current project does not consume
a global `PROJECT_ENABLE_COVERAGE` option.

## Recommended workflow

Use the CMake helper to enable coverage uniformly for the loaded modules and to
run its coverage workflow:

```bash
python Scripts/setup.py config.build.test.ninja.clang.debug.coverage

# Limit coverage to one library and its configured dependencies.
python Scripts/setup.py config.build.test.ninja.clang.debug.coverage --project.core
```

The helper fans the `coverage` selection into `<MODULE>_ENABLE_COVERAGE=ON`.
Coverage is incompatible with LTO for the affected targets and the build helper
uses a Debug configuration for coverage work.

## Direct CMake

For a single module, set that module's option:

```bash
cmake -S . -B build-core-coverage -G Ninja \
  -DXSIGMA_LIBRARY_PROJECT=Core \
  -DCORE_ENABLE_COVERAGE=ON \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build-core-coverage --parallel
ctest --test-dir build-core-coverage --output-on-failure
```

For a whole-project direct CMake configuration, set `*_ENABLE_COVERAGE=ON` for
every library that must be instrumented. The helper is preferred because it
keeps the module settings consistent.

## Bazel

Use Bazel's coverage action:

```bash
python Scripts/setup_bazel.py coverage.debug

# Or use raw Bazel with an explicit source filter.
bazel coverage --config=debug \
  --instrumentation_filter='//Library[/:]' \
  --combined_report=lcov //Library/...
```

The helper widens the instrumentation filter to the selected library scope so a
single test target can produce coverage for the library it exercises. Bazel
coverage support is independent of CMake's per-module coverage options.

## Notes

- Use a fresh build directory when changing instrumentation.
- Run tests from the matching build directory with `ctest --test-dir`.
- Tool availability and report-generation format depend on the active compiler
  and platform.
- See [Build configuration](build/build-configuration.md) for the CMake option
  model and [Setup](setup.md) for supported helper actions.
