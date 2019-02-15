//------------------------------------------------------------------------------
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// © H2O.ai 2018
//------------------------------------------------------------------------------
#ifndef dt_PYTHON_ARG_h
#define dt_PYTHON_ARG_h
#include <string>     // std::string
#include <vector>     // std::vector
#include <Python.h>
#include "python/obj.h"
#include "python/list.h"
#include "python/tuple.h"

namespace py {

class PKArgs;


/**
 * The argument may be in "undefined" state, meaning the user did not provide
 * a value for this argument in the function/method call. This state can be
 * checked with the `is_undefined()` method.
 */
class Arg : public _obj::error_manager {
  private:
    size_t pos;
    PKArgs* parent;
    py::robj pyobj;
    mutable std::string cached_name;

  public:
    Arg();
    Arg(const Arg&) = default;
    virtual ~Arg() override;
    void init(size_t i, PKArgs* args);
    void set(PyObject* value);

    //---- Type checks -----------------
    bool is_bool() const;
    bool is_bytes() const;
    bool is_dict() const;
    bool is_ellipsis() const;
    bool is_float() const;
    bool is_frame() const;
    bool is_int() const;
    bool is_list() const;
    bool is_list_or_tuple() const;
    bool is_none() const;
    bool is_none_or_undefined() const;
    bool is_numpy_array() const;
    bool is_pandas_frame() const;
    bool is_pandas_series() const;
    bool is_range() const;
    bool is_string() const;
    bool is_tuple() const;
    bool is_undefined() const;

    //---- Type conversions ------------
    bool        to_bool_strict        () const;
    int32_t     to_int32_strict       () const;
    int64_t     to_int64_strict       () const;
    size_t      to_size_t             () const;
    double      to_double             () const;
    py::olist   to_pylist             () const;
    py::odict   to_pydict             () const;
    py::rdict   to_rdict              () const;
    py::otuple  to_otuple             () const;
    std::string to_string             () const;
    strvec      to_stringlist         () const;
    SType       to_stype              () const;
    SType       to_stype              (const error_manager&) const;
    py::oobj    to_oobj               () const { return oobj(pyobj); }
    py::robj    to_pyobj              () const { return pyobj; }
    DataTable*  to_frame              () const;


    //---- Error messages --------------
    virtual Error error_not_list           (PyObject*) const override;
    virtual Error error_not_stype          (PyObject*) const override;
    virtual Error error_not_boolean        (PyObject*) const override;
    virtual Error error_not_integer        (PyObject*) const override;
    virtual Error error_int_negative       (PyObject*) const override;
    virtual Error error_not_double         (PyObject*) const override;

    // ?
    explicit operator bool() const noexcept { return pyobj.operator bool(); }
    PyObject* robj() const { return pyobj.to_pyobject_newref(); }
    PyObject* to_borrowed_ref() const { return pyobj.to_borrowed_ref(); }
    PyTypeObject* typeobj() const { return pyobj.typeobj(); }
    void print() const;

    /**
     * Convert argument to int32/int64. An exception will be thrown if the
     * argument is None, or not of integer type, or if the integer value is
     * too large.
     * This method must not be called if the argument is undefined.
     */
    operator int32_t() const;
    operator int64_t() const;
    operator size_t() const;

    /**
     * Convert argument to different list objects.
     */
    // operator list() const;
    // std::vector<std::string> to_list_of_strs() const;

    const std::string& name() const;

  private:
    void _check_list_or_tuple() const;
    void _check_missing() const;
};


}  // namespace py
#endif
