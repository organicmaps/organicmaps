//
// basic_io_object.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_BASIC_IO_OBJECT_HPP
#define BOOST_ASIO_BASIC_IO_OBJECT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/noncopyable.hpp>
#include <boost/asio/io_service.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

/// Base class for all I/O objects.
template <typename IoObjectService>
class basic_io_object
  : private noncopyable
{
public:
  /// The type of the service that will be used to provide I/O operations.
  typedef IoObjectService service_type;

  /// The underlying implementation type of I/O object.
  typedef typename service_type::implementation_type implementation_type;

  /// (Deprecated: use get_io_service().) Get the io_service associated with
  /// the object.
  /**
   * This function may be used to obtain the io_service object that the I/O
   * object uses to dispatch handlers for asynchronous operations.
   *
   * @return A reference to the io_service object that the I/O object will use
   * to dispatch handlers. Ownership is not transferred to the caller.
   */
  boost::asio::io_service& io_service()
  {
    return service.get_io_service();
  }

  /// Get the io_service associated with the object.
  /**
   * This function may be used to obtain the io_service object that the I/O
   * object uses to dispatch handlers for asynchronous operations.
   *
   * @return A reference to the io_service object that the I/O object will use
   * to dispatch handlers. Ownership is not transferred to the caller.
   */
  boost::asio::io_service& get_io_service()
  {
    return service.get_io_service();
  }

protected:
  /// Construct a basic_io_object.
  /**
   * Performs:
   * @code service.construct(implementation); @endcode
   */
  explicit basic_io_object(boost::asio::io_service& io_service)
    : service(boost::asio::use_service<IoObjectService>(io_service))
  {
    service.construct(implementation);
  }

  /// Protected destructor to prevent deletion through this type.
  /**
   * Performs:
   * @code service.destroy(implementation); @endcode
   */
  ~basic_io_object()
  {
    service.destroy(implementation);
  }

  /// The service associated with the I/O object.
  service_type& service;

  /// The underlying implementation of the I/O object.
  implementation_type implementation;
};

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_BASIC_IO_OBJECT_HPP
