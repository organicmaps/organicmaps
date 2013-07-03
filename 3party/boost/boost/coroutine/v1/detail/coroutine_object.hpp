
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_V1_DETAIL_COROUTINE_OBJECT_H
#define BOOST_COROUTINES_V1_DETAIL_COROUTINE_OBJECT_H

#include <cstddef>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/move/move.hpp>
#include <boost/ref.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/type_traits/function_traits.hpp>
#include <boost/utility.hpp>

#include <boost/coroutine/attributes.hpp>
#include <boost/coroutine/detail/config.hpp>
#include <boost/coroutine/detail/exceptions.hpp>
#include <boost/coroutine/detail/flags.hpp>
#include <boost/coroutine/detail/holder.hpp>
#include <boost/coroutine/detail/param.hpp>
#include <boost/coroutine/flags.hpp>
#include <boost/coroutine/stack_context.hpp>
#include <boost/coroutine/v1/detail/arg.hpp>
#include <boost/coroutine/v1/detail/coroutine_base.hpp>

#ifdef BOOST_MSVC
 #pragma warning (push)
 #pragma warning (disable: 4355) // using 'this' in initializer list
#endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines {
namespace detail {

template< typename Coroutine >
void trampoline1( intptr_t vp)
{
    BOOST_ASSERT( vp);

    reinterpret_cast< Coroutine * >( vp)->run();
}

template< typename Coroutine, typename Arg >
void trampoline2( intptr_t vp)
{
    BOOST_ASSERT( vp);

    tuple< Coroutine *, Arg > * tpl(
        reinterpret_cast< tuple< Coroutine *, Arg > * >( vp) );
    Coroutine * coro( get< 0 >( * tpl) );
    Arg arg( get< 1 >( * tpl) );

    coro->run( arg);
}

template< typename StackAllocator >
struct stack_tuple
{
    coroutines::stack_context   stack_ctx;
    StackAllocator              stack_alloc;

    stack_tuple( StackAllocator const& stack_alloc_, std::size_t size) :
        stack_ctx(),
        stack_alloc( stack_alloc_)
    {
        stack_alloc.allocate( stack_ctx, size);
    }

    ~stack_tuple()
    {
        stack_alloc.deallocate( stack_ctx);
    }
};

template<
    typename Signature,
    typename Fn, typename StackAllocator, typename Allocator,
    typename Caller,
    typename Result, int arity
>
class coroutine_object;

#include <boost/coroutine/v1/detail/coroutine_object_void_0.ipp>
#include <boost/coroutine/v1/detail/coroutine_object_void_1.ipp>
#include <boost/coroutine/v1/detail/coroutine_object_void_arity.ipp>
#include <boost/coroutine/v1/detail/coroutine_object_result_0.ipp>
#include <boost/coroutine/v1/detail/coroutine_object_result_1.ipp>
#include <boost/coroutine/v1/detail/coroutine_object_result_arity.ipp>

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#ifdef BOOST_MSVC
 #pragma warning (pop)
#endif

#endif // BOOST_COROUTINES_V1_DETAIL_COROUTINE_OBJECT_H
