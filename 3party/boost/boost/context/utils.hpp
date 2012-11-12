
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_UTILS_H
#define BOOST_CONTEXT_UTILS_H

#include <boost/config.hpp>

#if ! defined (BOOST_WINDOWS)
extern "C" {
#include <unistd.h>
}
#endif

//#if defined (BOOST_WINDOWS) || _POSIX_C_SOURCE >= 200112L

#include <cstddef>

#include <boost/context/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace context {

BOOST_CONTEXT_DECL std::size_t pagesize();

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

//#endif

#endif // BOOST_CONTEXT_UTILS_H
