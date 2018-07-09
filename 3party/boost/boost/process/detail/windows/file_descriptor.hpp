// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_WINDOWS_FILE_DESCRIPTOR_HPP_
#define BOOST_PROCESS_DETAIL_WINDOWS_FILE_DESCRIPTOR_HPP_

#include <boost/detail/winapi/basic_types.hpp>
#include <boost/detail/winapi/handles.hpp>
#include <boost/detail/winapi/file_management.hpp>
#include <string>
#include <boost/filesystem/path.hpp>

namespace boost { namespace process { namespace detail { namespace windows {

struct file_descriptor
{
    enum mode_t
    {
        read  = 1,
        write = 2,
        read_write = 3
    };
    static ::boost::detail::winapi::DWORD_ desired_access(mode_t mode)
    {
        switch(mode)
        {
        case read:
            return ::boost::detail::winapi::GENERIC_READ_;
        case write:
            return ::boost::detail::winapi::GENERIC_WRITE_;
        case read_write:
            return ::boost::detail::winapi::GENERIC_READ_
                 | ::boost::detail::winapi::GENERIC_WRITE_;
        default:
            return 0u;
        }
    }

    file_descriptor() = default;
    file_descriptor(const boost::filesystem::path& p, mode_t mode = read_write)
        : file_descriptor(p.native(), mode)
    {
    }

    file_descriptor(const std::string & path , mode_t mode = read_write)
        : file_descriptor(path.c_str(), mode) {}
    file_descriptor(const std::wstring & path, mode_t mode = read_write)
        : file_descriptor(path.c_str(), mode) {}

    file_descriptor(const char*    path, mode_t mode = read_write)
        : _handle(
                ::boost::detail::winapi::create_file(
                        path,
                        desired_access(mode),
                        ::boost::detail::winapi::FILE_SHARE_READ_ |
                        ::boost::detail::winapi::FILE_SHARE_WRITE_,
                        nullptr,
                        ::boost::detail::winapi::OPEN_ALWAYS_,

                        ::boost::detail::winapi::FILE_ATTRIBUTE_NORMAL_,
                        nullptr
                ))
    {

    }
    file_descriptor(const wchar_t * path, mode_t mode = read_write)
        : _handle(
            ::boost::detail::winapi::create_file(
                    path,
                    desired_access(mode),
                    ::boost::detail::winapi::FILE_SHARE_READ_ |
                    ::boost::detail::winapi::FILE_SHARE_WRITE_,
                    nullptr,
                    ::boost::detail::winapi::OPEN_ALWAYS_,

                    ::boost::detail::winapi::FILE_ATTRIBUTE_NORMAL_,
                    nullptr
            ))
{

}
    file_descriptor(const file_descriptor & ) = delete;
    file_descriptor(file_descriptor && ) = default;

    file_descriptor& operator=(const file_descriptor & ) = delete;
    file_descriptor& operator=(file_descriptor && ) = default;

    ~file_descriptor()
    {
        if (_handle != ::boost::detail::winapi::INVALID_HANDLE_VALUE_)
            ::boost::detail::winapi::CloseHandle(_handle);
    }

    ::boost::detail::winapi::HANDLE_ handle() const { return _handle;}

private:
    ::boost::detail::winapi::HANDLE_ _handle = ::boost::detail::winapi::INVALID_HANDLE_VALUE_;
};

}}}}

#endif /* BOOST_PROCESS_DETAIL_WINDOWS_FILE_DESCRIPTOR_HPP_ */
