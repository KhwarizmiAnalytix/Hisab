#pragma once

#include <fmt/format.h>

#include <algorithm>
#include <atomic>
#include <exception>  // for exception
#include <memory>     // for shared_ptr
#include <string>     // for string
#include <vector>     // for vector

#include "common/macros.h"
#include "logging/logger.h"
#include "util/lazy.h"
#include "util/string_util.h"

#define QUARISMA_STRINGIZE_IMPL(x) #x
#define QUARISMA_STRINGIZE(x) QUARISMA_STRINGIZE_IMPL(x)

namespace quarisma
{
// ============================================================================
// Exception Behavior Configuration
// ============================================================================

/**
 * @brief Exception handling mode
 *
 * Controls whether exceptions are thrown or logged as fatal errors.
 */
enum class exception_mode
{
    THROW,     ///< Throw exceptions (default behavior)
    LOG_FATAL  ///< Log as fatal error instead of throwing
};

/**
 * @brief Get current exception handling mode
 *
 * Thread-safe accessor for the global exception mode.
 *
 * @return Current exception mode
 */
QUARISMA_API exception_mode get_exception_mode() noexcept;

/**
 * @brief Set exception handling mode
 *
 * Thread-safe setter for the global exception mode.
 * Can be controlled via:
 * - Runtime API: set_exception_mode()
 * - Environment variable: QUARISMA_EXCEPTION_MODE=THROW|LOG_FATAL
 * - Compile-time: QUARISMA_DEFAULT_EXCEPTION_MODE
 *
 * @param mode New exception mode
 */
QUARISMA_API void set_exception_mode(exception_mode mode) noexcept;

/**
 * @brief Initialize exception mode from environment
 *
 * Reads QUARISMA_EXCEPTION_MODE environment variable and sets the mode.
 * Called automatically during library initialization.
 */
QUARISMA_API void init_exception_mode_from_env() noexcept;

// ============================================================================
// Exception Categories
// ============================================================================

/**
 * @brief Exception category enumeration
 *
 * Categorizes exceptions for better error handling and reporting.
 * Replaces the previous specialized exception class hierarchy.
 */
enum class exception_category : int
{
    GENERIC,          ///< Generic error (default)
    VALUE_ERROR,      ///< Invalid value error
    TYPE_ERROR,       ///< Type mismatch error
    INDEX_ERROR,      ///< Index out of bounds error
    NOT_IMPLEMENTED,  ///< Feature not implemented
    ENFORCE_FINITE,   ///< Non-finite value error
    RUNTIME_ERROR,    ///< Runtime error
    LOGIC_ERROR,      ///< Logic error
    SYSTEM_ERROR,     ///< System-level error
    MEMORY_ERROR      ///< Memory-related error
};

/**
 * @brief Enhanced exception class with configurable behavior
 *
 * Features:
 * - Automatic stack trace capture
 * - Source location tracking (file, line, function)
 * - Exception chaining/nesting
 * - Configurable throw vs log-fatal behavior
 * - fmt-style message formatting
 * - Context accumulation for error propagation
 * - Error categorization for better error handling
 *
 * **Thread Safety**: All methods are thread-safe except add_context()
 * which should only be called from the thread that created the exception.
 *
 * **Performance**: Stack trace capture adds ~25μs overhead. Consider
 * disabling in hot paths if needed.
 *
 * **Naming**: Uses lowercase 'exception' following standard C++ conventions.
 * Type alias 'Error' provided for backward compatibility.
 */
class QUARISMA_VISIBILITY exception : public std::exception
{
    // The actual error message.
    std::string msg_;

    // Context for the message (in order of decreasing specificity).  Context will
    // be automatically formatted appropriately, so it is not necessary to add
    // extra leading/trailing newlines to strings inside this vector
    std::vector<std::string> context_;

    // The C++ backtrace at the point when this exception was raised.  This
    // may be empty if there is no valid backtrace.  (We don't use optional
    // here to reduce the dependencies this file has.)
    std::string backtrace_;

    // These two are derived fields from msg_stack_ and backtrace_, but we need
    // fields for the strings so that we can return a const char* (as the
    // signature of std::exception requires).  Currently, the invariant
    // is that these fields are ALWAYS populated consistently with respect
    // to msg_stack_ and backtrace_.
    mutable optimistic_lazy<std::string> what_;
    std::string                          what_without_backtrace_;

    // This is a little debugging trick: you can stash a relevant pointer
    // in caller, and then when you catch the exception, you can compare
    // against pointers you have on hand to get more information about
    // where the exception came from.  In quarisma, this is used to figure
    // out which operator raised an exception.
    const void* caller_;

    // Nested exception for exception chaining
    std::shared_ptr<exception> nested_exception_ = nullptr;

