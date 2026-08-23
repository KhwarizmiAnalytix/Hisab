# Quarisma Logging System

## Overview

Quarisma provides a flexible logging system with multiple backend options to suit different use cases and performance requirements. The logging backend can be selected at compile time to optimize for your specific needs.

## Logging Backends

### LOGURU (Default)

**Features:**
- Full-featured logging with scopes and callbacks
- Advanced formatting and filtering
- Thread-safe logging
- Minimal performance overhead
- Rich output formatting

**Use when:**
- You need advanced logging features
- You want detailed diagnostic information
- Performance is not critical

**Configuration:**
```bash
cd Scripts
python setup.py config.build.ninja.clang --logging.loguru
```

### GLOG

**Features:**
- Google's production-grade logging library
- Minimal overhead
- Efficient file rotation
- Thread-safe logging
- Proven in production systems

**Use when:**
- You need production-grade logging
- You want minimal dependencies
- Performance is important

**Configuration:**
```bash
cd Scripts
python setup.py config.build.ninja.clang --logging.glog
```

### NATIVE

**Features:**
- Minimal native implementation
- Zero external dependencies
- Lightweight and fast
- Basic logging functionality
- Suitable for embedded systems

**Use when:**
- You need minimal dependencies
- You want maximum performance
- You only need basic logging

**Configuration:**
```bash
cd Scripts
python setup.py config.build.ninja.clang --logging.native
```

## Logging Levels

Quarisma supports standard logging levels:

| Level | Description | Use Case |
|-------|-------------|----------|
| **TRACE** | Most detailed information | Development debugging |
| **DEBUG** | Detailed diagnostic information | Development and testing |
| **INFO** | General informational messages | Normal operation |
| **WARN** | Warning messages | Potential issues |
| **ERROR** | Error messages | Error conditions |
| **CRITICAL** | Critical error messages | Fatal errors |
| **OFF** | Disable logging | Production optimization |

## Configuration

### Environment Variables

Control logging behavior through environment variables:

```bash
# Set logging level
export QUARISMA_LOG_LEVEL=DEBUG

# Set log file location
export QUARISMA_LOG_FILE=/var/log/quarisma.log

# Enable verbose logging
export QUARISMA_LOG_VERBOSE=1
```

### Programmatic Configuration

Configure logging in your code:

```cpp
#include <quarisma/logging/logger.h>

// Set logging level
quarisma::logging::set_level(quarisma::logging::Level::DEBUG);

// Set log file
quarisma::logging::set_file("/path/to/logfile.log");

// Enable/disable console output
quarisma::logging::enable_console(true);
```

### Configuration File

Create a logging configuration file (optional):

```yaml
# quarisma_logging.yaml
logging:
  level: DEBUG
  file: /var/log/quarisma.log
  console: true
  format: "[%Y-%m-%d %H:%M:%S] [%l] %v"
  max_file_size: 10485760  # 10MB
  max_files: 5
```

## Usage Examples

### Basic Logging

```cpp
#include <quarisma/logging/logger.h>

int main() {
    QUARISMA_LOG_INFO("Application started");
    QUARISMA_LOG_DEBUG("Debug information");
    QUARISMA_LOG_WARN("Warning message");
    QUARISMA_LOG_ERROR("Error occurred");
    return 0;
}
```

### Conditional Logging

```cpp
if (QUARISMA_LOG_ENABLED(DEBUG)) {
    QUARISMA_LOG_DEBUG("Expensive debug operation: {}", expensive_function());
}
```

### Scoped Logging

```cpp
{
    auto scope = QUARISMA_LOG_SCOPE("function_name");
    QUARISMA_LOG_INFO("Inside function");
    // Scope automatically logs exit
}
```

### Formatted Logging

```cpp
QUARISMA_LOG_INFO("Processing item {} of {}", current, total);
QUARISMA_LOG_ERROR("Failed to open file: {}", filename);
```

## Performance Considerations

### Logging Overhead

| Backend | Overhead | Best For |
|---------|----------|----------|
| LOGURU | Low | Development and debugging |
| GLOG | Very Low | Production systems |
| NATIVE | Minimal | Embedded and performance-critical |

### Optimization Tips

1. **Use appropriate log levels** - Set to WARN or ERROR in production
2. **Avoid expensive operations in log messages** - Use conditional logging
3. **Use NATIVE backend** for performance-critical code
4. **Disable console output** in production for better performance
5. **Use file rotation** to manage log file sizes

## Backend-Specific Features

### LOGURU Features

- Colored console output
- Automatic stack traces on errors
- Custom formatters
- Callback functions
- Thread names

### GLOG Features

- Automatic log rotation
- Severity-based file splitting
- Efficient memory usage
- Google-style formatting

### NATIVE Features

- Minimal memory footprint
- No external dependencies
- Fast logging operations
- Simple configuration

## Troubleshooting

### Logs not appearing

1. Check logging level is set correctly
2. Verify console output is enabled
3. Check file permissions if writing to file
4. Ensure logger is initialized before use

### Performance issues

1. Reduce logging level (set to WARN or ERROR)
2. Switch to NATIVE backend
3. Disable console output
4. Use conditional logging for expensive operations

### File size growing too large

1. Enable log rotation
2. Reduce logging level
3. Implement log cleanup policies
4. Use file size limits

## Migration Between Backends

To switch logging backends:

```bash
# From LOGURU to GLOG
cd Scripts
python setup.py config.build.ninja.clang --logging.glog

# From GLOG to NATIVE
cd Scripts
python setup.py config.build.ninja.clang --logging.native
```

No code changes are required - the logging API remains consistent across backends.

## Best Practices

1. **Use appropriate levels** - DEBUG for development, INFO for normal operation, WARN/ERROR for issues
2. **Include context** - Log relevant variables and state information
3. **Avoid logging sensitive data** - Don't log passwords, tokens, or personal information
4. **Use structured logging** - Include timestamps, thread IDs, and function names
5. **Monitor log files** - Implement log rotation and cleanup
6. **Test logging** - Verify logging works in your deployment environment

## Related Documentation

- **[Setup Guide](setup.md)** - How to configure logging backend during build
- **[Code Quality Tools](../README.md#code-quality-and-analysis-tools)** - Other analysis tools
