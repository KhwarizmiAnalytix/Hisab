# Building Enzyme and linking C++ on MSYS2 (MinGW64)

This document describes how to build Enzyme against the LLVM/Clang toolchain from MSYS2 and how to **compile and link** a small C++ program that uses `__enzyme_autodiff`.

Use the **MinGW x64** environment (`MSYSTEM=MINGW64`), not the plain “MSYS” shell, so you pick up `mingw64` compilers and libraries. You can open “MSYS2 MinGW x64” from the Start Menu, or from **cmd.exe**:

```bat
C:\msys64\msys2_shell.cmd -mingw64 -defterm -no-start -c "your bash command"
```

Paths below use POSIX style inside the MinGW shell (`/c/...`, `/mingw64/...`).

---

## MSYS2 shells in detail (MSYS vs MinGW64, cmd.exe, Warp)

### Why the “correct shortcut” matters

MSYS2 is not a single Linux-like environment. It ships **several separate environments**, each with its own **`$MSYSTEM`**, **`PATH`**, and **compiler/runtime** layout. The **Start Menu** shortcuts pick one of them before **`bash`** starts.

| Shortcut (typical name) | `MSYSTEM` | Prefix (native tools) | Role |
|-------------------------|-----------|------------------------|------|
| **MSYS2 MSYS** | `MSYS` | `/usr` (MSYS/Cygwin-like) | **Package maintenance**, building MSYS packages. **Not** where you want to build Enzyme with MinGW Clang. |
| **MSYS2 MinGW x64** | `MINGW64` | `/mingw64` | **64-bit MinGW** toolchain (`mingw-w64-x86_64-*` packages). **Use this** for the scripts in this doc unless you standardize on UCRT64/CLANG64. |
| **MSYS2 UCRT64** | `UCRT64` | `/ucrt64` | MinGW with **UCRT**; different package prefix (`mingw-w64-ucrt-x86_64-*`). |
| **MSYS2 CLANG64** | `CLANG64` | `/clang64` | LLVM-based **CLANG64** environment (`mingw-w64-clang-x86_64-*`). |

If you open **plain MSYS**, **`clang++`** may be missing, wrong, or not the MinGW-native one under **`/mingw64/bin`**. **`build-enzyme-msys2-from-scratch.sh`** reads **`$MSYSTEM`** and sets **`MSYS_PREFIX`** (e.g. **`/mingw64`**); that only matches your **PATH** if you actually started **MinGW x64** (or the matching UCRT64/CLANG64 shortcut).

### How to verify you are in the right environment

In the bash prompt, many MSYS2 setups show a colored label such as **`MINGW64`**, **`UCRT64`**, **`CLANG64`**, or **`MSYS`** before **`username@hostname`**. That label reflects **`MSYSTEM`**.

Confirm explicitly:

```bash
echo "$MSYSTEM"
command -v clang++
```

For **MinGW x64** you want **`MSYSTEM=MINGW64`** and **`clang++`** resolving to something like **`/mingw64/bin/clang++`**. If you see **`MSYSTEM=MSYS`** or **`clang++`** under **`/usr/bin`** only, close that window and open **MSYS2 MinGW x64** instead.

### How to launch MinGW64 (and siblings)

**1) Start Menu (simplest)**  
After installing MSYS2, open **“MSYS2 MinGW x64”** (or **UCRT64** / **CLANG64** if you deliberately use those).

**2) Explorer**  
In the MSYS2 install directory (default **`C:\msys64`**), double-click:

- **`mingw64.exe`** → **MINGW64** session  
- **`ucrt64.exe`** → **UCRT64**  
- **`clang64.exe`** → **CLANG64**  
- **`msys2.exe`** → **MSYS** (avoid for Enzyme builds)

**3) Command Prompt (`cmd.exe`)**  
Run the launcher batch file with a **`-mingw64`**, **`-ucrt64`**, or **`-clang64`** flag (not **`-msys`**):

```bat
C:\msys64\msys2_shell.cmd -mingw64
```

That opens an interactive terminal (often **mintty**) already configured for **MINGW64**.

Run **one bash command** without keeping a window open (good for scripts or CI):

