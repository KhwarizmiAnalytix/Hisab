# Compiler Caching Guide

XSigma supports multiple compiler cache types to significantly improve build performance, especially for incremental builds and CI/CD pipelines.

## Table of Contents

- [Overview](#overview)
- [Cache Types](#cache-types)
- [Installation](#installation)
- [Configuration](#configuration)
- [Performance Comparison](#performance-comparison)
- [Best Practices](#best-practices)
- [Troubleshooting](#troubleshooting)
- [Advanced Configuration](#advanced-configuration)

## Overview

Compiler caching stores compilation results to avoid recompiling unchanged code. This is particularly valuable for:
- **Incremental builds** - Only changed files are recompiled
- **CI/CD pipelines** - Faster feedback loops and reduced resource usage
- **Team development** - Shared cache across developers
- **Large projects** - Significant time savings on clean builds

### Expected Performance Improvements

| Scenario | Improvement |
|----------|-------------|
| First build | 5-15% faster (with optimized linker) |
| Incremental build (no changes) | 50-80% faster |
| Incremental build (few changes) | 30-60% faster |
| Clean rebuild | 10-30% faster |

## Cache Types

### ccache

**Best for:** Local development and single-machine builds

**Features:**
- Local compilation cache stored on disk
- Works with GCC, Clang, and MSVC
- Minimal configuration required
- Cache stored in `~/.ccache` by default

**Pros:**
- Simple setup and configuration
- Low overhead
- Works offline
- Excellent for local development

**Cons:**
- Not distributed (single machine only)
- Cache not shared between machines
- Requires manual cache management

### sccache

**Best for:** Distributed CI/CD with cloud storage

**Features:**
- Distributed compiler cache with cloud storage support
- Supports S3, Azure Blob Storage, Google Cloud Storage
- Works with GCC, Clang, and MSVC
- Ideal for CI/CD pipelines

**Pros:**
- Distributed cache across machines
- Cloud storage integration
- Excellent for CI/CD
- Supports multiple storage backends
- Better for large teams

**Cons:**
- More complex setup
- Requires cloud storage credentials
- Network latency for cache access
- Higher resource usage than ccache

### buildcache

**Best for:** Incremental CI/CD pipelines

**Features:**
- Incremental build cache optimized for CI/CD
- Stores build artifacts and dependencies
- Works with GCC, Clang, and MSVC
- Cache stored in `~/.buildcache` by default

**Pros:**
- Optimized for CI/CD workflows
- Incremental caching of build artifacts
- Good for monorepo builds
- Efficient storage

**Cons:**
- Less mature than ccache/sccache
- Limited cloud storage support
- Smaller community

### none

**Use when:** You want to disable caching

- No compiler caching
- Useful for testing or when cache causes issues
- Default value in XSigma

## Installation

### Linux

#### ccache

```bash
# Ubuntu/Debian
sudo apt-get install ccache

# Fedora/RHEL
sudo dnf install ccache

# Arch
sudo pacman -S ccache
```

#### sccache

```bash
# Download from releases
curl -L https://github.com/mozilla/sccache/releases/download/v0.7.7/sccache-v0.7.7-x86_64-unknown-linux-musl.tar.gz -o sccache.tar.gz
tar xzf sccache.tar.gz
sudo mv sccache-v0.7.7-x86_64-unknown-linux-musl/sccache /usr/local/bin/
chmod +x /usr/local/bin/sccache

# Or use package manager (if available)
sudo apt-get install sccache  # Some distributions
```

#### buildcache

```bash
# Download from releases
curl -L https://github.com/mbitsnbites/buildcache/releases/download/v0.28.0/buildcache-linux.tar.gz -o buildcache.tar.gz
tar xzf buildcache.tar.gz
sudo mv buildcache/bin/buildcache /usr/local/bin/
chmod +x /usr/local/bin/buildcache
```

### macOS

#### ccache

```bash
brew install ccache
```

#### sccache

```bash
brew install sccache
```

#### buildcache

```bash
brew install buildcache
```

### Windows

#### ccache

```bash
# Using Chocolatey
choco install ccache

# Using vcpkg
vcpkg install ccache:x64-windows

# Or download from: https://github.com/ccache/ccache/releases
```

#### sccache

```bash
# Download from: https://github.com/mozilla/sccache/releases
# Extract and add to PATH
```

#### buildcache

```bash
# Download from: https://github.com/mbitsnbites/buildcache/releases
# Extract and add to PATH
```

## Configuration

### Using setup.py (Recommended)

```bash
cd Scripts

# Use ccache
python setup.py config.build.ninja.clang.ccache

# Use sccache
python setup.py config.build.ninja.clang.sccache

# Use buildcache
python setup.py config.build.ninja.clang.buildcache

# Disable caching
python setup.py config.build.ninja.clang.none
```

### Using CMake

```bash
# Select cache type
cmake -DPROJECT_CACHE_BACKEND=ccache ..
cmake -DPROJECT_CACHE_BACKEND=sccache ..
cmake -DPROJECT_CACHE_BACKEND=buildcache ..
cmake -DPROJECT_CACHE_BACKEND=none ..

# Disable caching entirely
cmake -DPROJECT_ENABLE_CACHE=OFF ..
```

### Environment Variables

#### ccache

```bash
# Set cache size (default: 5GB)
export CCACHE_MAXSIZE=20G

# Set cache directory
export CCACHE_DIR=~/.ccache

# Enable compression
export CCACHE_COMPRESS=1

# Set compression level (1-9)
export CCACHE_COMPRESSLEVEL=6
```

#### sccache

```bash
# Set cache size
export SCCACHE_CACHE_SIZE=10G

# Configure S3 storage
export SCCACHE_S3_SERVER_SIDE_ENCRYPTION=true
export AWS_ACCESS_KEY_ID=your_key
export AWS_SECRET_ACCESS_KEY=your_secret

# Configure Azure storage
export SCCACHE_AZURE_CONNECTION_STRING=your_connection_string
```

#### buildcache

```bash
# Set cache directory
export BUILDCACHE_DIR=~/.buildcache

# Set cache size
export BUILDCACHE_MAX_CACHE_SIZE=10G
```

## Performance Comparison

| Feature | ccache | sccache | buildcache |
|---------|--------|---------|-----------|
| Local caching | ✅ | ✅ | ✅ |
| Distributed | ❌ | ✅ | ❌ |
| Cloud storage | ❌ | ✅ | ❌ |
| Setup complexity | Low | Medium | Medium |
| Memory overhead | Low | Medium | Low |
| Disk overhead | Low | Medium | Low |
| CI/CD friendly | ✅ | ✅✅ | ✅ |
| Local dev friendly | ✅✅ | ✅ | ✅ |

## Best Practices

1. **Choose the right cache type:**
   - Local development: Use **ccache**
   - CI/CD with shared cache: Use **sccache**
   - Incremental CI/CD: Use **buildcache**

2. **Configure appropriate cache sizes:**
   ```bash
   # For large projects
   ccache -M 50G
   export SCCACHE_CACHE_SIZE=50G
   ```

3. **Use consistent compiler paths:**
   - Ensure all builds use the same compiler path
   - Avoid symlinks that might change

4. **Monitor cache effectiveness:**
   ```bash
   ccache -s      # View ccache statistics
   sccache --show-stats  # View sccache statistics
   ```

5. **Clear cache when needed:**
   ```bash
   ccache -C      # Clear ccache
   sccache --stop-server  # Stop sccache
   rm -rf ~/.buildcache  # Clear buildcache
   ```

6. **Enable compression for large caches:**
   ```bash
   export CCACHE_COMPRESS=1
   export CCACHE_COMPRESSLEVEL=6
   ```

## Troubleshooting

### Cache not being used

**Problem:** CMake output shows "NOT FOUND" for the cache tool

**Solutions:**
1. Verify installation: `which ccache` / `which sccache` / `which buildcache`
2. Ensure tool is in PATH
3. Check CMake output for specific error messages
4. Reconfigure: `cmake --fresh -DPROJECT_CACHE_BACKEND=ccache ..`

### Cache misses

**Problem:** Cache hits are low, builds not getting faster

**Solutions:**
1. Verify compiler path consistency
2. Check for compiler flag variations
3. Ensure source files haven't changed
4. Monitor with: `ccache -s` or `sccache --show-stats`

### Out of disk space

**Problem:** Cache is consuming too much disk space

**Solutions:**
1. Reduce cache size: `ccache -M 10G`
2. Clear old cache: `ccache -C`
3. Enable compression: `export CCACHE_COMPRESS=1`

### Stale cache

**Problem:** Changes not reflected in builds

**Solutions:**
1. Clear cache: `ccache -C` or `sccache --stop-server`
2. Verify compiler version hasn't changed
3. Check for timestamp issues

## Advanced Configuration

### CI/CD Integration

#### GitHub Actions

```yaml
- name: Install ccache
  run: sudo apt-get install ccache

- name: Configure cache
  run: |
    ccache -M 5G
    export PATH="/usr/lib/ccache:$PATH"

- name: Build
  run: |
    cd Scripts
    python setup.py config.build.ninja.clang.ccache
```

#### GitLab CI

```yaml
cache:
  paths:
    - .ccache

before_script:
  - apt-get update && apt-get install -y ccache
  - export CCACHE_DIR=$CI_PROJECT_DIR/.ccache
  - export CCACHE_MAXSIZE=5G

build:
  script:
    - cd Scripts
    - python setup.py config.build.ninja.clang.ccache
```

### Multi-machine Setup with sccache

```bash
# Configure S3 backend
export SCCACHE_S3_SERVER_SIDE_ENCRYPTION=true
export AWS_ACCESS_KEY_ID=your_key
export AWS_SECRET_ACCESS_KEY=your_secret
export SCCACHE_S3_BUCKET=my-build-cache
export SCCACHE_S3_REGION=us-west-2

# Build with sccache
cd Scripts
python setup.py config.build.ninja.clang.sccache
```

---

**For more information:** See [Build Optimizations](../README.md#build-optimizations) in the main README.
