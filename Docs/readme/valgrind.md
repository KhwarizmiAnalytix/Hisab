# Valgrind

Valgrind support is configured per CMake library module. It is intended for
supported Unix environments; use platform-native diagnostic tools elsewhere.

## Recommended workflow

```bash
python Scripts/setup.py config.build.test.ninja.clang.debug.valgrind

# Restrict the configuration to one module and its dependencies.
python Scripts/setup.py config.build.test.ninja.clang.debug.valgrind --project.memory
```

The helper fans `valgrind` into `<MODULE>_ENABLE_VALGRIND=ON` for the loaded
modules. It does not use an `XSIGMA_ENABLE_VALGRIND` global flag.

## Direct CMake

```bash
cmake -S . -B build-memory-valgrind -G Ninja \
  -DXSIGMA_LIBRARY_PROJECT=Memory \
  -DMEMORY_ENABLE_VALGRIND=ON \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build-memory-valgrind --parallel
ctest --test-dir build-memory-valgrind -T memcheck
```

For whole-project direct CMake builds, set the matching
`<MODULE>_ENABLE_VALGRIND` option for every selected module.

## Notes

- Do not combine Valgrind and compiler sanitizers unless the specific workflow
  has been validated; they are generally alternative diagnostics.
- Ensure `valgrind` is available on `PATH` before configuring.
- Use `ctest --test-dir <build> -T memcheck` to execute the CTest memcheck
  dashboard step when the build has been configured for it.
