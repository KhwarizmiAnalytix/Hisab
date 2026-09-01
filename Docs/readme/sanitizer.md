# Sanitizers

XSigma configures sanitizers per library module. Sanitizer support requires a
Clang or GCC toolchain; MSVC configurations do not receive sanitizer flags from
the CMake module.

## Recommended workflow

Use `Scripts/setup.py` to enable the same sanitizer for the loaded libraries:

```bash
python Scripts/setup.py config.build.test.ninja.clang.debug --sanitizer.address
python Scripts/setup.py config.build.test.ninja.clang.debug --sanitizer.undefined
python Scripts/setup.py config.build.test.ninja.clang.debug --sanitizer.thread
python Scripts/setup.py config.build.test.ninja.clang.debug --sanitizer.memory
python Scripts/setup.py config.build.test.ninja.clang.debug --sanitizer.leak
```

The helper maps the long option to `<MODULE>_ENABLE_SANITIZER=ON` and
`<MODULE>_SANITIZER_TYPE=<type>` for every loaded module. It also uses a Debug
build and LTO is skipped for instrumented targets.

## Direct CMake

For an individual library, configure the module-prefixed variables:

```bash
cmake -S . -B build-memory-asan -G Ninja \
  -DXSIGMA_LIBRARY_PROJECT=Memory \
  -DMEMORY_ENABLE_SANITIZER=ON \
  -DMEMORY_SANITIZER_TYPE=address \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build-memory-asan --parallel
ctest --test-dir build-memory-asan --output-on-failure
```

Supported sanitizer types are `address`, `undefined`, `thread`, `memory`, and
`leak`. For a direct whole-project configuration, set the same two variables
for each library being instrumented. Avoid obsolete aggregate variables such as
`PROJECT_ENABLE_SANITIZER` and `PROJECT_SANITIZER_TYPE`.

## Bazel

```bash
python Scripts/setup_bazel.py build.test.debug.asan
python Scripts/setup_bazel.py build.test.debug.ubsan

# Raw Bazel configurations use short names.
bazel test --config=debug --config=asan //Library/...
```

The available Bazel configurations are `asan`, `tsan`, `ubsan`, `msan`, and
`lsan`. The helper also accepts the CMake-style long forms, for example
`--sanitizer.address`.

## Notes

- Use one sanitizer at a time unless the compiler and runtime combination is
  known to support the combination.
- Sanitizer runtimes can affect third-party libraries and test launchers; run
  the complete affected test suite before relying on a configuration.
- Use a separate build directory for instrumented builds.
