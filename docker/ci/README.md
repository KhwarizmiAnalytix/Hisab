# Local CI reproduction

Runs the Linux (`ubuntu-latest` / `ubuntu-24.04-arm`) jobs from
`.github/workflows/ci.yml` in Docker, on your own machine.

```
docker/ci/xci --list                  # every available config key
docker/ci/xci sanitizer-address       # one config
docker/ci/xci bazel-tbb bazel-proj-parallel-openmp   # several
docker/ci/xci --all                   # the whole matrix, pass/fail summary at the end
```

Each key's build lands in `build/<key>/` in the repo (bind-mounted into the
container), so builds are incremental across runs and inspectable from the
host afterwards.

## What's covered

Every CMake and Bazel job family in ci.yml, parameterized the same way the
workflow's own `strategy: matrix:` blocks are — see `jobs.sh` for the
CMake/Bazel invocation of each family and `run.sh` for the full key list.

## What's deliberately out of scope

- **Windows jobs** (`windows-cli-smoke`, the Windows entry in `build-matrix`).
  Docker Desktop on macOS/Linux can't run Windows containers.
- **GPU jobs** (`cmake-gpu-backend-tests` CUDA/HIP/Metal, `bazel-cuda-tests`,
  `bazel-hip-tests`, `bazel-metal-tests`). CUDA/HIP toolkits are too large for
  this image; Metal only exists on macOS. Those jobs live in ci.yml only.
- **macOS matrix entries** (`macos-latest`) in otherwise-covered jobs.
  Apple doesn't license macOS-in-Docker; these can only be run on an actual
  Mac runner, same as CI does today.
- **`sse` / `avx512` vectorization configs** (both the CMake
  `vectorization-simd-backend-tests` and Bazel
  `bazel-vectorization-simd-tests` families). This image is built natively
  for the host architecture — arm64 on Apple Silicon — so it reproduces the
  `neon`/`sve` entries exactly (same architecture as the `ubuntu-24.04-arm`
  runners) but has no meaningful way to exercise x86-only SIMD ISAs without
  QEMU emulation, which this setup doesn't do. `vec-neon`, `vec-sve`,
  `bazel-vec-neon`, `bazel-vec-sve` are covered.

## Known gap: `vec-sve` / `bazel-vec-sve` on non-SVE hosts

`VectorizationCxxTests` built with the real SVE backend can crash with
`Illegal instruction` here even though the build succeeds — Apple Silicon
does not implement ARM SVE at all, so any SVE instruction the compiler
actually emits will trap. This container can validate that the SVE backend
*compiles* correctly but cannot validate that it *runs* correctly. Whether
it also fails on GitHub's `ubuntu-24.04-arm` runners depends on whether
their underlying silicon supports SVE — that can only be confirmed by an
actual CI run, not by this local tool, on an Apple Silicon host.

## Notes

- `JOBS=N docker/ci/xci ...` overrides the build/test parallelism (defaults
  to all host cores; ci.yml itself uses `-j 2` to match GitHub's 2-core
  runners, but there's no reason to throttle locally).
- sccache and Bazel's cache persist across runs in named Docker volumes
  (`xsigma-ci-sccache`, `xsigma-ci-bazel`), not in the bind-mounted repo.
- `docker/ci/xci` rebuilds the image before every run; with Docker's layer
  cache this is a no-op unless `Dockerfile` changed.
