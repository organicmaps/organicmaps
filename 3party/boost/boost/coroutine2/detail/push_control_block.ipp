
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES2_DETAIL_PUSH_CONTROL_BLOCK_IPP
#define BOOST_COROUTINES2_DETAIL_PUSH_CONTROL_BLOCK_IPP

#include <algorithm>
#include <exception>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <boost/context/execution_context.hpp>

#include <boost/coroutine2/detail/config.hpp>
#include <boost/coroutine2/detail/forced_unwind.hpp>
#include <boost/coroutine2/detail/state.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines2 {
namespace detail {

// push_coroutine< T >

template< typename T >
template< typename StackAllocator, typename Fn >
push_coroutine< T >::control_block::control_block( context::preallocated palloc, StackAllocator salloc,
                                                   Fn && fn_, bool preserve_fpu_) :
    other( nullptr),
    caller( boost::context::execution_context::current() ),
    callee( palloc, salloc,
            [=,fn=std::forward< Fn >( fn_)] () mutable -> decltype( auto) {
               // create synthesized pull_coroutine< T >
               typename pull_coroutine< T >::control_block synthesized_cb( this);
               pull_coroutine< T > synthesized( & synthesized_cb);
               other = & synthesized_cb;
               try {
                   // call coroutine-fn with synthesized pull_coroutine as argument
                   fn( synthesized);
               } catch ( forced_unwind const&) {
                   // do nothing for unwinding exception
               } catch (...) {
                   // store other exceptions in exception-pointer
                   except = std::current_exception();
               }
               // set termination flags
               state |= static_cast< int >( state_t::complete);
               caller( preserve_fpu);
               BOOST_ASSERT_MSG( false, "push_coroutine is complete");
            }),
    preserve_fpu( preserve_fpu_),
    state( static_cast< int >( state_t::unwind) ),
    except(),
    t( nullptr) {
}

template< typename T >
push_coroutine< T >::control_block::control_block( typename pull_coroutine< T >::control_block * cb) :
    other( cb),
    caller( other->callee),
    callee( other->caller),
    preserve_fpu( other->preserve_fpu),
    state( 0),
    except(),
    t( nullptr) {
}

template< typename T >
push_coroutine< T >::control_block::~control_block() {
    if ( 0 == ( state & static_cast< int >( state_t::complete ) ) &&
         0 != ( state & static_cast< int >( state_t::unwind) ) ) {
        // set early-exit flag
        state |= static_cast< int >( state_t::early_exit);
        callee( preserve_fpu);
    }
}

template< typename T >
void
push_coroutine< T >::control_block::resume( T const& t_) {
    // store data on this stack
    // pass an pointer (address of tmp) to other context
    T tmp( t_);
    t = & tmp;
    callee( preserve_fpu);
    t = nullptr;
    if ( except) {
        std::rethrow_exception( except);
    }
    // test early-exit-flag
    if ( 0 != ( ( other->state) & static_cast< int >( state_t::early_exit) ) ) {
        throw forced_unwind();
    }
}

template< typename T >
void
push_coroutine< T >::control_block::resume( T && t_) {
    // store data on this stack
    // pass an pointer (address of tmp) to other context
    T tmp( std::move( t_) );
    t = & tmp;
    callee( preserve_fpu);
    t = nullptr;
    if ( except) {
        std::rethrow_exception( except);
    }
    // test early-exit-flag
    if ( 0 != ( ( other->state) & static_cast< int >( state_t::early_exit) ) ) {
        throw forced_unwind();
    }
}

template< typename T >
bool
push_coroutine< T >::control_block::valid() const noexcept {
    return 0 == ( state & static_cast< int >( state_t::complete) );
}


// push_coroutine< T & >

template< typename T >
template< typename StackAllocator, typename Fn >
push_coroutine< T & >::control_block::control_block( context::preallocated palloc, StackAllocator salloc,
                                                     Fn && fn_, bool preserve_fpu_) :
    other( nullptr),
    caller( boost::context::execution_context::current() ),
    callee( palloc, salloc,
            [=,fn=std::forward< Fn >( fn_)] () mutable -> decltype( auto) {
               // create synthesized pull_coroutine< T >
               typename pull_coroutine< T & >::control_block synthesized_cb( this);
               pull_coroutine< T & > synthesized( & synthesized_cb);
               other = & synthesized_cb;
               try {
                   // call coroutine-fn with synthesized pull_coroutine as argument
                   fn( synthesized);
               } catch ( forced_unwind const&) {
                   // do nothing for unwinding exception
               } catch (...) {
                   // store other exceptions in exception-pointer
                   except = std::current_exception();
               }
               // set termination flags
               state |= static_cast< int >( state_t::complete);
               caller( preserve_fpu);
               BOOST_ASSERT_MSG( false, "push_coroutine is complete");
            }),
    preserve_fpu( preserve_fpu_),
    state( static_cast< int >( state_t::unwind) ),
    except(),
    t( nullptr) {
}

template< typename T >
push_coroutine< T & >::control_block::control_block( typename pull_coroutine< T & >::control_block * cb) :
    other( cb),
    caller( other->callee),
    callee( other->caller),
    preserve_fpu( other->preserve_fpu),
    state( 0),
    except(),
    t( nullptr) {
}

template< typename T >
push_coroutine< T & >::control_block::~control_block() {
    if ( 0 == ( state & static_cast< int >( state_t::complete ) ) &&
         0 != ( state & static_cast< int >( state_t::unwind) ) ) {
        // set early-exit flag
        state |= static_cast< int >( state_t::early_exit);
        callee( preserve_fpu);
    }
}

template< typename T >
void
push_coroutine< T & >::control_block::resume( T & t_) {
    t = & t_;
    callee( preserve_fpu);
    t = nullptr;
    if ( except) {
        std::rethrow_exception( except);
    }
    // test early-exit-flag
    if ( 0 != ( ( other->state) & static_cast< int >( state_t::early_exit) ) ) {
        throw forced_unwind();
    }
}

template< typename T >
bool
push_coroutine< T & >::control_block::valid() const noexcept {
    return 0 == ( state & static_cast< int >( state_t::complete) );
}


// push_coroutine< void >

template< typename StackAllocator, typename Fn >
push_coroutine< void >::control_block::control_block( context::preallocated palloc, StackAllocator salloc, Fn && fn_, bool preserve_fpu_) :
    other( nullptr),
    caller( boost::context::execution_context::current() ),
    callee( palloc, salloc,
            [=,fn=std::forward< Fn >( fn_)] () mutable -> decltype( auto) {
               // create synthesized pull_coroutine< T >
               typename pull_coroutine< void >::control_block synthesized_cb( this);
               pull_coroutine< void > synthesized( & synthesized_cb);
               other = & synthesized_cb;
               try {
                   // call coroutine-fn with synthesized pull_coroutine as argument
                   fn( synthesized);
               } catch ( forced_unwind const&) {
                   // do nothing for unwinding exception
               } catch (...) {
                   // store other exceptions in exception-pointer
                   except = std::current_exception();
               }
               // set termination flags
               state |= static_cast< int >( state_t::complete);
               caller( preserve_fpu);
               BOOST_ASSERT_MSG( false, "push_coroutine is complete");
            }),
    preserve_fpu( preserve_fpu_),
    state( static_cast< int >( state_t::unwind) ),
    except() {
}

inline
push_coroutine< void >::control_block::control_block( pull_coroutine< void >::control_block * cb) :
    other( cb),
    caller( other->callee),
    callee( other->caller),
    preserve_fpu( other->preserve_fpu),
    state( 0),
    except() {
}

inline
push_coroutine< void >::control_block::~control_block() {
    if ( 0 == ( state & static_cast< int >( state_t::complete ) ) &&
         0 != ( state & static_cast< int >( state_t::unwind) ) ) {
        // set early-exit flag
        state |= static_cast< int >( state_t::early_exit);
        callee( preserve_fpu);
    }
}

inline
void
push_coroutine< void >::control_block::resume() {
    callee( preserve_fpu);
    if ( except) {
        std::rethrow_exception( except);
    }
    // test early-exit-flag
    if ( 0 != ( ( other->state) & static_cast< int >( state_t::early_exit) ) ) {
        throw forced_unwind();
    }
}

inline
bool
push_coroutine< void >::control_block::valid() const noexcept {
    return 0 == ( state & static_cast< int >( state_t::complete) );
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES2_DETAIL_PUSH_CONTROL_BLOCK_IPP
