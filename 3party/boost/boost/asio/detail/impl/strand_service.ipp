//
// detail/impl/strand_service.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_IMPL_STRAND_SERVICE_IPP
#define BOOST_ASIO_DETAIL_IMPL_STRAND_SERVICE_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/call_stack.hpp>
#include <boost/asio/detail/strand_service.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

struct strand_service::on_do_complete_exit
{
  io_service_impl* owner_;
  strand_impl* impl_;

  ~on_do_complete_exit()
  {
    impl_->mutex_.lock();
    bool more_handlers = (--impl_->count_ > 0);
    impl_->mutex_.unlock();

    if (more_handlers)
      owner_->post_immediate_completion(impl_);
  }
};

strand_service::strand_service(boost::asio::io_service& io_service)
  : boost::asio::detail::service_base<strand_service>(io_service),
    io_service_(boost::asio::use_service<io_service_impl>(io_service)),
    mutex_(),
    salt_(0)
{
}

void strand_service::shutdown_service()
{
  op_queue<operation> ops;

  boost::asio::detail::mutex::scoped_lock lock(mutex_);

  for (std::size_t i = 0; i < num_implementations; ++i)
    if (strand_impl* impl = implementations_[i].get())
      ops.push(impl->queue_);
}

void strand_service::construct(strand_service::implementation_type& impl)
{
  std::size_t salt = salt_++;
  std::size_t index = reinterpret_cast<std::size_t>(&impl);
  index += (reinterpret_cast<std::size_t>(&impl) >> 3);
  index ^= salt + 0x9e3779b9 + (index << 6) + (index >> 2);
  index = index % num_implementations;

  boost::asio::detail::mutex::scoped_lock lock(mutex_);

  if (!implementations_[index].get())
    implementations_[index].reset(new strand_impl);
  impl = implementations_[index].get();
}

bool strand_service::do_dispatch(implementation_type& impl, operation* op)
{
  // If we are running inside the io_service, and no other handler is queued
  // or running, then the handler can run immediately.
  bool can_dispatch = call_stack<io_service_impl>::contains(&io_service_);
  impl->mutex_.lock();
  bool first = (++impl->count_ == 1);
  if (can_dispatch && first)
  {
    // Immediate invocation is allowed.
    impl->mutex_.unlock();
    return true;
  }

  // Immediate invocation is not allowed, so enqueue for later.
  impl->queue_.push(op);
  impl->mutex_.unlock();

  // The first handler to be enqueued is responsible for scheduling the
  // strand.
  if (first)
    io_service_.post_immediate_completion(impl);

  return false;
}

void strand_service::do_post(implementation_type& impl, operation* op)
{
  // Add the handler to the queue.
  impl->mutex_.lock();
  bool first = (++impl->count_ == 1);
  impl->queue_.push(op);
  impl->mutex_.unlock();

  // The first handler to be enqueue is responsible for scheduling the strand.
  if (first)
    io_service_.post_immediate_completion(impl);
}

void strand_service::do_complete(io_service_impl* owner, operation* base,
    boost::system::error_code /*ec*/, std::size_t /*bytes_transferred*/)
{
  if (owner)
  {
    strand_impl* impl = static_cast<strand_impl*>(base);

    // Get the next handler to be executed.
    impl->mutex_.lock();
    operation* o = impl->queue_.front();
    impl->queue_.pop();
    impl->mutex_.unlock();

    // Indicate that this strand is executing on the current thread.
    call_stack<strand_impl>::context ctx(impl);

    // Ensure the next handler, if any, is scheduled on block exit.
    on_do_complete_exit on_exit = { owner, impl };
    (void)on_exit;

    o->complete(*owner);
  }
}

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_DETAIL_IMPL_STRAND_SERVICE_IPP
