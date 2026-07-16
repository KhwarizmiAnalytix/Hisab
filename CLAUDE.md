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
