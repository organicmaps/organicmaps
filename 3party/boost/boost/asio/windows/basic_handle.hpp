//
// windows/basic_handle.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_WINDOWS_BASIC_HANDLE_HPP
#define BOOST_ASIO_WINDOWS_BASIC_HANDLE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_WINDOWS_RANDOM_ACCESS_HANDLE) \
  || defined(BOOST_ASIO_HAS_WINDOWS_STREAM_HANDLE) \
  || defined(GENERATING_DOCUMENTATION)

#include <boost/asio/basic_io_object.hpp>
#include <boost/asio/detail/throw_error.hpp>
#include <boost/asio/error.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace windows {

/// Provides Windows handle functionality.
/**
 * The windows::basic_handle class template provides the ability to wrap a
 * Windows handle.
 *
 * @par Thread Safety
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Unsafe.
 */
template <typename HandleService>
class basic_handle
  : public basic_io_object<HandleService>
{
public:
  /// The native representation of a handle.
  typedef typename HandleService::native_type native_type;

  /// A basic_handle is always the lowest layer.
  typedef basic_handle<HandleService> lowest_layer_type;

  /// Construct a basic_handle without opening it.
  /**
   * This constructor creates a handle without opening it.
   *
   * @param io_service The io_service object that the handle will use to
   * dispatch handlers for any asynchronous operations performed on the handle.
   */
  explicit basic_handle(boost::asio::io_service& io_service)
    : basic_io_object<HandleService>(io_service)
  {
  }

  /// Construct a basic_handle on an existing native handle.
  /**
   * This constructor creates a handle object to hold an existing native handle.
   *
   * @param io_service The io_service object that the handle will use to
   * dispatch handlers for any asynchronous operations performed on the handle.
   *
   * @param native_handle A native handle.
   *
   * @throws boost::system::system_error Thrown on failure.
   */
  basic_handle(boost::asio::io_service& io_service,
      const native_type& native_handle)
    : basic_io_object<HandleService>(io_service)
  {
    boost::system::error_code ec;
    this->service.assign(this->implementation, native_handle, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Get a reference to the lowest layer.
  /**
   * This function returns a reference to the lowest layer in a stack of
   * layers. Since a basic_handle cannot contain any further layers, it simply
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
   * layers. Since a basic_handle cannot contain any further layers, it simply
   * returns a reference to itself.
   *
   * @return A const reference to the lowest layer in the stack of layers.
   * Ownership is not transferred to the caller.
   */
  const lowest_layer_type& lowest_layer() const
  {
    return *this;
  }

  /// Assign an existing native handle to the handle.
  /*
   * This function opens the handle to hold an existing native handle.
   *
   * @param native_handle A native handle.
   *
   * @throws boost::system::system_error Thrown on failure.
   */
  void assign(const native_type& native_handle)
  {
    boost::system::error_code ec;
    this->service.assign(this->implementation, native_handle, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Assign an existing native handle to the handle.
  /*
   * This function opens the handle to hold an existing native handle.
   *
   * @param native_handle A native handle.
   *
   * @param ec Set to indicate what error occurred, if any.
   */
  boost::system::error_code assign(const native_type& native_handle,
      boost::system::error_code& ec)
  {
    return this->service.assign(this->implementation, native_handle, ec);
  }

  /// Determine whether the handle is open.
  bool is_open() const
  {
    return this->service.is_open(this->implementation);
  }

  /// Close the handle.
  /**
   * This function is used to close the handle. Any asynchronous read or write
   * operations will be cancelled immediately, and will complete with the
   * boost::asio::error::operation_aborted error.
   *
   * @throws boost::system::system_error Thrown on failure.
   */
  void close()
  {
    boost::system::error_code ec;
    this->service.close(this->implementation, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Close the handle.
  /**
   * This function is used to close the handle. Any asynchronous read or write
   * operations will be cancelled immediately, and will complete with the
   * boost::asio::error::operation_aborted error.
   *
   * @param ec Set to indicate what error occurred, if any.
   */
  boost::system::error_code close(boost::system::error_code& ec)
  {
    return this->service.close(this->implementation, ec);
  }

  /// Get the native handle representation.
  /**
   * This function may be used to obtain the underlying representation of the
   * handle. This is intended to allow access to native handle functionality
   * that is not otherwise provided.
   */
  native_type native()
  {
    return this->service.native(this->implementation);
  }

  /// Cancel all asynchronous operations associated with the handle.
  /**
   * This function causes all outstanding asynchronous read or write operations
   * to finish immediately, and the handlers for cancelled operations will be
   * passed the boost::asio::error::operation_aborted error.
   *
   * @throws boost::system::system_error Thrown on failure.
   */
  void cancel()
  {
    boost::system::error_code ec;
    this->service.cancel(this->implementation, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Cancel all asynchronous operations associated with the handle.
  /**
   * This function causes all outstanding asynchronous read or write operations
   * to finish immediately, and the handlers for cancelled operations will be
   * passed the boost::asio::error::operation_aborted error.
   *
   * @param ec Set to indicate what error occurred, if any.
   */
  boost::system::error_code cancel(boost::system::error_code& ec)
  {
    return this->service.cancel(this->implementation, ec);
  }

protected:
  /// Protected destructor to prevent deletion through this type.
  ~basic_handle()
  {
  }
};

} // namespace windows
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(BOOST_ASIO_HAS_WINDOWS_RANDOM_ACCESS_HANDLE)
       //   || defined(BOOST_ASIO_HAS_WINDOWS_STREAM_HANDLE)
       //   || defined(GENERATING_DOCUMENTATION)

#endif // BOOST_ASIO_WINDOWS_BASIC_HANDLE_HPP
