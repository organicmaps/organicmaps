//
// impl/write_at.hpp
// ~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IMPL_WRITE_AT_HPP
#define BOOST_ASIO_IMPL_WRITE_AT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/buffer.hpp>
#include <boost/asio/completion_condition.hpp>
#include <boost/asio/detail/base_from_completion_cond.hpp>
#include <boost/asio/detail/bind_handler.hpp>
#include <boost/asio/detail/consuming_buffers.hpp>
#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>
#include <boost/asio/detail/throw_error.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

template <typename SyncRandomAccessWriteDevice, typename ConstBufferSequence,
    typename CompletionCondition>
std::size_t write_at(SyncRandomAccessWriteDevice& d,
    boost::uint64_t offset, const ConstBufferSequence& buffers,
    CompletionCondition completion_condition, boost::system::error_code& ec)
{
  ec = boost::system::error_code();
  boost::asio::detail::consuming_buffers<
    const_buffer, ConstBufferSequence> tmp(buffers);
  std::size_t total_transferred = 0;
  tmp.prepare(detail::adapt_completion_condition_result(
        completion_condition(ec, total_transferred)));
  while (tmp.begin() != tmp.end())
  {
    std::size_t bytes_transferred = d.write_some_at(
        offset + total_transferred, tmp, ec);
    tmp.consume(bytes_transferred);
    total_transferred += bytes_transferred;
    tmp.prepare(detail::adapt_completion_condition_result(
          completion_condition(ec, total_transferred)));
  }
  return total_transferred;
}

template <typename SyncRandomAccessWriteDevice, typename ConstBufferSequence>
inline std::size_t write_at(SyncRandomAccessWriteDevice& d,
    boost::uint64_t offset, const ConstBufferSequence& buffers)
{
  boost::system::error_code ec;
  std::size_t bytes_transferred = write_at(
      d, offset, buffers, transfer_all(), ec);
  boost::asio::detail::throw_error(ec);
  return bytes_transferred;
}

template <typename SyncRandomAccessWriteDevice, typename ConstBufferSequence,
    typename CompletionCondition>
inline std::size_t write_at(SyncRandomAccessWriteDevice& d,
    boost::uint64_t offset, const ConstBufferSequence& buffers,
    CompletionCondition completion_condition)
{
  boost::system::error_code ec;
  std::size_t bytes_transferred = write_at(
      d, offset, buffers, completion_condition, ec);
  boost::asio::detail::throw_error(ec);
  return bytes_transferred;
}

#if !defined(BOOST_NO_IOSTREAM)

template <typename SyncRandomAccessWriteDevice, typename Allocator,
    typename CompletionCondition>
std::size_t write_at(SyncRandomAccessWriteDevice& d,
    boost::uint64_t offset, boost::asio::basic_streambuf<Allocator>& b,
    CompletionCondition completion_condition, boost::system::error_code& ec)
{
  std::size_t bytes_transferred = write_at(
      d, offset, b.data(), completion_condition, ec);
  b.consume(bytes_transferred);
  return bytes_transferred;
}

template <typename SyncRandomAccessWriteDevice, typename Allocator>
inline std::size_t write_at(SyncRandomAccessWriteDevice& d,
    boost::uint64_t offset, boost::asio::basic_streambuf<Allocator>& b)
{
  boost::system::error_code ec;
  std::size_t bytes_transferred = write_at(d, offset, b, transfer_all(), ec);
  boost::asio::detail::throw_error(ec);
  return bytes_transferred;
}

template <typename SyncRandomAccessWriteDevice, typename Allocator,
    typename CompletionCondition>
inline std::size_t write_at(SyncRandomAccessWriteDevice& d,
    boost::uint64_t offset, boost::asio::basic_streambuf<Allocator>& b,
    CompletionCondition completion_condition)
{
  boost::system::error_code ec;
  std::size_t bytes_transferred = write_at(
      d, offset, b, completion_condition, ec);
  boost::asio::detail::throw_error(ec);
  return bytes_transferred;
}

#endif // !defined(BOOST_NO_IOSTREAM)

