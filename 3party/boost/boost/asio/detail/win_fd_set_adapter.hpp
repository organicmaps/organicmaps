//
// detail/win_fd_set_adapter.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_WIN_FD_SET_ADAPTER_HPP
#define BOOST_ASIO_DETAIL_WIN_FD_SET_ADAPTER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_WINDOWS) || defined(__CYGWIN__)

#include <boost/asio/detail/socket_types.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

// Adapts the FD_SET type to meet the Descriptor_Set concept's requirements.
class win_fd_set_adapter
{
public:
  enum { win_fd_set_size = 1024 };

  win_fd_set_adapter()
    : max_descriptor_(invalid_socket)
  {
    fd_set_.fd_count = 0;
  }

  bool set(socket_type descriptor)
  {
    for (u_int i = 0; i < fd_set_.fd_count; ++i)
      if (fd_set_.fd_array[i] == descriptor)
        return true;
    if (fd_set_.fd_count < win_fd_set_size)
    {
      fd_set_.fd_array[fd_set_.fd_count++] = descriptor;
      return true;
    }
    return false;
  }

  bool is_set(socket_type descriptor) const
  {
    return !!__WSAFDIsSet(descriptor,
        const_cast<fd_set*>(reinterpret_cast<const fd_set*>(&fd_set_)));
  }

  operator fd_set*()
  {
    return reinterpret_cast<fd_set*>(&fd_set_);
  }

  socket_type max_descriptor() const
  {
    return max_descriptor_;
  }

private:
  // This structure is defined to be compatible with the Windows API fd_set
  // structure, but without being dependent on the value of FD_SETSIZE.
  struct win_fd_set
  {
    u_int fd_count;
    SOCKET fd_array[win_fd_set_size];
  };

  win_fd_set fd_set_;
  socket_type max_descriptor_;
};

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(BOOST_WINDOWS) || defined(__CYGWIN__)

#endif // BOOST_ASIO_DETAIL_WIN_FD_SET_ADAPTER_HPP
