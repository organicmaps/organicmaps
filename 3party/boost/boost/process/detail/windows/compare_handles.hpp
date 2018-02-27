// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_WINDOWS_COMPARE_HANDLES_HPP_
#define BOOST_PROCESS_DETAIL_WINDOWS_COMPARE_HANDLES_HPP_

#include <boost/detail/winapi/handles.hpp>
#include <boost/detail/winapi/file_management.hpp>
#include <boost/process/detail/config.hpp>

namespace boost { namespace process { namespace detail { namespace windows {

inline bool compare_handles(boost::detail::winapi::HANDLE_ lhs, boost::detail::winapi::HANDLE_ rhs)
{
    if ( (lhs == ::boost::detail::winapi::INVALID_HANDLE_VALUE_)
      || (rhs == ::boost::detail::winapi::INVALID_HANDLE_VALUE_))
        return false;

    if (lhs == rhs)
        return true;

    ::boost::detail::winapi::BY_HANDLE_FILE_INFORMATION_ lhs_info{0,{0,0},{0,0},{0,0},0,0,0,0,0,0};
    ::boost::detail::winapi::BY_HANDLE_FILE_INFORMATION_ rhs_info{0,{0,0},{0,0},{0,0},0,0,0,0,0,0};

    if (!::boost::detail::winapi::GetFileInformationByHandle(lhs, &lhs_info))
        ::boost::process::detail::throw_last_error("GetFileInformationByHandle");

    if (!::boost::detail::winapi::GetFileInformationByHandle(rhs, &rhs_info))
        ::boost::process::detail::throw_last_error("GetFileInformationByHandle");

    return     (lhs_info.nFileIndexHigh == rhs_info.nFileIndexHigh)
            && (lhs_info.nFileIndexLow  == rhs_info.nFileIndexLow);
}

}}}}



#endif /* BOOST_PROCESS_DETAIL_WINDOWS_COMPARE_HANDLES_HPP_ */
