# XSigma local CI reproduction image.
#
# Mirrors the toolchain .github/workflows/ci.yml installs on ubuntu-latest /
# ubuntu-24.04-arm runners (see .github/workflows/install/install-deps-ubuntu.sh
# and the per-job "Install dependencies" steps). Built natively for the host
# architecture -- on Apple Silicon that's arm64, matching the ubuntu-24.04-arm
# runner; on an x86_64 host it matches ubuntu-latest.
#
# This image intentionally does not cover Windows or GPU (CUDA/HIP/Metal) jobs
# -- see docker/ci/README.md.
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive \
    LANG=C.UTF-8 \
    LC_ALL=C.UTF-8

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        ninja-build \
        git \
        curl \
        wget \
        ca-certificates \
        gnupg \
        pkg-config \
        python3 \
        python3-pip \
        python3-dev \
        python3-venv \
        clang \
        llvm \
        llvm-dev \
        lld \
        libclang-rt-18-dev \
        gcc \
        g++ \
        libssl-dev \
        zlib1g-dev \
        libnuma-dev \
        libtbb-dev \
        libtbbmalloc2 \
        libomp-dev \
        valgrind \
        cppcheck \
        sccache \
        unzip \
        zip \
    && rm -rf /var/lib/apt/lists/*

# Intel oneAPI TBB (matches the tbb-specific-tests job's extra install step on
# top of libtbb-dev; best-effort -- libtbb-dev above already covers every
# other TBB-enabled config).
RUN (curl -fsSL https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB \
        | gpg --dearmor -o /usr/share/keyrings/oneapi-archive-keyring.gpg \
     && echo "deb [signed-by=/usr/share/keyrings/oneapi-archive-keyring.gpg] https://apt.repos.intel.com/oneapi all main" \
        > /etc/apt/sources.list.d/oneAPI.list \
     && apt-get update \
     && apt-get install -y --no-install-recommends intel-oneapi-tbb-devel \
     && rm -rf /var/lib/apt/lists/*) \
    || echo "WARNING: Intel oneAPI TBB install failed/unreachable; continuing with libtbb-dev only."

# Bazelisk (drives bazel the same way bazel-contrib/setup-bazel does in CI).
RUN arch="$(dpkg --print-architecture)" \
    && case "$arch" in \
         arm64)  bazel_arch=arm64 ;; \
         amd64)  bazel_arch=amd64 ;; \
         *) echo "Unsupported arch: $arch" >&2; exit 1 ;; \
       esac \
    && curl -fsSL -o /usr/local/bin/bazel \
        "https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-${bazel_arch}" \
    && chmod +x /usr/local/bin/bazel

RUN python3 -m pip install --break-system-packages colorama==0.4.6 psutil==6.1.1 pytest

WORKDIR /workspace
