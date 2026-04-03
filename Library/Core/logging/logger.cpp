#include "logging/logger.h"

#include <array>    // for array
#include <cstdarg>  // for va_end, va_list, va_start
#include <cstdio>   // for vsnprintf
#include <cstdlib>  // for strtol
#include <cstring>  // for strcmp, strlen, strncpy
#include <memory>   // for make_shared, shared_ptr, make_unique, unique_ptr
#include <mutex>    // for mutex, lock_guard
#include <string>
#include <thread>   // for get_id, operator==, thread
#include <utility>  // for pair
#include <vector>   // for vector

#include "common/macros.h"
#include "logging/logger_verbosity_enum.h"
#include "util/flat_hash.h"

// Include appropriate logging backend headers
#if QUARISMA_HAS_LOGURU
#include <loguru.hpp>  // for LogScopeRAII, Message, Options, Verbosity, SignalOptions, g_stderr_verbosity, remov...
#elif QUARISMA_HAS_GLOG
#include <glog/logging.h>
#elif QUARISMA_HAS_NATIVE_LOGGING
#include <fmt/color.h>
#include <fmt/format.h>

#include <atomic>
#include <chrono>
#endif

//=============================================================================
// NATIVE LOGGING BACKEND - Simplified fmt-based Implementation
//=============================================================================
#if QUARISMA_HAS_NATIVE_LOGGING

namespace quarisma
{
namespace internal
{

// Helper to convert verbosity enum to string
static const char* verbosity_to_string(logger_verbosity_enum severity)
{
    switch (severity)
    {
    case logger_verbosity_enum::VERBOSITY_FATAL:
        return "FATAL";
    case logger_verbosity_enum::VERBOSITY_ERROR:
        return "ERROR";
    case logger_verbosity_enum::VERBOSITY_WARNING:
        return "WARNING";
    case logger_verbosity_enum::VERBOSITY_INFO:
        return "INFO";
    default:
        return "VLOG";
    }
}

// Helper to get color for severity level
static fmt::color get_severity_color(logger_verbosity_enum severity)
{
    switch (severity)
    {
    case logger_verbosity_enum::VERBOSITY_FATAL:
        return fmt::color::red;
    case logger_verbosity_enum::VERBOSITY_ERROR:
        return fmt::color::red;
    case logger_verbosity_enum::VERBOSITY_WARNING:
        return fmt::color::yellow;
    case logger_verbosity_enum::VERBOSITY_INFO:
        return fmt::color::green;
    default:
        return fmt::color::white;
    }
}

// Global verbosity level for VLOG
static std::atomic<int> g_max_vlog_level{0};

// Implementation functions called by inline classes in logger.h
void native_log_output(
    const char* fname, int line, logger_verbosity_enum severity, const std::string& message)
{
    if (message.empty())
    {
        return;
    }

    // Extract just the filename from the full path
    const char* filename = fname;
    for (const char* p = fname; *p; ++p)
    {
        if (*p == '/' || *p == '\\')
        {
            filename = p + 1;
        }
    }

    // Format and print the log message
    fmt::print(
        stderr,
        fg(get_severity_color(severity)),
        "[{}] {}:{} {}\n",
        verbosity_to_string(severity),
        filename,
        line,
        message);
}

int native_max_vlog_level()
{
    return g_max_vlog_level.load(std::memory_order_relaxed);
}

void native_fatal_exit()
{
    std::abort();
}

std::string native_check_failed(
    const char* exprtext, const std::string& v1_str, const std::string& v2_str)
{
    return fmt::format("Check failed: {} ({} vs. {})", exprtext, v1_str, v2_str);
}

}  // namespace internal
}  // namespace quarisma

#endif  // QUARISMA_HAS_NATIVE_LOGGING

