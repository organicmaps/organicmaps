// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_POSIX_ASIO_FWD_HPP_
#define BOOST_PROCESS_DETAIL_POSIX_ASIO_FWD_HPP_

#include <memory>

namespace boost { namespace asio {

class mutable_buffer;
class mutable_buffers_1;

class const_buffer;
class const_buffers_1;

template<typename Allocator>
class basic_streambuf;

typedef basic_streambuf<std::allocator<char>> streambuf;
class io_service;


class signal_set_service;
template <typename SignalSetService>

class basic_signal_set;
typedef basic_signal_set<signal_set_service> signal_set;

template <typename Handler>
class basic_yield_context;

namespace posix {

class stream_descriptor_service;

template <typename StreamDesscriptorService>
class basic_stream_descriptor;
typedef basic_stream_descriptor<stream_descriptor_service> stream_descriptor;



class object_handle_service;

template <typename ObjectHandleService>
class basic_object_handle;

typedef basic_object_handle<object_handle_service> object_handle;

} //posix
} //asio

namespace process { namespace detail { namespace posix {

class async_pipe;

template<typename T>
struct async_in_buffer;

template<int p1, int p2, typename Buffer>
struct async_out_buffer;

template<int p1, int p2, typename Type>
struct async_out_future;

} // posix
} // detail

using ::boost::process::detail::posix::async_pipe;

} // process
} // boost




#endif /* BOOST_PROCESS_DETAIL_POSIX_ASIO_FWD_HPP_ */
