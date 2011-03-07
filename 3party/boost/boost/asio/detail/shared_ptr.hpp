//
// detail/shared_ptr.hpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_SHARED_PTR_HPP
#define BOOST_ASIO_DETAIL_SHARED_PTR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# include <memory>
#else
# include <boost/shared_ptr.hpp>
#endif

namespace boost {
namespace asio {
namespace detail {

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
using std::shared_ptr;
#else
using boost::shared_ptr;
#endif

} // namespace detail
} // namespace asio
} // namespace boost

#endif // BOOST_ASIO_DETAIL_SHARED_PTR_HPP
