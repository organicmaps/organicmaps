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
#include <boost/chrono/process_cpu_clocks.hpp>
#include <cassert>

#include <boost/detail/win/GetLastError.hpp>
#include <boost/detail/win/GetCurrentProcess.hpp>
#include <boost/detail/win/GetProcessTimes.hpp>

namespace boost
{
namespace chrono
{

process_real_cpu_clock::time_point process_real_cpu_clock::now(
        system::error_code & ec) 
{

    //  note that Windows uses 100 nanosecond ticks for FILETIME
    boost::detail::win32::FILETIME_ creation, exit, user_time, system_time;

  #ifdef UNDER_CE
  // Windows CE does not support GetProcessTimes
    assert( 0 && "GetProcessTimes not supported under Windows CE" );
  return time_point();
  #else
    if ( boost::detail::win32::GetProcessTimes(
            boost::detail::win32::GetCurrentProcess(), &creation, &exit,
            &system_time, &user_time ) )
    {
        if (!BOOST_CHRONO_IS_THROWS(ec)) 
        {
            ec.clear();
        }
        return time_point(steady_clock::now().time_since_epoch());
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
                            "chrono::process_real_cpu_clock" ));
        } 
        else 
        {
            ec.assign( cause, BOOST_CHRONO_SYSTEM_CATEGORY );
            return time_point();
        }
    }
  #endif

}
process_user_cpu_clock::time_point process_user_cpu_clock::now(
        system::error_code & ec) 
{

    //  note that Windows uses 100 nanosecond ticks for FILETIME
    boost::detail::win32::FILETIME_ creation, exit, user_time, system_time;

  #ifdef UNDER_CE
  // Windows CE does not support GetProcessTimes
    assert( 0 && "GetProcessTimes not supported under Windows CE" );
  return time_point();
  #else
    if ( boost::detail::win32::GetProcessTimes(
            boost::detail::win32::GetCurrentProcess(), &creation, &exit,
            &system_time, &user_time ) )
    {
        if (!BOOST_CHRONO_IS_THROWS(ec)) 
        {
            ec.clear();
        }
        return time_point(duration(
                ((static_cast<process_user_cpu_clock::rep>(user_time.dwHighDateTime) << 32)
                  | user_time.dwLowDateTime) * 100
                ));
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
                            "chrono::process_user_cpu_clock" ));
        } 
        else 
        {
            ec.assign( cause, BOOST_CHRONO_SYSTEM_CATEGORY );
            return time_point();
        }
    }
  #endif

}
process_system_cpu_clock::time_point process_system_cpu_clock::now(
        system::error_code & ec) 
{

    //  note that Windows uses 100 nanosecond ticks for FILETIME
    boost::detail::win32::FILETIME_ creation, exit, user_time, system_time;

  #ifdef UNDER_CE
  // Windows CE does not support GetProcessTimes
    assert( 0 && "GetProcessTimes not supported under Windows CE" );
  return time_point();
  #else
    if ( boost::detail::win32::GetProcessTimes(
            boost::detail::win32::GetCurrentProcess(), &creation, &exit,
            &system_time, &user_time ) )
    {
        if (!BOOST_CHRONO_IS_THROWS(ec)) 
        {
            ec.clear();
        }
        return time_point(duration(
                ((static_cast<process_system_cpu_clock::rep>(system_time.dwHighDateTime) << 32)
                                    | system_time.dwLowDateTime) * 100
                ));
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
                            "chrono::process_system_cpu_clock" ));
        } 
        else 
        {
            ec.assign( cause, BOOST_CHRONO_SYSTEM_CATEGORY );
            return time_point();
        }
    }
  #endif
  
}
process_cpu_clock::time_point process_cpu_clock::now( 
        system::error_code & ec ) 
{

    //  note that Windows uses 100 nanosecond ticks for FILETIME
    boost::detail::win32::FILETIME_ creation, exit, user_time, system_time;

  #ifdef UNDER_CE
  // Windows CE does not support GetProcessTimes
    assert( 0 && "GetProcessTimes not supported under Windows CE" );
  return time_point();
  #else
    if ( boost::detail::win32::GetProcessTimes(
            boost::detail::win32::GetCurrentProcess(), &creation, &exit,
            &system_time, &user_time ) )
    {
        if (!BOOST_CHRONO_IS_THROWS(ec)) 
        {
            ec.clear();
        }
        time_point::rep r(
                steady_clock::now().time_since_epoch().count(), 
                ((static_cast<process_user_cpu_clock::rep>(user_time.dwHighDateTime) << 32)
                        | user_time.dwLowDateTime
                ) * 100, 
                ((static_cast<process_system_cpu_clock::rep>(system_time.dwHighDateTime) << 32)
                        | system_time.dwLowDateTime
                ) * 100 
        );
        return time_point(duration(r));
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
                            "chrono::process_cpu_clock" ));
        } 
        else 
        {
            ec.assign( cause, BOOST_CHRONO_SYSTEM_CATEGORY );
            return time_point();
        }
    }
  #endif

}
} // namespace chrono
} // namespace boost

#endif
