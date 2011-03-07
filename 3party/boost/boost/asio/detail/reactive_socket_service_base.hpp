//
// detail/reactive_socket_service_base.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_REACTIVE_SOCKET_SERVICE_BASE_HPP
#define BOOST_ASIO_DETAIL_REACTIVE_SOCKET_SERVICE_BASE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if !defined(BOOST_ASIO_HAS_IOCP)

#include <boost/utility/addressof.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/detail/buffer_sequence_adapter.hpp>
#include <boost/asio/detail/reactive_null_buffers_op.hpp>
#include <boost/asio/detail/reactive_socket_recv_op.hpp>
#include <boost/asio/detail/reactive_socket_send_op.hpp>
#include <boost/asio/detail/reactor.hpp>
#include <boost/asio/detail/reactor_op.hpp>
#include <boost/asio/detail/socket_holder.hpp>
#include <boost/asio/detail/socket_ops.hpp>
#include <boost/asio/detail/socket_types.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

class reactive_socket_service_base
{
public:
  // The native type of a socket.
  typedef socket_type native_type;

  // The implementation type of the socket.
  struct base_implementation_type
  {
    // The native socket representation.
    socket_type socket_;

    // The current state of the socket.
    socket_ops::state_type state_;

    // Per-descriptor data used by the reactor.
    reactor::per_descriptor_data reactor_data_;
  };

  // Constructor.
  BOOST_ASIO_DECL reactive_socket_service_base(
      boost::asio::io_service& io_service);

  // Destroy all user-defined handler objects owned by the service.
  BOOST_ASIO_DECL void shutdown_service();

  // Construct a new socket implementation.
  BOOST_ASIO_DECL void construct(base_implementation_type& impl);

  // Destroy a socket implementation.
  BOOST_ASIO_DECL void destroy(base_implementation_type& impl);

  // Determine whether the socket is open.
  bool is_open(const base_implementation_type& impl) const
  {
    return impl.socket_ != invalid_socket;
  }

  // Destroy a socket implementation.
  BOOST_ASIO_DECL boost::system::error_code close(
      base_implementation_type& impl, boost::system::error_code& ec);

  // Get the native socket representation.
  native_type native(base_implementation_type& impl)
  {
    return impl.socket_;
  }

  // Cancel all operations associated with the socket.
  BOOST_ASIO_DECL boost::system::error_code cancel(
      base_implementation_type& impl, boost::system::error_code& ec);

  // Determine whether the socket is at the out-of-band data mark.
  bool at_mark(const base_implementation_type& impl,
      boost::system::error_code& ec) const
  {
    return socket_ops::sockatmark(impl.socket_, ec);
  }

  // Determine the number of bytes available for reading.
  std::size_t available(const base_implementation_type& impl,
      boost::system::error_code& ec) const
  {
    return socket_ops::available(impl.socket_, ec);
  }

  // Place the socket into the state where it will listen for new connections.
  boost::system::error_code listen(base_implementation_type& impl,
      int backlog, boost::system::error_code& ec)
  {
    socket_ops::listen(impl.socket_, backlog, ec);
    return ec;
  }

  // Perform an IO control command on the socket.
  template <typename IO_Control_Command>
  boost::system::error_code io_control(base_implementation_type& impl,
      IO_Control_Command& command, boost::system::error_code& ec)
  {
    socket_ops::ioctl(impl.socket_, impl.state_, command.name(),
        static_cast<ioctl_arg_type*>(command.data()), ec);
    return ec;
  }

  /// Disable sends or receives on the socket.
  boost::system::error_code shutdown(base_implementation_type& impl,
      socket_base::shutdown_type what, boost::system::error_code& ec)
  {
    socket_ops::shutdown(impl.socket_, what, ec);
    return ec;
  }

  // Send the given data to the peer.
  template <typename ConstBufferSequence>
  size_t send(base_implementation_type& impl,
      const ConstBufferSequence& buffers,
      socket_base::message_flags flags, boost::system::error_code& ec)
  {
    buffer_sequence_adapter<boost::asio::const_buffer,
        ConstBufferSequence> bufs(buffers);

    return socket_ops::sync_send(impl.socket_, impl.state_,
        bufs.buffers(), bufs.count(), flags, bufs.all_empty(), ec);
  }

  // Wait until data can be sent without blocking.
  size_t send(base_implementation_type& impl, const null_buffers&,
      socket_base::message_flags, boost::system::error_code& ec)
  {
    // Wait for socket to become ready.
    socket_ops::poll_write(impl.socket_, ec);

    return 0;
  }

  // Start an asynchronous send. The data being sent must be valid for the
  // lifetime of the asynchronous operation.
  template <typename ConstBufferSequence, typename Handler>
  void async_send(base_implementation_type& impl,
      const ConstBufferSequence& buffers,
      socket_base::message_flags flags, Handler handler)
  {
    // Allocate and construct an operation to wrap the handler.
    typedef reactive_socket_send_op<ConstBufferSequence, Handler> op;
    typename op::ptr p = { boost::addressof(handler),
      boost_asio_handler_alloc_helpers::allocate(
        sizeof(op), handler), 0 };
    p.p = new (p.v) op(impl.socket_, buffers, flags, handler);

    start_op(impl, reactor::write_op, p.p, true,
        ((impl.state_ & socket_ops::stream_oriented)
          && buffer_sequence_adapter<boost::asio::const_buffer,
            ConstBufferSequence>::all_empty(buffers)));
    p.v = p.p = 0;
  }

