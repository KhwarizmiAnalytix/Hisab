
#pragma once
#if 0
#include "api.h"

namespace quarisma::profiler_impl::impl
{

using CallBackFnPtr =
    void (*)(const ProfilerConfig& config, const std::unordered_set<quarisma::RecordScope>& scopes);

class QUARISMA_VISIBILITY PushPRIVATEUSE1CallbacksStub
{
    PushPRIVATEUSE1CallbacksStub()                                               = default;
    PushPRIVATEUSE1CallbacksStub(const PushPRIVATEUSE1CallbacksStub&)            = delete;
    PushPRIVATEUSE1CallbacksStub& operator=(const PushPRIVATEUSE1CallbacksStub&) = delete;
    PushPRIVATEUSE1CallbacksStub(PushPRIVATEUSE1CallbacksStub&&)                 = default;
    PushPRIVATEUSE1CallbacksStub& operator=(PushPRIVATEUSE1CallbacksStub&&)      = default;
    ~PushPRIVATEUSE1CallbacksStub()                                              = default;

    template <typename... ArgTypes>
    void operator()(ArgTypes&&... args)
    {
        return (*push_privateuse1_callbacks_fn)(std::forward<ArgTypes>(args)...);
    }

    void set_privateuse1_dispatch_ptr(CallBackFnPtr fn_ptr)
    {
        push_privateuse1_callbacks_fn = fn_ptr;
    }

private:
    CallBackFnPtr push_privateuse1_callbacks_fn = nullptr;
};

extern QUARISMA_API struct PushPRIVATEUSE1CallbacksStub pushPRIVATEUSE1CallbacksStub;

class QUARISMA_VISIBILITY RegisterPRIVATEUSE1Observer
{
    RegisterPRIVATEUSE1Observer(PushPRIVATEUSE1CallbacksStub& stub, CallBackFnPtr value)
    {
        stub.set_privateuse1_dispatch_ptr(value);
    }
};

#define REGISTER_PRIVATEUSE1_OBSERVER(name, fn) \
    static RegisterPRIVATEUSE1Observer name##__register(name, fn);
}  // namespace quarisma::profiler_impl::impl
#endif