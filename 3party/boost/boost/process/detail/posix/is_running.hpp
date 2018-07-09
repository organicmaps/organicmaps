// Copyright (c) 2106 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_POSIX_IS_RUNNING_HPP
#define BOOST_PROCESS_DETAIL_POSIX_IS_RUNNING_HPP

#include <boost/process/detail/config.hpp>
#include <boost/process/detail/posix/child_handle.hpp>
#include <system_error>
#include <sys/wait.h>

namespace boost { namespace process { namespace detail { namespace posix {


constexpr int still_active = 0x7F;
static_assert(!WIFEXITED(still_active), "Internal Error");

inline bool is_running(const child_handle &p, int & exit_code)
{
    int status; 
    auto ret = ::waitpid(p.pid, &status, WNOHANG|WUNTRACED);
    
    if (ret == -1)
    {
        if (errno != ECHILD) //because it no child is running, than this one isn't either, obviously.
            ::boost::process::detail::throw_last_error("is_running error");

        return false;
    }
    else if (ret == 0)
        return true;
    else //exited
    {
        if (WIFEXITED(status))
            exit_code = status;
        return false;
    }
}

inline bool is_running(const child_handle &p, int & exit_code, std::error_code &ec) noexcept
{
    int status;
    auto ret = ::waitpid(p.pid, &status, WNOHANG|WUNTRACED); 
    
    if (ret == -1)
    {
        if (errno != ECHILD) //because it no child is running, than this one isn't either, obviously.
            ec = ::boost::process::detail::get_last_error();
        return false;
    }
    else if (ret == 0)
        return true;
    else
    {
        ec.clear();
        
        if (WIFEXITED(status))
            exit_code = status;
        
        return false;
    }
}

inline bool is_running(int code)
{
    return !WIFEXITED(code);
}

inline int eval_exit_status(int code)
{
    return WEXITSTATUS(code);
}

}}}}

#endif