//=============================================================================
namespace quarisma
{
class logger::LogScopeRAII::LSInternals
{
public:
#if QUARISMA_HAS_LOGURU
    std::unique_ptr<loguru::LogScopeRAII> Data;
#elif QUARISMA_HAS_GLOG
    // glog doesn't have a direct scope RAII equivalent
    // We'll track scope manually for consistency
    std::string scope_message_;
#elif QUARISMA_HAS_NATIVE_LOGGING
    // Native logging doesn't have scope support yet
    // Placeholder for future implementation
#endif
};

logger::LogScopeRAII::LogScopeRAII() = default;

// printf-style scope entry; variadic form matches loguru / vsnprintf usage.
// NOLINTNEXTLINE(modernize-avoid-variadic-functions)
logger::LogScopeRAII::LogScopeRAII(
    logger_verbosity_enum verbosity,
    const char*           fname,
    unsigned int          lineno,
    const char*           format,
    ...)
#if QUARISMA_HAS_LOGURU
    : Internals(new LSInternals())
#endif
{
#if QUARISMA_HAS_LOGURU
    va_list vlist;
    va_start(vlist, format);
    auto result = loguru::vstrprintf(format, vlist);
    va_end(vlist);
    this->Internals->Data = std::make_unique<loguru::LogScopeRAII>(
        static_cast<loguru::Verbosity>(verbosity), fname, lineno, "%s", result.c_str());
#elif QUARISMA_HAS_GLOG
    va_list vlist;
    va_start(vlist, format);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, vlist);
    va_end(vlist);
    this->Internals                 = new LSInternals();
    this->Internals->scope_message_ = buffer;
    // glog doesn't have built-in scope support, so we just log the scope entry
    VLOG(static_cast<int>(verbosity)) << "[SCOPE] " << buffer;
#elif QUARISMA_HAS_NATIVE_LOGGING
    // Native logging doesn't have scope support yet
    (void)verbosity;
    (void)fname;
    (void)lineno;
    (void)format;
#else
    (void)verbosity;
    (void)fname;
    (void)lineno;
    (void)format;
#endif
}

logger::LogScopeRAII::~LogScopeRAII()
{
    delete this->Internals;
}
//=============================================================================
namespace detail
{
#if QUARISMA_HAS_LOGURU
using scope_pair = std::pair<std::string, std::shared_ptr<loguru::LogScopeRAII>>;
static std::mutex g_mutex;

static quarisma_map<std::thread::id, std::vector<scope_pair>>& scope_vectors()
{
    static quarisma_map<std::thread::id, std::vector<scope_pair>> vectors;
    return vectors;
}

static std::vector<scope_pair>& get_vector()
{
    const std::scoped_lock guard(g_mutex);
    return scope_vectors()[std::this_thread::get_id()];
}

static void push_scope(const char* id, std::shared_ptr<loguru::LogScopeRAII> ptr)
{
    get_vector().emplace_back(std::string(id), ptr);
}

static void pop_scope(const char* id)
{
    auto& vector = get_vector();
    if (!vector.empty() && vector.back().first == id)
    {
        vector.pop_back();

        if (vector.empty())
        {
            const std::scoped_lock guard(g_mutex);
            scope_vectors().erase(std::this_thread::get_id());
        }
    }
    else
    {
        LOG_F(ERROR, "Mismatched scope! expected (%s), got (%s)", vector.back().first.c_str(), id);
    }
}
static thread_local char ThreadName[128] = {};
#elif QUARISMA_HAS_GLOG || QUARISMA_HAS_NATIVE_LOGGING
// For glog and native logging, we maintain a simple thread name storage
static thread_local char ThreadName[128] = {};
#endif
}  // namespace detail

//=============================================================================
bool                  logger::EnableUnsafeSignalHandler = true;
bool                  logger::EnableSigabrtHandler      = false;
bool                  logger::EnableSigbusHandler       = false;
bool                  logger::EnableSigfpeHandler       = false;
bool                  logger::EnableSigillHandler       = false;
bool                  logger::EnableSigintHandler       = false;
bool                  logger::EnableSigsegvHandler      = false;
bool                  logger::EnableSigtermHandler      = false;
logger_verbosity_enum logger::InternalVerbosityLevel    = logger_verbosity_enum::VERBOSITY_INFO;

//------------------------------------------------------------------------------
logger::logger() = default;

//------------------------------------------------------------------------------
logger::~logger() = default;

