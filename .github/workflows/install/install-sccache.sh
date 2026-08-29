#!/bin/bash
# XSigma CI - Sccache Installation Script
# This script installs sccache with automatic platform detection
# Usage: ./install-sccache.sh [VERSION]
# Example: ./install-sccache.sh 0.7.7

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

# Get sccache version from argument or use default
SCCACHE_VERSION="${1:-0.7.7}"
log_info "Installing sccache version: $SCCACHE_VERSION"

# Detect platform
PLATFORM=$(uname -s)
ARCH=$(uname -m)

log_info "Detected platform: $PLATFORM ($ARCH)"

# Determine download URL and binary name based on platform
case "$PLATFORM" in
    Linux)
        if [ "$ARCH" = "x86_64" ]; then
            SCCACHE_URL="https://github.com/mozilla/sccache/releases/download/v${SCCACHE_VERSION}/sccache-v${SCCACHE_VERSION}-x86_64-unknown-linux-musl.tar.gz"
            SCCACHE_BINARY="sccache"
        elif [ "$ARCH" = "aarch64" ]; then
            SCCACHE_URL="https://github.com/mozilla/sccache/releases/download/v${SCCACHE_VERSION}/sccache-v${SCCACHE_VERSION}-aarch64-unknown-linux-musl.tar.gz"
            SCCACHE_BINARY="sccache"
        else
            log_error "Unsupported Linux architecture: $ARCH"
            exit 1
        fi
        INSTALL_PATH="$HOME/.local/bin"
        ;;
    Darwin)
        if [ "$ARCH" = "x86_64" ]; then
            SCCACHE_URL="https://github.com/mozilla/sccache/releases/download/v${SCCACHE_VERSION}/sccache-v${SCCACHE_VERSION}-x86_64-apple-darwin.tar.gz"
            SCCACHE_BINARY="sccache"
        elif [ "$ARCH" = "arm64" ]; then
            SCCACHE_URL="https://github.com/mozilla/sccache/releases/download/v${SCCACHE_VERSION}/sccache-v${SCCACHE_VERSION}-aarch64-apple-darwin.tar.gz"
            SCCACHE_BINARY="sccache"
        else
            log_error "Unsupported macOS architecture: $ARCH"
            exit 1
        fi
        INSTALL_PATH="$HOME/.local/bin"
        ;;
    MINGW*|MSYS*|CYGWIN*)
        log_error "This script is for Unix-like systems. Use install-deps-windows.ps1 for Windows."
        exit 1
        ;;
    *)
        log_error "Unsupported platform: $PLATFORM"
        exit 1
        ;;
esac

log_info "Download URL: $SCCACHE_URL"

# Create installation directory
log_info "Creating installation directory: $INSTALL_PATH"
mkdir -p "$INSTALL_PATH" || {
    log_error "Failed to create installation directory"
    exit 1
}

# Download sccache
log_info "Downloading sccache..."
TEMP_DIR=$(mktemp -d)
trap "rm -rf $TEMP_DIR" EXIT

cd "$TEMP_DIR"
if ! curl -L "$SCCACHE_URL" -o sccache.tar.gz; then
    log_error "Failed to download sccache from $SCCACHE_URL"
    exit 1
fi

log_info "Extracting sccache..."
if ! tar xzf sccache.tar.gz; then
    log_error "Failed to extract sccache"
    exit 1
fi

# Find the binary (it might be in a subdirectory)
if [ -f "$SCCACHE_BINARY" ]; then
    BINARY_PATH="$SCCACHE_BINARY"
elif [ -f "sccache-v${SCCACHE_VERSION}-x86_64-unknown-linux-musl/$SCCACHE_BINARY" ]; then
    BINARY_PATH="sccache-v${SCCACHE_VERSION}-x86_64-unknown-linux-musl/$SCCACHE_BINARY"
elif [ -f "sccache-v${SCCACHE_VERSION}-x86_64-apple-darwin/$SCCACHE_BINARY" ]; then
    BINARY_PATH="sccache-v${SCCACHE_VERSION}-x86_64-apple-darwin/$SCCACHE_BINARY"
elif [ -f "sccache-v${SCCACHE_VERSION}-aarch64-apple-darwin/$SCCACHE_BINARY" ]; then
    BINARY_PATH="sccache-v${SCCACHE_VERSION}-aarch64-apple-darwin/$SCCACHE_BINARY"
elif [ -f "sccache-v${SCCACHE_VERSION}-aarch64-unknown-linux-musl/$SCCACHE_BINARY" ]; then
    BINARY_PATH="sccache-v${SCCACHE_VERSION}-aarch64-unknown-linux-musl/$SCCACHE_BINARY"
else
    # Try to find any sccache binary
    BINARY_PATH=$(find . -name "sccache" -type f 2>/dev/null | head -1)
    if [ -z "$BINARY_PATH" ]; then
        log_error "Could not find sccache binary in archive"
        exit 1
    fi
fi

log_info "Found sccache binary at: $BINARY_PATH"

# Copy to installation directory
log_info "Installing sccache to $INSTALL_PATH..."
cp "$BINARY_PATH" "$INSTALL_PATH/$SCCACHE_BINARY" || {
    log_error "Failed to copy sccache to $INSTALL_PATH"
    exit 1
}

chmod +x "$INSTALL_PATH/$SCCACHE_BINARY" || {
    log_error "Failed to make sccache executable"
    exit 1
}

# Add to PATH if not already there
if [[ ":$PATH:" != *":$INSTALL_PATH:"* ]]; then
    log_info "Adding $INSTALL_PATH to PATH"
    echo "export PATH=\"$INSTALL_PATH:\$PATH\"" >> "$HOME/.bashrc"
    echo "export PATH=\"$INSTALL_PATH:\$PATH\"" >> "$HOME/.zshrc" 2>/dev/null || true
    export PATH="$INSTALL_PATH:$PATH"
fi

# Verify installation
log_info "Verifying sccache installation..."
if "$INSTALL_PATH/$SCCACHE_BINARY" --version; then
    log_success "Sccache installed successfully!"
    log_info "Sccache location: $INSTALL_PATH/$SCCACHE_BINARY"
else
    log_error "Failed to verify sccache installation"
    exit 1
fi

log_success "Sccache installation completed!"
