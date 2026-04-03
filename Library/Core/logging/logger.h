#pragma once

#include <string>  // for string

#include "common/export.h"                  // for QUARISMA_API
#include "common/macros.h"                  // for QUARISMA_DELETE_COPY_AND_MOVE
#include "fmt/format.h"                     // for FMT_STRING
#include "logging/logger_verbosity_enum.h"  // for logger_verbosity_enum

// this is copied from `loguru.hpp`
#if defined(__clang__) || defined(__GNUC__)
// Helper macro for declaring functions as having similar signature to printf.
// This allows the compiler to catch format errors at compile-time.
#define QUARISMA_PRINTF_LIKE(fmtarg, firstvararg) \
    __attribute__((__format__(__printf__, fmtarg, firstvararg)))
#define QUARISMA_FORMAT_STRING_TYPE const char*
#elif defined(_MSC_VER)
#define QUARISMA_PRINTF_LIKE(fmtarg, firstvararg)
#define QUARISMA_FORMAT_STRING_TYPE _In_z_ _Printf_format_string_ const char*
#else
#define QUARISMA_PRINTF_LIKE(fmtarg, firstvararg)
#define QUARISMA_FORMAT_STRING_TYPE const char*
#endif

namespace quarisma
{
class QUARISMA_VISIBILITY logger
{
public:
    /**
   * Initializes logging. This should be called from the main thread, if at all.
   * Your application doesn't *need* to call this, but if you do:
   *  * signal handlers are installed
   *  * program arguments are logged
   *  * working directory is logged
   *  * optional -v verbosity flag is parsed
   *  * main thread name is set to "main thread"
   *  * explanation of the preamble (date, threadname, etc.) is logged.
   *
   * This method will look for arguments meant for logging subsystem and remove
   * them. Arguments meant for logging subsystem are:
   *
   * -v n Set stderr logging verbosity. Examples
   *    -v INFO     Only show INFO, WARNING, ERROR, FATAL (default).
   *    -v WARNING  Only show WARNING, ERROR, FATAL.
   *    -v ERROR    Only show ERROR, FATAL.
   *    -v FATAL    Only show FATAL.
   *    -v OFF      Turn off logging to stderr.
   *
   * You can set the default logging verbosity programmatically by calling
   * `logger::SetStderrVerbosity` before calling `logger::Init`. That
   * way, you can specify a default that the user can override using command
   * line arguments. Note that this does not affect file logging.
   *
   * You can also use something else instead of '-v' flag by the via
   * `verbosity_flag` argument. You can also set to nullptr to skip parsing
   * verbosity level from the command line arguments.
   *
   * For applications that do not want loguru to handle any signals, i.e.,
   * print a stack trace when a signal is intercepted, the
   * `logger::EnableUnsafeSignalHandler` static member variable
   * should be set to `false`.
   * @{
   */
    QUARISMA_API static void Init(int& argc, char* argv[], const char* verbosity_flag = "-v");
    QUARISMA_API static void Init();
    /** @} */

    /**
   * Set the verbosity level for the output logged to stderr. Everything with a
   * verbosity equal or less than the level specified will be written to
   * stderr. Set to `VERBOSITY_OFF` to write nothing to stderr.
   * Default is 0.
   */
    QUARISMA_API static void SetStderrVerbosity(logger_verbosity_enum level);

    /**
   * Set internal messages verbosity level. The library used by QUARISMA, `loguru`
   * generates log messages during initialization and at exit. These are logged
   * as log level VERBOSITY_1, by default. One can change that using this
   * method. Typically, you want to call this before `logger::Init`.
   */
    QUARISMA_API static void SetInternalVerbosityLevel(logger_verbosity_enum level);

    /**
   * Support log file modes: `TRUNCATE` truncates the file clearing any existing
   * contents while `APPEND` appends to the existing log file contents, if any.
   */
    enum FileMode
    {
        TRUNCATE,
        APPEND
    };

    /**
   * Enable logging to a file at the given path.
   * Any logging message with verbosity lower or equal to the given verbosity
   * will be included. This method will create all directories in the 'path' if
   * needed. To stop the file logging, call `EndLogToFile` with the same path.
   */
    QUARISMA_API static void LogToFile(
        const char* path, FileMode filemode, logger_verbosity_enum verbosity);

    /**
   * Stop logging to a file at the given path.
   */
    QUARISMA_API static void EndLogToFile(const char* path);