//------------------------------------------------------------------------------
void logger::Init(int& argc, char* argv[], const char* verbosity_flag /*= "-v"*/)
{
#if QUARISMA_HAS_LOGURU
    if (argc == 0)
    {  // loguru::init can't handle this case -- call the no-arg overload.
        logger::Init();
        return;
    }

    loguru::g_preamble_date      = false;
    loguru::g_preamble_time      = false;
    loguru::g_internal_verbosity = static_cast<loguru::Verbosity>(logger::InternalVerbosityLevel);

    const auto current_stderr_verbosity = loguru::g_stderr_verbosity;
    if (loguru::g_internal_verbosity > loguru::g_stderr_verbosity)
    {
        // this avoids printing the preamble-header on stderr except for cases
        // where the stderr log is guaranteed to have some log text generated.
        loguru::g_stderr_verbosity = loguru::Verbosity_WARNING;
    }
    loguru::Options options;
    options.verbosity_flag                       = verbosity_flag;
    options.signal_options.unsafe_signal_handler = logger::EnableUnsafeSignalHandler;
    options.signal_options.sigabrt               = logger::EnableSigabrtHandler;
    options.signal_options.sigbus                = logger::EnableSigbusHandler;
    options.signal_options.sigfpe                = logger::EnableSigfpeHandler;
    options.signal_options.sigill                = logger::EnableSigillHandler;
    options.signal_options.sigint                = logger::EnableSigintHandler;
    options.signal_options.sigsegv               = logger::EnableSigsegvHandler;
    options.signal_options.sigterm               = logger::EnableSigtermHandler;
    if (strlen(detail::ThreadName) > 0)
    {
        options.main_thread_name = detail::ThreadName;
    }
    loguru::init(argc, argv, options);
    loguru::g_stderr_verbosity = current_stderr_verbosity;
#elif QUARISMA_HAS_GLOG
    // Initialize glog
    if (!google::IsGoogleLoggingInitialized())
    {
        google::InitGoogleLogging(argc > 0 ? argv[0] : "quarisma");
    }

    // Enable colored output to stderr
    FLAGS_colorlogtostderr = true;

    // Ensure logs go to stderr for colored output
    FLAGS_logtostderr = true;

    // Disable logging to files by default (can be re-enabled via LogToFile)
    FLAGS_alsologtostderr = false;

    // Parse verbosity flag if provided
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == verbosity_flag && i + 1 < argc)
        {
            FLAGS_v = std::atoi(argv[i + 1]);
            break;
        }
    }

    // Configure glog based on signal handler settings
    google::InstallFailureSignalHandler();
#elif QUARISMA_HAS_NATIVE_LOGGING
    // Native logging initialization (minimal setup)
    // Parse verbosity flag if needed
    (void)argc;
    (void)argv;
    (void)verbosity_flag;
#else
    (void)argc;
    (void)argv;
    (void)verbosity_flag;
#endif
}

//------------------------------------------------------------------------------
void logger::Init()
{
    int                  argc  = 1;
    std::array<char, 1>  dummy = {'\0'};
    std::array<char*, 2> argv  = {dummy.data(), nullptr};
    logger::Init(argc, argv.data());
}

//------------------------------------------------------------------------------
void logger::SetStderrVerbosity(logger_verbosity_enum level)
{
#if QUARISMA_HAS_LOGURU
    loguru::g_stderr_verbosity = static_cast<loguru::Verbosity>(level);
#elif QUARISMA_HAS_GLOG
    // glog uses FLAGS_stderrthreshold to control stderr output
    // Map verbosity levels to glog severity levels
    if (level <= logger_verbosity_enum::VERBOSITY_ERROR)
    {
        FLAGS_stderrthreshold = google::GLOG_ERROR;
    }
    else if (level <= logger_verbosity_enum::VERBOSITY_WARNING)
    {
        FLAGS_stderrthreshold = google::GLOG_WARNING;
    }
    else
    {
        FLAGS_stderrthreshold = google::GLOG_INFO;
    }
#elif QUARISMA_HAS_NATIVE_LOGGING
    // Native logging doesn't have separate stderr verbosity control yet
    (void)level;
#else
    (void)level;
#endif
}

