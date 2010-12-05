//
// socket_acceptor_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_SOCKET_ACCEPTOR_SERVICE_HPP
#define BOOST_ASIO_SOCKET_ACCEPTOR_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/basic_socket.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/io_service.hpp>

#if defined(BOOST_ASIO_HAS_IOCP)
# include <boost/asio/detail/win_iocp_socket_service.hpp>
#else
# include <boost/asio/detail/reactive_socket_service.hpp>
#endif

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

/// Default service implementation for a socket acceptor.
template <typename Protocol>
class socket_acceptor_service
#if defined(GENERATING_DOCUMENTATION)
  : public boost::asio::io_service::service
#else
  : public boost::asio::detail::service_base<socket_acceptor_service<Protocol> >
#endif
{
public:
#if defined(GENERATING_DOCUMENTATION)
  /// The unique service identifier.
  static boost::asio::io_service::id id;
#endif

  /// The protocol type.
  typedef Protocol protocol_type;

  /// The endpoint type.
  typedef typename protocol_type::endpoint endpoint_type;

private:
  // The type of the platform-specific implementation.
#if defined(BOOST_ASIO_HAS_IOCP)
  typedef detail::win_iocp_socket_service<Protocol> service_impl_type;
#else
  typedef detail::reactive_socket_service<Protocol> service_impl_type;
#endif

public:
  /// The native type of the socket acceptor.
#if defined(GENERATING_DOCUMENTATION)
  typedef implementation_defined implementation_type;
#else
  typedef typename service_impl_type::implementation_type implementation_type;
#endif

  /// The native acceptor type.
#if defined(GENERATING_DOCUMENTATION)
  typedef implementation_defined native_type;
#else
  typedef typename service_impl_type::native_type native_type;
#endif

  /// Construct a new socket acceptor service for the specified io_service.
  explicit socket_acceptor_service(boost::asio::io_service& io_service)
    : boost::asio::detail::service_base<
        socket_acceptor_service<Protocol> >(io_service),
      service_impl_(io_service)
  {
  }

  /// Destroy all user-defined handler objects owned by the service.
  void shutdown_service()
  {
    service_impl_.shutdown_service();
  }

  /// Construct a new socket acceptor implementation.
  void construct(implementation_type& impl)
  {
    service_impl_.construct(impl);
  }

  /// Destroy a socket acceptor implementation.
  void destroy(implementation_type& impl)
  {
    service_impl_.destroy(impl);
  }

  /// Open a new socket acceptor implementation.
  boost::system::error_code open(implementation_type& impl,
      const protocol_type& protocol, boost::system::error_code& ec)
  {
    return service_impl_.open(impl, protocol, ec);
  }

  /// Assign an existing native acceptor to a socket acceptor.
  boost::system::error_code assign(implementation_type& impl,
      const protocol_type& protocol, const native_type& native_acceptor,
      boost::system::error_code& ec)
  {
    return service_impl_.assign(impl, protocol, native_acceptor, ec);
  }

  /// Determine whether the acceptor is open.
  bool is_open(const implementation_type& impl) const
  {
    return service_impl_.is_open(impl);
  }

  /// Cancel all asynchronous operations associated with the acceptor.
  boost::system::error_code cancel(implementation_type& impl,
      boost::system::error_code& ec)
  {
    return service_impl_.cancel(impl, ec);
  }

  /// Bind the socket acceptor to the specified local endpoint.
  boost::system::error_code bind(implementation_type& impl,
      const endpoint_type& endpoint, boost::system::error_code& ec)
  {
    return service_impl_.bind(impl, endpoint, ec);
  }

  /// Place the socket acceptor into the state where it will listen for new
  /// connections.
  boost::system::error_code listen(implementation_type& impl, int backlog,
      boost::system::error_code& ec)
  {
    return service_impl_.listen(impl, backlog, ec);
  }

  /// Close a socket acceptor implementation.
  boost::system::error_code close(implementation_type& impl,
      boost::system::error_code& ec)
  {
    return service_impl_.close(impl, ec);
  }

  /// Get the native acceptor implementation.
  native_type native(implementation_type& impl)
  {
    return service_impl_.native(impl);
  }

  /// Set a socket option.
  template <typename SettableSocketOption>
  boost::system::error_code set_option(implementation_type& impl,
      const SettableSocketOption& option, boost::system::error_code& ec)
  {
    return service_impl_.set_option(impl, option, ec);
  }

  /// Get a socket option.
  template <typename GettableSocketOption>
  boost::system::error_code get_option(const implementation_type& impl,
      GettableSocketOption& option, boost::system::error_code& ec) const
  {
    return service_impl_.get_option(impl, option, ec);
  }

  /// Perform an IO control command on the socket.
  template <typename IoControlCommand>
  boost::system::error_code io_control(implementation_type& impl,
      IoControlCommand& command, boost::system::error_code& ec)
  {
    return service_impl_.io_control(impl, command, ec);
  }

  /// Get the local endpoint.
  endpoint_type local_endpoint(const implementation_type& impl,
      boost::system::error_code& ec) const
  {
    return service_impl_.local_endpoint(impl, ec);
  }

  /// Accept a new connection.
  template <typename SocketService>
  boost::system::error_code accept(implementation_type& impl,
      basic_socket<protocol_type, SocketService>& peer,
      endpoint_type* peer_endpoint, boost::system::error_code& ec)
  {
    return service_impl_.accept(impl, peer, peer_endpoint, ec);
  }

  /// Start an asynchronous accept.
  template <typename SocketService, typename AcceptHandler>
  void async_accept(implementation_type& impl,
      basic_socket<protocol_type, SocketService>& peer,
      endpoint_type* peer_endpoint, AcceptHandler handler)
  {
    service_impl_.async_accept(impl, peer, peer_endpoint, handler);
  }

private:
  // The platform-specific implementation.
  service_impl_type service_impl_;
};

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_SOCKET_ACCEPTOR_SERVICE_HPP
