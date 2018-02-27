// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_WINDOWS_SHOW_WINDOW_HPP
#define BOOST_PROCESS_WINDOWS_SHOW_WINDOW_HPP

#include <boost/detail/winapi/process.hpp>
#include <boost/detail/winapi/show_window.hpp>
#include <boost/process/detail/handler_base.hpp>


namespace boost { namespace process { namespace detail { namespace windows {

template<::boost::detail::winapi::WORD_ Flags>
struct show_window : ::boost::process::detail::handler_base
{
    template <class WindowsExecutor>
    void on_setup(WindowsExecutor &e) const
    {
        e.startup_info.dwFlags |= ::boost::detail::winapi::STARTF_USESHOWWINDOW_;
        e.startup_info.wShowWindow |= Flags;
    }
};





}}}}

#endif

