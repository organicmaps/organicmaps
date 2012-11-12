
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_GUARDED_STACK_ALLOCATOR_H
#define BOOST_CONTEXT_GUARDED_STACK_ALLOCATOR_H

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

class BOOST_CONTEXT_DECL guarded_stack_allocator
{
public:
    static bool is_stack_unbound();

    static std::size_t default_stacksize();

    static std::size_t minimum_stacksize();

    static std::size_t maximum_stacksize();

    void * allocate( std::size_t) const;

    void deallocate( void *, std::size_t) const;
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

//#endif

#endif // BOOST_CONTEXT_GUARDED_STACK_ALLOCATOR_H
