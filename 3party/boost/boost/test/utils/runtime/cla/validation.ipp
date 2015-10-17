//  (C) Copyright Gennadiy Rozental 2005-2014.
//  Use, modification, and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//! @file
//! @brief Input validation helpers implementation
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_RUNTIME_CLA_VALIDATION_IPP
#define BOOST_TEST_UTILS_RUNTIME_CLA_VALIDATION_IPP

// Boost.Runtime.Parameter
#include <boost/test/utils/runtime/config.hpp>

#include <boost/test/utils/runtime/cla/argv_traverser.hpp>
#include <boost/test/utils/runtime/cla/validation.hpp>
#include <boost/test/utils/runtime/validation.hpp> // BOOST_TEST_UTILS_RUNTIME_PARAM_NAMESPACE::logic_error

// Boost.Test
#include <boost/test/utils/basic_cstring/io.hpp>
#include <boost/test/detail/throw_exception.hpp>

namespace boost {

namespace BOOST_TEST_UTILS_RUNTIME_PARAM_NAMESPACE {

namespace cla {

// ************************************************************************** //
// **************           runtime::cla::validation           ************** //
// ************************************************************************** //

BOOST_TEST_UTILS_RUNTIME_PARAM_INLINE void
report_input_error( argv_traverser const& tr, format_stream& msg )
{
    if( tr.eoi() )
        msg << BOOST_TEST_UTILS_RUNTIME_PARAM_LITERAL( " at the end of input" );
    else {
        msg << BOOST_TEST_UTILS_RUNTIME_PARAM_LITERAL( " in the following position: " );

        if( tr.input().size() > 5 )
            msg << tr.input().substr( 0, 5 ) << BOOST_TEST_UTILS_RUNTIME_PARAM_LITERAL( "..." );
        else
            msg << tr.input();
    }

    BOOST_TEST_IMPL_THROW( BOOST_TEST_UTILS_RUNTIME_PARAM_NAMESPACE::logic_error( msg.str() ) );
}

//____________________________________________________________________________//

} // namespace cla

} // namespace BOOST_TEST_UTILS_RUNTIME_PARAM_NAMESPACE

} // namespace boost

#endif // BOOST_TEST_UTILS_RUNTIME_CLA_VALIDATION_IPP