    // Error category for better error classification
    exception_category category_;

public:
    // Virtual destructor
    QUARISMA_API virtual ~exception();

    // quarisma-style exception constructor.
    // NB: the implementation of this is actually in exception.cpp
    QUARISMA_API exception(
        source_location source_location, std::string msg, exception_category category);

    // Base constructor
    QUARISMA_API exception(
        std::string        msg,
        std::string        backtrace,
        const void*        caller   = nullptr,
        exception_category category = exception_category::GENERIC);

    /**
     * @brief Constructor with nested exception
     *
     * @param source_location Source location where error occurred
     * @param msg Error message
     * @param nested Nested exception for chaining
     * @param category Error category
     */
    QUARISMA_API exception(
        source_location            source_location,
        std::string                msg,
        std::shared_ptr<exception> nested,
        exception_category         category = exception_category::GENERIC);

    // Add some new context to the message stack.  The last added context
    // will be formatted at the end of the context list upon printing.
    // WARNING: This method is O(n) in the size of the stack, so don't go
    // wild adding a ridiculous amount of context to error messages.
    QUARISMA_API void add_context(std::string msg);

    /// Get error message
    /// @note Inline for performance - frequently accessed in hot paths
    inline const std::string& msg() const noexcept { return msg_; }

    /// Get context stack
    /// @note Inline for performance - frequently accessed in hot paths
    inline const std::vector<std::string>& context() const noexcept { return context_; }

    /// Get backtrace
    /// @note Inline for performance - frequently accessed in hot paths
    inline const std::string& backtrace() const noexcept { return backtrace_; }

    /// Get error category
    /// @note Inline for performance - frequently accessed in hot paths
    inline exception_category category() const noexcept { return category_; }

    /// Returns the complete error message, including the source location.
    /// The returned pointer is invalidated if you call add_context() on
    /// this object.
    QUARISMA_API const char* what() const noexcept override;

    /// Get caller pointer (for debugging)
    /// @note Inline for performance - frequently accessed in hot paths
    inline const void* caller() const noexcept { return caller_; }

    /// Returns only the error message string, without source location.
    /// The returned pointer is invalidated if you call add_context() on
    /// this object.
    /// @note Inline for performance - frequently accessed in hot paths
    inline const char* what_without_backtrace() const noexcept
    {
        return what_without_backtrace_.c_str();
    }

    /// Get nested exception (if any)
    /// @note Inline for performance - frequently accessed in hot paths
    inline const std::shared_ptr<exception>& nested() const noexcept { return nested_exception_; }

private:
    void        refresh_what();
    std::string compute_what(bool include_backtrace) const;
};

}  // namespace quarisma

// ============================================================================
// Exception Throwing Macros
// ============================================================================

/**
 * @brief Internal helper for throwing exceptions with exception mode support
 *
 * Respects the global exception mode setting (THROW vs LOG_FATAL).
 *
 * @param error_cat Error category enum value
 * @param msg Error message string
 */
