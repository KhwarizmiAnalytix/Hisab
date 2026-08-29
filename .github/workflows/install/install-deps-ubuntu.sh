#!/bin/bash
# XSigma CI - Ubuntu/Linux Dependency Installation Script
# This script installs all required dependencies for building XSigma on Ubuntu/Linux
# Usage: ./install-deps-ubuntu.sh [--with-cuda] [--with-hip] [--with-tbb] [--with-memkind]

set -e  # Exit on error

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Parse command line arguments
WITH_CUDA=false
WITH_HIP=false
WITH_TBB=false
WITH_MEMKIND=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --with-cuda)
            WITH_CUDA=true
            shift
            ;;
        --with-hip)
            WITH_HIP=true
            shift
            ;;
        --with-tbb)
            WITH_TBB=true
            shift
            ;;
        --with-memkind)
            WITH_MEMKIND=true
            shift
            ;;
        --with-cuda=ON|--with-cuda=true)
            WITH_CUDA=true
            shift
            ;;
        --with-cuda=OFF|--with-cuda=false)
            WITH_CUDA=false
            shift
            ;;
        --with-tbb=ON|--with-tbb=true)
            WITH_TBB=true
            shift
            ;;
        --with-tbb=OFF|--with-tbb=false)
            WITH_TBB=false
            shift
            ;;
        *)
            log_warning "Unknown option: $1"
            shift
            ;;
    esac
done

log_info "Starting Ubuntu/Linux dependency installation..."
log_info "CUDA support: $WITH_CUDA"
log_info "TBB support: $WITH_TBB"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if command -v python3 >/dev/null 2>&1; then
    log_info "Sanitizing cached ThirdParty CMake trees (workspace path check)..."
    python3 "${SCRIPT_DIR}/sanitize_thirdparty_cache.py" || log_warning "ThirdParty cache sanitize skipped"
fi

# Update package manager
log_info "Updating package manager..."
sudo apt-get update || {
    log_error "Failed to update package manager"
    exit 1
}

# Core build tools
log_info "Installing core build tools..."
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    curl \
    wget \
    pkg-config \
    python3 \
    python3-pip \
    python3-dev \
    sccache \
    || {
        log_error "Failed to install core build tools"
        exit 1
    }

# Clang compiler
if command -v clang >/dev/null 2>&1 && command -v clang++ >/dev/null 2>&1; then
    log_info "Detected existing Clang toolchain ($(clang --version | head -n 1)); skipping package install."
else
    log_info "Installing Clang compiler..."
    sudo apt-get install -y \
        clang \
        clang++ \
        llvm \
        llvm-dev \
        || {
            log_error "Failed to install Clang"
            exit 1
        }
fi

# GCC compiler (for compiler version testing)
log_info "Installing GCC compiler..."
sudo apt-get install -y \
    gcc \
    g++ \
    || {
        log_error "Failed to install GCC"
        exit 1
    }

# Common libraries
log_info "Installing common libraries..."
sudo apt-get install -y \
    libssl-dev \
    zlib1g-dev \
    libnuma-dev \
    || {
        log_error "Failed to install common libraries"
        exit 1
    }

# TBB (Threading Building Blocks)
if [ "$WITH_TBB" = true ]; then
    log_info "Installing Intel TBB..."
    sudo apt-get install -y \
        libtbb-dev \
        libtbbmalloc2 \
        || {
            log_warning "Failed to install TBB from package manager"
        }
fi

# Extra Linux-only feature libraries
log_info "Installing optional feature libraries (OpenMP, memkind, clang-tidy, ccache)..."
sudo apt-get install -y \
    libomp-dev \
    ccache \
    clang-tidy \
    clang-tools \
    || log_warning "Some optional feature packages failed to install"

if [ "$WITH_MEMKIND" = true ]; then
    log_info "Installing memkind..."
    sudo apt-get install -y libmemkind-dev || log_warning "libmemkind-dev not available"
fi

# CUDA Toolkit (optional) — compile-only on hosted runners (no GPU)
if [ "$WITH_CUDA" = true ]; then
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    if [ -f "${SCRIPT_DIR}/install-cuda-ubuntu.sh" ]; then
        chmod +x "${SCRIPT_DIR}/install-cuda-ubuntu.sh"
        "${SCRIPT_DIR}/install-cuda-ubuntu.sh"
    else
        log_warning "install-cuda-ubuntu.sh not found; skipping CUDA toolkit install"
    fi
fi

# HIP / ROCm (optional) — compile-only on hosted runners (no GPU)
if [ "$WITH_HIP" = true ]; then
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    if [ -f "${SCRIPT_DIR}/install-hip-ubuntu.sh" ]; then
        chmod +x "${SCRIPT_DIR}/install-hip-ubuntu.sh"
        "${SCRIPT_DIR}/install-hip-ubuntu.sh"
    else
        log_warning "install-hip-ubuntu.sh not found; skipping HIP toolkit install"
    fi
fi

# Python dependencies
log_info "Installing Python dependencies..."
pip3 install --upgrade pip setuptools wheel || {
    log_warning "Failed to upgrade pip"
}

pip3 install colorama==0.4.6 psutil==6.1.1 || {
    log_warning "Failed to install Python dependencies"
}

log_success "Ubuntu/Linux dependency installation completed successfully!"
log_info "You can now build XSigma using: python Scripts/setup.py ninja clang config build test"
