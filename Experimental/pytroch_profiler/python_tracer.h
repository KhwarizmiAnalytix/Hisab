#pragma once

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "kineto_shim.h"
#include "util.h"
#include "util/approximate_clock.h"
#include "util/strong_type.h"

namespace quarisma::profiler_impl::impl
{

class RecordQueue;
struct Result;
namespace python_tracer
{

using TraceKey = strong::
    type<uint64_t, struct TraceKey_, strong::regular, strong::hashable, strong::ostreamable>;

class QUARISMA_VISIBILITY CompressedEvent
{
    TraceKey                  key_;
    uint64_t                  system_tid_{};
    kineto::DeviceAndResource kineto_info_{};
    quarisma::time_t          enter_t_{};
};

/*
Libtorch does not depend on Python (e.g. cannot #include <Python.h>); however
when we call the profiler from libtorch_python we need the profiler to be able
to ingest the data that we collect from the Python tracer. (`PyEval_SetProfile`)

In order to solve this dependency issue we define a virtual base and a function
to register a getter. The python tracer then implements these functions and
exposes itself by calling `registerTracer` from `quarisma/csrc/autograd/init.cpp`.
This pattern of registration for faux python dependencies in libtorch is common
in the PyTorch codebase.
*/
struct QUARISMA_API PythonTracerBase
{
    static std::unique_ptr<PythonTracerBase> make(RecordQueue* queue);
    virtual ~PythonTracerBase() = default;

    virtual void                                 stop()                 = 0;
    virtual void                                 restart()              = 0;
    virtual void                                 register_gc_callback() = 0;
    virtual std::vector<std::shared_ptr<Result>> getEvents(
        std::function<quarisma::time_t(quarisma::approx_time_t)> time_converter,
        std::vector<CompressedEvent>&                            enters,
        quarisma::time_t                                         end_time_ns) = 0;
};

using MakeFn = std::unique_ptr<PythonTracerBase> (*)(RecordQueue*);
QUARISMA_API void registerTracer(MakeFn make_tracer);

/**
 * Memory Tracer Implementation
 */
struct QUARISMA_API PythonMemoryTracerBase
{
    static std::unique_ptr<PythonMemoryTracerBase> make();
    virtual ~PythonMemoryTracerBase() = default;

    virtual void start()                                        = 0;
    virtual void stop()                                         = 0;
    virtual void export_memory_history(const std::string& path) = 0;
};

using MakeMemoryFn = std::unique_ptr<PythonMemoryTracerBase> (*)();
QUARISMA_API void registerMemoryTracer(MakeMemoryFn make_memory_tracer);

}  // namespace python_tracer
}  // namespace quarisma::profiler_impl::impl
