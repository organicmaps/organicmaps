#pragma once

#include <utility>

#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

namespace
{
using namespace boost::python;

// Converts a std::pair instance to a Python tuple.
template <typename T1, typename T2>
struct pair_to_tuple
{
  static PyObject * convert(std::pair<T1, T2> const & p)
  {
    return incref(boost::python::make_tuple(p.first, p.second).ptr());
  }

  static PyTypeObject const * get_pytype() { return &PyTuple_Type; }
};

template <typename T1, typename T2>
struct pair_to_python_converter
{
  pair_to_python_converter() { to_python_converter<std::pair<T1, T2>, pair_to_tuple<T1, T2>, true>(); }
};
}  // namespace
