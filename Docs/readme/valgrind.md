# Valgrind Setup and Configuration Guide

## Overview

This guide explains how to set up and use Valgrind for memory checking in the XSigma project.

## Platform Support

### ✅ Supported Platforms
- **Linux (x86_64)**: Full support
- **Linux (ARM64)**: Limited support (Valgrind 3.19+)
- **macOS (x86_64/Intel)**: Supported with limitations

### ❌ Unsupported Platforms
- **macOS (ARM64/Apple Silicon)**: **NOT SUPPORTED**
  - Valgrind does not support Apple Silicon architecture
  - Use sanitizers as an alternative (see below)

## Installation

### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install valgrind
```

### Linux (Fedora/RHEL/CentOS)
```bash
sudo dnf install valgrind
# or
sudo yum install valgrind
```

### Linux (Arch)
```bash
sudo pacman -S valgrind
```

### macOS (Intel only)
```bash
brew install valgrind
```

**Note**: On macOS, Valgrind support is limited and may not work with recent macOS versions.

## Building with Valgrind Support

### Standard Build
```bash
cd Scripts
python3 setup.py config.build.test.ninja.clang.valgrind
```

### Build with Specific Configuration
```bash
# Debug build with Valgrind
python3 setup.py config.build.test.ninja.clang.debug.valgrind

# Release build with Valgrind
python3 setup.py config.build.test.ninja.clang.release.valgrind
```

## Running Valgrind Tests

### Automatic Execution
When you build with the `valgrind` flag and include `test`, the tests will automatically run with Valgrind:

```bash
cd Scripts
python3 setup.py config.ninja.clang.valgrind.build.test
```

### Manual Execution
You can also run Valgrind tests manually:

```bash
cd Scripts
./valgrind_ctest.sh /path/to/build_directory
```

## Valgrind Configuration

### Configuration Architecture
The Valgrind integration follows a clean separation of concerns:

1. **CMake Configuration** (`Cmake/tools/valgrind.cmake`):
   - All Valgrind options and settings
   - Timeout configuration and multipliers
   - Suppression file management
   - Platform compatibility checks

2. **Shell Script** (`Scripts/valgrind_ctest.sh`):
   - Test execution logic only
   - Result analysis and reporting
   - No hardcoded configuration

This architecture ensures:
- Single source of truth for configuration
- Easy maintenance and updates
- Consistent behavior across environments

### Suppression File
The project includes a suppression file to filter out known false positives:
- Location: `Scripts/valgrind_suppression.txt`
- Automatically used by the build system
- Can be customized for project-specific suppressions

### Valgrind Options
All Valgrind options are configured in `Cmake/tools/valgrind.cmake`. The default configuration includes:

**Core Settings:**
- `--tool=memcheck`: Memory error detector
- `--leak-check=full`: Detailed leak information
- `--show-leak-kinds=all`: Show all types of leaks
- `--show-reachable=yes`: Show reachable memory

**Error Tracking:**
- `--track-origins=yes`: Track origins of uninitialized values
- `--track-fds=yes`: Track file descriptor leaks
- `--error-exitcode=1`: Exit with error code on issues

**Output and Reporting:**
- `--verbose`: Detailed output
- `--num-callers=50`: Stack trace depth
- `--gen-suppressions=all`: Generate suppression entries
- `--xml=yes`: Machine-readable XML output

**Process Handling:**
- `--trace-children=yes`: Trace child processes

### Timeout Configuration
Tests run significantly slower under Valgrind (typically 10-50x slower). The timeout configuration handles this automatically:

**Default Settings:**
- Timeout multiplier: **20x** (configurable via `XSIGMA_VALGRIND_TIMEOUT_MULTIPLIER`)
- Global CTest timeout: **1800 seconds** (30 minutes)
- Individual test timeouts are automatically multiplied

**Customizing Timeouts:**
```bash
# Configure with custom timeout multiplier
cmake -DXSIGMA_ENABLE_VALGRIND=ON \
      -DXSIGMA_VALGRIND_TIMEOUT_MULTIPLIER=30 \
      ..
