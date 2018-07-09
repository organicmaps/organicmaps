// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_WINDOWS_INITIALIZERS_NULL_OUT_HPP
#define BOOST_PROCESS_WINDOWS_INITIALIZERS_NULL_OUT_HPP

#include <boost/detail/winapi/process.hpp>
#include <boost/detail/winapi/handles.hpp>
#include <boost/detail/winapi/handle_info.hpp>
#include <boost/process/detail/handler_base.hpp>
#include <boost/process/detail/windows/file_descriptor.hpp>

namespace boost { namespace process { namespace detail { namespace windows {

template<int p1, int p2>
struct null_out : public ::boost::process::detail::handler_base
{
    file_descriptor sink {"NUL", file_descriptor::write}; //works because it gets destroyed AFTER launch.

    template <typename WindowsExecutor>
    void on_setup(WindowsExecutor &e) const;
};

template<>
template<typename WindowsExecutor>
void null_out<1,-1>::on_setup(WindowsExecutor &e) const
{
    boost::detail::winapi::SetHandleInformation(sink.handle(),
              boost::detail::winapi::HANDLE_FLAG_INHERIT_,
              boost::detail::winapi::HANDLE_FLAG_INHERIT_);

    e.startup_info.hStdOutput = sink.handle();
    e.startup_info.dwFlags   |= ::boost::detail::winapi::STARTF_USESTDHANDLES_;
    e.inherit_handles = true;

}

template<>
template<typename WindowsExecutor>
void null_out<2,-1>::on_setup(WindowsExecutor &e) const
{
    boost::detail::winapi::SetHandleInformation(sink.handle(),
              boost::detail::winapi::HANDLE_FLAG_INHERIT_,
              boost::detail::winapi::HANDLE_FLAG_INHERIT_);

    e.startup_info.hStdError = sink.handle();
    e.startup_info.dwFlags  |= ::boost::detail::winapi::STARTF_USESTDHANDLES_;
    e.inherit_handles = true;

}

template<>
template<typename WindowsExecutor>
void null_out<1,2>::on_setup(WindowsExecutor &e) const
{
    boost::detail::winapi::SetHandleInformation(sink.handle(),
            boost::detail::winapi::HANDLE_FLAG_INHERIT_,
            boost::detail::winapi::HANDLE_FLAG_INHERIT_);

    e.startup_info.hStdOutput = sink.handle();
    e.startup_info.hStdError  = sink.handle();
    e.startup_info.dwFlags   |= ::boost::detail::winapi::STARTF_USESTDHANDLES_;
    e.inherit_handles = true;

}

}}}}

#endif
