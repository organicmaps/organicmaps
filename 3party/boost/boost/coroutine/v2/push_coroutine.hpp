
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_V2_PUSH_COROUTINE_H
#define BOOST_COROUTINES_V2_PUSH_COROUTINE_H

#include <cstddef>
#include <memory>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/range.hpp>
#include <boost/type_traits/decay.hpp>
#include <boost/type_traits/function_traits.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/utility/result_of.hpp>

#include <boost/coroutine/attributes.hpp>
#include <boost/coroutine/detail/config.hpp>
#include <boost/coroutine/detail/coroutine_context.hpp>
#include <boost/coroutine/stack_allocator.hpp>
#include <boost/coroutine/v2/push_coroutine_base.hpp>
#include <boost/coroutine/v2/push_coroutine_object.hpp.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines {
namespace detail {

template< typename Arg >
class push_coroutine
{
private:
    typedef detail::push_coroutine_base< Arg >  base_t;
    typedef typename base_t::ptr_t              ptr_t;

    struct dummy
    { void nonnull() {} };

    typedef void ( dummy::*safe_bool)();

    ptr_t  impl_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( push_coroutine)

public:
    push_coroutine() BOOST_NOEXCEPT :
        impl_()
    {}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    template< typename Fn >
    explicit push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, push_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::push_coroutine_object<
                Arg, Fn, stack_allocator, std::allocator< push_coroutine >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator >
    explicit push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, push_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::push_coroutine_object<
                Arg, Fn, StackAllocator, std::allocator< push_coroutine >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_same< typename decay< Fn >::type, push_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::push_coroutine_object<
                Arg, Fn, StackAllocator, Allocator
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
    }
#else
    template< typename Fn >
    explicit push_coroutine( Fn fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::push_coroutine_object<
                Arg, Fn, stack_allocator, std::allocator< push_coroutine >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator >
    explicit push_coroutine( Fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::push_coroutine_object<
                Arg, Fn, StackAllocator, std::allocator< push_coroutine >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit push_coroutine( Fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::push_coroutine_object<
                Arg, Fn, StackAllocator, Allocator
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn >
    explicit push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, push_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::push_coroutine_object<
                Arg, Fn, stack_allocator, std::allocator< push_coroutine >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator >
    explicit push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, push_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::push_coroutine_object<
                Arg, Fn, StackAllocator, std::allocator< push_coroutine >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_same< typename decay< Fn >::type, push_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::push_coroutine_object<
                Arg, Fn, StackAllocator, Allocator
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }
#endif

    push_coroutine( BOOST_RV_REF( push_coroutine) other) BOOST_NOEXCEPT :
        impl_()
    { swap( other); }

    push_coroutine & operator=( BOOST_RV_REF( push_coroutine) other) BOOST_NOEXCEPT
    {
        push_coroutine tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    bool empty() const BOOST_NOEXCEPT
    { return ! impl_; }

    operator safe_bool() const BOOST_NOEXCEPT
    { return ( empty() || impl_->is_complete() ) ? 0 : & dummy::nonnull; }

    bool operator!() const BOOST_NOEXCEPT
    { return empty() || impl_->is_complete(); }

    void swap( push_coroutine & other) BOOST_NOEXCEPT
    { impl_.swap( other.impl_); }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    void operator()( Arg && arg)
    {
        BOOST_ASSERT( * this);

        impl_->resume( boost::forward< Arg >( arg) );
    }
#else
    void operator()( BOOST_RV_REF( Arg) arg)
    {
        BOOST_ASSERT( * this);

        impl_->resume( boost::forward< Arg >( arg) );
    }
#endif

    void operator()( Arg arg)
    {
        BOOST_ASSERT( * this);

        impl_->resume( arg);
    }
};

template< typename Arg >
void swap( push_coroutine< Arg > & l, push_coroutine< Arg > & r) BOOST_NOEXCEPT
{ l.swap( r); }
#if 0
template< typename Arg >
inline
typename push_coroutine< Arg >::iterator
range_begin( push_coroutine< Arg > & c)
{ return typename push_coroutine< Arg >::iterator( & c); }

template< typename Arg >
inline
typename push_coroutine< Arg >::const_iterator
range_begin( push_coroutine< Arg > const& c)
{ return typename push_coroutine< Arg >::const_iterator( & c); }

template< typename Arg >
inline
typename push_coroutine< Arg >::iterator
range_end( push_coroutine< Arg > &)
{ return typename push_coroutine< Arg >::iterator(); }

template< typename Arg >
inline
typename push_coroutine< Arg >::const_iterator
range_end( push_coroutine< Arg > const&)
{ return typename push_coroutine< Arg >::const_iterator(); }

template< typename Arg >
inline
typename push_coroutine< Arg >::iterator
begin( push_coroutine< Arg > & c)
{ return boost::begin( c); }

template< typename Arg >
inline
typename push_coroutine< Arg >::iterator
end( push_coroutine< Arg > & c)
{ return boost::end( c); }

template< typename Arg >
inline
typename push_coroutine< Arg >::const_iterator
begin( push_coroutine< Arg > const& c)
{ return boost::const_begin( c); }

template< typename Arg >
inline
typename push_coroutine< Arg >::const_iterator
end( push_coroutine< Arg > const& c)
{ return boost::const_end( c); }

}

template< typename Arg >
struct range_mutable_iterator< coroutines::coroutine< Arg > >
{ typedef typename coroutines::coroutine< Arg >::iterator type; };

template< typename Arg >
struct range_const_iterator< coroutines::coroutine< Arg > >
{ typedef typename coroutines::coroutine< Arg >::const_iterator type; };
#endif
}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES_V2_PUSH_COROUTINE_H
