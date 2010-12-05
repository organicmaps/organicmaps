//
// detail/strand_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_STRAND_SERVICE_HPP
#define BOOST_ASIO_DETAIL_STRAND_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/detail/mutex.hpp>
#include <boost/asio/detail/op_queue.hpp>
#include <boost/asio/detail/operation.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

// Default service implementation for a strand.
class strand_service
  : public boost::asio::detail::service_base<strand_service>
{
private:
  // Helper class to re-post the strand on exit.
  struct on_do_complete_exit;

  // Helper class to re-post the strand on exit.
  struct on_dispatch_exit;

public:

  // The underlying implementation of a strand.
  class strand_impl
    : public operation
  {
  public:
    strand_impl();

  private:
    // Only this service will have access to the internal values.
    friend class strand_service;
    friend struct on_do_complete_exit;
    friend struct on_dispatch_exit;

    // Mutex to protect access to internal data.
    boost::asio::detail::mutex mutex_;

    // The count of handlers in the strand, including the upcall (if any).
    std::size_t count_;

    // The handlers waiting on the strand.
    op_queue<operation> queue_;
  };

  typedef strand_impl* implementation_type;

  // Construct a new strand service for the specified io_service.
  BOOST_ASIO_DECL explicit strand_service(boost::asio::io_service& io_service);

  // Destroy all user-defined handler objects owned by the service.
  BOOST_ASIO_DECL void shutdown_service();

  // Construct a new strand implementation.
  BOOST_ASIO_DECL void construct(implementation_type& impl);

  // Destroy a strand implementation.
  void destroy(implementation_type& impl);

  // Request the io_service to invoke the given handler.
  template <typename Handler>
  void dispatch(implementation_type& impl, Handler handler);

  // Request the io_service to invoke the given handler and return immediately.
  template <typename Handler>
  void post(implementation_type& impl, Handler handler);

private:
  BOOST_ASIO_DECL static void do_complete(io_service_impl* owner,
      operation* base, boost::system::error_code ec,
      std::size_t bytes_transferred);

  // The io_service implementation used to post completions.
  io_service_impl& io_service_;

  // Mutex to protect access to the array of implementations.
  boost::asio::detail::mutex mutex_;

  // Number of implementations shared between all strand objects.
  enum { num_implementations = 193 };

  // The head of a linked list of all implementations.
  boost::scoped_ptr<strand_impl> implementations_[num_implementations];

  // Extra value used when hashing to prevent recycled memory locations from
  // getting the same strand implementation.
  std::size_t salt_;
};

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#include <boost/asio/detail/impl/strand_service.hpp>
#if defined(BOOST_ASIO_HEADER_ONLY)
# include <boost/asio/detail/impl/strand_service.ipp>
#endif // defined(BOOST_ASIO_HEADER_ONLY)

#endif // BOOST_ASIO_DETAIL_STRAND_SERVICE_HPP