```bat
C:\msys64\msys2_shell.cmd -mingw64 -defterm -no-start -c "echo $MSYSTEM && which clang++"
```

Inside **`-c`**, the string is executed by **bash**, so use **Unix syntax** (`$MSYSTEM`, **`&&`**). Adjust **`C:\msys64`** if you installed MSYS2 elsewhere. Use **`-ucrt64`** or **`-clang64`** instead of **`-mingw64`** when matching those environments.

**4) Warp**

[Warp](https://www.warp.dev/) is a terminal app; it needs to start a process that already sets **MSYS2 + MinGW64** (or you end up with plain **`bash.exe`** and the wrong **`PATH`**).

Practical options:

- **Easiest for long builds**  
  Run **MSYS2 MinGW x64** from the Start Menu or **`mingw64.exe`**, and use Warp for editing or other tasks. No Warp configuration required.

- **Integrated MinGW64 inside a Warp tab**  
  Warp’s exact **Settings → Features → Session** (or **Workspace**) labels change between versions; look for **default shell**, **custom shell**, or **startup command**. Goal: start **`cmd.exe`**, which then runs **`msys2_shell.cmd`** with **`-mingw64`**, similar to [MSYS2’s “Integrating with IDEs”](https://www.msys2.org/docs/ides/) pattern for VS Code.

  Example idea (adjust **`C:\msys64`**):

  - **Command:** `C:\Windows\System32\cmd.exe`  
  - **Arguments (illustrative):** `/c`, `C:\msys64\msys2_shell.cmd`, `-mingw64`, `-defterm`, `-full-path`, `-where`, `C:\Users\YourName`

  Or a single string your UI accepts:

  ```text
  cmd.exe /c C:\msys64\msys2_shell.cmd -mingw64 -defterm -full-path
  ```

  Some Warp versions prefer listing the executable and arguments separately; check **Warp documentation** for “custom shell” on Windows.

- **Why not only `C:\msys64\usr\bin\bash.exe`?**  
  Starting **`bash.exe`** directly from Windows often **skips** the logic in **`msys2_shell.cmd`** that sets **`MSYSTEM`**, **`PATH`**, and **`MSYS2_PATH_TYPE`**. You then get **MSYS**-like behavior or a broken **`PATH`**. Prefer **`msys2_shell.cmd -mingw64`** or **`mingw64.exe`**.

If nested **`cmd`** + mintty behaves oddly in Warp, use the **Start Menu** MinGW shell for **`build-enzyme-msys2-from-scratch.sh`**; that path is fully supported.

### Alignment with this document

Sections below assume **MINGW64** and paths like **`/mingw64/lib/cmake/llvm`**. If you standardize on **UCRT64** or **CLANG64**, use the matching shortcut, install the matching **`pacman`** packages, and the from-scratch script will set **`LLVM_DIR`** from **`$MSYSTEM`** automatically.

---

## Changes in this repository (fork / local tree)

These edits are relative to a stock Enzyme checkout and exist so **MSYS2 MinGW** can build usable plugins and so the **C++ smoke test** can link on Windows.

| Area | File | What changed |
|------|------|----------------|
| **LLD link flags** | `enzyme/Enzyme/CMakeLists.txt` | **`LLDEnzymeFlags`** only adds **`-Wl,--load-pass-plugin=…`** when **`NOT MINGW`**. MinGW’s **`ld.lld`** rejects **`--load-pass-plugin`**, so that flag is skipped; **`-Wl,-mllvm -Wl,-load=…`** remains. Without this, linking with **`LLDEnzymeFlags`** fails on MinGW. |
| **Clang plugin build** | `enzyme/Enzyme/CMakeLists.txt` | On **`MINGW`**, **`ClangEnzyme`** gets **`-fno-var-tracking-assignments`** to avoid a known bad interaction when building the Clang plugin on MinGW. |
| **C++ sample (MinGW)** | `enzyme-cpp-link-test/CMakeLists.txt` | On **MinGW**, the example does **not** rely on **`ClangEnzymeFlags`** or **`LLDEnzymeFlags`** alone; it uses **bitcode → `opt -load-pass-plugin=LLVMEnzyme -passes=enzyme` → link**, which is the reliable way to get **`__enzyme_autodiff`** lowered on this platform. |

If you use **upstream Enzyme** without the **`LLDEnzymeFlags`** guard, apply the same **`if (NOT MINGW)`** logic around **`--load-pass-plugin`** in **`enzyme/Enzyme/CMakeLists.txt`** (see the **`LLDEnzymeFlags`** target).

---

## 1. Install dependencies

From a MinGW64 shell:

```bash
pacman -S --needed mingw-w64-x86_64-{llvm,clang,lld,cmake,ninja,libxml2,mpfr} \
  mingw-w64-x86_64-clang-tools-extra
```

- **`lld`** supplies `ld.lld`, which Clang uses when you pass `-fuse-ld=lld`.
- **`clang-tools-extra`** is needed if you want the **Clang** Enzyme plugin (`ClangEnzyme`). It is optional for the **MinGW C++ linking recipe** in this tree, which uses the **IR** plugin (`LLVMEnzyme`) with `opt` instead.

If you use **UCRT64** or **CLANG64**, install the matching `mingw-w64-ucrt-x86_64-*` or `mingw-w64-clang-x86_64-*` packages and point `LLVM_DIR` at `/ucrt64/lib/cmake/llvm` or `/clang64/lib/cmake/llvm` when configuring Enzyme.

---

## 2. LLVM must allow loadable plugins

Enzyme ships **plugins** (`LLVMEnzyme`, `LLDEnzyme`, optionally `ClangEnzyme`). Your LLVM CMake config must report plugins enabled:

```bash
grep LLVM_ENABLE_PLUGINS /mingw64/lib/cmake/llvm/LLVMConfig.cmake
```

You should see `set(LLVM_ENABLE_PLUGINS ON)`. If it is `OFF`, Enzyme cannot build the LLD/LLVM plugins against that LLVM; you would need an LLVM build (or package) built with `-DLLVM_ENABLE_PLUGINS=ON`.

---

## 3. Build and install Enzyme

### Full portable script (recommended on a new machine)

**`enzyme/scripts/build-enzyme-msys2-from-scratch.sh`** is meant to be run on **any fresh MSYS2 PC**. It does **not** assume you already have the Enzyme tree checked out: by default it **`git clone`**s **`https://github.com/EnzymeAD/Enzyme.git`** into **`$HOME/src/Enzyme`** (override with **`ENZYME_SRC`** / **`ENZYME_REPO_URL`**). You can copy only this **`.sh`** file and run it with **`bash /path/to/build-enzyme-msys2-from-scratch.sh`**.

- Installs toolchain packages with **`pacman`** (optional: **`SKIP_PACMAN=1`**)
- **Clones** Enzyme when **`ENZYME_SRC/CMakeLists.txt`** is missing; if that file already exists, it **reuses** the checkout at **`ENZYME_SRC`**
- Creates a repo-local **`.venv-lit`** and **`pip install lit`** unless skipped (**`SKIP_VENV=1`**)
- Picks **`LLVM_DIR`** / **`Clang_DIR`** from **`$MSYSTEM`** (**MINGW64**, **UCRT64**, **CLANG64**)
- Configures with **Ninja**, **`ENZYME_ENABLE_PLUGINS=ON`**, builds, runs tests, installs to **`INSTALL_PREFIX`**, then **removes `BUILD_DIR`** (tests run first because they need the build tree)

#### How to run `build-enzyme-msys2-from-scratch.sh`

1. **Install [MSYS2](https://www.msys2.org/)** if needed. In any MSYS2 shell, run **`pacman -Syu`** until there are no more updates; restart the shell when the installer tells you to.

2. **Open the correct MSYS2 environment** — e.g. **“MSYS2 MinGW x64”** (not **“MSYS2 MSYS”**). Confirm **`echo $MSYSTEM`** prints **`MINGW64`** (or **`UCRT64`** / **`CLANG64`** if you use those). See **[MSYS2 shells in detail](#msys2-shells-in-detail-msys-vs-mingw64-cmdexe-warp)** for why this matters and how to launch from **cmd.exe** or **Warp**.

3. **Put the script somewhere convenient** (optional). You can run it from a checkout as **`./enzyme/scripts/build-enzyme-msys2-from-scratch.sh`**, or copy **`build-enzyme-msys2-from-scratch.sh`** to e.g. **`$HOME/bin`** and run it by path. **`cd`** is **not** required: the script clones into **`ENZYME_SRC`** by default.

4. **Allow execution** (once per file location):

   ```bash
   chmod +x /c/YourPath/build-enzyme-msys2-from-scratch.sh
   ```

5. **Administrator rights (default install only)** — the script installs to **`C:\Program Files (x86)\Enzyme`** by default. If **`cmake --install`** fails with “Permission denied”, close the shell, open **“MSYS2 MinGW x64”** with **Run as administrator**, and run the script again. To install somewhere under your user profile instead, set e.g. **`export INSTALL_PREFIX="$HOME/enzyme-install"`** before step 6.

6. **Run the script** (clones to **`$HOME/src/Enzyme`** on first run unless **`ENZYME_SRC`** already contains Enzyme’s **`CMakeLists.txt`**):

   ```bash
   bash /c/YourPath/to/build-enzyme-msys2-from-scratch.sh
   ```

   On the first run it may install many **`pacman`** packages, **clone** Enzyme, create **`$ENZYME_SRC/.venv-lit`** for **lit**, then configure, build, run **`check-*`** targets, install artifacts, and delete the build directory **`$ENZYME_SRC/build-$MSYSTEM-enzyme`**. Success ends with **`OK: Enzyme built and installed.`** and **`EnzymeConfig.cmake`** at **`$INSTALL_PREFIX/CMake/EnzymeConfig.cmake`** (default: **`C:\Program Files (x86)\Enzyme\CMake\EnzymeConfig.cmake`**).

7. **Optional flags** — see the environment table below (e.g. **`SKIP_TESTS=1`**, **`KEEP_BUILD_DIR=1`**, **`SKIP_PACMAN=1`**).

**Run from Windows cmd.exe** (still uses MinGW bash inside MSYS2):

```bat
C:\msys64\msys2_shell.cmd -mingw64 -defterm -no-start -c "bash /c/YourPath/to/build-enzyme-msys2-from-scratch.sh"
```

Change **`C:\msys64`** if MSYS2 is installed elsewhere; use **`-ucrt64`** or **`-clang64`** instead of **`-mingw64`** if that matches your setup.

**Fork or custom clone location** — set **`ENZYME_REPO_URL`** and/or **`ENZYME_SRC`** before running:

```bash
export ENZYME_REPO_URL=https://github.com/you/Enzyme.git
export ENZYME_SRC=/c/src/Enzyme
bash /c/path/to/build-enzyme-msys2-from-scratch.sh
```

**Rebuild without re-cloning** — point **`ENZYME_SRC`** at an existing Enzyme root (directory whose top-level **`CMakeLists.txt`** is the Enzyme project):

```bash
export ENZYME_SRC=/c/LLVM/Enzyme2/Enzyme-0.0.256/enzyme
bash /c/path/to/build-enzyme-msys2-from-scratch.sh
```

| Variable | Meaning |
|----------|---------|
| `ENZYME_SRC` | Enzyme source root. Default: **`$HOME/src/Enzyme`**. If **`$ENZYME_SRC/CMakeLists.txt`** exists, that checkout is **reused**; otherwise the script **clones** into **`ENZYME_SRC`**. |
| `ENZYME_REPO_URL` | Git URL for **`git clone`** when **`ENZYME_SRC`** is not already an Enzyme root (default: **`https://github.com/EnzymeAD/Enzyme.git`**) |
| `ENZYME_GIT_BRANCH` | Branch or tag to clone (**default: `v0.0.256`**). Set **`ENZYME_GIT_BRANCH=`** (empty) before running to clone the remote **default branch** instead; use **`main`** or another tag to override explicitly |
| `ENZYME_SHALLOW_CLONE=1` | Use **`git --depth 1`** (default is **off**: full clone, more reliable on Windows) |
| `BUILD_DIR` | Default: **`$ENZYME_SRC/build-$MSYSTEM-enzyme`** |
| `INSTALL_PREFIX` | Default: **`/c/Program Files (x86)/Enzyme`** (override with **`INSTALL_PREFIX=...`**; may need an elevated shell) |
| `LLVM_DIR` | Default: **`$MSYS_PREFIX/lib/cmake/llvm`** (e.g. **`/mingw64/lib/cmake/llvm`**) |
| `SKIP_PACMAN=1` | Do not run **`pacman`** |
| `SKIP_VENV=1` | Do not create **`.venv-lit`** / install **lit** |
| `SKIP_INSTALL=1` | Build only; leaves **`BUILD_DIR`** on disk |
| `SKIP_TESTS=1` | Skip **`check-*`** targets |
| `KEEP_BUILD_DIR=1` | After **`cmake --install`**, keep **`BUILD_DIR`** (default: script deletes it) |
| `ENZYME_CLANG=OFF` | Force Clang plugin off |
| `LLVM_EXTERNAL_LIT` | Path to **lit** for CMake |

### Shorter script (fixed MinGW64 paths)

**`enzyme/scripts/build-install-test-msys2-mingw64.sh`** does the same configure/build/install/test flow with defaults hard-coded to **`/mingw64`**. Use it if you always use **MINGW64** and already have dependencies installed.

| Variable | Meaning |
|----------|---------|
| `ENZYME_SRC` | Root of the Enzyme source tree (default: parent of `scripts/`) |
| `BUILD_DIR` | CMake build directory (default: `$ENZYME_SRC/build-msys2-mingw64`) |
| `INSTALL_PREFIX` | Install prefix (default: `$ENZYME_SRC/install-msys2-mingw64`) |
| `LLVM_DIR` | `LLVMConfig.cmake` directory (default: `/mingw64/lib/cmake/llvm`) |
| `SKIP_INSTALL=1` | Build only; skip `cmake --install` |
| `SKIP_TESTS=1` | Skip `ninja check-*` targets |
| `ENZYME_CLANG=OFF` | Build without the Clang plugin |

### Manual CMake (equivalent idea)

```bash
cmake -G Ninja -S /path/to/enzyme -B /path/to/enzyme/build-msys2-mingw64 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_INSTALL_PREFIX=/path/to/enzyme/install-msys2-mingw64 \
  -DLLVM_DIR=/mingw64/lib/cmake/llvm \
  -DClang_DIR=/mingw64/lib/cmake/clang \
  -DENZYME_ENABLE_PLUGINS=ON

cmake --build /path/to/enzyme/build-msys2-mingw64 --parallel
cmake --install /path/to/enzyme/build-msys2-mingw64 --prefix /path/to/enzyme/install-msys2-mingw64
```

After install you should have, under the install prefix, **`lib/LLVMEnzyme-<llvm-major>.dll`**, **`lib/LLDEnzyme-<llvm-major>.dll`**, and (if enabled) **`lib/ClangEnzyme-<llvm-major>.dll`**, plus **`CMake/EnzymeConfig.cmake`** for `find_package(Enzyme)`.

---

## 4. Linking your own C++ code on MinGW

On Linux and many Unix setups, Enzyme’s CMake package exposes **`LLDEnzymeFlags`**: you link with `lld` and pass the Enzyme pass at link time. On **MinGW**, that path is fragile (for example, `__enzyme_autodiff` can remain undefined, and the Clang plugin has Windows/VFS issues).

The supported **MinGW** workflow in this repository’s **`enzyme-cpp-link-test`** example is:

1. **Compile to LLVM bitcode** (no Enzyme yet):

   ```bash
   clang++ -O0 -emit-llvm -c main.cpp -o main.bc
   ```

2. **Run the Enzyme IR pass** with `opt` and the installed **`LLVMEnzyme`** plugin:

   ```bash
   opt -load-pass-plugin=/path/to/install/lib/LLVMEnzyme-21.dll \
       -passes=enzyme main.bc -o main_opt.ll
   ```

   Replace `21` with your LLVM major version; use the `.dll` path from your Enzyme install `lib/` directory.

3. **Link** the optimized IR into an executable with **GNU** `clang++` and **lld**:

   ```bash
   clang++ -fuse-ld=lld main_opt.ll -o example.exe
   ```

At **run time**, the executable normally only needs the usual MinGW runtime plus anything your code links; if a tool loads `LLVMEnzyme` (e.g. `opt` during the build), ensure **`PATH`** includes the Enzyme install `lib` folder and `/mingw64/bin` so `LLVMEnzyme-*.dll` and `libLLVM-*.dll` resolve.

### Use the CMake example

The **`enzyme-cpp-link-test`** project automates the three steps above when **`MINGW`** is true. Point **`Enzyme_DIR`** at your install’s **`CMake`** directory (the folder that contains **`EnzymeConfig.cmake`**), then:

```bash
cd /path/to/enzyme-cpp-link-test
cmake -B build-mingw -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DEnzyme_DIR=/path/to/enzyme/install-msys2-mingw64/CMake
cmake --build build-mingw
ctest --test-dir build-mingw --output-on-failure
./build-mingw/example.exe 20
```

Default `Enzyme_DIR` in that `CMakeLists.txt` is a sibling **`../enzyme/install-msys2-mingw64/CMake`**; override `-DEnzyme_DIR=...` if your layout differs.

---

## 5. Compiler driver notes (Windows)

- Prefer **`clang.exe` / `clang++.exe`** (GNU driver) over **`clang-cl`**. The MSVC-style linker does not honor the same `-Wl,-mllvm` / plugin flags Enzyme uses on other platforms.
- **`enzyme-cpp-link-test`** sets `CMAKE_CXX_COMPILER` to **`${Enzyme_LLVM_BINARY_DIR}/bin/clang++.exe`** from `find_package(Enzyme)` so the toolchain matches the Enzyme/LLVM you installed.

---

## 6. Troubleshooting

| Symptom | Likely cause / fix |
|--------|---------------------|
| `invalid linker name in argument '-fuse-ld=lld'` | Install **`mingw-w64-x86_64-lld`** and ensure `ld.lld.exe` is on `PATH`. |
| `undefined symbol: __enzyme_autodiff` after link | Enzyme did not run on the TU that contains the call. On MinGW, use the **`opt -load-pass-plugin=... -passes=enzyme`** pipeline (or the `enzyme-cpp-link-test` CMake logic), not only plain compile+link with LLD flags. |
| `lld: unknown argument: --load-pass-plugin=...` | MinGW `ld.lld` may not accept that flag; Enzyme’s installed CMake targets adjust flags per platform—prefer **`LLVMEnzyme` + `opt`** for MinGW C++ as in this doc. |
| Clang plugin assert / crash on Windows | The Clang plugin path is not the recommended MinGW story here; use **`LLVMEnzyme`** + **`opt`** for IR lowering. |
| **`CMakeLists.txt` missing after clone** (`build-enzyme-msys2-from-scratch.sh`) | **`rm -rf "$ENZYME_SRC"`** and rerun (script defaults to a **full** clone, not shallow). Ensure **`ENZYME_REPO_URL`** is the real **Enzyme** repo (root **`CMakeLists.txt`**), not a wrapper monorepo—if your layout is **`…/enzyme/CMakeLists.txt`**, the script now detects that. Use MinGW64 shell; **`which git`** should prefer **`/usr/bin/git`** when present. |

---

## 7. Further reading

- **Portable full build:** `enzyme/scripts/build-enzyme-msys2-from-scratch.sh`
- **MinGW64-focused build + tests:** `enzyme/scripts/build-install-test-msys2-mingw64.sh`
- **Minimal C++ sample and CMake (MinGW vs non-MinGW):** `enzyme-cpp-link-test/`

If you use **`build-enzyme-msys2-from-scratch.sh`**, point **`enzyme-cpp-link-test`** at your install with **`-DEnzyme_DIR=$INSTALL_PREFIX/CMake`** (default prefix: **`C:\Program Files (x86)\Enzyme\CMake`**).