namespace detail
{
  template <typename AsyncRandomAccessWriteDevice, typename ConstBufferSequence,
      typename CompletionCondition, typename WriteHandler>
  class write_at_op
    : detail::base_from_completion_cond<CompletionCondition>
  {
  public:
    write_at_op(AsyncRandomAccessWriteDevice& device,
        boost::uint64_t offset, const ConstBufferSequence& buffers,
        CompletionCondition completion_condition, WriteHandler handler)
      : detail::base_from_completion_cond<
          CompletionCondition>(completion_condition),
        device_(device),
        offset_(offset),
        buffers_(buffers),
        total_transferred_(0),
        handler_(handler)
    {
    }

    void operator()(const boost::system::error_code& ec,
        std::size_t bytes_transferred, int start = 0)
    {
      switch (start)
      {
        case 1:
        buffers_.prepare(this->check_for_completion(ec, total_transferred_));
        for (;;)
        {
          device_.async_write_some_at(
              offset_ + total_transferred_, buffers_, *this);
          return; default:
          total_transferred_ += bytes_transferred;
          buffers_.consume(bytes_transferred);
          buffers_.prepare(this->check_for_completion(ec, total_transferred_));
          if ((!ec && bytes_transferred == 0)
              || buffers_.begin() == buffers_.end())
            break;
        }

        handler_(ec, static_cast<const std::size_t>(total_transferred_));
      }
    }

  //private:
    AsyncRandomAccessWriteDevice& device_;
    boost::uint64_t offset_;
    boost::asio::detail::consuming_buffers<
      const_buffer, ConstBufferSequence> buffers_;
    std::size_t total_transferred_;
    WriteHandler handler_;
  };

  template <typename AsyncRandomAccessWriteDevice,
      typename CompletionCondition, typename WriteHandler>
  class write_at_op<AsyncRandomAccessWriteDevice,
      boost::asio::mutable_buffers_1, CompletionCondition, WriteHandler>
    : detail::base_from_completion_cond<CompletionCondition>
  {
  public:
    write_at_op(AsyncRandomAccessWriteDevice& device,
        boost::uint64_t offset, const boost::asio::mutable_buffers_1& buffers,
        CompletionCondition completion_condition,
        WriteHandler handler)
      : detail::base_from_completion_cond<
          CompletionCondition>(completion_condition),
        device_(device),
        offset_(offset),
        buffer_(buffers),
        total_transferred_(0),
        handler_(handler)
    {
    }

    void operator()(const boost::system::error_code& ec,
        std::size_t bytes_transferred, int start = 0)
    {
      std::size_t n = 0;
      switch (start)
      {
        case 1:
        n = this->check_for_completion(ec, total_transferred_);
        for (;;)
        {
          device_.async_write_some_at(offset_ + total_transferred_,
              boost::asio::buffer(buffer_ + total_transferred_, n), *this);
          return; default:
          total_transferred_ += bytes_transferred;
          if ((!ec && bytes_transferred == 0)
              || (n = this->check_for_completion(ec, total_transferred_)) == 0
              || total_transferred_ == boost::asio::buffer_size(buffer_))
            break;
        }

        handler_(ec, static_cast<const std::size_t>(total_transferred_));
      }
    }

  //private:
    AsyncRandomAccessWriteDevice& device_;
    boost::uint64_t offset_;
    boost::asio::mutable_buffer buffer_;
    std::size_t total_transferred_;
    WriteHandler handler_;
  };

  template <typename AsyncRandomAccessWriteDevice,
      typename CompletionCondition, typename WriteHandler>
  class write_at_op<AsyncRandomAccessWriteDevice, boost::asio::const_buffers_1,
      CompletionCondition, WriteHandler>
    : detail::base_from_completion_cond<CompletionCondition>
  {
  public:
    write_at_op(AsyncRandomAccessWriteDevice& device,
        boost::uint64_t offset, const boost::asio::const_buffers_1& buffers,
        CompletionCondition completion_condition,
        WriteHandler handler)
      : detail::base_from_completion_cond<
          CompletionCondition>(completion_condition),
        device_(device),
        offset_(offset),
        buffer_(buffers),
        total_transferred_(0),
        handler_(handler)
    {
    }

    void operator()(const boost::system::error_code& ec,
        std::size_t bytes_transferred, int start = 0)
    {
      std::size_t n = 0;
      switch (start)
      {
        case 1:
        n = this->check_for_completion(ec, total_transferred_);
        for (;;)
        {
          device_.async_write_some_at(offset_ + total_transferred_,
              boost::asio::buffer(buffer_ + total_transferred_, n), *this);
          return; default:
          total_transferred_ += bytes_transferred;
          if ((!ec && bytes_transferred == 0)
              || (n = this->check_for_completion(ec, total_transferred_)) == 0
              || total_transferred_ == boost::asio::buffer_size(buffer_))
            break;
        }

        handler_(ec, static_cast<const std::size_t>(total_transferred_));
      }
    }

  //private:
    AsyncRandomAccessWriteDevice& device_;
    boost::uint64_t offset_;
    boost::asio::const_buffer buffer_;
    std::size_t total_transferred_;
    WriteHandler handler_;
  };

  template <typename AsyncRandomAccessWriteDevice, typename ConstBufferSequence,
      typename CompletionCondition, typename WriteHandler>
  inline void* asio_handler_allocate(std::size_t size,
      write_at_op<AsyncRandomAccessWriteDevice, ConstBufferSequence,
        CompletionCondition, WriteHandler>* this_handler)
  {
    return boost_asio_handler_alloc_helpers::allocate(
        size, this_handler->handler_);
  }

