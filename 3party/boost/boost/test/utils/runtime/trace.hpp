//  (C) Copyright Gennadiy Rozental 2005-2014.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision$
//
//  Description : optional internal tracing
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_RUNTIME_TRACE_HPP
#define BOOST_TEST_UTILS_RUNTIME_TRACE_HPP

// Boost.Runtime.Parameter
#include <boost/test/utils/runtime/config.hpp>

#ifdef BOOST_TEST_UTILS_RUNTIME_PARAM_DEBUG

#include <iostream>

#  define BOOST_TEST_UTILS_RUNTIME_PARAM_TRACE( str ) std::cerr << str << std::endl
#else
#  define BOOST_TEST_UTILS_RUNTIME_PARAM_TRACE( str )
#endif

#endif // BOOST_TEST_UTILS_RUNTIME_TRACE_HPP
