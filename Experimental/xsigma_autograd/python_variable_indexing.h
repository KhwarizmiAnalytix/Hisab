#pragma once

#include <quarisma/core/SymInt.h>
#include <torch/csrc/autograd/python_variable.h>
#include <torch/csrc/python_headers.h>
#include <torch/csrc/utils/pybind.h>
#include <torch/csrc/utils/python_symnode.h>

namespace torch::autograd
{

struct UnpackedSlice
{
    quarisma::SymInt start;
    quarisma::SymInt stop;
    quarisma::SymInt step;
};

// This mirrors Cpython's PySlice_Unpack method
inline UnpackedSlice __PySlice_Unpack(PyObject* _r)
{
    PySliceObject* r = (PySliceObject*)_r;
    /* this is harder to get right than you might think */

    quarisma::SymInt start_sym, stop_sym, step_sym;

    auto clip_val = [](Py_ssize_t val)
    {
        if (val < quarisma::SymInt::min_representable_int())
        {
            auto r = PyErr_WarnEx(
                PyExc_UserWarning,
                "Truncating the start/stop/step "
                "of slice. This is likely because of "
                "saved old models when the start/stop/step were larger.",
                1);
            if (r != 0)
            {
                throw python_error();
            }
            return (Py_ssize_t)(quarisma::SymInt::min_representable_int());
        }
        return val;
    };

    if (r->step == Py_None)
    {
        step_sym = quarisma::SymInt(1);
    }
    else
    {
        if (torch::is_symint(r->step))
        {
            step_sym = py::handle(r->step).cast<quarisma::SymInt>();
        }
        else
        {
            Py_ssize_t step = 0;
            if (!_PyEval_SliceIndex(r->step, &step))
            {
                throw python_error();
            }
            if (step == 0)
            {
                PyErr_SetString(PyExc_ValueError, "slice step cannot be zero");
            }

            step     = clip_val(step);
            step_sym = quarisma::SymInt(step);
        }
    }

    if (torch::is_symint(r->start))
    {
        start_sym = py::handle(r->start).cast<quarisma::SymInt>();
    }
    else if (r->start == Py_None)
    {
        start_sym = quarisma::SymInt(step_sym < 0 ? PY_SSIZE_T_MAX : 0);
    }
    else
    {
        Py_ssize_t start = 0;
        if (!_PyEval_SliceIndex(r->start, &start))
        {
            throw python_error();
        }
        start     = clip_val(start);
        start_sym = quarisma::SymInt(start);
    }

    if (torch::is_symint(r->stop))
    {
        stop_sym = py::handle(r->stop).cast<quarisma::SymInt>();
    }
    else if (r->stop == Py_None)
    {
        stop_sym = quarisma::SymInt(
            step_sym < 0 ? quarisma::SymInt::min_representable_int() : PY_SSIZE_T_MAX);
    }
    else
    {
        Py_ssize_t stop = 0;
        if (!_PyEval_SliceIndex(r->stop, &stop))
        {
            throw python_error();
        }
        stop     = clip_val(stop);
        stop_sym = quarisma::SymInt(stop);
    }

    return UnpackedSlice{std::move(start_sym), std::move(stop_sym), std::move(step_sym)};
}

Py_ssize_t THPVariable_length(PyObject* self);
PyObject*  THPVariable_getitem(PyObject* self, PyObject* index);
int        THPVariable_setitem(PyObject* self, PyObject* index, PyObject* value);

Variable valueToTensor(
    quarisma::TensorOptions options, PyObject* value, const quarisma::Device& device);

}  // namespace torch::autograd
