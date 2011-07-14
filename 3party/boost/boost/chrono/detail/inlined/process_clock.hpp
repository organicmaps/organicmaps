//  boost process_timer.cpp  -----------------------------------------------------------//

//  Copyright Beman Dawes 1994, 2006, 2008
//  Copyright 2009 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  See http://www.boost.org/libs/chrono for documentation.

//--------------------------------------------------------------------------------------//
#ifndef BOOST_CHRONO_DETAIL_INLINED_PROCESS_CLOCK_HPP
#define BOOST_CHRONO_DETAIL_INLINED_PROCESS_CLOCK_HPP


#include <boost/chrono/config.hpp>
#include <boost/version.hpp>
#include <boost/chrono/process_times.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

//----------------------------------------------------------------------------//
//                                Windows                                     //
//----------------------------------------------------------------------------//
#if defined(BOOST_CHRONO_WINDOWS_API)
#include <boost/chrono/detail/inlined/win/process_clock.hpp>

//----------------------------------------------------------------------------//
//                                 Mac                                        //
//----------------------------------------------------------------------------//
#elif defined(BOOST_CHRONO_MAC_API)
#include <boost/chrono/detail/inlined/mac/process_clock.hpp>

//----------------------------------------------------------------------------//
//                                POSIX                                     //
//----------------------------------------------------------------------------//
#elif defined(BOOST_CHRONO_POSIX_API)
#include <boost/chrono/detail/inlined/posix/process_clock.hpp>

#endif  // POSIX
namespace boost { namespace chrono {

    void process_clock::now( time_points & tps, system::error_code & ec ) 
    {
        process_times t;
        process_clock::now(t,ec);
        tps.real=process_clock::time_point(t.real);
        tps.user=process_clock::time_point(t.user);
        tps.system=process_clock::time_point(t.system);
    }

}}

#endif