    ///@{
    /**
   * Get/Set the name to identify the current thread in the log output.
   */
    QUARISMA_API static void        SetThreadName(const std::string& name);
    QUARISMA_API static std::string GetThreadName();
    ///@}

    /**
   * The message structure that is passed to custom callbacks registered using
   * `logger::AddCallback`.
   */
    struct Message
    {
        // You would generally print a Message by just concatenating the buffers without spacing.
        // Optionally, ignore preamble and indentation.
        logger_verbosity_enum verbosity;    // Already part of preamble
        const char*           filename;     // Already part of preamble
        unsigned              line;         // Already part of preamble
        const char*           preamble;     // Date, time, uptime, thread, file:line, verbosity.
        const char*           indentation;  // Just a bunch of spacing.
        const char*           prefix;       // Assertion failure info goes here (or "").
        const char*           message;      // User message goes here.
    };

    ///@{
    /**
   * Callback handle types.
   */
    using LogHandlerCallbackT   = void (*)(void* user_data, const Message& message);
    using CloseHandlerCallbackT = void (*)(void* user_data);
    using FlushHandlerCallbackT = void (*)(void* user_data);
    ///@}

    /**
   * Add a callback to call on each log message with a  verbosity less or equal
   * to the given one.  Useful for displaying messages in an application output
   * window, for example. The given `on_close` is also expected to flush (if
   * desired).
   *
   * Note that if logging is disabled at compile time, then these callback will
   * never be called.
   */
#if !defined(__WRAP__)

    QUARISMA_API static void AddCallback(
        const char*           id,
        LogHandlerCallbackT   callback,
        void*                 user_data,
        logger_verbosity_enum verbosity,
        CloseHandlerCallbackT on_close = nullptr,
        FlushHandlerCallbackT on_flush = nullptr);

#endif  // #if !defined(__WRAP__)

    /**
   * Remove a callback using the id specified.
   * Returns true if and only if the callback was found (and removed).
   */
    QUARISMA_API static bool RemoveCallback(const char* id);

    /**
   * Returns true if QUARISMA is built with logging support enabled.
   */
    QUARISMA_API static bool IsEnabled();

    /**
   * Returns the maximum verbosity of all log outputs. A log item for a
   * verbosity higher than this will not be generated in any of the currently
   * active outputs.
   */
    QUARISMA_API static logger_verbosity_enum GetCurrentVerbosityCutoff();

    /**
   * Convenience function to convert an integer to matching verbosity level. If
   * val is less than or equal to logger::VERBOSITY_INVALID, then
   * logger::VERBOSITY_INVALID is returned. If value is greater than
   * logger::VERBOSITY_MAX, then logger::VERBOSITY_MAX is returned.
   */
    QUARISMA_API static logger_verbosity_enum ConvertToVerbosity(int value);

    /**
   * Convenience function to convert a string to matching verbosity level.
   * logger::VERBOSITY_INVALID will be return for invalid strings.
   * Accepted string values are OFF, ERROR, WARNING, INFO, TRACE, MAX, INVALID or ASCII
   * representation for an integer in the range [-9,9].
   */
    QUARISMA_API static logger_verbosity_enum ConvertToVerbosity(const char* text);

    ///@{
    /**
   * @internal
   *
   * Not intended for public use, please use the logging macros instead.
   */
    QUARISMA_API static void Log(
        logger_verbosity_enum verbosity, const char* fname, unsigned int lineno, const char* txt);
    QUARISMA_API static void StartScope(
        logger_verbosity_enum verbosity, const char* id, const char* fname, unsigned int lineno);
    QUARISMA_API static void EndScope(const char* id);
#if !defined(__WRAP__)
    QUARISMA_API static void LogF(
        logger_verbosity_enum       verbosity,
        const char*                 fname,
        unsigned int                lineno,
        QUARISMA_FORMAT_STRING_TYPE format,
        ...) QUARISMA_PRINTF_LIKE(4, 5);
    QUARISMA_API static void StartScopeF(
        logger_verbosity_enum       verbosity,
        const char*                 id,
        const char*                 fname,
        unsigned int                lineno,
        QUARISMA_FORMAT_STRING_TYPE format,
        ...) QUARISMA_PRINTF_LIKE(5, 6);

