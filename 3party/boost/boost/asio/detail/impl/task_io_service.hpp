//
// detail/impl/task_io_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_IMPL_TASK_IO_SERVICE_HPP
#define BOOST_ASIO_DETAIL_IMPL_TASK_IO_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/call_stack.hpp>
#include <boost/asio/detail/completion_handler.hpp>
#include <boost/asio/detail/fenced_block.hpp>
#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

template <typename Handler>
void task_io_service::dispatch(Handler handler)
{
  if (call_stack<task_io_service>::contains(this))
  {
    boost::asio::detail::fenced_block b;
    boost_asio_handler_invoke_helpers::invoke(handler, handler);
  }
  else
    post(handler);
}

template <typename Handler>
void task_io_service::post(Handler handler)
{
  // Allocate and construct an operation to wrap the handler.
  typedef completion_handler<Handler> op;
  typename op::ptr p = { boost::addressof(handler),
    boost_asio_handler_alloc_helpers::allocate(
      sizeof(op), handler), 0 };
  p.p = new (p.v) op(handler);

  post_immediate_completion(p.p);
  p.v = p.p = 0;
}

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_DETAIL_IMPL_TASK_IO_SERVICE_HPP
