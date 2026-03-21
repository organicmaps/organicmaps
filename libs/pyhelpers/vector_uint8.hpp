#pragma once

#include <vector>

// These headers are necessary for cross-python compilation.
// Python3 does not have PyString_* methods. One should use PyBytes_* instead.
// bytesobject.h contains a mapping from PyBytes_* to PyString_*.
// See https://docs.python.org/2/howto/cporting.html for more.
#include "Python.h"
#include "bytesobject.h"

#include <boost/python.hpp>
#include <boost/python/suite/indexing/map_indexing_suite.hpp>

namespace
{
// Converts a vector<uint8_t> to Python2 str or Python3 bytes.
struct vector_uint8t_to_str
{
  static PyObject * convert(std::vector<uint8_t> const & v)
  {
    auto bytes = PyBytes_FromStringAndSize(reinterpret_cast<char const *>(v.data()), v.size());
    Py_INCREF(bytes);

    return bytes;
  }
};

// Converts a vector<uint8_t> from Python2 str or Python3 bytes.
struct vector_uint8t_from_python_str
{
  vector_uint8t_from_python_str()
  {
    boost::python::converter::registry::push_back(&convertible, &construct,
                                                  boost::python::type_id<std::vector<uint8_t>>());
  }

  static void * convertible(PyObject * obj_ptr)
  {
    if (!PyBytes_Check(obj_ptr))
      return nullptr;
    return obj_ptr;
  }

  static void construct(PyObject * obj_ptr, boost::python::converter::rvalue_from_python_stage1_data * data)
  {
    char const * value = PyBytes_AsString(obj_ptr);
    if (value == nullptr)
      boost::python::throw_error_already_set();
    void * storage =
        ((boost::python::converter::rvalue_from_python_storage<std::vector<uint8_t>> *)data)->storage.bytes;
    new (storage) std::vector<uint8_t>(value, value + PyBytes_Size(obj_ptr));
    data->convertible = storage;
  }
};
}  // namespace
