// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef MODULE_INIT_DWA20020722_HPP
# define MODULE_INIT_DWA20020722_HPP

# include <boost/python/detail/prefix.hpp>
# include <boost/preprocessor/cat.hpp>
# include <boost/preprocessor/stringize.hpp>

# ifndef BOOST_PYTHON_MODULE_INIT

namespace boost { namespace python { namespace detail {

BOOST_PYTHON_DECL PyObject* init_module(char const* name, void(*)());

}}}

#  if PY_VERSION_HEX >= 0x03000000

#   define _BOOST_PYTHON_MODULE_INIT(name)              \
  PyObject* BOOST_PP_CAT (PyInit_,name)()		\
{                                                       \
    return boost::python::detail::init_module(          \
					      BOOST_PP_STRINGIZE(name),&BOOST_PP_CAT(init_module_,name)); \
}                                                       \
  void BOOST_PP_CAT(init_module_,name)()

#  else

#   define _BOOST_PYTHON_MODULE_INIT(name)              \
  void BOOST_PP_CAT(init,name)()			\
{                                                       \
    boost::python::detail::init_module(                 \
				       BOOST_PP_STRINGIZE(name),&BOOST_PP_CAT(init_module_,name)); \
}                                                       \
  void BOOST_PP_CAT(init_module_,name)()

#  endif

#  if (defined(_WIN32) || defined(__CYGWIN__)) && !defined(BOOST_PYTHON_STATIC_MODULE)

#   define BOOST_PYTHON_MODULE_INIT(name)                           \
  void BOOST_PP_CAT(init_module_,name)();			    \
extern "C" __declspec(dllexport) _BOOST_PYTHON_MODULE_INIT(name)

#  elif BOOST_PYTHON_USE_GCC_SYMBOL_VISIBILITY

#   define BOOST_PYTHON_MODULE_INIT(name)                               \
  void BOOST_PP_CAT(init_module_,name)();				\
extern "C" __attribute__ ((visibility("default"))) _BOOST_PYTHON_MODULE_INIT(name)

#  else

#   define BOOST_PYTHON_MODULE_INIT(name)                               \
  void BOOST_PP_CAT(init_module_,name)();				\
extern "C" _BOOST_PYTHON_MODULE_INIT(name)

#  endif

# endif 

#endif // MODULE_INIT_DWA20020722_HPP