  // Start an asynchronous wait until data can be sent without blocking.
  template <typename Handler>
  void async_send(base_implementation_type& impl, const null_buffers&,
      socket_base::message_flags, Handler handler)
  {
    // Allocate and construct an operation to wrap the handler.
    typedef reactive_null_buffers_op<Handler> op;
    typename op::ptr p = { boost::addressof(handler),
      boost_asio_handler_alloc_helpers::allocate(
        sizeof(op), handler), 0 };
    p.p = new (p.v) op(handler);

    start_op(impl, reactor::write_op, p.p, false, false);
    p.v = p.p = 0;
  }

  // Receive some data from the peer. Returns the number of bytes received.
  template <typename MutableBufferSequence>
  size_t receive(base_implementation_type& impl,
      const MutableBufferSequence& buffers,
      socket_base::message_flags flags, boost::system::error_code& ec)
  {
    buffer_sequence_adapter<boost::asio::mutable_buffer,
        MutableBufferSequence> bufs(buffers);

    return socket_ops::sync_recv(impl.socket_, impl.state_,
        bufs.buffers(), bufs.count(), flags, bufs.all_empty(), ec);
  }

  // Wait until data can be received without blocking.
  size_t receive(base_implementation_type& impl, const null_buffers&,
      socket_base::message_flags, boost::system::error_code& ec)
  {
    // Wait for socket to become ready.
    socket_ops::poll_read(impl.socket_, ec);

    return 0;
  }

  // Start an asynchronous receive. The buffer for the data being received
  // must be valid for the lifetime of the asynchronous operation.
  template <typename MutableBufferSequence, typename Handler>
  void async_receive(base_implementation_type& impl,
      const MutableBufferSequence& buffers,
      socket_base::message_flags flags, Handler handler)
  {
    // Allocate and construct an operation to wrap the handler.
    typedef reactive_socket_recv_op<MutableBufferSequence, Handler> op;
    typename op::ptr p = { boost::addressof(handler),
      boost_asio_handler_alloc_helpers::allocate(
        sizeof(op), handler), 0 };
    p.p = new (p.v) op(impl.socket_, impl.state_, buffers, flags, handler);

    start_op(impl,
        (flags & socket_base::message_out_of_band)
          ? reactor::except_op : reactor::read_op,
        p.p, (flags & socket_base::message_out_of_band) == 0,
        ((impl.state_ & socket_ops::stream_oriented)
          && buffer_sequence_adapter<boost::asio::mutable_buffer,
            MutableBufferSequence>::all_empty(buffers)));
    p.v = p.p = 0;
  }

  // Wait until data can be received without blocking.
  template <typename Handler>
  void async_receive(base_implementation_type& impl, const null_buffers&,
      socket_base::message_flags flags, Handler handler)
  {
    // Allocate and construct an operation to wrap the handler.
    typedef reactive_null_buffers_op<Handler> op;
    typename op::ptr p = { boost::addressof(handler),
      boost_asio_handler_alloc_helpers::allocate(
        sizeof(op), handler), 0 };
    p.p = new (p.v) op(handler);

    start_op(impl,
        (flags & socket_base::message_out_of_band)
          ? reactor::except_op : reactor::read_op,
        p.p, false, false);
    p.v = p.p = 0;
  }

protected:
  // Open a new socket implementation.
  BOOST_ASIO_DECL boost::system::error_code do_open(
      base_implementation_type& impl, int af,
      int type, int protocol, boost::system::error_code& ec);

  // Assign a native socket to a socket implementation.
  BOOST_ASIO_DECL boost::system::error_code do_assign(
      base_implementation_type& impl, int type,
      const native_type& native_socket, boost::system::error_code& ec);

  // Start the asynchronous read or write operation.
  BOOST_ASIO_DECL void start_op(base_implementation_type& impl, int op_type,
      reactor_op* op, bool non_blocking, bool noop);

  // Start the asynchronous accept operation.
  BOOST_ASIO_DECL void start_accept_op(base_implementation_type& impl,
      reactor_op* op, bool peer_is_open);

  // Start the asynchronous connect operation.
  BOOST_ASIO_DECL void start_connect_op(base_implementation_type& impl,
      reactor_op* op, const socket_addr_type* addr, size_t addrlen);

  // The selector that performs event demultiplexing for the service.
  reactor& reactor_;
};

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#if defined(BOOST_ASIO_HEADER_ONLY)
# include <boost/asio/detail/impl/reactive_socket_service_base.ipp>
#endif // defined(BOOST_ASIO_HEADER_ONLY)

#endif // !defined(BOOST_ASIO_HAS_IOCP)

#endif // BOOST_ASIO_DETAIL_REACTIVE_SOCKET_SERVICE_BASE_HPP
