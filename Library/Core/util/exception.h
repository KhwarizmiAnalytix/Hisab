#pragma once

#include "common/macros.h"
#include "../../Logging/util/exception.h"

namespace quarisma
{
using exception_mode = logging::exception_mode;
using exception_category = logging::exception_category;
using source_location = logging::source_location;
using exception = logging::exception;

inline exception_mode get_exception_mode() noexcept { return logging::get_exception_mode(); }
inline void set_exception_mode(exception_mode mode) noexcept { logging::set_exception_mode(mode); }
inline void init_exception_mode_from_env() noexcept { logging::init_exception_mode_from_env(); }
namespace details = logging::details;
}  // namespace quarisma

#ifndef QUARISMA_CHECK
#define QUARISMA_CHECK(cond, ...) LOGGING_CHECK(cond, ##__VA_ARGS__)
#endif

#ifndef QUARISMA_CHECK_DEBUG
#define QUARISMA_CHECK_DEBUG(cond, ...) LOGGING_CHECK_DEBUG(cond, ##__VA_ARGS__)
#endif

#ifndef LOGGING_CHECK_DEBUG
#define LOGGING_CHECK_DEBUG(cond, ...) LOGGING_CHECK(cond, ##__VA_ARGS__)
#endif

#ifndef QUARISMA_THROW
#define QUARISMA_THROW(format_str, ...) LOGGING_THROW(format_str, ##__VA_ARGS__)
#endif

#ifndef QUARISMA_NOT_IMPLEMENTED
#define QUARISMA_NOT_IMPLEMENTED(format_str, ...) LOGGING_NOT_IMPLEMENTED(format_str, ##__VA_ARGS__)
#endif
