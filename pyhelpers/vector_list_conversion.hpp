#pragma once

#include <vector>

#include <boost/python.hpp>
#include <boost/python/stl_iterator.hpp>

namespace pyhelpers
{
template <typename T>
std::vector<T> PythonListToStdVector(boost::python::object const & iterable)
{
  return std::vector<T>(boost::python::stl_input_iterator<T>(iterable), boost::python::stl_input_iterator<T>());
}

// For this to work one should define
// class_<std::vector<YourClass>>("YourClassList")
//   .def(vector_indexing_suite<std::vector<YourClass>>());
template <typename T>
boost::python::list StdVectorToPythonList(std::vector<T> const & v)
{
  boost::python::object get_iter = boost::python::iterator<std::vector<T>>();
  return boost::python::list(get_iter(v));
}
}  // namespace pyhelpers
