
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CTX_STACK_UTILS_H
#define BOOST_CTX_STACK_UTILS_H

#include <cstddef>

#include <boost/config.hpp>

#include <boost/context/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace ctx {

BOOST_CONTEXT_DECL std::size_t default_stacksize();

BOOST_CONTEXT_DECL std::size_t minimum_stacksize();

BOOST_CONTEXT_DECL std::size_t maximum_stacksize();

BOOST_CONTEXT_DECL std::size_t pagesize();

BOOST_CONTEXT_DECL std::size_t page_count( std::size_t stacksize);

BOOST_CONTEXT_DECL bool is_stack_unbound();

}}

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_CTX_STACK_UTILS_H