//------------------------------------------------------------------------------
void logger::SetInternalVerbosityLevel(logger_verbosity_enum level)
{
#if QUARISMA_HAS_LOGURU
    loguru::g_internal_verbosity   = static_cast<loguru::Verbosity>(level);
    logger::InternalVerbosityLevel = level;
#elif QUARISMA_HAS_GLOG
    FLAGS_v                        = static_cast<int>(level);
    logger::InternalVerbosityLevel = level;
#elif QUARISMA_HAS_NATIVE_LOGGING
    logger::InternalVerbosityLevel = level;
#else
    (void)level;
#endif
}

//------------------------------------------------------------------------------
void logger::LogToFile(const char* path, logger::FileMode filemode, logger_verbosity_enum verbosity)
{
#if QUARISMA_HAS_LOGURU
    loguru::add_file(
        path, static_cast<loguru::FileMode>(filemode), static_cast<loguru::Verbosity>(verbosity));
#elif QUARISMA_HAS_GLOG
    // glog file logging
    if (filemode == logger::FileMode::TRUNCATE)
    {
        FLAGS_log_dir = "";  // Clear log directory to use custom path
    }
    google::SetLogDestination(google::GLOG_INFO, path);
#elif QUARISMA_HAS_NATIVE_LOGGING
    // Native logging file support not implemented yet
    (void)path;
    (void)filemode;
    (void)verbosity;
#else
    (void)path;
    (void)filemode;
    (void)verbosity;
#endif
}

//------------------------------------------------------------------------------
void logger::EndLogToFile(const char* path)
{
#if QUARISMA_HAS_LOGURU
    loguru::remove_callback(path);
#elif QUARISMA_HAS_GLOG
    // glog doesn't have a direct way to remove a specific log file
    // We can flush and close all log files
    google::FlushLogFiles(google::GLOG_INFO);
#elif QUARISMA_HAS_NATIVE_LOGGING
    (void)path;
#else
    (void)path;
#endif
}

//------------------------------------------------------------------------------
void logger::SetThreadName(const std::string& name)
{
#if QUARISMA_HAS_LOGURU
    loguru::set_thread_name(name.c_str());
    // Save threadname so if this is called before `Init`, we can pass the thread
    // name to loguru::init().
    strncpy(detail::ThreadName, name.c_str(), sizeof(detail::ThreadName) - 1);
#elif QUARISMA_HAS_GLOG || QUARISMA_HAS_NATIVE_LOGGING
    // Store thread name for potential future use
    strncpy(detail::ThreadName, name.c_str(), sizeof(detail::ThreadName) - 1);
    detail::ThreadName[sizeof(detail::ThreadName) - 1] = '\0';
#else
    (void)name;
#endif
}

//------------------------------------------------------------------------------
std::string logger::GetThreadName()
{
#if QUARISMA_HAS_LOGURU
    char buffer[128];
    loguru::get_thread_name(buffer, 128, false);
    return {buffer};
#elif QUARISMA_HAS_GLOG || QUARISMA_HAS_NATIVE_LOGGING
    if (strlen(detail::ThreadName) > 0)
    {
        return {detail::ThreadName};
    }
    return {"N/A"};
#else
    return {"N/A"};
#endif
}

namespace
{
#if QUARISMA_HAS_LOGURU
struct CallbackBridgeData
{
    logger::LogHandlerCallbackT   handler;
    logger::CloseHandlerCallbackT close;
    logger::FlushHandlerCallbackT flush;
    void*                         inner_data;
};

void loguru_callback_bridge_handler(void* user_data, const loguru::Message& message)
{
    auto* data = reinterpret_cast<CallbackBridgeData*>(user_data);

    auto quarisma_message = logger::Message{
        static_cast<logger_verbosity_enum>(message.verbosity),
        message.filename,
        message.line,
        message.preamble,
        message.indentation,
        message.prefix,
        message.message,
    };

    data->handler(data->inner_data, quarisma_message);
}

void loguru_callback_bridge_close(void* user_data)
{
    auto* data = reinterpret_cast<CallbackBridgeData*>(user_data);

    if (data->close != nullptr)
    {
        data->close(data->inner_data);
        data->inner_data = nullptr;
    }

    delete data;
}

void loguru_callback_bridge_flush(void* user_data)
{
    auto* data = reinterpret_cast<CallbackBridgeData*>(user_data);

    if (data->flush != nullptr)
    {
        data->flush(data->inner_data);
    }
}
#endif
}  // namespace

