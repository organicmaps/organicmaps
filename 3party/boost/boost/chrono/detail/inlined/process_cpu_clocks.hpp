//  boost process_cpu_clocks.cpp  -----------------------------------------------------------//

//  Copyright 2009-2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  See http://www.boost.org/libs/chrono for documentation.

//--------------------------------------------------------------------------------------//
#ifndef BOOST_CHRONO_DETAIL_INLINED_PROCESS_CPU_CLOCKS_HPP
#define BOOST_CHRONO_DETAIL_INLINED_PROCESS_CPU_CLOCKS_HPP


#include <boost/chrono/config.hpp>
#include <boost/version.hpp>
#include <boost/chrono/process_cpu_clocks.hpp>
#include <boost/throw_exception.hpp>
#include <boost/system/system_error.hpp>

//----------------------------------------------------------------------------//
//                                Windows                                     //
//----------------------------------------------------------------------------//
#if defined(BOOST_CHRONO_WINDOWS_API)
#include <boost/chrono/detail/inlined/win/process_cpu_clocks.hpp>

//----------------------------------------------------------------------------//
//                                 Mac                                        //
//----------------------------------------------------------------------------//
#elif defined(BOOST_CHRONO_MAC_API)
#include <boost/chrono/detail/inlined/mac/process_cpu_clocks.hpp>

//----------------------------------------------------------------------------//
//                                POSIX                                     //
//----------------------------------------------------------------------------//
#elif defined(BOOST_CHRONO_POSIX_API)
#include <boost/chrono/detail/inlined/posix/process_cpu_clocks.hpp>

#endif  // POSIX
#if 0
namespace boost { namespace chrono {

    process_real_cpu_clock::time_point process_real_cpu_clock::now(
            system::error_code & ec) 
    {
        process_times t;
        process_clock::now(t, ec);
        return process_real_cpu_clock::time_point(t.real);
    }

    process_user_cpu_clock::time_point process_user_cpu_clock::now(
            system::error_code & ec)
    {
        process_times t;
        process_clock::now(t, ec);
        return process_user_cpu_clock::time_point(t.user);
    }

    process_system_cpu_clock::time_point process_system_cpu_clock::now(
            system::error_code & ec) 
    {
        process_times t;
        process_clock::now(t, ec);
        return process_system_cpu_clock::time_point(t.system);
    }

    process_cpu_clock::time_point process_cpu_clock::now( 
            system::error_code & ec ) 
    {
        process_times t;
        process_clock::now(t,ec);
        time_point::rep r(t.real.count(), t.user.count(), t.system.count());
        return time_point(duration(r));
    }

} // namespace chrono
} // namespace boost
#endif

#endif
