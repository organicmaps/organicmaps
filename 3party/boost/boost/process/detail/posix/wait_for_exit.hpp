// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_POSIX_WAIT_FOR_EXIT_HPP
#define BOOST_PROCESS_DETAIL_POSIX_WAIT_FOR_EXIT_HPP

#include <boost/process/detail/config.hpp>
#include <boost/process/detail/posix/child_handle.hpp>
#include <system_error>
#include <sys/types.h>
#include <sys/wait.h>


namespace boost { namespace process { namespace detail { namespace posix {

inline void wait(const child_handle &p, int & exit_code)
{
    pid_t ret;
    int status;
    do
    {
        ret = ::waitpid(p.pid, &status, 0);
    } while (((ret == -1) && (errno == EINTR)) || (ret != -1 && !WIFEXITED(status)));
    if (ret == -1)
        boost::process::detail::throw_last_error("waitpid(2) failed");
     exit_code = status;
}

inline void wait(const child_handle &p, int & exit_code, std::error_code &ec) noexcept
{
    pid_t ret;
    int status;

    do
    {
        ret = ::waitpid(p.pid, &status, 0);
    } 
    while (((ret == -1) && (errno == EINTR)) || (ret != -1 && !WIFEXITED(status)));
    
    if (ret == -1)
        ec = boost::process::detail::get_last_error();
    else
    {
        ec.clear();
        exit_code = status;
    }
  

}

template< class Rep, class Period >
inline bool wait_for(
        const child_handle &p,
        int & exit_code,
        const std::chrono::duration<Rep, Period>& rel_time)
{

    pid_t ret;
    int status;

    auto start = std::chrono::system_clock::now();
    auto time_out = start + rel_time;

    bool time_out_occured = false;
    do
    {
        ret = ::waitpid(p.pid, &status, WUNTRACED | WNOHANG);
        if (std::chrono::system_clock::now() >= time_out)
        {
            time_out_occured = true;
            break;
        }
    } 
    while (((ret == -1) && errno == EINTR)       || 
           ((ret != -1) && !WIFEXITED(status)));


    if (ret == -1)
        boost::process::detail::throw_last_error("waitpid(2) failed");
     
    exit_code = status;

    return !time_out_occured;
}


template< class Rep, class Period >
inline bool wait_for(
        const child_handle &p,
        int & exit_code,
        const std::chrono::duration<Rep, Period>& rel_time,
        std::error_code & ec) noexcept
{

    pid_t ret;
    int status;

    auto start = std::chrono::system_clock::now();
    auto time_out = start + rel_time;

    bool time_out_occured = false;
    do
    {
        ret = ::waitpid(p.pid, &status, WUNTRACED | WNOHANG);
        if (std::chrono::system_clock::now() >= time_out)
        {
            time_out_occured = true;
            break;
        }
    } 
    while (((ret == -1) && errno == EINTR)       || 
           ((ret != -1) && !WIFEXITED(status)));


    if (ret == -1)
        ec = boost::process::detail::get_last_error();
    else
    {
        ec.clear();
        exit_code = status;
    }

    return !time_out_occured;
}



template< class Rep, class Period >
inline bool wait_until(
        const child_handle &p,
        int & exit_code,
        const std::chrono::duration<Rep, Period>& time_out)
{

    pid_t ret;
    int status;

    bool time_out_occured = false;
    do
    {
        ret = ::waitpid(p.pid, &status, WUNTRACED | WNOHANG);
        if (std::chrono::system_clock::now() >= time_out)
        {
            time_out_occured = true;
            break;
        }
    } 
    while (((ret == -1) && errno == EINTR)       || 
           ((ret != -1) && !WIFEXITED(status)));


    if (ret == -1)
        boost::process::detail::throw_last_error("waitpid(2) failed");

    exit_code = status;

    return !time_out_occured;
}


template< class Rep, class Period >
inline bool wait_until(
        const child_handle &p,
        int & exit_code,
        const std::chrono::duration<Rep, Period>& time_out,
        std::error_code & ec) noexcept
{

    pid_t ret;
    int status;

    bool time_out_occured = false;
    do
    {
        ret = ::waitpid(p.pid, &status, WUNTRACED | WNOHANG);
        if (std::chrono::system_clock::now() >= time_out)
        {
            time_out_occured = true;
            break;
        }
    } 
    while (((ret == -1) && errno == EINTR)       || 
           ((ret != -1) && !WIFEXITED(status)));


    if (ret == -1)
        ec = boost::process::detail::get_last_error();
    else
    {
        ec.clear();
        exit_code = status;
    }

    return !time_out_occured;
}

}}}}

#endif
