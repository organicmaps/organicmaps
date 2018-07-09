
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_FIXEDSIZE_H
#define BOOST_CONTEXT_FIXEDSIZE_H

extern "C" {
#include <windows.h>
}

#include <cmath>
#include <cstddef>
#include <new>

#include <boost/config.hpp>

#include <boost/context/detail/config.hpp>
#include <boost/context/stack_context.hpp>
#include <boost/context/stack_traits.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace context {

template< typename traitsT >
class basic_fixedsize_stack {
private:
    std::size_t     size_;

public:
    typedef traitsT traits_type;

    basic_fixedsize_stack( std::size_t size = traits_type::default_size() ) BOOST_NOEXCEPT_OR_NOTHROW :
        size_( size) {
    }

    stack_context allocate() {
        // page at bottom will be used as guard-page
        const std::size_t pages(
            static_cast< std::size_t >( 
                std::floor(
                    static_cast< float >( size_) / traits_type::page_size() ) ) );
        BOOST_ASSERT_MSG( 1 <= pages, "at least one page must fit into stack");
        const std::size_t size__( pages * traits_type::page_size() );
        BOOST_ASSERT( 0 != size_ && 0 != size__);
        BOOST_ASSERT( size__ <= size_);

        void * vp = ::VirtualAlloc( 0, size__, MEM_COMMIT, PAGE_READWRITE);
        if ( ! vp) throw std::bad_alloc();

        stack_context sctx;
        sctx.size = size__;
        sctx.sp = static_cast< char * >( vp) + sctx.size;
        return sctx;
    }

    void deallocate( stack_context & sctx) BOOST_NOEXCEPT_OR_NOTHROW {
        BOOST_ASSERT( sctx.sp);

        void * vp = static_cast< char * >( sctx.sp) - sctx.size;
        ::VirtualFree( vp, 0, MEM_RELEASE);
    }
};

typedef basic_fixedsize_stack< stack_traits > fixedsize_stack;
typedef fixedsize_stack default_stack;

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_CONTEXT_FIXEDSIZE_H