    class QUARISMA_VISIBILITY LogScopeRAII
    {
    public:
        QUARISMA_API LogScopeRAII();
        QUARISMA_API LogScopeRAII(
            logger_verbosity_enum       verbosity,
            const char*                 fname,
            unsigned int                lineno,
            QUARISMA_FORMAT_STRING_TYPE format,
            ...) QUARISMA_PRINTF_LIKE(5, 6);
        QUARISMA_API ~LogScopeRAII();
#if defined(_MSC_VER) && _MSC_VER > 1800
        // see loguru.hpp for the reason why this is needed on MSVC
        LogScopeRAII(LogScopeRAII&& other) : Internals(other.Internals)
        {
            other.Internals = nullptr;
        }
#else
        LogScopeRAII(LogScopeRAII&&) = default;
#endif

    private:
        LogScopeRAII(const LogScopeRAII&)   = delete;
        void operator=(const LogScopeRAII&) = delete;
        class LSInternals;
        LSInternals* Internals = nullptr;
    };
#endif
    ///@}

    /**
   * Flag to enable/disable the logging frameworks printing of a stack trace
   * when catching signals, which could lead to crashes and deadlocks in
   * certain circumstances.
   */
    static bool EnableUnsafeSignalHandler;
    static bool EnableSigabrtHandler;
    static bool EnableSigbusHandler;
    static bool EnableSigfpeHandler;
    static bool EnableSigillHandler;
    static bool EnableSigintHandler;
    static bool EnableSigsegvHandler;
    static bool EnableSigtermHandler;

    QUARISMA_DELETE_COPY_AND_MOVE(logger)

protected:
    logger();
    ~logger();

private:
    static logger_verbosity_enum InternalVerbosityLevel;
};
}  // namespace quarisma

///@{
/**
 * @brief Primary logging macros using fmt-style formatting.
 *
 * These macros use modern fmt-style formatting with {} placeholders for type-safe,
 * efficient logging. All macros support compile-time format string checking via FMT_STRING.
 *
 * Examples:
 *     QUARISMA_LOG(INFO, "Simple message");
 *     QUARISMA_LOG(INFO, "Value: {}", 42);
 *     QUARISMA_LOG(INFO, "{} + {} = {}", 1, 2, 3);
 *     QUARISMA_LOG(INFO, "Pi: {:.2f}", 3.14159);
 *     QUARISMA_LOG_IF(WARNING, ptr == nullptr, "Pointer is null");
 */

/**
 * @brief Log a message with the specified verbosity level (named).
 * @param verbosity_name Verbosity level name (INFO, WARNING, ERROR, FATAL, TRACE, etc.)
 * @param format_string Format string with {} placeholders
 * @param ... Optional arguments to format
 */
