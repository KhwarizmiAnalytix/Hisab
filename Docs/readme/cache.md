# Compiler Caching

XSigma configures compiler-cache launchers per library module. Each module
defaults to `<MODULE>_ENABLE_CACHE=ON` and
`<MODULE>_CACHE_BACKEND=none`. Supported backends are `none`, `ccache`,
`sccache`, and `buildcache`.

## Recommended workflow

```bash
python Scripts/setup.py config.build.ninja.clang.ccache
python Scripts/setup.py config.build.ninja.clang.sccache
python Scripts/setup.py config.build.ninja.clang.buildcache
python Scripts/setup.py config.build.ninja.clang.none
```

The helper fans the selected backend to the loaded library modules. The
`cache` token is an inverse toggle: it disables the per-module cache launcher.

## Direct CMake

```bash
# Core-only ccache configuration.
cmake -S . -B build-core -G Ninja \
  -DXSIGMA_LIBRARY_PROJECT=Core \
  -DCORE_ENABLE_CACHE=ON \
  -DCORE_CACHE_BACKEND=ccache

# Disable caching for one module.
cmake -S . -B build-memory -G Ninja \
  -DXSIGMA_LIBRARY_PROJECT=Memory \
  -DMEMORY_ENABLE_CACHE=OFF
```

For whole-project direct configurations, set the matching `*_ENABLE_CACHE` and
`*_CACHE_BACKEND` variables for every loaded module. There are no
`PROJECT_CACHE_BACKEND`, `PROJECT_ENABLE_CACHE`, or `XSIGMA_CACHE_BACKEND`
variables in the current build.

## Notes

- Install the selected cache executable and make it available on `PATH` before
  configuring.
- Cache-hit rates depend on stable compiler paths, command lines, and build
  directories.
- The LTO module rejects `sccache` for an LTO-enabled target; select another
  backend or turn LTO off for that target.
- Manage cache storage with the cache tool's own documented commands and
  environment variables.

## Bazel

Bazel's local and remote caches are configured through Bazel options, not the
CMake module cache variables. See [the Bazel guide](bazel.md) for the supported
Bazel workflow.
