//
// detail/reactive_socket_service_base.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_IMPL_REACTIVE_SOCKET_SERVICE_BASE_IPP
#define BOOST_ASIO_DETAIL_IMPL_REACTIVE_SOCKET_SERVICE_BASE_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if !defined(BOOST_ASIO_HAS_IOCP)

#include <boost/asio/detail/reactive_socket_service_base.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

reactive_socket_service_base::reactive_socket_service_base(
    boost::asio::io_service& io_service)
  : reactor_(use_service<reactor>(io_service))
{
  reactor_.init_task();
}

void reactive_socket_service_base::shutdown_service()
{
}

void reactive_socket_service_base::construct(
    reactive_socket_service_base::base_implementation_type& impl)
{
  impl.socket_ = invalid_socket;
  impl.state_ = 0;
}

void reactive_socket_service_base::destroy(
    reactive_socket_service_base::base_implementation_type& impl)
{
  if (impl.socket_ != invalid_socket)
  {
    reactor_.close_descriptor(impl.socket_, impl.reactor_data_);

    boost::system::error_code ignored_ec;
    socket_ops::close(impl.socket_, impl.state_, true, ignored_ec);
  }
}

boost::system::error_code reactive_socket_service_base::close(
    reactive_socket_service_base::base_implementation_type& impl,
    boost::system::error_code& ec)
{
  if (is_open(impl))
    reactor_.close_descriptor(impl.socket_, impl.reactor_data_);

  if (socket_ops::close(impl.socket_, impl.state_, true, ec) == 0)
    construct(impl);

  return ec;
}

boost::system::error_code reactive_socket_service_base::cancel(
    reactive_socket_service_base::base_implementation_type& impl,
    boost::system::error_code& ec)
{
  if (!is_open(impl))
  {
    ec = boost::asio::error::bad_descriptor;
    return ec;
  }

  reactor_.cancel_ops(impl.socket_, impl.reactor_data_);
  ec = boost::system::error_code();
  return ec;
}

boost::system::error_code reactive_socket_service_base::do_open(
    reactive_socket_service_base::base_implementation_type& impl,
    int af, int type, int protocol, boost::system::error_code& ec)
{
  if (is_open(impl))
  {
    ec = boost::asio::error::already_open;
    return ec;
  }

  socket_holder sock(socket_ops::socket(af, type, protocol, ec));
  if (sock.get() == invalid_socket)
    return ec;

  if (int err = reactor_.register_descriptor(sock.get(), impl.reactor_data_))
  {
    ec = boost::system::error_code(err,
        boost::asio::error::get_system_category());
    return ec;
  }

  impl.socket_ = sock.release();
  switch (type)
  {
  case SOCK_STREAM: impl.state_ = socket_ops::stream_oriented; break;
  case SOCK_DGRAM: impl.state_ = socket_ops::datagram_oriented; break;
  default: impl.state_ = 0; break;
  }
  ec = boost::system::error_code();
  return ec;
}

boost::system::error_code reactive_socket_service_base::do_assign(
    reactive_socket_service_base::base_implementation_type& impl, int type,
    const reactive_socket_service_base::native_type& native_socket,
    boost::system::error_code& ec)
{
  if (is_open(impl))
  {
    ec = boost::asio::error::already_open;
    return ec;
  }

  if (int err = reactor_.register_descriptor(
        native_socket, impl.reactor_data_))
  {
    ec = boost::system::error_code(err,
        boost::asio::error::get_system_category());
    return ec;
  }

  impl.socket_ = native_socket;
  switch (type)
  {
  case SOCK_STREAM: impl.state_ = socket_ops::stream_oriented; break;
  case SOCK_DGRAM: impl.state_ = socket_ops::datagram_oriented; break;
  default: impl.state_ = 0; break;
  }
  ec = boost::system::error_code();
  return ec;
}

void reactive_socket_service_base::start_op(
    reactive_socket_service_base::base_implementation_type& impl,
    int op_type, reactor_op* op, bool non_blocking, bool noop)
{
  if (!noop)
  {
    if ((impl.state_ & socket_ops::non_blocking)
        || socket_ops::set_internal_non_blocking(
          impl.socket_, impl.state_, op->ec_))
    {
      reactor_.start_op(op_type, impl.socket_,
          impl.reactor_data_, op, non_blocking);
      return;
    }
  }

  reactor_.post_immediate_completion(op);
}

void reactive_socket_service_base::start_accept_op(
    reactive_socket_service_base::base_implementation_type& impl,
    reactor_op* op, bool peer_is_open)
{
  if (!peer_is_open)
    start_op(impl, reactor::read_op, op, true, false);
  else
  {
    op->ec_ = boost::asio::error::already_open;
    reactor_.post_immediate_completion(op);
  }
}

void reactive_socket_service_base::start_connect_op(
    reactive_socket_service_base::base_implementation_type& impl,
    reactor_op* op, const socket_addr_type* addr, size_t addrlen)
{
  if ((impl.state_ & socket_ops::non_blocking)
      || socket_ops::set_internal_non_blocking(
        impl.socket_, impl.state_, op->ec_))
  {
    if (socket_ops::connect(impl.socket_, addr, addrlen, op->ec_) != 0)
    {
      if (op->ec_ == boost::asio::error::in_progress
          || op->ec_ == boost::asio::error::would_block)
      {
        op->ec_ = boost::system::error_code();
        reactor_.start_op(reactor::connect_op,
            impl.socket_, impl.reactor_data_, op, false);
        return;
      }
    }
  }

  reactor_.post_immediate_completion(op);
}

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // !defined(BOOST_ASIO_HAS_IOCP)

#endif // BOOST_ASIO_DETAIL_IMPL_REACTIVE_SOCKET_SERVICE_BASE_IPP
