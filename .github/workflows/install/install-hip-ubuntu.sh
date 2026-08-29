#!/bin/bash
# Install HIP/ROCm headers + hipcc on Ubuntu so MEMORY_GPU_BACKEND=hip can
# configure and compile. Hosted runners have no AMD GPU; CMAKE_HIP_ARCHITECTURES
# is set to a fixed gfx target in the CI job (native detection would fail).
set -euo pipefail

SUDO=""
if [ "$(id -u)" -ne 0 ]; then
    SUDO="sudo"
fi

# Real SDK root: hip-config.cmake and/or hip_runtime.h. /usr is NOT an SDK —
# apt's hipcc alternatives wrapper lives in /usr/bin, so dirname(hipcc)=/usr.
is_rocm_root() {
    local r="${1:-}"
    case "${r}" in
        "" | / | /usr | /usr/local) return 1 ;;
    esac
    [ -f "${r}/lib/cmake/hip/hip-config.cmake" ] \
        || [ -f "${r}/lib64/cmake/hip/hip-config.cmake" ] \
        || [ -f "${r}/hip/lib/cmake/hip/hip-config.cmake" ] \
        || [ -f "${r}/include/hip/hip_runtime.h" ] \
        || [ -f "${r}/hip/include/hip/hip_runtime.h" ]
}

hip_env_is_sdk() {
    is_rocm_root "${HIP_PATH:-}" || is_rocm_root "${ROCM_PATH:-}"
}

if command -v hipcc >/dev/null 2>&1 && hip_env_is_sdk; then
    echo "hipcc already available: $(hipcc --version | head -n 1)"
    echo "ROCm root: ${ROCM_PATH:-${HIP_PATH}}"
    exit 0
fi

. /etc/os-release
case "${VERSION_ID}" in
    22.04) repo_codename=jammy ;;
    24.04) repo_codename=noble ;;
    *)     repo_codename=noble ;;
esac

echo "Installing ROCm/HIP SDK (usecase=hiplibsdk) for ${repo_codename}..."
$SUDO apt-get update
$SUDO apt-get install -y wget ca-certificates gnupg

# Official AMD installer package (no DKMS / no kernel driver — compile-only).
# usecase=hip is runtime-only and does not ship hipcc; hiplibsdk adds the
# compiler, headers, and device libs needed to configure MEMORY_GPU_BACKEND=hip.
amdgpu_install_deb="amdgpu-install_6.3.60303-1_all.deb"
wget -q "https://repo.radeon.com/amdgpu-install/6.3.3/ubuntu/${repo_codename}/${amdgpu_install_deb}" \
    -O "/tmp/${amdgpu_install_deb}"
$SUDO apt-get install -y "/tmp/${amdgpu_install_deb}"
$SUDO amdgpu-install --usecase=hiplibsdk --no-dkms --no-32 -y

# Prefer /opt/rocm* over dirname(/usr/bin/hipcc) which resolves to /usr.
rocm_root=""
shopt -s nullglob
for cand in /opt/rocm /opt/rocm-*; do
    if is_rocm_root "${cand}"; then
        rocm_root="$(cd "${cand}" && pwd)"
        break
    fi
done
shopt -u nullglob

if [ -z "${rocm_root}" ] && command -v hipconfig >/dev/null 2>&1; then
    hp="$(hipconfig --rocmpath 2>/dev/null || hipconfig --path 2>/dev/null || true)"
    if is_rocm_root "${hp}"; then
        rocm_root="${hp}"
    fi
fi

if [ -z "${rocm_root}" ]; then
    echo "error: ROCm SDK root not found after install (need hip-config.cmake or hip_runtime.h)" >&2
    ls -la /opt/rocm* 2>/dev/null || true
    command -v hipcc && echo "hipcc=$(command -v hipcc)"
    exit 1
fi

rocm_bin=""
if [ -x "${rocm_root}/bin/hipcc" ]; then
    rocm_bin="${rocm_root}/bin"
elif command -v hipcc >/dev/null 2>&1; then
    rocm_bin="$(dirname "$(command -v hipcc)")"
