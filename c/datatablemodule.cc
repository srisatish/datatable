//------------------------------------------------------------------------------
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// © H2O.ai 2018
//------------------------------------------------------------------------------
#include "datatablemodule.h"
#include <Python.h>
#include "../datatable/include/datatable.h"
#include "csv/py_csv.h"
#include "csv/writer.h"
#include "expr/base_expr.h"
#include "expr/by_node.h"
#include "expr/join_node.h"
#include "expr/py_expr.h"
#include "expr/sort_node.h"
#include "extras/aggregator.h"
#include "extras/py_ftrl.h"
#include "frame/py_frame.h"
#include "options.h"
#include "py_column.h"
#include "py_datatable.h"
#include "py_encodings.h"
#include "py_rowindex.h"
#include "py_types.h"
#include "utils/assert.h"
#include "ztest.h"


namespace py {
  PyObject* fread_fn = nullptr;
}



//------------------------------------------------------------------------------
// These functions are exported as `datatable.internal.*`
//------------------------------------------------------------------------------

static std::pair<DataTable*, size_t> _unpack_args(const py::PKArgs& args) {
  if (!args[0] || !args[1]) throw ValueError() << "Expected 2 arguments";
  DataTable* dt = args[0].to_frame();
  size_t col    = args[1].to_size_t();

  if (!dt) throw TypeError() << "First parameter should be a Frame";
  if (col >= dt->ncols) throw ValueError() << "Index out of bounds";
  return std::make_pair(dt, col);
}


static py::PKArgs args_frame_column_rowindex(
    2, 0, 0, false, false, {"frame", "i"},
    "frame_column_rowindex",
R"(frame_column_rowindex(frame, i)
--

Return the RowIndex of the `i`th column of the `frame`, or None if that column
has no row index.
)");

static py::oobj frame_column_rowindex(const py::PKArgs& args) {
  auto u = _unpack_args(args);
  DataTable* dt = u.first;
  size_t col = u.second;

  RowIndex ri = dt->columns[col]->rowindex();
  return ri? py::orowindex(ri) : py::None();
}


static py::PKArgs args_frame_column_data_r(
    2, 0, 0, false, false, {"frame", "i"},
    "frame_column_data_r",
R"(frame_column_data_r(frame, i)
--

Return C pointer to the main data array of the column `frame[i]`. The pointer
is returned as a `ctypes.c_void_p` object.
)");

static py::oobj frame_column_data_r(const py::PKArgs& args) {
  static py::oobj c_void_p = py::oobj::import("ctypes", "c_void_p");

  auto u = _unpack_args(args);
  DataTable* dt = u.first;
  size_t col = u.second;
  const void* ptr = dt->columns[col]->data();
  py::otuple init_args(1);
  init_args.set(0, py::oint(reinterpret_cast<size_t>(ptr)));
  return c_void_p.call(init_args);
}


static py::PKArgs args_in_debug_mode(
    0, 0, 0, false, false, {}, "in_debug_mode",
    "Return True if datatable was compiled in debug mode");

static py::oobj in_debug_mode(const py::PKArgs&) {
  #ifdef DTDEBUG
    return py::True();
  #else
    return py::False();
  #endif
}



static py::PKArgs args_has_omp_support(
    0, 0, 0, false, false, {}, "has_omp_support",
R"(Return True if datatable was built with OMP support, and False otherwise.
Without OMP datatable will be significantly slower, performing all
operations in single-threaded mode.
)");

static py::oobj has_omp_support(const py::PKArgs&) {
  #ifdef DTNOOPENMP
    return py::False();
  #else
    return py::True();
  #endif
}


static py::PKArgs args__register_function(
    2, 0, 0, false, false, {"n", "fn"}, "_register_function", nullptr);

static void _register_function(const py::PKArgs& args) {
  size_t n = args.get<size_t>(0);
  py::oobj fn = args[1].to_oobj();

  PyObject* fnref = std::move(fn).release();
  switch (n) {
    case 2: init_py_stype_objs(fnref); break;
    case 3: init_py_ltype_objs(fnref); break;
    case 4: replace_typeError(fnref); break;
    case 5: replace_valueError(fnref); break;
    case 6: replace_dtWarning(fnref); break;
    case 7: py::Frame_Type = fnref; break;
    case 8: py::fread_fn = fnref; break;
    default: throw ValueError() << "Unknown index: " << n;
  }
}




//------------------------------------------------------------------------------
// Module definition
//------------------------------------------------------------------------------

void DatatableModule::init_methods() {
  add(METHODv(pydatatable::datatable_load));
  add(METHODv(pydatatable::open_jay));
  add(METHODv(pydatatable::install_buffer_hooks));
  add(METHODv(gread));
  add(METHODv(write_csv));

  ADD_FN(&_register_function, args__register_function);
  ADD_FN(&has_omp_support, args_has_omp_support);
  ADD_FN(&in_debug_mode, args_in_debug_mode);
  ADD_FN(&frame_column_rowindex, args_frame_column_rowindex);
  ADD_FN(&frame_column_data_r, args_frame_column_data_r);

  init_methods_aggregate();
  init_methods_join();
  init_methods_kfold();
  init_methods_options();
  init_methods_repeat();
  init_methods_sets();
  init_methods_str();
  #ifdef DTTEST
    init_tests();
  #endif
}


/* Called when Python program imports the module */
PyMODINIT_FUNC PyInit__datatable() noexcept
{
  static DatatableModule dtmod;
  PyObject* m = nullptr;

  try {
    init_csvwrite_constants();
    init_exceptions();

    force_stype = SType::VOID;

    m = dtmod.init();

    // Initialize submodules
    if (!init_py_types(m)) return nullptr;
    if (!pycolumn::static_init(m)) return nullptr;
    if (!pydatatable::static_init(m)) return nullptr;
    if (!init_py_encodings(m)) return nullptr;
    init_jay();

    py::Frame::Type::init(m);
    py::Ftrl::Type::init(m);
    py::base_expr::Type::init(m);
    py::orowindex::pyobject::Type::init(m);
    py::oby::init(m);
    py::ojoin::init(m);
    py::osort::init(m);

  } catch (const std::exception& e) {
    exception_to_python(e);
    m = nullptr;
  }

  return m;
}
