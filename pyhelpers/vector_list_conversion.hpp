#pragma once

#include "std/vector.hpp"

#include <boost/python.hpp>
#include <boost/python/stl_iterator.hpp>

namespace
{
template <typename T>
vector<T> python_list_to_std_vector(boost::python::object const & iterable)
{
  return vector<T>(boost::python::stl_input_iterator<T>(iterable),
                   boost::python::stl_input_iterator<T>());
}

template <typename T>
boost::python::list std_vector_to_python_list(vector<T> const & v)
{
  boost::python::object get_iter = boost::python::iterator<vector<T>>();
  return boost::python::list(get_iter(v));
}
}  // namespace
