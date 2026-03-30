#include "base.h"
#include "itt_wrapper.h"
#include "util/irange.h"

//QUARISMA_DIAGNOSTIC_PUSH("-Wunused-parameter")

namespace quarisma::profiler::impl
{
namespace
{

struct ITTMethods : public ProfilerStubs
{
    void record(
        QUARISMA_UNUSED quarisma::device_enum* device,
        QUARISMA_UNUSED ProfilerVoidEventStub* event,
        QUARISMA_UNUSED int64_t*               cpu_ns) const override
    {
    }

    float elapsed(
        QUARISMA_UNUSED const ProfilerVoidEventStub* event,
        QUARISMA_UNUSED const ProfilerVoidEventStub* event2) const override
    {
        return 0;
    }

    void mark(QUARISMA_UNUSED const char* name) const override
    {
        quarisma::profiler::itt_mark(name);
    }

    void rangePush(QUARISMA_UNUSED const char* name) const override
    {
        quarisma::profiler::itt_range_push(name);
    }

    void rangePop() const override { quarisma::profiler::itt_range_pop(); }

    void onEachDevice(QUARISMA_UNUSED std::function<void(int)> op) const override {}

    void synchronize() const override {}

    bool enabled() const override { return true; }
};

struct RegisterITTMethods
{
    RegisterITTMethods()
    {
        static ITTMethods methods;
        registerITTMethods(&methods);
    }
};
RegisterITTMethods reg;

}  // namespace
}  // namespace quarisma::profiler::impl
//QUARISMA_DIAGNOSTIC_POP()
