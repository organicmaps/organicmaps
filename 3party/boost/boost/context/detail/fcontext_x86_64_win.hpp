
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CTX_DETAIL_FCONTEXT_X86_64_H
#define BOOST_CTX_DETAIL_FCONTEXT_X86_64_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/cstdint.hpp>

#include <boost/context/detail/config.hpp>

#if defined(BOOST_MSVC)
#pragma warning(push)
#pragma warning(disable:4351)
#endif

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace ctx {

extern "C" {

#define BOOST_CONTEXT_CALLDECL

struct stack_t
{
    void    *   base;
    void    *   limit;

    stack_t() :
        base( 0), limit( 0)
    {}
};

struct fp_t
{
    boost::uint32_t     fc_freg[2];
    void            *   fc_xmm;
    char                fc_buffer[175];

    fp_t() :
        fc_freg(),
        fc_xmm( 0),
        fc_buffer()
    {
        fc_xmm = fc_buffer;
        if ( 0 != ( ( ( uintptr_t) fc_xmm) & 15) )
            fc_xmm = ( char *) ( ( ( ( uintptr_t) fc_xmm) + 15) & ~0x0F);
    }
};

struct fcontext_t
{
    boost::uint64_t     fc_greg[10];
    stack_t             fc_stack;
    void            *   fc_local_storage;
    fp_t                fc_fp;

    fcontext_t() :
        fc_greg(),
        fc_stack(),
        fc_local_storage( 0),
        fc_fp()
    {}
};

}

}}

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
#endif

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

#endif // BOOST_CTX_DETAIL_FCONTEXT_X86_64_H