fi

if [ -n "${rocm_bin}" ]; then
    echo "${rocm_bin}" >> "${GITHUB_PATH:-/dev/null}"
fi
echo "ROCM_PATH=${rocm_root}" >> "${GITHUB_ENV:-/dev/null}"
echo "HIP_PATH=${rocm_root}" >> "${GITHUB_ENV:-/dev/null}"
echo "CMAKE_PREFIX_PATH=${rocm_root}${CMAKE_PREFIX_PATH:+:${CMAKE_PREFIX_PATH}}" >> "${GITHUB_ENV:-/dev/null}"
export PATH="${rocm_bin:+${rocm_bin}:}${PATH}"
export ROCM_PATH="${rocm_root}"
export HIP_PATH="${rocm_root}"
export CMAKE_PREFIX_PATH="${rocm_root}${CMAKE_PREFIX_PATH:+:${CMAKE_PREFIX_PATH}}"
_rocm_libs=""
if [ -d "${rocm_root}/lib" ]; then
    _rocm_libs="${rocm_root}/lib"
fi
if [ -d "${rocm_root}/lib64" ]; then
    _rocm_libs="${rocm_root}/lib64${_rocm_libs:+:${_rocm_libs}}"
fi
if [ -n "${_rocm_libs}" ]; then
    echo "LD_LIBRARY_PATH=${_rocm_libs}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}" >> "${GITHUB_ENV:-/dev/null}"
    export LD_LIBRARY_PATH="${_rocm_libs}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
fi
unset _rocm_libs

# hipcc 6.x defaults to /usr/lib/llvm/bin/clang++ which is not shipped by
# hiplibsdk. Point it at ROCm's LLVM (or amdclang++) before calling hipcc.
hip_clang=""
for d in \
    "${rocm_root}/llvm/bin" \
    "${rocm_root}/lib/llvm/bin" \
    /opt/rocm/llvm/bin \
    /usr/lib/llvm-20/bin \
    /usr/lib/llvm-19/bin \
    /usr/lib/llvm-18/bin; do
    if [ -x "${d}/clang++" ]; then
        hip_clang="${d}"
        break
    fi
done
if [ -z "${hip_clang}" ] && command -v amdclang++ >/dev/null 2>&1; then
    hip_clang="$(dirname "$(command -v amdclang++)")"
fi
if [ -z "${hip_clang}" ]; then
    found="$(find "${rocm_root}" \( -name clang++ -o -name amdclang++ \) -type f 2>/dev/null | head -n 1)"
    if [ -n "${found}" ]; then
        hip_clang="$(dirname "${found}")"
    fi
fi
if [ -n "${hip_clang}" ]; then
    echo "HIP_CLANG_PATH=${hip_clang}" >> "${GITHUB_ENV:-/dev/null}"
    export HIP_CLANG_PATH="${hip_clang}"
    clangxx_src="${hip_clang}/clang++"
    if [ ! -x "${clangxx_src}" ] && [ -x "${hip_clang}/amdclang++" ]; then
        clangxx_src="${hip_clang}/amdclang++"
    fi
    echo "HIPCXX=${clangxx_src}" >> "${GITHUB_ENV:-/dev/null}"
    export HIPCXX="${clangxx_src}"
    if [ ! -x /usr/lib/llvm/bin/clang++ ]; then
        $SUDO mkdir -p /usr/lib/llvm/bin
        $SUDO ln -sf "${clangxx_src}" /usr/lib/llvm/bin/clang++
        clang_src="${hip_clang}/clang"
        if [ ! -x "${clang_src}" ] && [ -x "${hip_clang}/amdclang" ]; then
            clang_src="${hip_clang}/amdclang"
        fi
        if [ -x "${clang_src}" ]; then
            $SUDO ln -sf "${clang_src}" /usr/lib/llvm/bin/clang
        fi
    fi
fi

echo "HIP_PATH=${HIP_PATH} ROCM_PATH=${ROCM_PATH}"
hipcc --version
