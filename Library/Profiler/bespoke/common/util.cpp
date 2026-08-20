//#include <profiler/csrc/autograd/function.h>

#include "bespoke/common/util.h"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <sstream>

#include "bespoke/base/thread_local_debug_info.h"
#include "bespoke/common/collection.h"
#include "common/array_ref.h"
#include "common/irange.h"

#if PROFILER_HAS_KINETO
#include <libkineto.h>
#endif

namespace profiler::profiler_impl::impl
{

namespace
{
std::optional<bool> soft_assert_raises_;
}  // namespace

void setSoftAssertRaises(std::optional<bool> value)
{
    soft_assert_raises_ = value;
}

bool softAssertRaises()
{
    return soft_assert_raises_.value_or(false);
}

void logSoftAssert(
    // @lint-ignore CLANGTIDY
    const char* func,
    // @lint-ignore CLANGTIDY
    const char* file,
    // @lint-ignore CLANGTIDY
    uint32_t line,
    // @lint-ignore CLANGTIDY
    const char* cond,
    // @lint-ignore CLANGTIDY
    const char* args)
{
#if PROFILER_HAS_KINETO
    std::string error;
    error = fmt::format(
        "{} SOFT ASSERT FAILED at {}:{}, func: {}, args: {}", cond, file, line, func, args);
    // TODO: Implement profile_id and group_profile_id as 3rd/4th arguments.
    kineto::logInvariantViolation(cond, error, "", "");
#endif
}

void logSoftAssert(
    // @lint-ignore CLANGTIDY
    const char* func,
    // @lint-ignore CLANGTIDY
    const char* file,
    // @lint-ignore CLANGTIDY
    uint32_t line,
    // @lint-ignore CLANGTIDY
    const char* cond,
    // @lint-ignore CLANGTIDY
    const std::string& args)
{
#if PROFILER_HAS_KINETO
    std::string error;
    error = fmt::format(
        "{} SOFT ASSERT FAILED at {}:{}, func: {}, args: {}", cond, file, line, func, args);
    // TODO: Implement profile_id and group_profile_id as 3rd/4th arguments.
    kineto::logInvariantViolation(cond, error, "", "");
#endif
}

// ----------------------------------------------------------------------------
// -- NVTX --------------------------------------------------------------------
// ----------------------------------------------------------------------------
std::string getNvtxStr(
    const char*                                                      name,
    int64_t                                                          sequence_nr,
    const std::vector<std::vector<int64_t>>&                         shapes,
    profiler::RecordFunctionHandle                                   op_id,
    const std::list<std::pair<profiler::RecordFunctionHandle, int>>& input_op_ids)
{
    if (sequence_nr >= -1 || !shapes.empty())
    {
        std::string str;
        if (sequence_nr >= 0)
        {
            str = fmt::format("{}, seq = {}", name, sequence_nr);
        }
        else if (sequence_nr == -1)
        {
            str = name;
        }
        else
        {
#ifdef PROFILER_USE_ROCM
            // Only ROCM supports < -1 sequence_nr
            str = name;
#endif
        }
        if (op_id > 0)
        {
            str = fmt::format("{}, op_id = {}", str, op_id);
        }
        if (!shapes.empty())
        {
            str = fmt::format("{}, sizes = {}", str, shapesToStr(shapes));
        }
        // Include the op ids of the input edges so
        // you can build the network graph
        if (!input_op_ids.empty())
        {
            str = fmt::format("{}, input_op_ids = {}", str, inputOpIdsToStr(input_op_ids));
        }
        return str;
    }

    return name;
}

std::string stacksToStr(const std::vector<std::string>& stacks, const char* delim)
{
    std::ostringstream oss;
    std::transform(
        stacks.begin(),
        stacks.end(),
        std::ostream_iterator<std::string>(oss, delim),
        [](const std::string& s) -> std::string
        {
#ifdef _WIN32
            // replace the windows backslash with forward slash
            std::string result = s;
            std::replace(result.begin(), result.end(), '\\', '/');
            return result;
#else
            return s;
#endif
        });
    auto rc = oss.str();
    return "\"" + rc + "\"";
}

// XSigma has no tensor type (profiler::IValue carries no tensor/list payload);
// this always returns empty.
std::vector<std::vector<int64_t>> inputSizes(
    const profiler::RecordFunction& /*fn*/, bool /*flatten_list_enabled*/)
{
    return {};
}

std::string shapesToStr(const std::vector<std::vector<int64_t>>& shapes)
{
    std::string str("[");
    for (const auto t_idx : profiler::irange(shapes.size()))
    {
        if (t_idx > 0)
        {
            str = fmt::format("{}, ", str);
        }
        str = fmt::format("{}{}", str, shapeToStr(shapes[t_idx]));
    }
    str = fmt::format("{}]", str);
    return str;
}

std::string variantShapesToStr(const std::vector<shape>& shapes)
{
    std::string str("[");
    for (const auto t_idx : profiler::irange(shapes.size()))
    {
        if (t_idx > 0)
        {
            str = fmt::format("{}, ", str);
        }
        if (std::holds_alternative<std::vector<int64_t>>(shapes[t_idx]))
        {
            const auto& shape = std::get<std::vector<int64_t>>(shapes[t_idx]);
            str               = fmt::format("{}{}", str, shapeToStr(shape));
        }
        else if (std::holds_alternative<std::vector<std::vector<int64_t>>>(shapes[t_idx]))
        {
            const auto& tensor_shape = std::get<std::vector<std::vector<int64_t>>>(shapes[t_idx]);
            if (tensor_shape.size() > TENSOR_LIST_DISPLAY_LENGTH_LIMIT)
            {
                // skip if the tensor list is too long
                str = fmt::format("{}[]", str);
                continue;
            }
            str = fmt::format("{}[", str);
            for (const auto s_idx : profiler::irange(tensor_shape.size()))
            {
                if (s_idx > 0)
                {
                    str = fmt::format("{}, ", str);
                }
                str = fmt::format("{}{}", str, shapeToStr(tensor_shape[s_idx]));
            }
            str = fmt::format("{}]", str);
        }
    }
    str = fmt::format("{}]", str);
    return str;
}

std::string shapeToStr(const std::vector<int64_t>& shape)
{
    std::string str("[");
    for (const auto s_idx : profiler::irange(shape.size()))
    {
        if (s_idx > 0)
        {
            str = fmt::format("{}, ", str);
        }
        str = fmt::format("{}{}", str, shape[s_idx]);
    }
    str = fmt::format("{}]", str);
    return str;
}

std::string inputOpIdsToStr(
    const std::list<std::pair<profiler::RecordFunctionHandle, int>>& input_op_ids)
{
    std::string str("[");
    int         idx = 0;

    for (const auto& op_id_info_pair : input_op_ids)
    {
        if (idx++ > 0)
        {
            str = fmt::format("{}, ", str);
        }
        // (OpId,OutputNr)
        str = fmt::format("{}({},{})", str, op_id_info_pair.first, op_id_info_pair.second);
    }
    str = fmt::format("{}]", str);
    return str;
}

std::string strListToStr(const std::vector<std::string>& types)
{
    if (types.empty())
    {
        return "[]";
    }

    std::ostringstream oss;
    std::transform(
        types.begin(),
        types.end(),
        std::ostream_iterator<std::string>(oss, ", "),
        [](const std::string& s) -> std::string { return "\"" + s + "\""; });
    auto rc = oss.str();
    rc.erase(rc.length() - 2);  // remove last ", "
    return "[" + rc + "]";
}
#if 0
// Disabled: IValue methods (isNone, isBool, operator<<) not available in profiler-only build.
std::string ivalueToStr(const profiler::IValue& val, bool isString)
{
    std::stringstream ss;
    if (val.isNone())
    {
        return "\"None\"";
    }
    else
    {
        ss.str("");
        if (isString)
        {
            ss << "\"";
        }
        ss << val;
        if (isString)
        {
            ss << "\"";
        }
        std::string mystr = ss.str();

        // For boolean the values that ivalue gives is "True" and "False" but
        // json only takes "true" and "false" so we convert the string to lower case
        if (val.isBool())
        {
            for (char& c : mystr)
            {
                c = static_cast<char>(std::tolower(c));
            }
        }

        // A double quote can cause issues with the chrome tracing so force
        // all inputs to not contain more than the 2 we add in this function
        auto count = std::count(mystr.begin(), mystr.end(), '"');
        return count > 2 ? "\"None\"" : mystr;
    }
}
#else
// Stub implementation when IValue methods are not available.
std::string ivalueToStr(const profiler::IValue& /*val*/, bool /*isString*/)
{
    return "\"None\"";
}
#endif

#if 0
// Disabled: IValue methods (isNone, operator<<) not available in profiler-only build.
std::string ivalueListToStr(const std::vector<profiler::IValue>& list)
{
    std::vector<std::string> concrete_str_inputs;
    std::stringstream        ss;
    for (const auto& val : list)
    {
        if (val.isNone())
        {
            concrete_str_inputs.emplace_back("");
        }
        else
        {
            ss.str("");
            ss << val;
            concrete_str_inputs.emplace_back(ss.str());
        }
    }
    return strListToStr(concrete_str_inputs);
}
#else
// Stub implementation when IValue methods are not available.
std::string ivalueListToStr(const std::vector<profiler::IValue>& /*list*/)
{
    return "[]";
}
#endif

// XSigma has no tensor type (profiler::IValue carries no tensor/scalar/list
// payload); this always returns one empty string per input.
std::vector<std::string> inputTypes(const profiler::RecordFunction& fn)
{
    std::vector<std::string> types;
    types.reserve(fn.inputs().size());
    for (const auto& input_val : fn.inputs())
    {
        (void)input_val;       // Suppress unused variable warning
        types.emplace_back();  // Return empty string for each input
    }
    return types;
}

// ----------------------------------------------------------------------------
// -- NCCL Metadata -----------------------------------------------------------
// ----------------------------------------------------------------------------

// XSigma has no tensor type (profiler::IValue carries no tensor/tuple/list
// payload); this always reports "not found".
std::pair<bool, std::variant<int, std::vector<int>>> findStartAddrForTensors(
    const profiler::IValue& /*val*/)
{
    return {false, -1};
}

// XSigma has no distributed/multi-node collective-communication concept
// (Library/Parallel is a local thread-pool library), so there is no NCCL
// metadata to report.
std::unordered_map<std::string, std::string> saveNcclMeta(
    const profiler::RecordFunction& /*fn*/, const SaveNcclMetaConfig& /*config*/)
{
    return {};
}

// XSigma has no tensor type (profiler::IValue carries no tensor payload); these
// FLOPS-estimation helpers always return empty/zero.
[[maybe_unused]] static std::vector<profiler::IntArrayRef> getInputSizes(
    const std::string& /*op_name*/,
    size_t /*min_size*/,
    profiler::array_ref<const profiler::IValue> /*inputs*/,
    const profiler::array_ref<int>& /*should_be_tensor*/)
{
    return {};
}

std::unordered_map<std::string, profiler::IValue> saveExtraArgs(
    const profiler::RecordFunction& /*fn*/)
{
    return {};
}

uint64_t computeFlops(
    const std::string& /*op_name*/,
    const std::unordered_map<std::string, profiler::IValue>& /*extra_args*/)
{
    return 0;
}

}  // namespace profiler::profiler_impl::impl
