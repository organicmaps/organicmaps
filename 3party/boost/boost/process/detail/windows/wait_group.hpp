// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_WINDOWS_WAIT_GROUP_HPP_
#define BOOST_PROCESS_DETAIL_WINDOWS_WAIT_GROUP_HPP_

#include <boost/process/detail/config.hpp>
#include <boost/detail/winapi/jobs.hpp>
#include <boost/detail/winapi/wait.hpp>


namespace boost { namespace process { namespace detail { namespace windows {

struct group_handle;

inline void wait(const group_handle &p)
{
    if (::boost::detail::winapi::WaitForSingleObject(p.handle(),
        ::boost::detail::winapi::infinite) == ::boost::detail::winapi::wait_failed)
            throw_last_error("WaitForSingleObject() failed");

}

inline void wait(const group_handle &p, std::error_code &ec)
{
    if (::boost::detail::winapi::WaitForSingleObject(p.handle(),
        ::boost::detail::winapi::infinite) == ::boost::detail::winapi::wait_failed)
            ec = get_last_error();
}


template< class Rep, class Period >
inline bool wait_for(
        const group_handle &p,
        const std::chrono::duration<Rep, Period>& rel_time)
{

    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(rel_time);

    ::boost::detail::winapi::DWORD_ wait_code;
    wait_code = ::boost::detail::winapi::WaitForSingleObject(p.handle(), ms.count());
    if (wait_code == ::boost::detail::winapi::wait_failed)
        throw_last_error("WaitForSingleObject() failed");
    else if (wait_code == ::boost::detail::winapi::wait_timeout)
        return false; //

    return true;
}


template< class Rep, class Period >
inline bool wait_for(
        const group_handle &p,
        const std::chrono::duration<Rep, Period>& rel_time,
        std::error_code &ec)
{

    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(rel_time);


    ::boost::detail::winapi::DWORD_ wait_code;
    wait_code = ::boost::detail::winapi::WaitForSingleObject(p.handle(), ms.count());
    if (wait_code == ::boost::detail::winapi::wait_failed)
        ec = get_last_error();

    else if (wait_code == ::boost::detail::winapi::wait_timeout)
        return false; //

    return true;
}

template< class Clock, class Duration >
inline bool wait_until(
        const group_handle &p,
        const std::chrono::time_point<Clock, Duration>& timeout_time)
{
    std::chrono::milliseconds ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    timeout_time - std::chrono::system_clock::now());

    ::boost::detail::winapi::DWORD_ wait_code;
    wait_code = ::boost::detail::winapi::WaitForSingleObject(p.handle(), ms.count());

    if (wait_code == ::boost::detail::winapi::wait_failed)
        throw_last_error("WaitForSingleObject() failed");

    else if (wait_code == ::boost::detail::winapi::wait_timeout)
        return false; //

    return true;
}


template< class Clock, class Duration >
inline bool wait_until(
        const group_handle &p,
        const std::chrono::time_point<Clock, Duration>& timeout_time,
        std::error_code &ec)
{
    std::chrono::milliseconds ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    timeout_time - std::chrono::system_clock::now());

    ::boost::detail::winapi::DWORD_ wait_code;
    wait_code = ::boost::detail::winapi::WaitForSingleObject(p.handle(), ms.count());

    if (wait_code == ::boost::detail::winapi::wait_failed)
        ec = get_last_error();

    else if (wait_code == ::boost::detail::winapi::wait_timeout)
        return false; //

    return true;
;
}

}}}}

#endif /* BOOST_PROCESS_DETAIL_WINDOWS_WAIT_GROUP_HPP_ */
