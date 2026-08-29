#!/bin/bash
# XSigma CI - macOS Dependency Installation Script
# This script installs all required dependencies for building XSigma on macOS
# Usage: ./install-deps-macos.sh [--with-cuda] [--with-tbb]

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
WITH_TBB=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --with-cuda)
            WITH_CUDA=true
            shift
            ;;
        --with-tbb)
            WITH_TBB=true
            shift
            ;;
        *)
            log_warning "Unknown option: $1"
            shift
            ;;
    esac
done

log_info "Starting macOS dependency installation..."
log_info "CUDA support: $WITH_CUDA"
log_info "TBB support: $WITH_TBB"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if command -v python3 >/dev/null 2>&1; then
    log_info "Sanitizing cached ThirdParty CMake trees (workspace path check)..."
    python3 "${SCRIPT_DIR}/sanitize_thirdparty_cache.py" || log_warning "ThirdParty cache sanitize skipped"
fi

# Check if Homebrew is installed
if ! command -v brew &> /dev/null; then
    log_error "Homebrew is not installed. Please install Homebrew first:"
    log_info "  /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
    exit 1
fi

log_info "Updating Homebrew..."
brew update || {
    log_warning "Failed to update Homebrew"
}

# Core build tools
log_info "Installing core build tools..."
brew install \
    cmake \
    ninja \
    git \
    curl \
    wget \
    pkg-config \
    python@3 \
    sccache \
    || {
        log_error "Failed to install core build tools"
        exit 1
    }

# Clang compiler (usually comes with Xcode)
log_info "Checking for Clang compiler..."
if ! command -v clang++ &> /dev/null; then
    log_warning "Clang not found. Installing Xcode Command Line Tools..."
    xcode-select --install || {
        log_error "Failed to install Xcode Command Line Tools"
        exit 1
    }
else
    log_success "Clang compiler found"
fi

# GCC compiler (for compiler version testing)
log_info "Installing GCC compiler..."
brew install gcc || {
    log_warning "Failed to install GCC"
}

# Common libraries
log_info "Installing common libraries..."
brew install \
    openssl \
    zlib \
    || {
        log_error "Failed to install common libraries"
        exit 1
    }

# TBB (Threading Building Blocks)
if [ "$WITH_TBB" = true ]; then
    log_info "Installing Intel TBB..."
    brew install tbb || {
        log_warning "Failed to install TBB"
    }
fi

# CUDA Toolkit (optional)
if [ "$WITH_CUDA" = true ]; then
    log_info "Installing CUDA Toolkit..."
    # CUDA on macOS is deprecated, but we can try to install it
    log_warning "CUDA support on macOS is limited. Skipping CUDA installation."
fi

# Python dependencies. Homebrew/macOS Python is PEP 668 managed, so pip
# needs --break-system-packages on GitHub-hosted runners.
log_info "Installing Python dependencies..."
python3 -m pip install --upgrade pip setuptools wheel --break-system-packages || {
    log_warning "Failed to upgrade pip"
}

python3 -m pip install colorama==0.4.6 psutil==6.1.1 --break-system-packages || {
    log_warning "Failed to install Python dependencies"
}

log_success "macOS dependency installation completed successfully!"
log_info "You can now build XSigma using: python Scripts/setup.py ninja clang config build test"
