//
// detail/impl/strand_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_IMPL_STRAND_SERVICE_HPP
#define BOOST_ASIO_DETAIL_IMPL_STRAND_SERVICE_HPP

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

inline strand_service::strand_impl::strand_impl()
  : operation(&strand_service::do_complete),
    count_(0)
{
}

struct strand_service::on_dispatch_exit
{
  io_service_impl* io_service_;
  strand_impl* impl_;

  ~on_dispatch_exit()
  {
    impl_->mutex_.lock();
    bool more_handlers = (--impl_->count_ > 0);
    impl_->mutex_.unlock();

    if (more_handlers)
      io_service_->post_immediate_completion(impl_);
  }
};

inline void strand_service::destroy(strand_service::implementation_type& impl)
{
  impl = 0;
}

template <typename Handler>
void strand_service::dispatch(strand_service::implementation_type& impl,
    Handler handler)
{
  // If we are already in the strand then the handler can run immediately.
  if (call_stack<strand_impl>::contains(impl))
  {
    boost::asio::detail::fenced_block b;
    boost_asio_handler_invoke_helpers::invoke(handler, handler);
    return;
  }

  // Allocate and construct an operation to wrap the handler.
  typedef completion_handler<Handler> op;
  typename op::ptr p = { boost::addressof(handler),
    boost_asio_handler_alloc_helpers::allocate(
      sizeof(op), handler), 0 };
  p.p = new (p.v) op(handler);

  // If we are running inside the io_service, and no other handler is queued
  // or running, then the handler can run immediately.
  bool can_dispatch = call_stack<io_service_impl>::contains(&io_service_);
  impl->mutex_.lock();
  bool first = (++impl->count_ == 1);
  if (can_dispatch && first)
  {
    // Immediate invocation is allowed.
    impl->mutex_.unlock();

    // Memory must be releaesed before any upcall is made.
    p.reset();

    // Indicate that this strand is executing on the current thread.
    call_stack<strand_impl>::context ctx(impl);

    // Ensure the next handler, if any, is scheduled on block exit.
    on_dispatch_exit on_exit = { &io_service_, impl };
    (void)on_exit;

    boost::asio::detail::fenced_block b;
    boost_asio_handler_invoke_helpers::invoke(handler, handler);
    return;
  }

  // Immediate invocation is not allowed, so enqueue for later.
  impl->queue_.push(p.p);
  impl->mutex_.unlock();
  p.v = p.p = 0;

  // The first handler to be enqueued is responsible for scheduling the
  // strand.
  if (first)
    io_service_.post_immediate_completion(impl);
}

// Request the io_service to invoke the given handler and return immediately.
template <typename Handler>
void strand_service::post(strand_service::implementation_type& impl,
    Handler handler)
{
  // Allocate and construct an operation to wrap the handler.
  typedef completion_handler<Handler> op;
  typename op::ptr p = { boost::addressof(handler),
    boost_asio_handler_alloc_helpers::allocate(
      sizeof(op), handler), 0 };
  p.p = new (p.v) op(handler);

  // Add the handler to the queue.
  impl->mutex_.lock();
  bool first = (++impl->count_ == 1);
  impl->queue_.push(p.p);
  impl->mutex_.unlock();
  p.v = p.p = 0;

  // The first handler to be enqueue is responsible for scheduling the strand.
  if (first)
    io_service_.post_immediate_completion(impl);
}

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_DETAIL_IMPL_STRAND_SERVICE_HPP