#define QUARISMA_THROW_IMPL(error_cat, msg)                                               \
    do                                                                                    \
    {                                                                                     \
        quarisma::source_location loc;                                                    \
        loc.function = __func__;                                                          \
        loc.file     = __FILE__;                                                          \
        loc.line     = static_cast<int>(__LINE__);                                        \
        if (quarisma::get_exception_mode() == quarisma::exception_mode::THROW)            \
        {                                                                                 \
            throw quarisma::exception(loc, msg, quarisma::exception_category::error_cat); \
        }                                                                                 \
        {                                                                                 \
            QUARISMA_LOG_FATAL("Fatal error ({}): {}", #error_cat, msg);                  \
        }                                                                                 \
    } while (0)

/**
 * @brief Throw a generic exception with fmt-style formatted message
 *
 * Uses fmt-style formatting with {} placeholders. Throws quarisma::Error
 * (alias for quarisma::exception with GENERIC category).
 *
 * @param format_str Format string with {} placeholders
 * @param ... Format arguments
 *
 * **Example**:
 * ```cpp
 * QUARISMA_THROW("Invalid state: {}", state_name);
 * ```
 */
#define QUARISMA_THROW(format_str, ...) \
    QUARISMA_THROW_IMPL(GENERIC, fmt::format(FMT_STRING(format_str), ##__VA_ARGS__))

// ============================================================================
// Exception Check Macros
// ============================================================================

namespace quarisma
{
namespace details
{
[[noreturn]] QUARISMA_API void check_fail(
    const char* func, const char* file, int line, const std::string& msg);

// Helper to format check messages with optional arguments
template <typename... Args>
inline std::string format_check_msg(
    const char* cond_str, fmt::format_string<Args...> fmt, Args&&... args)
{
    std::string user_msg = fmt::format(fmt, std::forward<Args>(args)...);
    if (user_msg.empty())
    {
        return fmt::format(FMT_STRING("Check failed: {}"), cond_str);
    }
    return fmt::format(FMT_STRING("Check failed: {} - {}"), cond_str, user_msg);
}

// Overload for no arguments
inline std::string format_check_msg(const char* cond_str)
{
    return fmt::format(FMT_STRING("Check failed: {}"), cond_str);
}
}  // namespace details
}  // namespace quarisma

/**
 * @brief Check condition and throw exception with fmt-style message if false
 *
 * Uses fmt-style formatting with {} placeholders. If the condition is false,
 * logs a warning and throws quarisma::Error (GENERIC category).
 *
 * @param cond Condition to check
 * @param format_str Format string with {} placeholders (optional)
 * @param ... Format arguments
 *
 * **Examples**:
 * ```cpp
 * QUARISMA_CHECK(x > 0, "x must be positive, got {}", x);
 * QUARISMA_CHECK(ptr != nullptr, "Null pointer encountered");
 * QUARISMA_CHECK(!empty());  // Simple check without message
 * ```
 */
#define QUARISMA_CHECK(cond, ...)                                                    \
    if QUARISMA_UNLIKELY (!(cond))                                                   \
    {                                                                                \
        std::string msg = quarisma::details::format_check_msg(#cond, ##__VA_ARGS__); \
        QUARISMA_THROW("{}", msg);                                                   \
    }

#define QUARISMA_CHECK_ALL_POSITIVE(V)                                 \
    QUARISMA_CHECK(                                                    \
        std::all_of(V.begin(), V.end(), [](auto x) { return x > 0; }), \
        "All elements must be positive");

#define QUARISMA_CHECK_ALL_FINITE(V)                                                             \
    QUARISMA_CHECK(                                                                              \
        std::none_of(V.begin(), V.end(), [](auto x) { return std::isnan(x) || std::isinf(x); }), \
        "All elements must be finite numbers");

#define QUARISMA_CHECK_STRICTLY_INCREASING(V)                                                     \
    QUARISMA_CHECK(                                                                               \
        std::adjacent_find(V.begin(), V.end(), [](auto a, auto b) { return a >= b; }) == V.end(), \
        "Elements must be in strictly increasing order");

#define QUARISMA_CHECK_STRICTLY_DECREASING(V)                                                     \
    QUARISMA_CHECK(                                                                               \
        std::adjacent_find(V.begin(), V.end(), [](auto a, auto b) { return a <= b; }) == V.end(), \
        "Elements must be in strictly decreasing order");

#define QUARISMA_CHECK_STRICTLY_ORDERED(V)                                                 \
    QUARISMA_CHECK(                                                                        \
        ((std::adjacent_find(V.begin(), V.end(), [](auto a, auto b) { return a >= b; }) == \
          V.end()) ||                                                                      \
         (std::adjacent_find(V.begin(), V.end(), [](auto a, auto b) { return a <= b; }) == \
          V.end())),                                                                       \
        "Elements must be strictly ordered (increasing or decreasing)");

/**
 * @brief Throw NotImplementedError with fmt-style message
 *
 * Throws quarisma::NotImplementedError (alias for quarisma::exception with
 * NOT_IMPLEMENTED category).
 *
 * @param format_str Format string with {} placeholders
 * @param ... Format arguments
 *
 * **Example**:
 * ```cpp
 * QUARISMA_NOT_IMPLEMENTED("Feature {} not yet implemented", feature_name);
 * ```
 */
#define QUARISMA_NOT_IMPLEMENTED(format_str, ...) \
    QUARISMA_THROW_IMPL(NOT_IMPLEMENTED, fmt::format(FMT_STRING(format_str), ##__VA_ARGS__))

/**
 * @brief Debug-only version of QUARISMA_CHECK
 *
 * Only active in debug builds (when NDEBUG is not defined).
 * In release builds, this macro expands to nothing.
 *
 * @param condition Condition to check
 * @param ... Optional format string and arguments
 *
 * **Example**:
 * ```cpp
 * QUARISMA_CHECK_DEBUG(x > 0, "x must be positive, got {}", x);
 * QUARISMA_CHECK_DEBUG(!empty());
 * ```
 */
#ifdef NDEBUG
#define QUARISMA_CHECK_DEBUG(condition, ...)
#else
#define QUARISMA_CHECK_DEBUG(condition, ...)      \
    do                                            \
    {                                             \
        QUARISMA_CHECK(condition, ##__VA_ARGS__); \
    } while (0)
#endif

#define QUARISMA_WARN_ONCE(msg)                 \
    do                                          \
    {                                           \
        static std::atomic<bool> warned{false}; \
        if (!warned.exchange(true))             \
        {                                       \
            QUARISMA_LOG_WARNING(msg);          \
        }                                       \
    } while (0)
