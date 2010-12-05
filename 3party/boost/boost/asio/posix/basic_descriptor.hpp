//
// posix/basic_descriptor.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_POSIX_BASIC_DESCRIPTOR_HPP
#define BOOST_ASIO_POSIX_BASIC_DESCRIPTOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR) \
  || defined(GENERATING_DOCUMENTATION)

#include <boost/asio/basic_io_object.hpp>
#include <boost/asio/detail/throw_error.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/posix/descriptor_base.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace posix {

/// Provides POSIX descriptor functionality.
/**
 * The posix::basic_descriptor class template provides the ability to wrap a
 * POSIX descriptor.
 *
 * @par Thread Safety
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Unsafe.
 */
template <typename DescriptorService>
class basic_descriptor
  : public basic_io_object<DescriptorService>,
    public descriptor_base
{
public:
  /// The native representation of a descriptor.
  typedef typename DescriptorService::native_type native_type;

  /// A basic_descriptor is always the lowest layer.
  typedef basic_descriptor<DescriptorService> lowest_layer_type;

  /// Construct a basic_descriptor without opening it.
  /**
   * This constructor creates a descriptor without opening it.
   *
   * @param io_service The io_service object that the descriptor will use to
   * dispatch handlers for any asynchronous operations performed on the
   * descriptor.
   */
  explicit basic_descriptor(boost::asio::io_service& io_service)
    : basic_io_object<DescriptorService>(io_service)
  {
  }

  /// Construct a basic_descriptor on an existing native descriptor.
  /**
   * This constructor creates a descriptor object to hold an existing native
   * descriptor.
   *
   * @param io_service The io_service object that the descriptor will use to
   * dispatch handlers for any asynchronous operations performed on the
   * descriptor.
   *
   * @param native_descriptor A native descriptor.
   *
   * @throws boost::system::system_error Thrown on failure.
   */
  basic_descriptor(boost::asio::io_service& io_service,
      const native_type& native_descriptor)
    : basic_io_object<DescriptorService>(io_service)
  {
    boost::system::error_code ec;
    this->service.assign(this->implementation, native_descriptor, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Get a reference to the lowest layer.
  /**
   * This function returns a reference to the lowest layer in a stack of
   * layers. Since a basic_descriptor cannot contain any further layers, it
   * simply returns a reference to itself.
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
   * layers. Since a basic_descriptor cannot contain any further layers, it
   * simply returns a reference to itself.
   *
   * @return A const reference to the lowest layer in the stack of layers.
   * Ownership is not transferred to the caller.
   */
  const lowest_layer_type& lowest_layer() const
  {
    return *this;
  }

  /// Assign an existing native descriptor to the descriptor.
  /*
   * This function opens the descriptor to hold an existing native descriptor.
   *
   * @param native_descriptor A native descriptor.
   *
   * @throws boost::system::system_error Thrown on failure.
   */
  void assign(const native_type& native_descriptor)
  {
    boost::system::error_code ec;
    this->service.assign(this->implementation, native_descriptor, ec);
    boost::asio::detail::throw_error(ec);
  }

  /// Assign an existing native descriptor to the descriptor.
  /*
   * This function opens the descriptor to hold an existing native descriptor.
   *
   * @param native_descriptor A native descriptor.
   *
   * @param ec Set to indicate what error occurred, if any.
   */
  boost::system::error_code assign(const native_type& native_descriptor,
      boost::system::error_code& ec)
  {
    return this->service.assign(this->implementation, native_descriptor, ec);
  }

  /// Determine whether the descriptor is open.
  bool is_open() const
  {
    return this->service.is_open(this->implementation);
  }

  /// Close the descriptor.
  /**
   * This function is used to close the descriptor. Any asynchronous read or
   * write operations will be cancelled immediately, and will complete with the
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

  /// Close the descriptor.
  /**
   * This function is used to close the descriptor. Any asynchronous read or
   * write operations will be cancelled immediately, and will complete with the
   * boost::asio::error::operation_aborted error.
   *
   * @param ec Set to indicate what error occurred, if any.
   */
  boost::system::error_code close(boost::system::error_code& ec)
  {
    return this->service.close(this->implementation, ec);
  }

  /// Get the native descriptor representation.
  /**
   * This function may be used to obtain the underlying representation of the
   * descriptor. This is intended to allow access to native descriptor
   * functionality that is not otherwise provided.
   */
  native_type native()
  {
    return this->service.native(this->implementation);
  }

  /// Cancel all asynchronous operations associated with the descriptor.
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

  /// Cancel all asynchronous operations associated with the descriptor.
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

  /// Perform an IO control command on the descriptor.
  /**
   * This function is used to execute an IO control command on the descriptor.
   *
   * @param command The IO control command to be performed on the descriptor.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @sa IoControlCommand @n
   * boost::asio::posix::descriptor_base::bytes_readable @n
   * boost::asio::posix::descriptor_base::non_blocking_io
   *
   * @par Example
   * Getting the number of bytes ready to read:
   * @code
   * boost::asio::posix::stream_descriptor descriptor(io_service);
   * ...
   * boost::asio::posix::stream_descriptor::bytes_readable command;
   * descriptor.io_control(command);
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

  /// Perform an IO control command on the descriptor.
  /**
   * This function is used to execute an IO control command on the descriptor.
   *
   * @param command The IO control command to be performed on the descriptor.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @sa IoControlCommand @n
   * boost::asio::posix::descriptor_base::bytes_readable @n
   * boost::asio::posix::descriptor_base::non_blocking_io
   *
   * @par Example
   * Getting the number of bytes ready to read:
   * @code
   * boost::asio::posix::stream_descriptor descriptor(io_service);
   * ...
   * boost::asio::posix::stream_descriptor::bytes_readable command;
   * boost::system::error_code ec;
   * descriptor.io_control(command, ec);
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

protected:
  /// Protected destructor to prevent deletion through this type.
  ~basic_descriptor()
  {
  }
};

} // namespace posix
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
       //   || defined(GENERATING_DOCUMENTATION)

#endif // BOOST_ASIO_POSIX_BASIC_DESCRIPTOR_HPP
