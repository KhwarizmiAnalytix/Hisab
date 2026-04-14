# Third-Party Dependencies

Quarisma uses a **conditional compilation pattern** where each library is controlled by `QUARISMA_ENABLE_XXX` options. This allows you to customize your build by including only the dependencies you need.

## Table of Contents

- [Dependency Categories](#dependency-categories)
- [Dependency Management Pattern](#dependency-management-pattern)
- [Setting Up Dependencies](#setting-up-dependencies)
- [Build Configuration](#build-configuration)
- [Using Dependencies in Code](#using-dependencies-in-code)
- [CMake Target Usage](#cmake-target-usage)

## Dependency Categories

### Mandatory Core Libraries (Always Included)

These libraries are always included in the build:

| Library | Description | Target Alias |
|---------|-------------|--------------|
| fmt | Modern C++ formatting | `Fmt::fmt` |
| cpuinfo | CPU feature detection | `Cpuinfo::cpuinfo` |

### Optional Libraries (Enabled by Default)

These libraries are included by default but can be disabled:

| Library | Option | Description | Target Alias |
|---------|--------|-------------|--------------|
| magic_enum | `QUARISMA_ENABLE_MAGICENUM=ON` | Enum reflection | `MagicEnum::magic_enum` |
| loguru | `LOGGING_ENABLE_LOGURU=ON` | Lightweight logging | `Loguru::loguru` |

### Optional Libraries (Disabled by Default)

These libraries must be explicitly enabled:

| Library | Option | Description | Target Alias |
|---------|--------|-------------|--------------|
| mimalloc | `MEMORY_ENABLE_MIMALLOC=OFF` | High-performance allocator | `Mimalloc::mimalloc` |
| Google Test | `QUARISMA_ENABLE_GTEST=OFF` | Testing framework | `Gtest::gtest` |
| Benchmark | `QUARISMA_ENABLE_BENCHMARK=OFF` | Microbenchmarking | `Benchmark::benchmark` |

## Dependency Management Pattern

### Mandatory Libraries (fmt, cpuinfo)

For mandatory libraries:
- Always included in the build
- Always create namespaced third-party aliases (`Xxx::xxx`)
- Always add `QUARISMA_HAS_XXX` compile definitions
- Always linked to Core target

### Optional Libraries (controlled by `QUARISMA_ENABLE_XXX`)

**When `QUARISMA_ENABLE_XXX=ON`:**
1. Include the library in the build
2. Create an `Xxx::xxx` target alias
3. Add `QUARISMA_HAS_XXX` compile definition
4. Link to Core target

**When `QUARISMA_ENABLE_XXX=OFF`:**
1. Skip library completely
2. No target aliases created
3. No compile definitions added
4. No linking attempted

## Setting Up Dependencies

### Option 1: Git Submodules (Recommended)

Use Git submodules to include dependencies directly in your repository:

```bash
# Initialize all submodules from the repository configuration
git submodule sync --recursive
git submodule update --init --recursive
```

**Note**: The repository already has submodules configured in `.gitmodules`. Use the commands above to initialize them locally. Do NOT use `git submodule add` unless you're adding a new dependency to the project.

### Option 2: External Libraries

Use system-installed libraries instead of bundled submodules:

```bash
# Use system-installed libraries
cmake -B build -S . -DQUARISMA_ENABLE_EXTERNAL=ON
```

**Benefits:**
- Faster build times (libraries already compiled)
- Smaller repository size
- Shared libraries across projects

**Requirements:**
- Libraries must be installed on the system
- May require additional system packages
- Consult upstream documentation if `find_package()` fails

## Build Configuration

### Enabling/Disabling Optional Libraries

```bash
# Enable high-performance allocator
cmake -B build -S . -DMEMORY_ENABLE_MIMALLOC=ON

# Disable magic_enum
cmake -B build -S . -DQUARISMA_ENABLE_MAGICENUM=OFF

# Enable testing and benchmarking
cmake -B build -S . \
    -DQUARISMA_ENABLE_GTEST=ON \
    -DQUARISMA_ENABLE_BENCHMARK=ON
```

### Build Configuration for Third-Party Libraries

Third-party targets are configured to:
- Suppress warnings (using `-w` or `/w`)
- Use the same C++ standard as the main project
- Avoid altering the main project's compiler/linker settings
- Provide consistent target aliases with the `Quarisma::` prefix

## Using Dependencies in Code

### Conditional Compilation

Use compile definitions to conditionally use libraries:

```cpp
#include "quarisma_features.h"

void example_function() {
    #ifdef QUARISMA_HAS_FMT
        // Use fmt library for formatting
        fmt::print("Hello, {}!\n", "World");
    #else
        // Fallback to standard library
        std::cout << "Hello, World!" << std::endl;
    #endif

    #ifdef QUARISMA_HAS_TBB
        // Use TBB for parallel algorithms
        tbb::parallel_for(/*...*/);
    #else
        // Use standard threading
        std::thread t(/*...*/);
    #endif

    #ifdef QUARISMA_HAS_MIMALLOC
        // mimalloc is available as drop-in replacement
        // No code changes needed - just link with Mimalloc::mimalloc
    #endif
}
```

## CMake Target Usage

### Linking with Quarisma Libraries

```cmake
# Your custom target
add_executable(my_app main.cpp)

# Link with Quarisma Core (always available)
target_link_libraries(my_app PRIVATE Quarisma::Core)

# Conditionally link with third-party libraries
if(TARGET Fmt::fmt)
    target_link_libraries(my_app PRIVATE Fmt::fmt)
endif()

if(TARGET Benchmark::benchmark)
    target_link_libraries(my_app PRIVATE Benchmark::benchmark)
endif()
```

## Troubleshooting

### Missing Third-Party Libraries

If you see warnings about missing third-party libraries:

1. **Initialize Git submodules** (recommended):
   ```bash
   git submodule sync --recursive
   git submodule update --init --recursive
   ```

2. **Use external libraries**:
   ```bash
   cmake -B build -S . -DQUARISMA_ENABLE_EXTERNAL=ON
   ```

3. **Disable unused optional features**:
   ```bash
   cmake -B build -S . -DQUARISMA_ENABLE_MAGICENUM=OFF
   ```

### Build Performance Issues

For faster builds:

1. **Use external libraries**:
   ```bash
   cmake -B build -S . -DQUARISMA_ENABLE_EXTERNAL=ON
   ```

2. **Disable unused features**:
   ```bash
   cmake -B build -S . \
       -DQUARISMA_ENABLE_BENCHMARK=OFF \
       -DQUARISMA_ENABLE_GTEST=OFF
   ```

## Notes

- When `QUARISMA_ENABLE_EXTERNAL=ON`, system-installed libraries are preferred over bundled submodules
- Some libraries may require additional system packages; consult their upstream documentation if `find_package()` fails
- All third-party libraries use the `Quarisma::` namespace prefix for consistency

## Related Documentation

- [Build Configuration](build/build-configuration.md) - Build system configuration
- [Logging System](logging.md) - Logging backend selection
- [Usage Examples](usage-examples.md) - Example build configurations
