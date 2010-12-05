//
// ssl/context.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2005 Voipster / Indrek dot Juhani at voipster dot com
// Copyright (c) 2005-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_SSL_CONTEXT_HPP
#define BOOST_ASIO_SSL_CONTEXT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/ssl/basic_context.hpp>
#include <boost/asio/ssl/context_service.hpp>

namespace boost {
namespace asio {
namespace ssl {

/// Typedef for the typical usage of context.
typedef basic_context<context_service> context;

} // namespace ssl
} // namespace asio
} // namespace boost

#endif // BOOST_ASIO_SSL_CONTEXT_HPP