#define QUARISMA_LOG(verbosity_name, format_string, ...)                        \
    do                                                                          \
    {                                                                           \
        if (quarisma::logger_verbosity_enum::VERBOSITY_##verbosity_name <=      \
            quarisma::logger::GetCurrentVerbosityCutoff())                      \
        {                                                                       \
            quarisma::logger::Log(                                              \
                quarisma::logger_verbosity_enum::VERBOSITY_##verbosity_name,    \
                __FILE__,                                                       \
                __LINE__,                                                       \
                fmt::format(FMT_STRING(format_string), ##__VA_ARGS__).c_str()); \
        }                                                                       \
    } while (0)

/**
 * @brief Debug-only logging macro (only active in debug builds).
 * @param verbosity_name Verbosity level name
 * @param format_string Format string with {} placeholders
 * @param ... Optional arguments to format
 */
#ifndef NDEBUG
#define QUARISMA_LOG_DEBUG(verbosity_name, format_string, ...) \
    QUARISMA_LOG(verbosity_name, format_string, ##__VA_ARGS__)
#else
#define QUARISMA_LOG_DEBUG(verbosity_name, format_string, ...)
#endif
///@}

///@{
/**
 * @brief Conditional logging macros - log only when condition is true.
 *
 * Examples:
 *     QUARISMA_LOG_IF(ERROR, ptr == nullptr, "Pointer is null");
 *     QUARISMA_VLOG_IF(1, value > 100, "Value {} exceeds threshold", value);
 */

/**
 * @brief Log a message with numeric verbosity level only if condition is true.
 * @param level Numeric verbosity level
 * @param cond Condition to check
 * @param format_string Format string with {} placeholders
 * @param ... Optional arguments to format
 */
#define QUARISMA_VLOG_IF(level, cond, format_string, ...)                       \
    do                                                                          \
    {                                                                           \
        if ((cond) && static_cast<quarisma::logger_verbosity_enum>(level) <=    \
                          quarisma::logger::GetCurrentVerbosityCutoff())        \
        {                                                                       \
            quarisma::logger::Log(                                              \
                static_cast<quarisma::logger_verbosity_enum>(level),            \
                __FILE__,                                                       \
                __LINE__,                                                       \
                fmt::format(FMT_STRING(format_string), ##__VA_ARGS__).c_str()); \
        }                                                                       \
    } while (0)

/**
 * @brief Log a message with named verbosity level only if condition is true.
 * @param verbosity_name Verbosity level name (INFO, WARNING, ERROR, FATAL, etc.)
 * @param cond Condition to check
 * @param format_string Format string with {} placeholders
 * @param ... Optional arguments to format
 */
#define QUARISMA_LOG_IF(verbosity_name, cond, format_string, ...)                    \
    do                                                                               \
    {                                                                                \
        if ((cond) && quarisma::logger_verbosity_enum::VERBOSITY_##verbosity_name <= \
                          quarisma::logger::GetCurrentVerbosityCutoff())             \
        {                                                                            \
            quarisma::logger::Log(                                                   \
                quarisma::logger_verbosity_enum::VERBOSITY_##verbosity_name,         \
                __FILE__,                                                            \
                __LINE__,                                                            \
                fmt::format(FMT_STRING(format_string), ##__VA_ARGS__).c_str());      \
        }                                                                            \
    } while (0)
///@}

///@{
/**
 * @brief Scope logging macros for RAII-style logging.
 *
 * These macros create a scope that logs entry (and optionally exit with timing).
 * The scope is automatically closed when the variable goes out of scope.
 *
 * Note: Scope logging with formatted messages is not supported in the fmt-style API.
 * Use QUARISMA_LOG_START_SCOPE and QUARISMA_LOG_END_SCOPE for explicit scope control.
 *
 * Examples:
 *     {
 *         QUARISMA_LOG_SCOPE_FUNCTION(INFO);  // Logs function name
 *         // ... function body ...
 *     }  // Automatically logs exit
 *
 *     QUARISMA_LOG_START_SCOPE(INFO, "my-scope");
 *     // ... some work ...
 *     QUARISMA_LOG_END_SCOPE("my-scope");
 */

#define QUARISMALOG_CONCAT_IMPL(s1, s2) s1##s2
#define QUARISMALOG_CONCAT(s1, s2) QUARISMALOG_CONCAT_IMPL(s1, s2)
#define QUARISMALOG_ANONYMOUS_VARIABLE(x) QUARISMALOG_CONCAT(x, __LINE__)

/**
 * @brief Log the current function name as a scope (RAII).
 * @param verbosity_name Verbosity level name
 */
#define QUARISMA_LOG_SCOPE_FUNCTION(verbosity_name)                            \
    auto QUARISMALOG_ANONYMOUS_VARIABLE(msg_context) =                         \
        (quarisma::logger_verbosity_enum::VERBOSITY_##verbosity_name >         \
         quarisma::logger::GetCurrentVerbosityCutoff())                        \
            ? quarisma::logger::LogScopeRAII()                                 \
            : quarisma::logger::LogScopeRAII(                                  \
                  quarisma::logger_verbosity_enum::VERBOSITY_##verbosity_name, \
                  __FILE__,                                                    \
                  __LINE__,                                                    \
                  "%s",                                                        \
                  __func__)

/**
 * @brief Log the current function name as a scope with numeric verbosity (RAII).
 * @param level Numeric verbosity level
 */
#define QUARISMA_VLOG_SCOPE_FUNCTION(level)                            \
    auto QUARISMALOG_ANONYMOUS_VARIABLE(msg_context) =                 \
        (static_cast<quarisma::logger_verbosity_enum>(level) >         \
         quarisma::logger::GetCurrentVerbosityCutoff())                \
            ? quarisma::logger::LogScopeRAII()                         \
            : quarisma::logger::LogScopeRAII(                          \
                  static_cast<quarisma::logger_verbosity_enum>(level), \
                  __FILE__,                                            \
                  __LINE__,                                            \
                  "%s",                                                \
                  __func__)

/**
 * @brief Explicitly mark the start of a log scope.
 * @param verbosity_name Verbosity level name
 * @param id Unique identifier for the scope
 */
#define QUARISMA_LOG_START_SCOPE(verbosity_name, id) \
    quarisma::logger::StartScope(                    \
        quarisma::logger_verbosity_enum::VERBOSITY_##verbosity_name, id, __FILE__, __LINE__)

/**
 * @brief Explicitly mark the start of a log scope with numeric verbosity.
 * @param level Numeric verbosity level
 * @param id Unique identifier for the scope
 */
#define QUARISMA_VLOG_START_SCOPE(level, id) \
    quarisma::logger::StartScope(level, id, __FILE__, __LINE__)

/**
 * @brief Explicitly mark the end of a log scope.
 * @param id Unique identifier for the scope (must match the start)
 */
#define QUARISMA_LOG_END_SCOPE(id) quarisma::logger::EndScope(id)
///@}

///@{
/**
 * @brief Convenience logging macros for common severity levels.
 *
 * These macros provide shortcuts for the most commonly used severity levels.
 *
 * Examples:
 *     QUARISMA_LOG_INFO("Application started");
 *     QUARISMA_LOG_WARNING("Low memory: {} MB remaining", free_mb);
 *     QUARISMA_LOG_ERROR("Failed to open file: {}", filename);
 *     QUARISMA_LOG_FATAL("Critical error: {}", error_msg);
 */

/**
 * @brief Log an informational message.
 * @param format_string Format string with {} placeholders
 * @param ... Optional arguments to format
 */
#define QUARISMA_LOG_INFO(format_string, ...) QUARISMA_LOG(INFO, format_string, ##__VA_ARGS__)

/**
 * @brief Log an informational message (debug builds only).
 * @param format_string Format string with {} placeholders
 * @param ... Optional arguments to format
 */
#ifndef NDEBUG
#define QUARISMA_LOG_INFO_DEBUG(format_string, ...) QUARISMA_LOG_INFO(format_string, ##__VA_ARGS__)
#else
#define QUARISMA_LOG_INFO_DEBUG(format_string, ...)
#endif  // !NDEBUG

/**
 * @brief Log a warning message.
 * @param format_string Format string with {} placeholders
 * @param ... Optional arguments to format
 */
#define QUARISMA_LOG_WARNING(format_string, ...) QUARISMA_LOG(WARNING, format_string, ##__VA_ARGS__)
//#define QUARISMA_WARN_ONCE(format_string, ...) QUARISMA_LOG_WARNING(format_string, ##__VA_ARGS__)

/**
 * @brief Log an error message.
 * @param format_string Format string with {} placeholders
 * @param ... Optional arguments to format
 */
#define QUARISMA_LOG_ERROR(format_string, ...) QUARISMA_LOG(ERROR, format_string, ##__VA_ARGS__)

/**
 * @brief Log a fatal error message.
 * @param format_string Format string with {} placeholders
 * @param ... Optional arguments to format
 */
#define QUARISMA_LOG_FATAL(format_string, ...) QUARISMA_LOG(FATAL, format_string, ##__VA_ARGS__)
///@}

/**
 * Convenience macros to start and end logging to a file. provide a file name
 * with the full path and extension.
 */
#define START_LOG_TO_FILE(file_name)                                                            \
    if (!file_name.empty())                                                                     \
    {                                                                                           \
        quarisma::logger::SetStderrVerbosity(quarisma::logger_verbosity_enum::VERBOSITY_TRACE); \
        quarisma::logger::LogToFile(                                                            \
            file_name.c_str(),                                                                  \
            quarisma::logger::FileMode::TRUNCATE,                                               \
            quarisma::logger_verbosity_enum::VERBOSITY_TRACE);                                  \
    }

#define END_LOG_TO_FILE(file_name)                         \
    if (!file_name.empty())                                \
    {                                                      \
        quarisma::logger::EndLogToFile(file_name.c_str()); \
    }

/**
 * Convenience macros to start and end logging to a file. provide a file name
 * without the extension. The extension `.log` will be added automatically.
 */
#define LOG_TO_FILE_NAME(file_name) std::string(std::string(#file_name) + ".log")
#define START_LOG_TO_FILE_NAME(file_name) START_LOG_TO_FILE(LOG_TO_FILE_NAME(file_name))
#define END_LOG_TO_FILE_NAME(file_name) END_LOG_TO_FILE(LOG_TO_FILE_NAME(file_name))