//------------------------------------------------------------------------------
void logger::AddCallback(
    const char*                   id,
    logger::LogHandlerCallbackT   callback,
    void*                         user_data,
    logger_verbosity_enum         verbosity,
    logger::CloseHandlerCallbackT on_close,
    logger::FlushHandlerCallbackT on_flush)
{
#if QUARISMA_HAS_LOGURU
    auto* callback_data = new CallbackBridgeData{callback, on_close, on_flush, user_data};
    loguru::add_callback(
        id,
        loguru_callback_bridge_handler,
        callback_data,
        static_cast<loguru::Verbosity>(verbosity),
        loguru_callback_bridge_close,
        loguru_callback_bridge_flush);
#elif QUARISMA_HAS_GLOG || QUARISMA_HAS_NATIVE_LOGGING
    // glog and native logging don't support custom callbacks in the same way
    // FIXME: Should we call the `close` callback with `user_data` to free any
    // resources expected to be passed in here?
    (void)id;
    (void)callback;
    (void)user_data;
    (void)verbosity;
    (void)on_close;
    (void)on_flush;
#else
    (void)id;
    (void)callback;
    (void)user_data;
    (void)verbosity;
    (void)on_close;
    (void)on_flush;
#endif
}

//------------------------------------------------------------------------------
bool logger::RemoveCallback(const char* id)
{
#if QUARISMA_HAS_LOGURU
    return loguru::remove_callback(id);
#elif QUARISMA_HAS_GLOG || QUARISMA_HAS_NATIVE_LOGGING
    (void)id;
    return false;
#else
    (void)id;
    return false;
#endif
}

//------------------------------------------------------------------------------
bool logger::IsEnabled()
{
#if QUARISMA_HAS_LOGURU || QUARISMA_HAS_GLOG || QUARISMA_HAS_NATIVE_LOGGING
    return true;
#else
    return false;
#endif
}

//------------------------------------------------------------------------------
logger_verbosity_enum logger::GetCurrentVerbosityCutoff()
{
#if QUARISMA_HAS_LOGURU
    return static_cast<logger_verbosity_enum>(loguru::current_verbosity_cutoff());
#elif QUARISMA_HAS_GLOG
    return static_cast<logger_verbosity_enum>(FLAGS_v);
#elif QUARISMA_HAS_NATIVE_LOGGING
    return logger::InternalVerbosityLevel;
#else
    return logger_verbosity_enum::
        VERBOSITY_INVALID;  // return lowest value so no logging macros will be evaluated.
#endif
}

//------------------------------------------------------------------------------
void logger::Log(
    QUARISMA_UNUSED logger_verbosity_enum verbosity,
    QUARISMA_UNUSED const char*           fname,
    QUARISMA_UNUSED unsigned int          lineno,
    QUARISMA_UNUSED const char*           txt)
{
#if QUARISMA_HAS_LOGURU
    loguru::log(static_cast<loguru::Verbosity>(verbosity), fname, lineno, "%s", txt);
#elif QUARISMA_HAS_GLOG
    // Map verbosity to glog severity
    int glog_level = static_cast<int>(verbosity);
    VLOG(glog_level) << txt;
#elif QUARISMA_HAS_NATIVE_LOGGING
    // Use native logging - call the implementation function directly
    internal::native_log_output(fname, lineno, verbosity, txt);
#else
    (void)verbosity;
    (void)fname;
    (void)lineno;
    (void)txt;
#endif
}

//------------------------------------------------------------------------------
// NOLINTNEXTLINE(modernize-avoid-variadic-functions)
void logger::LogF(
    logger_verbosity_enum verbosity,
    const char*           fname,
    unsigned int          lineno,
    const char*           format,
    ...)
{
#if QUARISMA_HAS_LOGURU || QUARISMA_HAS_GLOG || QUARISMA_HAS_NATIVE_LOGGING
    va_list vlist;
    va_start(vlist, format);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, vlist);
    va_end(vlist);
    logger::Log(verbosity, fname, lineno, buffer);
#else
    (void)verbosity;
    (void)fname;
    (void)lineno;
    (void)format;
#endif
}

