//  (C) Copyright Gennadiy Rozental 2005-2014.
//  Use, modification, and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision$
//
//  Description : input validation helpers definition
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_RUNTIME_CLA_VALIDATION_HPP
#define BOOST_TEST_UTILS_RUNTIME_CLA_VALIDATION_HPP

// Boost.Runtime.Parameter
#include <boost/test/utils/runtime/config.hpp>

#include <boost/test/utils/runtime/cla/fwd.hpp>

namespace boost {

namespace BOOST_TEST_UTILS_RUNTIME_PARAM_NAMESPACE {

namespace cla {

// ************************************************************************** //
// **************       runtime::cla::report_input_error       ************** //
// ************************************************************************** //

void report_input_error( argv_traverser const& tr, format_stream& msg );

//____________________________________________________________________________//

#define BOOST_TEST_UTILS_RUNTIME_CLA_VALIDATE_INPUT( b, tr, msg ) \
    if( b ) ; else ::boost::BOOST_TEST_UTILS_RUNTIME_PARAM_NAMESPACE::cla::report_input_error( tr, format_stream().ref() << msg )

//____________________________________________________________________________//

} // namespace cla

} // namespace BOOST_TEST_UTILS_RUNTIME_PARAM_NAMESPACE

} // namespace boost

#ifndef BOOST_TEST_UTILS_RUNTIME_PARAM_OFFLINE

#ifndef BOOST_TEST_UTILS_RUNTIME_PARAM_INLINE
#   define BOOST_TEST_UTILS_RUNTIME_PARAM_INLINE inline
#endif
#   include <boost/test/utils/runtime/cla/validation.ipp>

#endif

#endif // BOOST_TEST_UTILS_RUNTIME_CLA_VALIDATION_HPP
