// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_WINDOWS_SEARCH_PATH_HPP
#define BOOST_PROCESS_WINDOWS_SEARCH_PATH_HPP

#include <boost/process/detail/config.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/system/error_code.hpp>
#include <string>
#include <stdexcept>
#include <array>
#include <atomic>
#include <cstdlib>
#include <boost/detail/winapi/shell.hpp>



namespace boost { namespace process { namespace detail { namespace windows {

inline boost::filesystem::path search_path(
        const boost::filesystem::path &filename,
        const std::vector<boost::filesystem::path> &path)
{
    for (const boost::filesystem::path & pp : path)
    {
        auto full = pp / filename;
        std::array<std::string, 4> extensions {{ "", ".exe", ".com", ".bat" }};
        for (boost::filesystem::path ext : extensions)
        {
            auto p = full;
            p += ext;
            boost::system::error_code ec;
            bool file = boost::filesystem::is_regular_file(p, ec);
            if (!ec && file &&
                ::boost::detail::winapi::sh_get_file_info(p.string().c_str(), 0, 0, 0, ::boost::detail::winapi::SHGFI_EXETYPE_))
            {
                return p;
            }
        }
    }
    return "";
}

}}}}

#endif
