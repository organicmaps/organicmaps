//
// basic_socket.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_BASIC_SOCKET_HPP
#define BOOST_ASIO_BASIC_SOCKET_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/basic_io_object.hpp>
#include <boost/asio/detail/throw_error.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/socket_base.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

/// Provides socket functionality.
/**
 * The basic_socket class template provides functionality that is common to both
 * stream-oriented and datagram-oriented sockets.
 *
 * @par Thread Safety
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Unsafe.
 */
template <typename Protocol, typename SocketService>
class basic_socket
  : public basic_io_object<SocketService>,
    public socket_base
{
public:
  /// The native representation of a socket.
  typedef typename SocketService::native_type native_type;

  /// The protocol type.
  typedef Protocol protocol_type;

  /// The endpoint type.
  typedef typename Protocol::endpoint endpoint_type;

  /// A basic_socket is always the lowest layer.
  typedef basic_socket<Protocol, SocketService> lowest_layer_type;

  /// Construct a basic_socket without opening it.
  /**
   * This constructor creates a socket without opening it.
   *
   * @param io_service The io_service object that the socket will use to
   * dispatch handlers for any asynchronous operations performed on the socket.
   */
  explicit basic_socket(boost::asio::io_service& io_service)
    : basic_io_object<SocketService>(io_service)
  {
  }

  /// Construct and open a basic_socket.
  /**
   * This constructor creates and opens a socket.
   *
   * @param io_service The io_service object that the socket will use to
   * dispatch handlers for any asynchronous operations performed on the socket.
   *
   * @param protocol An object specifying protocol parameters to be used.
   *
   * @throws boost::system::system_error Thrown on failure.
   */
  basic_socket(boost::asio::io_service& io_service,
      const protocol_type& protocol)
    : basic_io_object<SocketService>(io_service)
  {
    boost::system::error_code ec;
    this->service.open(this->implementation, protocol, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Construct a basic_socket, opening it and binding it to the given local
  /// endpoint.
  /**
   * This constructor creates a socket and automatically opens it bound to the
   * specified endpoint on the local machine. The protocol used is the protocol
   * associated with the given endpoint.
   *
   * @param io_service The io_service object that the socket will use to
   * dispatch handlers for any asynchronous operations performed on the socket.
   *
   * @param endpoint An endpoint on the local machine to which the socket will
   * be bound.
   *
   * @throws boost::system::system_error Thrown on failure.
   */
  basic_socket(boost::asio::io_service& io_service,
      const endpoint_type& endpoint)
    : basic_io_object<SocketService>(io_service)
  {
    boost::system::error_code ec;
    this->service.open(this->implementation, endpoint.protocol(), ec);
    boost::asio::detail::throw_error(ec);
    this->service.bind(this->implementation, endpoint, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Construct a basic_socket on an existing native socket.
  /**
   * This constructor creates a socket object to hold an existing native socket.
   *
   * @param io_service The io_service object that the socket will use to
   * dispatch handlers for any asynchronous operations performed on the socket.
   *
   * @param protocol An object specifying protocol parameters to be used.
   *
   * @param native_socket A native socket.
   *
   * @throws boost::system::system_error Thrown on failure.
   */
  basic_socket(boost::asio::io_service& io_service,
      const protocol_type& protocol, const native_type& native_socket)
    : basic_io_object<SocketService>(io_service)
  {
    boost::system::error_code ec;
    this->service.assign(this->implementation, protocol, native_socket, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Get a reference to the lowest layer.
  /**
   * This function returns a reference to the lowest layer in a stack of
   * layers. Since a basic_socket cannot contain any further layers, it simply
   * returns a reference to itself.
   *
   * @return A reference to the lowest layer in the stack of layers. Ownership
   * is not transferred to the caller.
   */
  lowest_layer_type& lowest_layer()
  {
    return *this;
  }

  /// Get a const reference to the lowest layer.
  /**
   * This function returns a const reference to the lowest layer in a stack of
   * layers. Since a basic_socket cannot contain any further layers, it simply
   * returns a reference to itself.
   *
   * @return A const reference to the lowest layer in the stack of layers.
   * Ownership is not transferred to the caller.
   */
  const lowest_layer_type& lowest_layer() const
  {
    return *this;
  }

  /// Open the socket using the specified protocol.
  /**
   * This function opens the socket so that it will use the specified protocol.
   *
   * @param protocol An object specifying protocol parameters to be used.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @par Example
   * @code
   * boost::asio::ip::tcp::socket socket(io_service);
   * socket.open(boost::asio::ip::tcp::v4());
   * @endcode
   */
  void open(const protocol_type& protocol = protocol_type())
  {
    boost::system::error_code ec;
    this->service.open(this->implementation, protocol, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Open the socket using the specified protocol.
  /**
   * This function opens the socket so that it will use the specified protocol.
   *
   * @param protocol An object specifying which protocol is to be used.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @par Example
   * @code
   * boost::asio::ip::tcp::socket socket(io_service);
   * boost::system::error_code ec;
   * socket.open(boost::asio::ip::tcp::v4(), ec);
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

  /// Assign an existing native socket to the socket.
  /*
   * This function opens the socket to hold an existing native socket.
   *
   * @param protocol An object specifying which protocol is to be used.
   *
   * @param native_socket A native socket.
   *
   * @throws boost::system::system_error Thrown on failure.
   */
  void assign(const protocol_type& protocol, const native_type& native_socket)
  {
    boost::system::error_code ec;
    this->service.assign(this->implementation, protocol, native_socket, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Assign an existing native socket to the socket.
  /*
   * This function opens the socket to hold an existing native socket.
   *
   * @param protocol An object specifying which protocol is to be used.
   *
   * @param native_socket A native socket.
   *
   * @param ec Set to indicate what error occurred, if any.
   */
  boost::system::error_code assign(const protocol_type& protocol,
      const native_type& native_socket, boost::system::error_code& ec)
  {
    return this->service.assign(this->implementation,
        protocol, native_socket, ec);
  }

  /// Determine whether the socket is open.
  bool is_open() const
  {
    return this->service.is_open(this->implementation);
  }

  /// Close the socket.
  /**
   * This function is used to close the socket. Any asynchronous send, receive
   * or connect operations will be cancelled immediately, and will complete
   * with the boost::asio::error::operation_aborted error.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @note For portable behaviour with respect to graceful closure of a
   * connected socket, call shutdown() before closing the socket.
   */
  void close()
  {
    boost::system::error_code ec;
    this->service.close(this->implementation, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Close the socket.
  /**
   * This function is used to close the socket. Any asynchronous send, receive
   * or connect operations will be cancelled immediately, and will complete
   * with the boost::asio::error::operation_aborted error.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @par Example
   * @code
   * boost::asio::ip::tcp::socket socket(io_service);
   * ...
   * boost::system::error_code ec;
   * socket.close(ec);
   * if (ec)
   * {
   *   // An error occurred.
   * }
   * @endcode
   *
   * @note For portable behaviour with respect to graceful closure of a
   * connected socket, call shutdown() before closing the socket.
   */
  boost::system::error_code close(boost::system::error_code& ec)
  {
    return this->service.close(this->implementation, ec);
  }

  /// Get the native socket representation.
  /**
   * This function may be used to obtain the underlying representation of the
   * socket. This is intended to allow access to native socket functionality
   * that is not otherwise provided.
   */
  native_type native()
  {
    return this->service.native(this->implementation);
  }

  /// Cancel all asynchronous operations associated with the socket.
  /**
   * This function causes all outstanding asynchronous connect, send and receive
   * operations to finish immediately, and the handlers for cancelled operations
   * will be passed the boost::asio::error::operation_aborted error.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @note Calls to cancel() will always fail with
   * boost::asio::error::operation_not_supported when run on Windows XP, Windows
   * Server 2003, and earlier versions of Windows, unless
   * BOOST_ASIO_ENABLE_CANCELIO is defined. However, the CancelIo function has
   * two issues that should be considered before enabling its use:
   *
   * @li It will only cancel asynchronous operations that were initiated in the
   * current thread.
   *
   * @li It can appear to complete without error, but the request to cancel the
   * unfinished operations may be silently ignored by the operating system.
   * Whether it works or not seems to depend on the drivers that are installed.
   *
   * For portable cancellation, consider using one of the following
   * alternatives:
   *
   * @li Disable asio's I/O completion port backend by defining
   * BOOST_ASIO_DISABLE_IOCP.
   *
   * @li Use the close() function to simultaneously cancel the outstanding
   * operations and close the socket.
   *
   * When running on Windows Vista, Windows Server 2008, and later, the
   * CancelIoEx function is always used. This function does not have the
   * problems described above.
   */
#if defined(BOOST_MSVC) && (BOOST_MSVC >= 1400) \
  && (!defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0600) \
  && !defined(BOOST_ASIO_ENABLE_CANCELIO)
  __declspec(deprecated("By default, this function always fails with "
        "operation_not_supported when used on Windows XP, Windows Server 2003, "
        "or earlier. Consult documentation for details."))
#endif
  void cancel()
  {
    boost::system::error_code ec;
    this->service.cancel(this->implementation, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Cancel all asynchronous operations associated with the socket.
  /**
   * This function causes all outstanding asynchronous connect, send and receive
   * operations to finish immediately, and the handlers for cancelled operations
   * will be passed the boost::asio::error::operation_aborted error.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @note Calls to cancel() will always fail with
   * boost::asio::error::operation_not_supported when run on Windows XP, Windows
   * Server 2003, and earlier versions of Windows, unless
   * BOOST_ASIO_ENABLE_CANCELIO is defined. However, the CancelIo function has
   * two issues that should be considered before enabling its use:
   *
   * @li It will only cancel asynchronous operations that were initiated in the
   * current thread.
   *
   * @li It can appear to complete without error, but the request to cancel the
   * unfinished operations may be silently ignored by the operating system.
   * Whether it works or not seems to depend on the drivers that are installed.
   *
   * For portable cancellation, consider using one of the following
   * alternatives:
   *
   * @li Disable asio's I/O completion port backend by defining
   * BOOST_ASIO_DISABLE_IOCP.
   *
   * @li Use the close() function to simultaneously cancel the outstanding
   * operations and close the socket.
   *
   * When running on Windows Vista, Windows Server 2008, and later, the
   * CancelIoEx function is always used. This function does not have the
   * problems described above.
   */
#if defined(BOOST_MSVC) && (BOOST_MSVC >= 1400) \
  && (!defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0600) \
  && !defined(BOOST_ASIO_ENABLE_CANCELIO)
  __declspec(deprecated("By default, this function always fails with "
        "operation_not_supported when used on Windows XP, Windows Server 2003, "
        "or earlier. Consult documentation for details."))
#endif
  boost::system::error_code cancel(boost::system::error_code& ec)
  {
    return this->service.cancel(this->implementation, ec);
  }

  /// Determine whether the socket is at the out-of-band data mark.
  /**
   * This function is used to check whether the socket input is currently
   * positioned at the out-of-band data mark.
   *
   * @return A bool indicating whether the socket is at the out-of-band data
   * mark.
   *
   * @throws boost::system::system_error Thrown on failure.
   */
  bool at_mark() const
  {
    boost::system::error_code ec;
    bool b = this->service.at_mark(this->implementation, ec);
    boost::asio::detail::throw_error(ec);
    return b;
  }

  /// Determine whether the socket is at the out-of-band data mark.
  /**
   * This function is used to check whether the socket input is currently
   * positioned at the out-of-band data mark.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @return A bool indicating whether the socket is at the out-of-band data
   * mark.
   */
  bool at_mark(boost::system::error_code& ec) const
  {
    return this->service.at_mark(this->implementation, ec);
  }

  /// Determine the number of bytes available for reading.
  /**
   * This function is used to determine the number of bytes that may be read
   * without blocking.
   *
   * @return The number of bytes that may be read without blocking, or 0 if an
   * error occurs.
   *
   * @throws boost::system::system_error Thrown on failure.
   */
  std::size_t available() const
  {
    boost::system::error_code ec;
    std::size_t s = this->service.available(this->implementation, ec);
    boost::asio::detail::throw_error(ec);
    return s;
  }

  /// Determine the number of bytes available for reading.
  /**
   * This function is used to determine the number of bytes that may be read
   * without blocking.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @return The number of bytes that may be read without blocking, or 0 if an
   * error occurs.
   */
  std::size_t available(boost::system::error_code& ec) const
  {
    return this->service.available(this->implementation, ec);
  }

  /// Bind the socket to the given local endpoint.
  /**
   * This function binds the socket to the specified endpoint on the local
   * machine.
   *
   * @param endpoint An endpoint on the local machine to which the socket will
   * be bound.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @par Example
   * @code
   * boost::asio::ip::tcp::socket socket(io_service);
   * socket.open(boost::asio::ip::tcp::v4());
   * socket.bind(boost::asio::ip::tcp::endpoint(
   *       boost::asio::ip::tcp::v4(), 12345));
   * @endcode
   */
  void bind(const endpoint_type& endpoint)
  {
    boost::system::error_code ec;
    this->service.bind(this->implementation, endpoint, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Bind the socket to the given local endpoint.
  /**
   * This function binds the socket to the specified endpoint on the local
   * machine.
   *
   * @param endpoint An endpoint on the local machine to which the socket will
   * be bound.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @par Example
   * @code
   * boost::asio::ip::tcp::socket socket(io_service);
   * socket.open(boost::asio::ip::tcp::v4());
   * boost::system::error_code ec;
   * socket.bind(boost::asio::ip::tcp::endpoint(
   *       boost::asio::ip::tcp::v4(), 12345), ec);
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

  /// Connect the socket to the specified endpoint.
  /**
   * This function is used to connect a socket to the specified remote endpoint.
   * The function call will block until the connection is successfully made or
   * an error occurs.
   *
   * The socket is automatically opened if it is not already open. If the
   * connect fails, and the socket was automatically opened, the socket is
   * not returned to the closed state.
   *
   * @param peer_endpoint The remote endpoint to which the socket will be
   * connected.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @par Example
   * @code
   * boost::asio::ip::tcp::socket socket(io_service);
   * boost::asio::ip::tcp::endpoint endpoint(
   *     boost::asio::ip::address::from_string("1.2.3.4"), 12345);
   * socket.connect(endpoint);
   * @endcode
   */
  void connect(const endpoint_type& peer_endpoint)
  {
    boost::system::error_code ec;
    if (!is_open())
    {
      this->service.open(this->implementation, peer_endpoint.protocol(), ec);
      boost::asio::detail::throw_error(ec);
    }
    this->service.connect(this->implementation, peer_endpoint, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Connect the socket to the specified endpoint.
  /**
   * This function is used to connect a socket to the specified remote endpoint.
   * The function call will block until the connection is successfully made or
   * an error occurs.
   *
   * The socket is automatically opened if it is not already open. If the
   * connect fails, and the socket was automatically opened, the socket is
   * not returned to the closed state.
   *
   * @param peer_endpoint The remote endpoint to which the socket will be
   * connected.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @par Example
   * @code
   * boost::asio::ip::tcp::socket socket(io_service);
   * boost::asio::ip::tcp::endpoint endpoint(
   *     boost::asio::ip::address::from_string("1.2.3.4"), 12345);
   * boost::system::error_code ec;
   * socket.connect(endpoint, ec);
   * if (ec)
   * {
   *   // An error occurred.
   * }
   * @endcode
   */
  boost::system::error_code connect(const endpoint_type& peer_endpoint,
      boost::system::error_code& ec)
  {
    if (!is_open())
    {
      if (this->service.open(this->implementation,
            peer_endpoint.protocol(), ec))
      {
        return ec;
      }
    }

    return this->service.connect(this->implementation, peer_endpoint, ec);
  }

  /// Start an asynchronous connect.
  /**
   * This function is used to asynchronously connect a socket to the specified
   * remote endpoint. The function call always returns immediately.
   *
   * The socket is automatically opened if it is not already open. If the
   * connect fails, and the socket was automatically opened, the socket is
   * not returned to the closed state.
   *
   * @param peer_endpoint The remote endpoint to which the socket will be
   * connected. Copies will be made of the endpoint object as required.
   *
   * @param handler The handler to be called when the connection operation
   * completes. Copies will be made of the handler as required. The function
   * signature of the handler must be:
   * @code void handler(
   *   const boost::system::error_code& error // Result of operation
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the handler will not be invoked from within this function. Invocation
   * of the handler will be performed in a manner equivalent to using
   * boost::asio::io_service::post().
   *
   * @par Example
   * @code
   * void connect_handler(const boost::system::error_code& error)
   * {
   *   if (!error)
   *   {
   *     // Connect succeeded.
   *   }
   * }
   *
   * ...
   *
   * boost::asio::ip::tcp::socket socket(io_service);
   * boost::asio::ip::tcp::endpoint endpoint(
   *     boost::asio::ip::address::from_string("1.2.3.4"), 12345);
   * socket.async_connect(endpoint, connect_handler);
   * @endcode
   */
  template <typename ConnectHandler>
  void async_connect(const endpoint_type& peer_endpoint, ConnectHandler handler)
  {
    if (!is_open())
    {
      boost::system::error_code ec;
      if (this->service.open(this->implementation,
            peer_endpoint.protocol(), ec))
      {
        this->get_io_service().post(
            boost::asio::detail::bind_handler(handler, ec));
        return;
      }
    }

    this->service.async_connect(this->implementation, peer_endpoint, handler);
  }

  /// Set an option on the socket.
  /**
   * This function is used to set an option on the socket.
   *
   * @param option The new option value to be set on the socket.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @sa SettableSocketOption @n
   * boost::asio::socket_base::broadcast @n
   * boost::asio::socket_base::do_not_route @n
   * boost::asio::socket_base::keep_alive @n
   * boost::asio::socket_base::linger @n
   * boost::asio::socket_base::receive_buffer_size @n
   * boost::asio::socket_base::receive_low_watermark @n
   * boost::asio::socket_base::reuse_address @n
   * boost::asio::socket_base::send_buffer_size @n
   * boost::asio::socket_base::send_low_watermark @n
   * boost::asio::ip::multicast::join_group @n
   * boost::asio::ip::multicast::leave_group @n
   * boost::asio::ip::multicast::enable_loopback @n
   * boost::asio::ip::multicast::outbound_interface @n
   * boost::asio::ip::multicast::hops @n
   * boost::asio::ip::tcp::no_delay
   *
   * @par Example
   * Setting the IPPROTO_TCP/TCP_NODELAY option:
   * @code
   * boost::asio::ip::tcp::socket socket(io_service);
   * ...
   * boost::asio::ip::tcp::no_delay option(true);
   * socket.set_option(option);
   * @endcode
   */
  template <typename SettableSocketOption>
  void set_option(const SettableSocketOption& option)
  {
    boost::system::error_code ec;
    this->service.set_option(this->implementation, option, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Set an option on the socket.
  /**
   * This function is used to set an option on the socket.
   *
   * @param option The new option value to be set on the socket.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @sa SettableSocketOption @n
   * boost::asio::socket_base::broadcast @n
   * boost::asio::socket_base::do_not_route @n
   * boost::asio::socket_base::keep_alive @n
   * boost::asio::socket_base::linger @n
   * boost::asio::socket_base::receive_buffer_size @n
   * boost::asio::socket_base::receive_low_watermark @n
   * boost::asio::socket_base::reuse_address @n
   * boost::asio::socket_base::send_buffer_size @n
   * boost::asio::socket_base::send_low_watermark @n
   * boost::asio::ip::multicast::join_group @n
   * boost::asio::ip::multicast::leave_group @n
   * boost::asio::ip::multicast::enable_loopback @n
   * boost::asio::ip::multicast::outbound_interface @n
   * boost::asio::ip::multicast::hops @n
   * boost::asio::ip::tcp::no_delay
   *
   * @par Example
   * Setting the IPPROTO_TCP/TCP_NODELAY option:
   * @code
   * boost::asio::ip::tcp::socket socket(io_service);
   * ...
   * boost::asio::ip::tcp::no_delay option(true);
   * boost::system::error_code ec;
   * socket.set_option(option, ec);
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

  /// Get an option from the socket.
  /**
   * This function is used to get the current value of an option on the socket.
   *
   * @param option The option value to be obtained from the socket.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @sa GettableSocketOption @n
   * boost::asio::socket_base::broadcast @n
   * boost::asio::socket_base::do_not_route @n
   * boost::asio::socket_base::keep_alive @n
   * boost::asio::socket_base::linger @n
   * boost::asio::socket_base::receive_buffer_size @n
   * boost::asio::socket_base::receive_low_watermark @n
   * boost::asio::socket_base::reuse_address @n
   * boost::asio::socket_base::send_buffer_size @n
   * boost::asio::socket_base::send_low_watermark @n
   * boost::asio::ip::multicast::join_group @n
   * boost::asio::ip::multicast::leave_group @n
   * boost::asio::ip::multicast::enable_loopback @n
   * boost::asio::ip::multicast::outbound_interface @n
   * boost::asio::ip::multicast::hops @n
   * boost::asio::ip::tcp::no_delay
   *
   * @par Example
   * Getting the value of the SOL_SOCKET/SO_KEEPALIVE option:
   * @code
   * boost::asio::ip::tcp::socket socket(io_service);
   * ...
   * boost::asio::ip::tcp::socket::keep_alive option;
   * socket.get_option(option);
   * bool is_set = option.get();
   * @endcode
   */
  template <typename GettableSocketOption>
  void get_option(GettableSocketOption& option) const
  {
    boost::system::error_code ec;
    this->service.get_option(this->implementation, option, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Get an option from the socket.
  /**
   * This function is used to get the current value of an option on the socket.
   *
   * @param option The option value to be obtained from the socket.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @sa GettableSocketOption @n
   * boost::asio::socket_base::broadcast @n
   * boost::asio::socket_base::do_not_route @n
   * boost::asio::socket_base::keep_alive @n
   * boost::asio::socket_base::linger @n
   * boost::asio::socket_base::receive_buffer_size @n
   * boost::asio::socket_base::receive_low_watermark @n
   * boost::asio::socket_base::reuse_address @n
   * boost::asio::socket_base::send_buffer_size @n
   * boost::asio::socket_base::send_low_watermark @n
   * boost::asio::ip::multicast::join_group @n
   * boost::asio::ip::multicast::leave_group @n
   * boost::asio::ip::multicast::enable_loopback @n
   * boost::asio::ip::multicast::outbound_interface @n
   * boost::asio::ip::multicast::hops @n
   * boost::asio::ip::tcp::no_delay
   *
   * @par Example
   * Getting the value of the SOL_SOCKET/SO_KEEPALIVE option:
   * @code
   * boost::asio::ip::tcp::socket socket(io_service);
   * ...
   * boost::asio::ip::tcp::socket::keep_alive option;
   * boost::system::error_code ec;
   * socket.get_option(option, ec);
   * if (ec)
   * {
   *   // An error occurred.
   * }
   * bool is_set = option.get();
   * @endcode
   */
  template <typename GettableSocketOption>
  boost::system::error_code get_option(GettableSocketOption& option,
      boost::system::error_code& ec) const
  {
    return this->service.get_option(this->implementation, option, ec);
  }

  /// Perform an IO control command on the socket.
  /**
   * This function is used to execute an IO control command on the socket.
   *
   * @param command The IO control command to be performed on the socket.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @sa IoControlCommand @n
   * boost::asio::socket_base::bytes_readable @n
   * boost::asio::socket_base::non_blocking_io
   *
   * @par Example
   * Getting the number of bytes ready to read:
   * @code
   * boost::asio::ip::tcp::socket socket(io_service);
   * ...
   * boost::asio::ip::tcp::socket::bytes_readable command;
   * socket.io_control(command);
   * std::size_t bytes_readable = command.get();
   * @endcode
   */
  template <typename IoControlCommand>
  void io_control(IoControlCommand& command)
  {
    boost::system::error_code ec;
    this->service.io_control(this->implementation, command, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Perform an IO control command on the socket.
  /**
   * This function is used to execute an IO control command on the socket.
   *
   * @param command The IO control command to be performed on the socket.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @sa IoControlCommand @n
   * boost::asio::socket_base::bytes_readable @n
   * boost::asio::socket_base::non_blocking_io
   *
   * @par Example
   * Getting the number of bytes ready to read:
   * @code
   * boost::asio::ip::tcp::socket socket(io_service);
   * ...
   * boost::asio::ip::tcp::socket::bytes_readable command;
   * boost::system::error_code ec;
   * socket.io_control(command, ec);
   * if (ec)
   * {
   *   // An error occurred.
   * }
   * std::size_t bytes_readable = command.get();
   * @endcode
   */
  template <typename IoControlCommand>
  boost::system::error_code io_control(IoControlCommand& command,
      boost::system::error_code& ec)
  {
    return this->service.io_control(this->implementation, command, ec);
  }

  /// Get the local endpoint of the socket.
  /**
   * This function is used to obtain the locally bound endpoint of the socket.
   *
   * @returns An object that represents the local endpoint of the socket.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @par Example
   * @code
   * boost::asio::ip::tcp::socket socket(io_service);
   * ...
   * boost::asio::ip::tcp::endpoint endpoint = socket.local_endpoint();
   * @endcode
   */
  endpoint_type local_endpoint() const
  {
    boost::system::error_code ec;
    endpoint_type ep = this->service.local_endpoint(this->implementation, ec);
    boost::asio::detail::throw_error(ec);
    return ep;
  }

  /// Get the local endpoint of the socket.
  /**
   * This function is used to obtain the locally bound endpoint of the socket.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @returns An object that represents the local endpoint of the socket.
   * Returns a default-constructed endpoint object if an error occurred.
   *
   * @par Example
   * @code
   * boost::asio::ip::tcp::socket socket(io_service);
   * ...
   * boost::system::error_code ec;
   * boost::asio::ip::tcp::endpoint endpoint = socket.local_endpoint(ec);
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

  /// Get the remote endpoint of the socket.
  /**
   * This function is used to obtain the remote endpoint of the socket.
   *
   * @returns An object that represents the remote endpoint of the socket.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @par Example
   * @code
   * boost::asio::ip::tcp::socket socket(io_service);
   * ...
   * boost::asio::ip::tcp::endpoint endpoint = socket.remote_endpoint();
   * @endcode
   */
  endpoint_type remote_endpoint() const
  {
    boost::system::error_code ec;
    endpoint_type ep = this->service.remote_endpoint(this->implementation, ec);
    boost::asio::detail::throw_error(ec);
    return ep;
  }

  /// Get the remote endpoint of the socket.
  /**
   * This function is used to obtain the remote endpoint of the socket.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @returns An object that represents the remote endpoint of the socket.
   * Returns a default-constructed endpoint object if an error occurred.
   *
   * @par Example
   * @code
   * boost::asio::ip::tcp::socket socket(io_service);
   * ...
   * boost::system::error_code ec;
   * boost::asio::ip::tcp::endpoint endpoint = socket.remote_endpoint(ec);
   * if (ec)
   * {
   *   // An error occurred.
   * }
   * @endcode
   */
  endpoint_type remote_endpoint(boost::system::error_code& ec) const
  {
    return this->service.remote_endpoint(this->implementation, ec);
  }

  /// Disable sends or receives on the socket.
  /**
   * This function is used to disable send operations, receive operations, or
   * both.
   *
   * @param what Determines what types of operation will no longer be allowed.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @par Example
   * Shutting down the send side of the socket:
   * @code
   * boost::asio::ip::tcp::socket socket(io_service);
   * ...
   * socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send);
   * @endcode
   */
  void shutdown(shutdown_type what)
  {
    boost::system::error_code ec;
    this->service.shutdown(this->implementation, what, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Disable sends or receives on the socket.
  /**
   * This function is used to disable send operations, receive operations, or
   * both.
   *
   * @param what Determines what types of operation will no longer be allowed.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @par Example
   * Shutting down the send side of the socket:
   * @code
   * boost::asio::ip::tcp::socket socket(io_service);
   * ...
   * boost::system::error_code ec;
   * socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
   * if (ec)
   * {
   *   // An error occurred.
   * }
   * @endcode
   */
  boost::system::error_code shutdown(shutdown_type what,
      boost::system::error_code& ec)
  {
    return this->service.shutdown(this->implementation, what, ec);
  }

protected:
  /// Protected destructor to prevent deletion through this type.
  ~basic_socket()
  {
  }
};

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_BASIC_SOCKET_HPP
