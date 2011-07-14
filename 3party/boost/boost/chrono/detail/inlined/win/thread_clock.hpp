//  boost thread_clock.cpp  -----------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  See http://www.boost.org/libs/chrono for documentation.

//--------------------------------------------------------------------------------------//
#ifndef BOOST_CHRONO_DETAIL_INLINED_WIN_THREAD_CLOCK_HPP
#define BOOST_CHRONO_DETAIL_INLINED_WIN_THREAD_CLOCK_HPP

#include <boost/chrono/config.hpp>
#include <boost/chrono/thread_clock.hpp>
#include <cassert>

#include <boost/detail/win/GetLastError.hpp>
#include <boost/detail/win/GetCurrentThread.hpp>
#include <boost/detail/win/GetThreadTimes.hpp>

namespace boost
{
namespace chrono
{

thread_clock::time_point thread_clock::now( system::error_code & ec )
{
    //  note that Windows uses 100 nanosecond ticks for FILETIME
    boost::detail::win32::FILETIME_ creation, exit, user_time, system_time;

    if ( boost::detail::win32::GetThreadTimes(
            boost::detail::win32::GetCurrentThread (), &creation, &exit,
            &system_time, &user_time ) )
    {
        duration user = duration(
                ((static_cast<duration::rep>(user_time.dwHighDateTime) << 32)
                        | user_time.dwLowDateTime) * 100 );

        duration system = duration(
                ((static_cast<duration::rep>(system_time.dwHighDateTime) << 32)
                        | system_time.dwLowDateTime) * 100 );

        if (!BOOST_CHRONO_IS_THROWS(ec)) 
        {
            ec.clear();
        }
        return time_point(system+user);

    }
    else
    {
        if (BOOST_CHRONO_IS_THROWS(ec)) 
        {
            boost::throw_exception(
                    system::system_error( 
                            boost::detail::win32::GetLastError(), 
                            BOOST_CHRONO_SYSTEM_CATEGORY, 
                            "chrono::thread_clock" ));
        } 
        else 
        {
            ec.assign( boost::detail::win32::GetLastError(), BOOST_CHRONO_SYSTEM_CATEGORY );
            return thread_clock::time_point(duration(0));
        }
    }
}

thread_clock::time_point thread_clock::now( )
{

    //  note that Windows uses 100 nanosecond ticks for FILETIME
    boost::detail::win32::FILETIME_ creation, exit, user_time, system_time;

    if ( boost::detail::win32::GetThreadTimes( 
            boost::detail::win32::GetCurrentThread (), &creation, &exit,
            &system_time, &user_time ) )
    {
        duration user   = duration(
                ((static_cast<duration::rep>(user_time.dwHighDateTime) << 32)
                        | user_time.dwLowDateTime) * 100 );

        duration system = duration(
                ((static_cast<duration::rep>(system_time.dwHighDateTime) << 32)
                        | system_time.dwLowDateTime) * 100 );

        return time_point(system+user);

    }
    else
    {
        boost::throw_exception(
                system::system_error( 
                        boost::detail::win32::GetLastError(), 
                        BOOST_CHRONO_SYSTEM_CATEGORY, 
                        "chrono::thread_clock" ));
    }

}

} // namespace chrono
} // namespace boost

#endif