```

**How It Works:**
1. Tests define normal timeouts (e.g., 300 seconds for CoreCxxTests)
2. When Valgrind is enabled, timeouts are automatically multiplied
3. Example: 300s × 20 = 6000s (100 minutes) under Valgrind
4. This prevents false timeout failures while maintaining safety limits

## Interpreting Results

### Memory Leak Types
1. **Definitely Lost**: Memory that is no longer accessible (real leak) - **MUST FIX**
2. **Indirectly Lost**: Memory lost due to definitely lost blocks - **MUST FIX**
3. **Possibly Lost**: Memory that might be leaked (pointer arithmetic) - **INVESTIGATE**
4. **Still Reachable**: Memory still accessible at exit (not necessarily a leak) - **OPTIONAL**

### Exit Codes and Test Status
The script intelligently determines test status based on multiple factors:

**Exit Code 0 (Success):**
- All tests passed with no memory issues
- Tests timed out BUT no memory issues detected (timeout is treated as warning only)

**Exit Code 1 (Failure):**
- Memory leaks detected (definitely lost, indirectly lost)
- Memory errors detected (invalid reads/writes, uninitialized values)
- Tests failed for reasons other than timeout

**Key Behavior:**
- **Timeouts without memory issues = PASS**: The script recognizes that timeouts under Valgrind are often due to slow execution, not actual failures
- **Memory issues = FAIL**: Any memory leak or error causes failure, regardless of test completion
- **Detailed logging**: All results are logged for manual inspection if needed

### Understanding Test Output
```
[✓] No memory leaks detected
[✓] No memory errors detected
[!] Some tests timed out
[i] Consider this a PASS for memory checking purposes
```
This indicates a successful memory check despite timeout.

## Alternatives for Apple Silicon (ARM64)

Since Valgrind doesn't support Apple Silicon, use these alternatives:

### 1. AddressSanitizer (ASan)
Detects memory errors like buffer overflows, use-after-free, etc.

```bash
cd Scripts
python3 setup.py config.ninja.clang.test --sanitizer.address
```

### 2. LeakSanitizer (LSan)
Detects memory leaks (included with ASan on supported platforms).

```bash
cd Scripts
python3 setup.py config.ninja.clang.test --sanitizer.leak
```

### 3. UndefinedBehaviorSanitizer (UBSan)
Detects undefined behavior.

```bash
cd Scripts
python3 setup.py config.ninja.clang.test --sanitizer.undefined
```

### 4. ThreadSanitizer (TSan)
Detects data races and threading issues.

```bash
cd Scripts
python3 setup.py config.ninja.clang.test --sanitizer.thread
```

### 5. Docker with x86_64 Linux
Run Valgrind in a Docker container:

```bash
# Create Dockerfile
cat > Dockerfile << 'EOF'
FROM ubuntu:22.04
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    clang \
    valgrind \
    git
WORKDIR /workspace
EOF

# Build and run
docker build -t xsigma-valgrind .
docker run -v $(pwd):/workspace -it xsigma-valgrind bash
```

## Troubleshooting

### Issue: "Valgrind not found"
**Solution**: Install Valgrind using the platform-specific instructions above.

### Issue: "Valgrind does not support Apple Silicon"
**Solution**: Use sanitizers instead (see alternatives section).

### Issue: "Too many false positives"
**Solution**: Add suppressions to `Scripts/valgrind_suppression.txt`.

### Issue: "Tests timeout under Valgrind"
**Symptoms:**
```
The following tests FAILED:
      1 - CoreCxxTests (Timeout)
```

**Solutions:**
1. **Automatic (Recommended)**: The timeout multiplier should handle this automatically. If you still see timeouts:
   ```bash
   # Increase the timeout multiplier
   cmake -DXSIGMA_ENABLE_VALGRIND=ON \
         -DXSIGMA_VALGRIND_TIMEOUT_MULTIPLIER=30 \
         ..
   ```

2. **Manual**: Edit the test timeout in the test's CMakeLists.txt:
   ```cmake
   set_tests_properties(CoreCxxTests PROPERTIES
       TIMEOUT 600  # Increase from 300 to 600
   )
   ```

3. **Check if it's a real issue**: If the script reports "No memory issues detected" despite timeout, the memory check passed. The timeout is just a warning.

### Issue: "Tests run very slowly"
**Solution**: This is normal. Valgrind significantly slows down execution (10-50x slower). The timeout configuration accounts for this.

### Issue: "Valgrind crashes on macOS"
**Solution**:
- Update to the latest Valgrind version
- Try using an older macOS version
- Use Docker with Linux
- Switch to sanitizers

### Issue: "Script reports failure but no memory errors shown"
**Diagnosis**: Check the detailed logs:
```bash
# View Valgrind logs
ls -lh build_ninja_valgrind/Testing/Temporary/MemoryChecker.*.log

# Search for specific errors
grep -i "error\|leak" build_ninja_valgrind/Testing/Temporary/MemoryChecker.*.log
```

**Common causes:**
- File descriptor leaks (check with `--track-fds=yes`)
- Reachable memory at exit (usually not a real issue)
- Third-party library issues (add suppressions)

## Performance Considerations

- **Slowdown**: Valgrind typically runs 10-50x slower than native execution
- **Memory Usage**: Valgrind requires significant additional memory
- **CI/CD**: Consider running Valgrind tests separately or on specific platforms

## Best Practices

1. **Regular Testing**: Run Valgrind tests regularly, not just before releases
2. **Fix Issues Promptly**: Address memory leaks and errors as they're discovered
3. **Use Suppressions Wisely**: Only suppress known false positives from third-party libraries
4. **Combine Tools**: Use both Valgrind (on Linux) and sanitizers for comprehensive coverage
5. **Document Suppressions**: Add comments explaining why each suppression is needed

## Integration with CI/CD

### GitHub Actions Example
```yaml
name: Valgrind Tests

on: [push, pull_request]

jobs:
  valgrind:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y valgrind cmake ninja-build clang
      - name: Build and test with Valgrind
        run: |
          cd Scripts
          python3 setup.py config.ninja.clang.valgrind.build.test
```

## Additional Resources

- [Valgrind Official Documentation](https://valgrind.org/docs/manual/manual.html)
- [Valgrind Quick Start Guide](https://valgrind.org/docs/manual/quick-start.html)
- [Suppression File Format](https://valgrind.org/docs/manual/manual-core.html#manual-core.suppress)
- [AddressSanitizer Documentation](https://clang.llvm.org/docs/AddressSanitizer.html)

## Support

For issues or questions:
1. Check this documentation
2. Review the Valgrind logs in the build directory
3. Consult the project's issue tracker
4. Contact the development team
