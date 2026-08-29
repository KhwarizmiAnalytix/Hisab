#!/bin/bash
# Install the CUDA toolkit on Ubuntu so MEMORY_GPU_BACKEND=cuda / Bazel --config=cuda
# can configure and compile. GitHub-hosted runners have no NVIDIA GPU; tests that
# need a device skip (see TestCudaCachingAllocator).
set -euo pipefail

SUDO=""
if [ "$(id -u)" -ne 0 ]; then
    SUDO="sudo"
fi

# Hosted runners have the toolkit but no driver. Binaries DT_NEEDED libcuda.so.1
# (CUDA::cuda_driver / libcudart). The stub is often only libcuda.so, and on
# CUDA 12 it lives under targets/x86_64-linux/lib/stubs, not lib64/stubs.
ensure_cuda_driver_stub() {
    local stubs="" d found
    for d in \
        /usr/local/cuda/lib64/stubs \
        /usr/local/cuda/targets/x86_64-linux/lib/stubs \
        /usr/local/cuda-12.6/lib64/stubs \
        /usr/local/cuda-12.6/targets/x86_64-linux/lib/stubs \
        /usr/lib/x86_64-linux-gnu/stubs
    do
        if [ -e "${d}/libcuda.so" ] || [ -e "${d}/libcuda.so.1" ]; then
            stubs="${d}"
            break
        fi
    done
    if [ -z "${stubs}" ]; then
        found="$(find /usr/local/cuda /usr/local/cuda-* /usr/lib -name 'libcuda.so' \
            -path '*/stubs/*' 2>/dev/null | head -n 1 || true)"
        if [ -n "${found}" ]; then
            stubs="$(dirname "${found}")"
        fi
    fi
    if [ -z "${stubs}" ]; then
        echo "warning: CUDA driver stub (libcuda.so) not found; tests may fail to load" >&2
        return 0
    fi
    if [ -e "${stubs}/libcuda.so" ] && [ ! -e "${stubs}/libcuda.so.1" ]; then
        echo "Creating ${stubs}/libcuda.so.1 -> libcuda.so"
        if [ -w "${stubs}" ]; then
            ln -sf libcuda.so "${stubs}/libcuda.so.1"
        else
            $SUDO ln -sf libcuda.so "${stubs}/libcuda.so.1"
        fi
    fi
    echo "CUDA driver stubs: ${stubs}"
    echo "LD_LIBRARY_PATH=${stubs}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}" >> "${GITHUB_ENV:-/dev/null}"
    export LD_LIBRARY_PATH="${stubs}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
}

if ! command -v nvcc >/dev/null 2>&1; then
    . /etc/os-release
    case "${VERSION_ID}" in
        22.04) repo_distro=ubuntu2204 ;;
        24.04) repo_distro=ubuntu2404 ;;
        *)     repo_distro=ubuntu2404 ;;
    esac

    echo "Installing CUDA toolkit (cuda-toolkit-12-6) for ${repo_distro}..."
    $SUDO apt-get update
    $SUDO apt-get install -y wget ca-certificates gnupg
    wget -q "https://developer.download.nvidia.com/compute/cuda/repos/${repo_distro}/x86_64/cuda-keyring_1.1-1_all.deb" \
        -O /tmp/cuda-keyring.deb
    $SUDO dpkg -i /tmp/cuda-keyring.deb
    $SUDO apt-get update
    $SUDO apt-get install -y cuda-toolkit-12-6
else
    echo "nvcc already available: $(nvcc --version | tail -n 1)"
fi

if [ -d /usr/local/cuda/bin ]; then
    echo "/usr/local/cuda/bin" >> "${GITHUB_PATH:-/dev/null}"
    export PATH="/usr/local/cuda/bin:${PATH}"
    echo "CUDA_PATH=/usr/local/cuda" >> "${GITHUB_ENV:-/dev/null}"
    echo "CUDA_HOME=/usr/local/cuda" >> "${GITHUB_ENV:-/dev/null}"
    export CUDA_PATH="/usr/local/cuda"
    export CUDA_HOME="/usr/local/cuda"
fi

ensure_cuda_driver_stub
nvcc --version
