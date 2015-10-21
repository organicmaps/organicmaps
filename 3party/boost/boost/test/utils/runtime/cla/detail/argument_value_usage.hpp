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
//  Description : argument usage printing helpers
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_RUNTIME_CLA_ARGUMENT_VALUE_USAGE_HPP
#define BOOST_TEST_UTILS_RUNTIME_CLA_ARGUMENT_VALUE_USAGE_HPP

// Boost.Runtime.Parameter
#include <boost/test/utils/runtime/config.hpp>
#include <boost/test/utils/runtime/cla/argv_traverser.hpp>

// Boost.Test
#include <boost/test/utils/basic_cstring/io.hpp>
#include <boost/test/utils/basic_cstring/compare.hpp>

#include <boost/lexical_cast.hpp>

// STL
// !! can we eliminate these includes?
#include <list>

namespace boost {

namespace BOOST_TEST_UTILS_RUNTIME_PARAM_NAMESPACE {

namespace cla {

namespace rt_cla_detail {

// ************************************************************************** //
// **************             argument_value_usage             ************** //
// ************************************************************************** //

// generic case
template<typename T>
inline void
argument_value_usage( format_stream& fs, long, T* = 0 )
{
    fs << BOOST_TEST_UTILS_RUNTIME_PARAM_CSTRING_LITERAL( "<value>" );
}

//____________________________________________________________________________//

// specialization for list of values
template<typename T>
inline void
argument_value_usage( format_stream& fs, int, std::list<T>* = 0 )
{
    fs << BOOST_TEST_UTILS_RUNTIME_PARAM_CSTRING_LITERAL( "(<value1>, ..., <valueN>)" );
}

//____________________________________________________________________________//

// specialization for type bool
inline void
argument_value_usage( format_stream& fs,  int, bool* = 0 )
{
    fs << BOOST_TEST_UTILS_RUNTIME_PARAM_CSTRING_LITERAL( "yes|y|no|n" );
}

//____________________________________________________________________________//

} // namespace rt_cla_detail

} // namespace cla

} // namespace BOOST_TEST_UTILS_RUNTIME_PARAM_NAMESPACE

} // namespace boost

#endif // BOOST_TEST_UTILS_RUNTIME_CLA_ARGUMENT_VALUE_USAGE_HPP
