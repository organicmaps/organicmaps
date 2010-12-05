//
// basic_socket_acceptor.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_BASIC_SOCKET_ACCEPTOR_HPP
#define BOOST_ASIO_BASIC_SOCKET_ACCEPTOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/basic_io_object.hpp>
#include <boost/asio/basic_socket.hpp>
#include <boost/asio/detail/throw_error.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/socket_acceptor_service.hpp>
#include <boost/asio/socket_base.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

/// Provides the ability to accept new connections.
/**
 * The basic_socket_acceptor class template is used for accepting new socket
 * connections.
 *
 * @par Thread Safety
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Unsafe.
 *
 * @par Example
 * Opening a socket acceptor with the SO_REUSEADDR option enabled:
 * @code
 * boost::asio::ip::tcp::acceptor acceptor(io_service);
 * boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
 * acceptor.open(endpoint.protocol());
 * acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
 * acceptor.bind(endpoint);
 * acceptor.listen();
 * @endcode
 */
template <typename Protocol,
    typename SocketAcceptorService = socket_acceptor_service<Protocol> >
class basic_socket_acceptor
  : public basic_io_object<SocketAcceptorService>,
    public socket_base
{
public:
  /// The native representation of an acceptor.
  typedef typename SocketAcceptorService::native_type native_type;

  /// The protocol type.
  typedef Protocol protocol_type;

  /// The endpoint type.
  typedef typename Protocol::endpoint endpoint_type;

  /// Construct an acceptor without opening it.
  /**
   * This constructor creates an acceptor without opening it to listen for new
   * connections. The open() function must be called before the acceptor can
   * accept new socket connections.
   *
   * @param io_service The io_service object that the acceptor will use to
   * dispatch handlers for any asynchronous operations performed on the
   * acceptor.
   */
  explicit basic_socket_acceptor(boost::asio::io_service& io_service)
    : basic_io_object<SocketAcceptorService>(io_service)
  {
  }

  /// Construct an open acceptor.
  /**
   * This constructor creates an acceptor and automatically opens it.
   *
   * @param io_service The io_service object that the acceptor will use to
   * dispatch handlers for any asynchronous operations performed on the
   * acceptor.
   *
   * @param protocol An object specifying protocol parameters to be used.
   *
   * @throws boost::system::system_error Thrown on failure.
   */
  basic_socket_acceptor(boost::asio::io_service& io_service,
      const protocol_type& protocol)
    : basic_io_object<SocketAcceptorService>(io_service)
  {
    boost::system::error_code ec;
    this->service.open(this->implementation, protocol, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Construct an acceptor opened on the given endpoint.
  /**
   * This constructor creates an acceptor and automatically opens it to listen
   * for new connections on the specified endpoint.
   *
   * @param io_service The io_service object that the acceptor will use to
   * dispatch handlers for any asynchronous operations performed on the
   * acceptor.
   *
   * @param endpoint An endpoint on the local machine on which the acceptor
   * will listen for new connections.
   *
   * @param reuse_addr Whether the constructor should set the socket option
   * socket_base::reuse_address.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @note This constructor is equivalent to the following code:
   * @code
   * basic_socket_acceptor<Protocol> acceptor(io_service);
   * acceptor.open(endpoint.protocol());
   * if (reuse_addr)
   *   acceptor.set_option(socket_base::reuse_address(true));
   * acceptor.bind(endpoint);
   * acceptor.listen(listen_backlog);
   * @endcode
   */
  basic_socket_acceptor(boost::asio::io_service& io_service,
      const endpoint_type& endpoint, bool reuse_addr = true)
    : basic_io_object<SocketAcceptorService>(io_service)
  {
    boost::system::error_code ec;
    this->service.open(this->implementation, endpoint.protocol(), ec);
    boost::asio::detail::throw_error(ec);
    if (reuse_addr)
    {
      this->service.set_option(this->implementation,
          socket_base::reuse_address(true), ec);
      boost::asio::detail::throw_error(ec);
    }
    this->service.bind(this->implementation, endpoint, ec);
    boost::asio::detail::throw_error(ec);
    this->service.listen(this->implementation,
        socket_base::max_connections, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Construct a basic_socket_acceptor on an existing native acceptor.
  /**
   * This constructor creates an acceptor object to hold an existing native
   * acceptor.
   *
   * @param io_service The io_service object that the acceptor will use to
   * dispatch handlers for any asynchronous operations performed on the
   * acceptor.
   *
   * @param protocol An object specifying protocol parameters to be used.
   *
   * @param native_acceptor A native acceptor.
   *
   * @throws boost::system::system_error Thrown on failure.
   */
  basic_socket_acceptor(boost::asio::io_service& io_service,
      const protocol_type& protocol, const native_type& native_acceptor)
    : basic_io_object<SocketAcceptorService>(io_service)
  {
    boost::system::error_code ec;
    this->service.assign(this->implementation, protocol, native_acceptor, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Open the acceptor using the specified protocol.
  /**
   * This function opens the socket acceptor so that it will use the specified
   * protocol.
   *
   * @param protocol An object specifying which protocol is to be used.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @par Example
   * @code
   * boost::asio::ip::tcp::acceptor acceptor(io_service);
   * acceptor.open(boost::asio::ip::tcp::v4());
   * @endcode
   */
  void open(const protocol_type& protocol = protocol_type())
  {
    boost::system::error_code ec;
    this->service.open(this->implementation, protocol, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Open the acceptor using the specified protocol.
  /**
   * This function opens the socket acceptor so that it will use the specified
   * protocol.
   *
   * @param protocol An object specifying which protocol is to be used.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @par Example
   * @code
   * boost::asio::ip::tcp::acceptor acceptor(io_service);
   * boost::system::error_code ec;
   * acceptor.open(boost::asio::ip::tcp::v4(), ec);
   * if (ec)
   * {
   *   // An error occurred.
   * }
   * @endcode
   */
  boost::system::error_code open(const protocol_type& protocol,
      boost::system::error_code& ec)
  {
    return this->service.open(this->implementation, protocol, ec);
  }

  /// Assigns an existing native acceptor to the acceptor.
  /*
   * This function opens the acceptor to hold an existing native acceptor.
   *
   * @param protocol An object specifying which protocol is to be used.
   *
   * @param native_acceptor A native acceptor.
   *
   * @throws boost::system::system_error Thrown on failure.
   */
  void assign(const protocol_type& protocol, const native_type& native_acceptor)
  {
    boost::system::error_code ec;
    this->service.assign(this->implementation, protocol, native_acceptor, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Assigns an existing native acceptor to the acceptor.
  /*
   * This function opens the acceptor to hold an existing native acceptor.
   *
   * @param protocol An object specifying which protocol is to be used.
   *
   * @param native_acceptor A native acceptor.
   *
   * @param ec Set to indicate what error occurred, if any.
   */
  boost::system::error_code assign(const protocol_type& protocol,
      const native_type& native_acceptor, boost::system::error_code& ec)
  {
    return this->service.assign(this->implementation,
        protocol, native_acceptor, ec);
  }

  /// Determine whether the acceptor is open.
  bool is_open() const
  {
    return this->service.is_open(this->implementation);
  }

  /// Bind the acceptor to the given local endpoint.
  /**
   * This function binds the socket acceptor to the specified endpoint on the
   * local machine.
   *
   * @param endpoint An endpoint on the local machine to which the socket
   * acceptor will be bound.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @par Example
   * @code
   * boost::asio::ip::tcp::acceptor acceptor(io_service);
   * acceptor.open(boost::asio::ip::tcp::v4());
   * acceptor.bind(boost::asio::ip::tcp::endpoint(12345));
   * @endcode
   */
  void bind(const endpoint_type& endpoint)
  {
    boost::system::error_code ec;
    this->service.bind(this->implementation, endpoint, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Bind the acceptor to the given local endpoint.
  /**
   * This function binds the socket acceptor to the specified endpoint on the
   * local machine.
   *
   * @param endpoint An endpoint on the local machine to which the socket
   * acceptor will be bound.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @par Example
   * @code
   * boost::asio::ip::tcp::acceptor acceptor(io_service);
   * acceptor.open(boost::asio::ip::tcp::v4());
   * boost::system::error_code ec;
   * acceptor.bind(boost::asio::ip::tcp::endpoint(12345), ec);
   * if (ec)
   * {
   *   // An error occurred.
   * }
   * @endcode
   */
  boost::system::error_code bind(const endpoint_type& endpoint,
      boost::system::error_code& ec)
  {
    return this->service.bind(this->implementation, endpoint, ec);
  }

  /// Place the acceptor into the state where it will listen for new
  /// connections.
  /**
   * This function puts the socket acceptor into the state where it may accept
   * new connections.
   *
   * @param backlog The maximum length of the queue of pending connections.
   *
   * @throws boost::system::system_error Thrown on failure.
   */
  void listen(int backlog = socket_base::max_connections)
  {
    boost::system::error_code ec;
    this->service.listen(this->implementation, backlog, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Place the acceptor into the state where it will listen for new
  /// connections.
  /**
   * This function puts the socket acceptor into the state where it may accept
   * new connections.
   *
   * @param backlog The maximum length of the queue of pending connections.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @par Example
   * @code
   * boost::asio::ip::tcp::acceptor acceptor(io_service);
   * ...
   * boost::system::error_code ec;
   * acceptor.listen(boost::asio::socket_base::max_connections, ec);
   * if (ec)
   * {
   *   // An error occurred.
   * }
   * @endcode
   */
  boost::system::error_code listen(int backlog, boost::system::error_code& ec)
  {
    return this->service.listen(this->implementation, backlog, ec);
  }

  /// Close the acceptor.
  /**
   * This function is used to close the acceptor. Any asynchronous accept
   * operations will be cancelled immediately.
   *
   * A subsequent call to open() is required before the acceptor can again be
   * used to again perform socket accept operations.
   *
   * @throws boost::system::system_error Thrown on failure.
   */
  void close()
  {
    boost::system::error_code ec;
    this->service.close(this->implementation, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Close the acceptor.
  /**
   * This function is used to close the acceptor. Any asynchronous accept
   * operations will be cancelled immediately.
   *
   * A subsequent call to open() is required before the acceptor can again be
   * used to again perform socket accept operations.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @par Example
   * @code
   * boost::asio::ip::tcp::acceptor acceptor(io_service);
   * ...
   * boost::system::error_code ec;
   * acceptor.close(ec);
   * if (ec)
   * {
   *   // An error occurred.
   * }
   * @endcode
   */
  boost::system::error_code close(boost::system::error_code& ec)
  {
    return this->service.close(this->implementation, ec);
  }

  /// Get the native acceptor representation.
  /**
   * This function may be used to obtain the underlying representation of the
   * acceptor. This is intended to allow access to native acceptor functionality
   * that is not otherwise provided.
   */
  native_type native()
  {
    return this->service.native(this->implementation);
  }

  /// Cancel all asynchronous operations associated with the acceptor.
  /**
   * This function causes all outstanding asynchronous connect, send and receive
   * operations to finish immediately, and the handlers for cancelled operations
   * will be passed the boost::asio::error::operation_aborted error.
   *
   * @throws boost::system::system_error Thrown on failure.
   */
  void cancel()
  {
    boost::system::error_code ec;
    this->service.cancel(this->implementation, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Cancel all asynchronous operations associated with the acceptor.
  /**
   * This function causes all outstanding asynchronous connect, send and receive
   * operations to finish immediately, and the handlers for cancelled operations
   * will be passed the boost::asio::error::operation_aborted error.
   *
   * @param ec Set to indicate what error occurred, if any.
   */
  boost::system::error_code cancel(boost::system::error_code& ec)
  {
    return this->service.cancel(this->implementation, ec);
  }

  /// Set an option on the acceptor.
  /**
   * This function is used to set an option on the acceptor.
   *
   * @param option The new option value to be set on the acceptor.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @sa SettableSocketOption @n
   * boost::asio::socket_base::reuse_address
   * boost::asio::socket_base::enable_connection_aborted
   *
   * @par Example
   * Setting the SOL_SOCKET/SO_REUSEADDR option:
   * @code
   * boost::asio::ip::tcp::acceptor acceptor(io_service);
   * ...
   * boost::asio::ip::tcp::acceptor::reuse_address option(true);
   * acceptor.set_option(option);
   * @endcode
   */
  template <typename SettableSocketOption>
  void set_option(const SettableSocketOption& option)
  {
    boost::system::error_code ec;
    this->service.set_option(this->implementation, option, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Set an option on the acceptor.
  /**
   * This function is used to set an option on the acceptor.
   *
   * @param option The new option value to be set on the acceptor.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @sa SettableSocketOption @n
   * boost::asio::socket_base::reuse_address
   * boost::asio::socket_base::enable_connection_aborted
   *
   * @par Example
   * Setting the SOL_SOCKET/SO_REUSEADDR option:
   * @code
   * boost::asio::ip::tcp::acceptor acceptor(io_service);
   * ...
   * boost::asio::ip::tcp::acceptor::reuse_address option(true);
   * boost::system::error_code ec;
   * acceptor.set_option(option, ec);
   * if (ec)
   * {
   *   // An error occurred.
   * }
   * @endcode
   */
  template <typename SettableSocketOption>
  boost::system::error_code set_option(const SettableSocketOption& option,
      boost::system::error_code& ec)
  {
    return this->service.set_option(this->implementation, option, ec);
  }

  /// Get an option from the acceptor.
  /**
   * This function is used to get the current value of an option on the
   * acceptor.
   *
   * @param option The option value to be obtained from the acceptor.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @sa GettableSocketOption @n
   * boost::asio::socket_base::reuse_address
   *
   * @par Example
   * Getting the value of the SOL_SOCKET/SO_REUSEADDR option:
   * @code
   * boost::asio::ip::tcp::acceptor acceptor(io_service);
   * ...
   * boost::asio::ip::tcp::acceptor::reuse_address option;
   * acceptor.get_option(option);
   * bool is_set = option.get();
   * @endcode
   */
  template <typename GettableSocketOption>
  void get_option(GettableSocketOption& option)
  {
    boost::system::error_code ec;
    this->service.get_option(this->implementation, option, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Get an option from the acceptor.
  /**
   * This function is used to get the current value of an option on the
   * acceptor.
   *
   * @param option The option value to be obtained from the acceptor.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @sa GettableSocketOption @n
   * boost::asio::socket_base::reuse_address
   *
   * @par Example
   * Getting the value of the SOL_SOCKET/SO_REUSEADDR option:
   * @code
   * boost::asio::ip::tcp::acceptor acceptor(io_service);
   * ...
   * boost::asio::ip::tcp::acceptor::reuse_address option;
   * boost::system::error_code ec;
   * acceptor.get_option(option, ec);
   * if (ec)
   * {
   *   // An error occurred.
   * }
   * bool is_set = option.get();
   * @endcode
   */
  template <typename GettableSocketOption>
  boost::system::error_code get_option(GettableSocketOption& option,
      boost::system::error_code& ec)
  {
    return this->service.get_option(this->implementation, option, ec);
  }

  /// Get the local endpoint of the acceptor.
  /**
   * This function is used to obtain the locally bound endpoint of the acceptor.
   *
   * @returns An object that represents the local endpoint of the acceptor.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @par Example
   * @code
   * boost::asio::ip::tcp::acceptor acceptor(io_service);
   * ...
   * boost::asio::ip::tcp::endpoint endpoint = acceptor.local_endpoint();
   * @endcode
   */
  endpoint_type local_endpoint() const
  {
    boost::system::error_code ec;
    endpoint_type ep = this->service.local_endpoint(this->implementation, ec);
    boost::asio::detail::throw_error(ec);
    return ep;
  }

  /// Get the local endpoint of the acceptor.
  /**
   * This function is used to obtain the locally bound endpoint of the acceptor.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @returns An object that represents the local endpoint of the acceptor.
   * Returns a default-constructed endpoint object if an error occurred and the
   * error handler did not throw an exception.
   *
   * @par Example
   * @code
   * boost::asio::ip::tcp::acceptor acceptor(io_service);
   * ...
   * boost::system::error_code ec;
   * boost::asio::ip::tcp::endpoint endpoint = acceptor.local_endpoint(ec);
   * if (ec)
   * {
   *   // An error occurred.
   * }
   * @endcode
   */
  endpoint_type local_endpoint(boost::system::error_code& ec) const
  {
    return this->service.local_endpoint(this->implementation, ec);
  }

  /// Accept a new connection.
  /**
   * This function is used to accept a new connection from a peer into the
   * given socket. The function call will block until a new connection has been
   * accepted successfully or an error occurs.
   *
   * @param peer The socket into which the new connection will be accepted.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @par Example
   * @code
   * boost::asio::ip::tcp::acceptor acceptor(io_service);
   * ...
   * boost::asio::ip::tcp::socket socket(io_service);
   * acceptor.accept(socket);
   * @endcode
   */
  template <typename SocketService>
  void accept(basic_socket<protocol_type, SocketService>& peer)
  {
    boost::system::error_code ec;
    this->service.accept(this->implementation, peer, 0, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Accept a new connection.
  /**
   * This function is used to accept a new connection from a peer into the
   * given socket. The function call will block until a new connection has been
   * accepted successfully or an error occurs.
   *
   * @param peer The socket into which the new connection will be accepted.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @par Example
   * @code
   * boost::asio::ip::tcp::acceptor acceptor(io_service);
   * ...
   * boost::asio::ip::tcp::soocket socket(io_service);
   * boost::system::error_code ec;
   * acceptor.accept(socket, ec);
   * if (ec)
   * {
   *   // An error occurred.
   * }
   * @endcode
   */
  template <typename SocketService>
  boost::system::error_code accept(
      basic_socket<protocol_type, SocketService>& peer,
      boost::system::error_code& ec)
  {
    return this->service.accept(this->implementation, peer, 0, ec);
  }

  /// Start an asynchronous accept.
  /**
   * This function is used to asynchronously accept a new connection into a
   * socket. The function call always returns immediately.
   *
   * @param peer The socket into which the new connection will be accepted.
   * Ownership of the peer object is retained by the caller, which must
   * guarantee that it is valid until the handler is called.
   *
   * @param handler The handler to be called when the accept operation
   * completes. Copies will be made of the handler as required. The function
   * signature of the handler must be:
   * @code void handler(
   *   const boost::system::error_code& error // Result of operation.
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the handler will not be invoked from within this function. Invocation
   * of the handler will be performed in a manner equivalent to using
   * boost::asio::io_service::post().
   *
   * @par Example
   * @code
   * void accept_handler(const boost::system::error_code& error)
   * {
   *   if (!error)
   *   {
   *     // Accept succeeded.
   *   }
   * }
   *
   * ...
   *
   * boost::asio::ip::tcp::acceptor acceptor(io_service);
   * ...
   * boost::asio::ip::tcp::socket socket(io_service);
   * acceptor.async_accept(socket, accept_handler);
   * @endcode
   */
  template <typename SocketService, typename AcceptHandler>
  void async_accept(basic_socket<protocol_type, SocketService>& peer,
      AcceptHandler handler)
  {
    this->service.async_accept(this->implementation, peer, 0, handler);
  }

  /// Accept a new connection and obtain the endpoint of the peer
  /**
   * This function is used to accept a new connection from a peer into the
   * given socket, and additionally provide the endpoint of the remote peer.
   * The function call will block until a new connection has been accepted
   * successfully or an error occurs.
   *
   * @param peer The socket into which the new connection will be accepted.
   *
   * @param peer_endpoint An endpoint object which will receive the endpoint of
   * the remote peer.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @par Example
   * @code
   * boost::asio::ip::tcp::acceptor acceptor(io_service);
   * ...
   * boost::asio::ip::tcp::socket socket(io_service);
   * boost::asio::ip::tcp::endpoint endpoint;
   * acceptor.accept(socket, endpoint);
   * @endcode
   */
  template <typename SocketService>
  void accept(basic_socket<protocol_type, SocketService>& peer,
      endpoint_type& peer_endpoint)
  {
    boost::system::error_code ec;
    this->service.accept(this->implementation, peer, &peer_endpoint, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Accept a new connection and obtain the endpoint of the peer
  /**
   * This function is used to accept a new connection from a peer into the
   * given socket, and additionally provide the endpoint of the remote peer.
   * The function call will block until a new connection has been accepted
   * successfully or an error occurs.
   *
   * @param peer The socket into which the new connection will be accepted.
   *
   * @param peer_endpoint An endpoint object which will receive the endpoint of
   * the remote peer.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @par Example
   * @code
   * boost::asio::ip::tcp::acceptor acceptor(io_service);
   * ...
   * boost::asio::ip::tcp::socket socket(io_service);
   * boost::asio::ip::tcp::endpoint endpoint;
   * boost::system::error_code ec;
   * acceptor.accept(socket, endpoint, ec);
   * if (ec)
   * {
   *   // An error occurred.
   * }
   * @endcode
   */
  template <typename SocketService>
  boost::system::error_code accept(
      basic_socket<protocol_type, SocketService>& peer,
      endpoint_type& peer_endpoint, boost::system::error_code& ec)
  {
    return this->service.accept(this->implementation, peer, &peer_endpoint, ec);
  }

  /// Start an asynchronous accept.
  /**
   * This function is used to asynchronously accept a new connection into a
   * socket, and additionally obtain the endpoint of the remote peer. The
   * function call always returns immediately.
   *
   * @param peer The socket into which the new connection will be accepted.
   * Ownership of the peer object is retained by the caller, which must
   * guarantee that it is valid until the handler is called.
   *
   * @param peer_endpoint An endpoint object into which the endpoint of the
   * remote peer will be written. Ownership of the peer_endpoint object is
   * retained by the caller, which must guarantee that it is valid until the
   * handler is called.
   *
   * @param handler The handler to be called when the accept operation
   * completes. Copies will be made of the handler as required. The function
   * signature of the handler must be:
   * @code void handler(
   *   const boost::system::error_code& error // Result of operation.
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the handler will not be invoked from within this function. Invocation
   * of the handler will be performed in a manner equivalent to using
   * boost::asio::io_service::post().
   */
  template <typename SocketService, typename AcceptHandler>
  void async_accept(basic_socket<protocol_type, SocketService>& peer,
      endpoint_type& peer_endpoint, AcceptHandler handler)
  {
    this->service.async_accept(this->implementation,
        peer, &peer_endpoint, handler);
  }
};

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_BASIC_SOCKET_ACCEPTOR_HPP
