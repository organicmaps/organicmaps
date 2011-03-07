//
// detail/call_stack.hpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_CALL_STACK_HPP
#define BOOST_ASIO_DETAIL_CALL_STACK_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/noncopyable.hpp>
#include <boost/asio/detail/tss_ptr.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

// Helper class to determine whether or not the current thread is inside an
// invocation of io_service::run() for a specified io_service object.
template <typename Owner>
class call_stack
{
public:
  // Context class automatically pushes an owner on to the stack.
  class context
    : private noncopyable
  {
  public:
    // Push the owner on to the stack.
    explicit context(Owner* d)
      : owner_(d),
        next_(call_stack<Owner>::top_)
    {
      call_stack<Owner>::top_ = this;
    }

    // Pop the owner from the stack.
    ~context()
    {
      call_stack<Owner>::top_ = next_;
    }

  private:
    friend class call_stack<Owner>;

    // The owner associated with the context.
    Owner* owner_;

    // The next element in the stack.
    context* next_;
  };

  friend class context;

  // Determine whether the specified owner is on the stack.
  static bool contains(Owner* d)
  {
    context* elem = top_;
    while (elem)
    {
      if (elem->owner_ == d)
        return true;
      elem = elem->next_;
    }
    return false;
  }

private:
  // The top of the stack of calls for the current thread.
  static tss_ptr<context> top_;
};

template <typename Owner>
tss_ptr<typename call_stack<Owner>::context>
call_stack<Owner>::top_;

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_DETAIL_CALL_STACK_HPP