  template <typename AsyncRandomAccessWriteDevice, typename ConstBufferSequence,
      typename CompletionCondition, typename WriteHandler>
  inline void asio_handler_deallocate(void* pointer, std::size_t size,
      write_at_op<AsyncRandomAccessWriteDevice, ConstBufferSequence,
        CompletionCondition, WriteHandler>* this_handler)
  {
    boost_asio_handler_alloc_helpers::deallocate(
        pointer, size, this_handler->handler_);
  }

  template <typename Function, typename AsyncRandomAccessWriteDevice,
      typename ConstBufferSequence, typename CompletionCondition,
      typename WriteHandler>
  inline void asio_handler_invoke(const Function& function,
      write_at_op<AsyncRandomAccessWriteDevice, ConstBufferSequence,
        CompletionCondition, WriteHandler>* this_handler)
  {
    boost_asio_handler_invoke_helpers::invoke(
        function, this_handler->handler_);
  }
} // namespace detail

template <typename AsyncRandomAccessWriteDevice, typename ConstBufferSequence,
    typename CompletionCondition, typename WriteHandler>
inline void async_write_at(AsyncRandomAccessWriteDevice& d,
    boost::uint64_t offset, const ConstBufferSequence& buffers,
    CompletionCondition completion_condition, WriteHandler handler)
{
  detail::write_at_op<AsyncRandomAccessWriteDevice,
    ConstBufferSequence, CompletionCondition, WriteHandler>(
      d, offset, buffers, completion_condition, handler)(
        boost::system::error_code(), 0, 1);
}

template <typename AsyncRandomAccessWriteDevice, typename ConstBufferSequence,
    typename WriteHandler>
inline void async_write_at(AsyncRandomAccessWriteDevice& d,
    boost::uint64_t offset, const ConstBufferSequence& buffers,
    WriteHandler handler)
{
  async_write_at(d, offset, buffers, transfer_all(), handler);
}

#if !defined(BOOST_NO_IOSTREAM)

namespace detail
{
  template <typename AsyncRandomAccessWriteDevice,
      typename Allocator, typename WriteHandler>
  class write_at_streambuf_op
  {
  public:
    write_at_streambuf_op(
        boost::asio::basic_streambuf<Allocator>& streambuf,
        WriteHandler handler)
      : streambuf_(streambuf),
        handler_(handler)
    {
    }

    void operator()(const boost::system::error_code& ec,
        const std::size_t bytes_transferred)
    {
      streambuf_.consume(bytes_transferred);
      handler_(ec, bytes_transferred);
    }

  //private:
    boost::asio::basic_streambuf<Allocator>& streambuf_;
    WriteHandler handler_;
  };

  template <typename AsyncRandomAccessWriteDevice, typename Allocator,
      typename WriteHandler>
  inline void* asio_handler_allocate(std::size_t size,
      write_at_streambuf_op<AsyncRandomAccessWriteDevice,
        Allocator, WriteHandler>* this_handler)
  {
    return boost_asio_handler_alloc_helpers::allocate(
        size, this_handler->handler_);
  }

  template <typename AsyncRandomAccessWriteDevice, typename Allocator,
      typename WriteHandler>
  inline void asio_handler_deallocate(void* pointer, std::size_t size,
      write_at_streambuf_op<AsyncRandomAccessWriteDevice,
        Allocator, WriteHandler>* this_handler)
  {
    boost_asio_handler_alloc_helpers::deallocate(
        pointer, size, this_handler->handler_);
  }

  template <typename Function, typename AsyncRandomAccessWriteDevice,
      typename Allocator, typename WriteHandler>
  inline void asio_handler_invoke(const Function& function,
      write_at_streambuf_op<AsyncRandomAccessWriteDevice,
        Allocator, WriteHandler>* this_handler)
  {
    boost_asio_handler_invoke_helpers::invoke(
        function, this_handler->handler_);
  }
} // namespace detail

template <typename AsyncRandomAccessWriteDevice, typename Allocator,
    typename CompletionCondition, typename WriteHandler>
inline void async_write_at(AsyncRandomAccessWriteDevice& d,
    boost::uint64_t offset, boost::asio::basic_streambuf<Allocator>& b,
    CompletionCondition completion_condition, WriteHandler handler)
{
  async_write_at(d, offset, b.data(), completion_condition,
      detail::write_at_streambuf_op<
        AsyncRandomAccessWriteDevice, Allocator, WriteHandler>(b, handler));
}

template <typename AsyncRandomAccessWriteDevice, typename Allocator,
    typename WriteHandler>
inline void async_write_at(AsyncRandomAccessWriteDevice& d,
    boost::uint64_t offset, boost::asio::basic_streambuf<Allocator>& b,
    WriteHandler handler)
{
  async_write_at(d, offset, b, transfer_all(), handler);
}

#endif // !defined(BOOST_NO_IOSTREAM)

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_IMPL_WRITE_AT_HPP
