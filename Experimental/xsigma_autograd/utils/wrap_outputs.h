#pragma once

// Wrap tensor operation outputs as PyObject*

#include <Quarisma/ScalarOps.h>
#include <Quarisma/core/Tensor.h>
#include <quarisma/util/irange.h>
#include <torch/csrc/Dtype.h>
#include <torch/csrc/DynamicTypes.h>
#include <torch/csrc/Layout.h>
#include <torch/csrc/QScheme.h>
#include <torch/csrc/autograd/python_variable.h>
#include <torch/csrc/autograd/variable.h>
#include <torch/csrc/python_headers.h>
#include <torch/csrc/utils/python_numbers.h>
#include <torch/csrc/utils/tensor_qschemes.h>

#include <initializer_list>
#include <tuple>

namespace torch::autograd::utils
{

inline PyObject* wrap(bool value)
{
    if (value)
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

inline PyObject* wrap(quarisma::DeviceIndex value)
{
    return THPUtils_packDeviceIndex(value);
}

inline PyObject* wrap(int64_t value)
{
    return THPUtils_packInt64(value);
}

inline PyObject* wrap(double value)
{
    return PyFloat_FromDouble(value);
}

inline PyObject* wrap(quarisma::complex<double> value)
{
    // I could probably also use FromComplex with a reinterpret cast,
    // but... eh.
    return PyComplex_FromDoubles(value.real(), value.imag());
}

inline PyObject* wrap(void* value)
{
    return PyLong_FromVoidPtr(value);
}

inline PyObject* wrap(THPDtype* dtype)
{
    return Py_NewRef(dtype);
}

inline PyObject* wrap(quarisma::ScalarType scalarType)
{
    return Py_NewRef(getTHPDtype(scalarType));
}

inline PyObject* wrap(THPLayout* layout)
{
    return Py_NewRef(layout);
}

inline PyObject* wrap(quarisma::Layout layout)
{
    return Py_NewRef(getTHPLayout(layout));
}

inline PyObject* wrap(const quarisma::Tensor& tensor)
{
    return THPVariable_Wrap(tensor);
}

inline PyObject* wrap(const quarisma::Scalar& scalar)
{
    return wrap(scalar_to_tensor(scalar));
}

inline PyObject* wrap(quarisma::QScheme qscheme)
{
    auto* thp_qscheme = torch::utils::getTHPQScheme(qscheme);
    Py_INCREF(thp_qscheme);
    return thp_qscheme;
}

inline PyObject* wrap(quarisma::TensorList tl)
{
    auto r = THPObjectPtr{PyTuple_New(static_cast<Py_ssize_t>(tl.size()))};
    if (!r)
        throw python_error();
    for (const auto i : quarisma::irange(tl.size()))
    {
        PyTuple_SET_ITEM(r.get(), i, wrap(tl[i]));
    }
    return r.release();
}

inline PyObject* wrap(quarisma::IntArrayRef list)
{
    auto r = THPObjectPtr{PyTuple_New(static_cast<Py_ssize_t>(list.size()))};
    if (!r)
        throw python_error();
    for (const auto i : quarisma::irange(list.size()))
    {
        PyTuple_SET_ITEM(r.get(), i, wrap(list[i]));
    }
    return r.release();
}

inline PyObject* wrap(quarisma::Stream stream)
{
    return THPStream_Wrap(stream);
}

namespace detail
{
template <typename F, typename Tuple, size_t... Is>
void apply_with_idx_impl(const F& f, Tuple& t, std::index_sequence<Is...> /*indices*/)
{
    (void)std::initializer_list<int>{(f(std::get<Is>(t), Is), 0)...};
}

// For tuple(a, b, c), calls f(a, 0), f(b, 1), f(c, 2)
template <typename F, typename... Ts>
void apply_with_idx(const F& f, std::tuple<Ts...>& t)
{
    apply_with_idx_impl(f, t, std::index_sequence_for<Ts...>{});
}
}  // namespace detail

template <typename... Ts>
PyObject* wrap(std::tuple<Ts...> values)
{
    auto r = THPObjectPtr{PyTuple_New(sizeof...(Ts))};
    if (!r)
        throw python_error();
    detail::apply_with_idx(
        [&](auto& value, size_t idx) { PyTuple_SET_ITEM(r.get(), idx, wrap(std::move(value))); },
        values);
    return r.release();
}

template <typename... Ts>
PyObject* wrap(PyTypeObject* type, std::tuple<Ts...> values)
{
    auto r = THPObjectPtr{PyStructSequence_New(type)};
    if (!r)
        throw python_error();
    detail::apply_with_idx(
        [&](auto& value, size_t idx)
        { PyStructSequence_SET_ITEM(r.get(), idx, wrap(std::move(value))); },
        values);
    return r.release();
}

}  // namespace torch::autograd::utils
