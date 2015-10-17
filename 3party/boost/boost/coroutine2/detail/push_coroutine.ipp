
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES2_DETAIL_PUSH_COROUTINE_IPP
#define BOOST_COROUTINES2_DETAIL_PUSH_COROUTINE_IPP

#include <memory>
#include <utility>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <boost/context/execution_context.hpp>
#include <boost/context/stack_context.hpp>

#include <boost/coroutine2/detail/config.hpp>
#include <boost/coroutine2/fixedsize_stack.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines2 {
namespace detail {

// push_coroutine< T >

template< typename T >
push_coroutine< T >::push_coroutine( control_block * cb) :
    cb_( cb) {
}

template< typename T >
template< typename Fn >
push_coroutine< T >::push_coroutine( Fn && fn, bool preserve_fpu) :
    push_coroutine( fixedsize_stack(), std::forward< Fn >( fn), preserve_fpu) {
}

template< typename T >
template< typename StackAllocator, typename Fn >
push_coroutine< T >::push_coroutine( StackAllocator salloc, Fn && fn, bool preserve_fpu) :
    cb_( nullptr) {
    context::stack_context sctx( salloc.allocate() );
    // reserve space for control structure
#if defined(BOOST_NO_CXX14_CONSTEXPR) || defined(BOOST_NO_CXX11_STD_ALIGN)
    void * sp = static_cast< char * >( sctx.sp) - sizeof( control_block);
    std::size_t size = sctx.size - sizeof( control_block);
#else
    constexpr std::size_t func_alignment = 64; // alignof( control_block);
    constexpr std::size_t func_size = sizeof( control_block);
    // reserve space on stack
    void * sp = static_cast< char * >( sctx.sp) - func_size - func_alignment;
    // align sp pointer
    std::size_t space = func_size + func_alignment;
    sp = std::align( func_alignment, func_size, sp, space);
    BOOST_ASSERT( nullptr != sp);
    // calculate remaining size
    std::size_t size = sctx.size - ( static_cast< char * >( sctx.sp) - static_cast< char * >( sp) );
#endif
    // placment new for control structure on coroutine stack
    cb_= new ( sp) control_block( context::preallocated( sp, size, sctx),
                                  salloc, std::forward< Fn >( fn), preserve_fpu);
}

template< typename T >
push_coroutine< T >::~push_coroutine() {
    if ( nullptr != cb_) {
        cb_->~control_block();
    }
}

template< typename T >
push_coroutine< T >::push_coroutine( push_coroutine && other) :
    cb_( other.cb_) {
    other.cb_ = nullptr;
}

template< typename T >
push_coroutine< T > &
push_coroutine< T >::operator()( T const& t) {
    cb_->resume( t);
    return * this;
}

template< typename T >
push_coroutine< T > &
push_coroutine< T >::operator()( T && t) {
    cb_->resume( std::forward< T >( t) );
    return * this;
}

template< typename T >
push_coroutine< T >::operator bool() const noexcept {
    return nullptr != cb_ && cb_->valid();
}

template< typename T >
bool
push_coroutine< T >::operator!() const noexcept {
    return nullptr == cb_ || ! cb_->valid();
}


// push_coroutine< T & >

template< typename T >
push_coroutine< T & >::push_coroutine( control_block * cb) :
    cb_( cb) {
}

template< typename T >
template< typename Fn >
push_coroutine< T & >::push_coroutine( Fn && fn, bool preserve_fpu) :
    push_coroutine( fixedsize_stack(), std::forward< Fn >( fn), preserve_fpu) {
}

template< typename T >
template< typename StackAllocator, typename Fn >
push_coroutine< T & >::push_coroutine( StackAllocator salloc, Fn && fn, bool preserve_fpu) :
    cb_( nullptr) {
    context::stack_context sctx( salloc.allocate() );
    // reserve space for control structure
#if defined(BOOST_NO_CXX14_CONSTEXPR) || defined(BOOST_NO_CXX11_STD_ALIGN)
    void * sp = static_cast< char * >( sctx.sp) - sizeof( control_block);
    std::size_t size = sctx.size - sizeof( control_block);
#else
    constexpr std::size_t func_alignment = 64; // alignof( control_block);
    constexpr std::size_t func_size = sizeof( control_block);
    // reserve space on stack
    void * sp = static_cast< char * >( sctx.sp) - func_size - func_alignment;
    // align sp pointer
    std::size_t space = func_size + func_alignment;
    sp = std::align( func_alignment, func_size, sp, space);
    BOOST_ASSERT( nullptr != sp);
    // calculate remaining size
    std::size_t size = sctx.size - ( static_cast< char * >( sctx.sp) - static_cast< char * >( sp) );
#endif
    // placment new for control structure on coroutine stack
    cb_ = new ( sp) control_block( context::preallocated( sp, size, sctx),
                                   salloc, std::forward< Fn >( fn), preserve_fpu);
}

template< typename T >
push_coroutine< T & >::~push_coroutine() {
    if ( nullptr != cb_) {
        cb_->~control_block();
    }
}

template< typename T >
push_coroutine< T & >::push_coroutine( push_coroutine && other) :
    cb_( other.cb_) {
    other.cb_ = nullptr;
}

template< typename T >
push_coroutine< T & > &
push_coroutine< T & >::operator()( T & t) {
    cb_->resume( t);
    return * this;
}

template< typename T >
push_coroutine< T & >::operator bool() const noexcept {
    return nullptr != cb_ && cb_->valid();
}

template< typename T >
bool
push_coroutine< T & >::operator!() const noexcept {
    return nullptr == cb_ || ! cb_->valid();
}


// push_coroutine< void >

inline
push_coroutine< void >::push_coroutine( control_block * cb) :
    cb_( cb) {
}

template< typename Fn >
push_coroutine< void >::push_coroutine( Fn && fn, bool preserve_fpu) :
    push_coroutine( fixedsize_stack(), std::forward< Fn >( fn), preserve_fpu) {
}

template< typename StackAllocator, typename Fn >
push_coroutine< void >::push_coroutine( StackAllocator salloc, Fn && fn, bool preserve_fpu) :
    cb_( nullptr) {
    context::stack_context sctx( salloc.allocate() );
    // reserve space for control structure
#if defined(BOOST_NO_CXX14_CONSTEXPR) || defined(BOOST_NO_CXX11_STD_ALIGN)
    void * sp = static_cast< char * >( sctx.sp) - sizeof( control_block);
    std::size_t size = sctx.size - sizeof( control_block);
#else
    constexpr std::size_t func_alignment = 64; // alignof( control_block);
    constexpr std::size_t func_size = sizeof( control_block);
    // reserve space on stack
    void * sp = static_cast< char * >( sctx.sp) - func_size - func_alignment;
    // align sp pointer
    std::size_t space = func_size + func_alignment;
    sp = std::align( func_alignment, func_size, sp, space);
    BOOST_ASSERT( nullptr != sp);
    // calculate remaining size
    std::size_t size = sctx.size - ( static_cast< char * >( sctx.sp) - static_cast< char * >( sp) );
#endif
    // placment new for control structure on coroutine stack
    cb_ = new ( sp) control_block( context::preallocated( sp, size, sctx),
                                   salloc, std::forward< Fn >( fn), preserve_fpu);
}

inline
push_coroutine< void >::~push_coroutine() {
    if ( nullptr != cb_) {
        cb_->~control_block();
    }
}

inline
push_coroutine< void >::push_coroutine( push_coroutine && other) :
    cb_( other.cb_) {
    other.cb_ = nullptr;
}

inline
push_coroutine< void > &
push_coroutine< void >::operator()() {
    cb_->resume();
    return * this;
}

inline
push_coroutine< void >::operator bool() const noexcept {
    return nullptr != cb_ && cb_->valid();
}

inline
bool
push_coroutine< void >::operator!() const noexcept {
    return nullptr == cb_ || ! cb_->valid();
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES2_DETAIL_PUSH_COROUTINE_IPP
