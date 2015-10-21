//  (C) Copyright Gennadiy Rozental 2015.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//!@file
//!@brief contains wrappers, which allows to build Boost.Test with no exception
// ***************************************************************************

#ifndef BOOST_TEST_DETAIL_THROW_EXCEPTION_HPP
#define BOOST_TEST_DETAIL_THROW_EXCEPTION_HPP

// Boost
#include <boost/config.hpp> // BOOST_NO_EXCEPTION

#ifdef BOOST_NO_EXCEPTION
// C RUNTIME
#include <stdlib.h>

#endif

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace ut_detail {

#ifdef BOOST_NO_EXCEPTION

template<typename E>
inline int
throw_exception(E const& e) { abort(); return 0; }

#define BOOST_TEST_IMPL_TRY
#define BOOST_TEST_IMPL_CATCH( T, var ) for(T const& var = *(T*)0; false;)
#define BOOST_TEST_IMPL_CATCH0( T ) if(0)
#define BOOST_TEST_IMPL_CATCHALL() if(0)
#define BOOST_TEST_IMPL_RETHROW

#else

template<typename E>
inline int
throw_exception(E const& e) { throw e; return 0; }

#define BOOST_TEST_IMPL_TRY try
#define BOOST_TEST_IMPL_CATCH( T, var ) catch( T const& var )
#define BOOST_TEST_IMPL_CATCH0( T ) catch( T const& )
#define BOOST_TEST_IMPL_CATCHALL() catch(...)
#define BOOST_TEST_IMPL_RETHROW throw
#endif

//____________________________________________________________________________//

#define BOOST_TEST_IMPL_THROW( E ) unit_test::ut_detail::throw_exception( E )

} // namespace ut_detail
} // namespace unit_test
} // namespace boost

//____________________________________________________________________________//

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_DETAIL_THROW_EXCEPTION_HPP
