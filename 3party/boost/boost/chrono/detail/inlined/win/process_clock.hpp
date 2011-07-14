//  boost process_timer.cpp  -----------------------------------------------------------//

//  Copyright Beman Dawes 1994, 2006, 2008
//  Copyright 2009-2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  See http://www.boost.org/libs/chrono for documentation.

//--------------------------------------------------------------------------------------//
#ifndef BOOST_CHRONO_DETAIL_INLINED_WIN_PROCESS_CLOCK_HPP
#define BOOST_CHRONO_DETAIL_INLINED_WIN_PROCESS_CLOCK_HPP

#include <boost/chrono/config.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/chrono/process_times.hpp>
#include <cassert>

#include <boost/detail/win/GetLastError.hpp>
#include <boost/detail/win/GetCurrentProcess.hpp>
#include <boost/detail/win/GetProcessTimes.hpp>

namespace boost
{
namespace chrono
{

void process_clock::now( process_times & times_, system::error_code & ec )
{

    //  note that Windows uses 100 nanosecond ticks for FILETIME
    boost::detail::win32::FILETIME_ creation, exit, user_time, system_time;

    times_.real = duration( steady_clock::now().time_since_epoch().count() );
       
  #ifdef UNDER_CE
  // Windows CE does not support GetProcessTimes
    assert( 0 && "GetProcessTimes not supported under Windows CE" );
  times_.real = times_.system = times_.user = nanoseconds(-1);
  #else
    if ( boost::detail::win32::GetProcessTimes(
            boost::detail::win32::GetCurrentProcess(), &creation, &exit,
            &system_time, &user_time ) )
    {
        if (!BOOST_CHRONO_IS_THROWS(ec)) 
        {
            ec.clear();
        }
        times_.user   = duration(
          ((static_cast<time_point::rep>(user_time.dwHighDateTime) << 32)
            | user_time.dwLowDateTime) * 100 );

        times_.system = duration(
          ((static_cast<time_point::rep>(system_time.dwHighDateTime) << 32)
            | system_time.dwLowDateTime) * 100 );
    }
    else
    {
        boost::detail::win32::DWORD_ cause = boost::detail::win32::GetLastError();
        if (BOOST_CHRONO_IS_THROWS(ec)) 
        {
            boost::throw_exception(
                    system::system_error( 
                            cause, 
                            BOOST_CHRONO_SYSTEM_CATEGORY, 
                            "chrono::process_clock" ));
        } 
        else 
        {
            ec.assign( cause, BOOST_CHRONO_SYSTEM_CATEGORY );
            times_.real = times_.system = times_.user = nanoseconds(-1);
        }
    }
  #endif
}
} // namespace chrono
} // namespace boost

#endif