//------------------------------------------------------------------------------
void logger::StartScope(
    logger_verbosity_enum verbosity, const char* id, const char* fname, unsigned int lineno)
{
#if QUARISMA_HAS_LOGURU
    detail::push_scope(
        id,
        verbosity > logger::GetCurrentVerbosityCutoff()
            ? std::make_shared<loguru::LogScopeRAII>()
            : std::make_shared<loguru::LogScopeRAII>(
                  static_cast<loguru::Verbosity>(verbosity), fname, lineno, "%s", id));
#elif QUARISMA_HAS_GLOG || QUARISMA_HAS_NATIVE_LOGGING
    // glog and native logging don't have built-in scope support
    // Just log the scope entry
    logger::Log(verbosity, fname, lineno, id);
#else
    (void)verbosity;
    (void)id;
    (void)fname;
    (void)lineno;
#endif
}

//------------------------------------------------------------------------------
void logger::EndScope(const char* id)
{
#if QUARISMA_HAS_LOGURU
    detail::pop_scope(id);
#elif QUARISMA_HAS_GLOG || QUARISMA_HAS_NATIVE_LOGGING
    // No-op for glog and native logging
    (void)id;
#else
    (void)id;
#endif
}

//------------------------------------------------------------------------------
// NOLINTNEXTLINE(modernize-avoid-variadic-functions)
void logger::StartScopeF(
    logger_verbosity_enum verbosity,
    const char*           id,
    const char*           fname,
    unsigned int          lineno,
    const char*           format,
    ...)
{
#if QUARISMA_HAS_LOGURU
    if (verbosity > logger::GetCurrentVerbosityCutoff())
    {
        detail::push_scope(id, std::make_shared<loguru::LogScopeRAII>());
    }
    else
    {
        va_list vlist;
        va_start(vlist, format);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), format, vlist);
        va_end(vlist);

        detail::push_scope(
            id,
            std::make_shared<loguru::LogScopeRAII>(
                static_cast<loguru::Verbosity>(verbosity), fname, lineno, "%s", buffer));
    }
#elif QUARISMA_HAS_GLOG || QUARISMA_HAS_NATIVE_LOGGING
    va_list vlist;
    va_start(vlist, format);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, vlist);
    va_end(vlist);
    logger::Log(verbosity, fname, lineno, buffer);
#else
    (void)verbosity;
    (void)id;
    (void)fname;
    (void)lineno;
    (void)format;
#endif
}

//------------------------------------------------------------------------------
logger_verbosity_enum logger::ConvertToVerbosity(int value)
{
    if (value <= static_cast<int>(logger_verbosity_enum::VERBOSITY_INVALID))
    {
        return logger_verbosity_enum::VERBOSITY_INVALID;
    }
    if (value > static_cast<int>(logger_verbosity_enum::VERBOSITY_MAX))
    {
        return logger_verbosity_enum::VERBOSITY_MAX;
    }
    return static_cast<logger_verbosity_enum>(value);
}

//------------------------------------------------------------------------------
logger_verbosity_enum logger::ConvertToVerbosity(const char* text)
{
    if (text != nullptr)
    {
        char*     end    = nullptr;  //NOLINT
        const int ivalue = static_cast<int>(std::strtol(text, &end, 10));
        if (end != text && *end == '\0')
        {
            return logger::ConvertToVerbosity(ivalue);
        }
        if (strcmp(text, "OFF") == 0)
        {
            return logger_verbosity_enum::VERBOSITY_OFF;
        }
        if (strcmp(text, "ERROR") == 0)
        {
            return logger_verbosity_enum::VERBOSITY_ERROR;
        }
        if (strcmp(text, "WARNING") == 0)
        {
            return logger_verbosity_enum::VERBOSITY_WARNING;
        }
        if (strcmp(text, "INFO") == 0)
        {
            return logger_verbosity_enum::VERBOSITY_INFO;
        }
        if (strcmp(text, "TRACE") == 0)
        {
            return logger_verbosity_enum::VERBOSITY_TRACE;
        }
        if (strcmp(text, "MAX") == 0)
        {
            return logger_verbosity_enum::VERBOSITY_MAX;
        }
    }
    return logger_verbosity_enum::VERBOSITY_INVALID;
}
}  // namespace quarisma
