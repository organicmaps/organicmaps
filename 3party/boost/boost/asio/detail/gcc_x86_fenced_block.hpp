//
// detail/gcc_x86_fenced_block.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_GCC_X86_FENCED_BLOCK_HPP
#define BOOST_ASIO_DETAIL_GCC_X86_FENCED_BLOCK_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

class gcc_x86_fenced_block
  : private noncopyable
{
public:
  // Constructor.
  gcc_x86_fenced_block()
  {
    barrier();
  }

  // Destructor.
  ~gcc_x86_fenced_block()
  {
    barrier();
  }

private:
  static int barrier()
  {
    int r = 0;
    __asm__ __volatile__ ("xchgl %%eax, %0" : "=m" (r) : : "memory", "cc");
    return r;
  }
};

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))

#endif // BOOST_ASIO_DETAIL_GCC_X86_FENCED_BLOCK_HPP
