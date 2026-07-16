# XSigma

## Hard rules

- **Never modify anything under `ThirdParty/`.** These are vendored git
  submodules (fmt, googletest, mimalloc, benchmark, sleef, kineto, dynolog,
  etc.) and must stay pristine — no source edits, no patches applied in
  place, under any condition, even to fix a build error that traces back to
  vendored code.
  - If a build/compile issue is caused by code inside `ThirdParty/`, fix it
    from our own side instead: an ADL shim / compat header in the consuming
    library (see `Library/Vectorization/Testing/Cxx/cuda_fmt_int128_fix.h`
    for an example — it supplies a missing `fmt::detail::operator~` via ADL
    rather than editing `ThirdParty/fmt`), a force-included header
    (`-include`), extra compile definitions, or a CMake-level guard/exclusion.
  - If no such workaround is possible without editing vendored code, stop and
    ask the user before touching anything in `ThirdParty/`.

## Configuring, building, and testing

- **Use `Scripts/setup.py` to configure, build, and test** — do not invoke
  CMake/ninja/ctest directly. Run it from the `Scripts/` directory, e.g.:
  ```
  cd Scripts
  python3 setup.py build.TEST.native.avx2.vv.torch.benchmark.cuda.config --project.vectorization --packet-size=8
  ```
  - Dotted tokens (`config`, `build`, `test`, plus compiler/generator/feature
    flags like `native`, `avx2`, `torch`, `benchmark`, `cuda`) are chained
    together and are case-insensitive.
  - `--project.NAME` restricts the build to a single library (e.g.
    `--project.vectorization`) instead of the whole tree.
  - `--packet-size=N` sets the SIMD lane count
    (`VECTORIZATION_PACKET_SIZE`, default 4).
  - Run `python3 setup.py --help` from `Scripts/` for the full list of flags
    (sanitizers, logging backend, coverage, spell check, etc.).
