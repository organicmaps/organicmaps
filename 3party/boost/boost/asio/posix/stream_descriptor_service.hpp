//
// posix/stream_descriptor_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_POSIX_STREAM_DESCRIPTOR_SERVICE_HPP
#define BOOST_ASIO_POSIX_STREAM_DESCRIPTOR_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR) \
  || defined(GENERATING_DOCUMENTATION)

#include <cstddef>
#include <boost/asio/error.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/detail/reactive_descriptor_service.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace posix {

/// Default service implementation for a stream descriptor.
class stream_descriptor_service
#if defined(GENERATING_DOCUMENTATION)
  : public boost::asio::io_service::service
#else
  : public boost::asio::detail::service_base<stream_descriptor_service>
#endif
{
public:
#if defined(GENERATING_DOCUMENTATION)
  /// The unique service identifier.
  static boost::asio::io_service::id id;
#endif

private:
  // The type of the platform-specific implementation.
  typedef detail::reactive_descriptor_service service_impl_type;

public:
  /// The type of a stream descriptor implementation.
#if defined(GENERATING_DOCUMENTATION)
  typedef implementation_defined implementation_type;
#else
  typedef service_impl_type::implementation_type implementation_type;
#endif

  /// The native descriptor type.
#if defined(GENERATING_DOCUMENTATION)
  typedef implementation_defined native_type;
#else
  typedef service_impl_type::native_type native_type;
#endif

  /// Construct a new stream descriptor service for the specified io_service.
  explicit stream_descriptor_service(boost::asio::io_service& io_service)
    : boost::asio::detail::service_base<stream_descriptor_service>(io_service),
      service_impl_(io_service)
  {
  }

  /// Destroy all user-defined descriptorr objects owned by the service.
  void shutdown_service()
  {
    service_impl_.shutdown_service();
  }

  /// Construct a new stream descriptor implementation.
  void construct(implementation_type& impl)
  {
    service_impl_.construct(impl);
  }

  /// Destroy a stream descriptor implementation.
  void destroy(implementation_type& impl)
  {
    service_impl_.destroy(impl);
  }

  /// Assign an existing native descriptor to a stream descriptor.
  boost::system::error_code assign(implementation_type& impl,
      const native_type& native_descriptor, boost::system::error_code& ec)
  {
    return service_impl_.assign(impl, native_descriptor, ec);
  }

  /// Determine whether the descriptor is open.
  bool is_open(const implementation_type& impl) const
  {
    return service_impl_.is_open(impl);
  }

  /// Close a stream descriptor implementation.
  boost::system::error_code close(implementation_type& impl,
      boost::system::error_code& ec)
  {
    return service_impl_.close(impl, ec);
  }

  /// Get the native descriptor implementation.
  native_type native(implementation_type& impl)
  {
    return service_impl_.native(impl);
  }

  /// Cancel all asynchronous operations associated with the descriptor.
  boost::system::error_code cancel(implementation_type& impl,
      boost::system::error_code& ec)
  {
    return service_impl_.cancel(impl, ec);
  }

  /// Perform an IO control command on the descriptor.
  template <typename IoControlCommand>
  boost::system::error_code io_control(implementation_type& impl,
      IoControlCommand& command, boost::system::error_code& ec)
  {
    return service_impl_.io_control(impl, command, ec);
  }

  /// Write the given data to the stream.
  template <typename ConstBufferSequence>
  std::size_t write_some(implementation_type& impl,
      const ConstBufferSequence& buffers, boost::system::error_code& ec)
  {
    return service_impl_.write_some(impl, buffers, ec);
  }

  /// Start an asynchronous write.
  template <typename ConstBufferSequence, typename WriteHandler>
  void async_write_some(implementation_type& impl,
      const ConstBufferSequence& buffers, WriteHandler descriptorr)
  {
    service_impl_.async_write_some(impl, buffers, descriptorr);
  }

  /// Read some data from the stream.
  template <typename MutableBufferSequence>
  std::size_t read_some(implementation_type& impl,
      const MutableBufferSequence& buffers, boost::system::error_code& ec)
  {
    return service_impl_.read_some(impl, buffers, ec);
  }

  /// Start an asynchronous read.
  template <typename MutableBufferSequence, typename ReadHandler>
  void async_read_some(implementation_type& impl,
      const MutableBufferSequence& buffers, ReadHandler descriptorr)
  {
    service_impl_.async_read_some(impl, buffers, descriptorr);
  }

private:
  // The platform-specific implementation.
  service_impl_type service_impl_;
};

} // namespace posix
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
       //   || defined(GENERATING_DOCUMENTATION)

#endif // BOOST_ASIO_POSIX_STREAM_DESCRIPTOR_SERVICE_HPP
