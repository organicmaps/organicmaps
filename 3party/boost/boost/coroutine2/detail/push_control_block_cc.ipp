
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES2_DETAIL_PUSH_CONTROL_BLOCK_IPP
#define BOOST_COROUTINES2_DETAIL_PUSH_CONTROL_BLOCK_IPP

#include <algorithm>
#include <exception>
#include <memory>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <boost/context/continuation.hpp>

#include <boost/coroutine2/detail/config.hpp>
#include <boost/coroutine2/detail/forced_unwind.hpp>
#include <boost/coroutine2/detail/wrap.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines2 {
namespace detail {

// push_coroutine< T >

template< typename T >
void
push_coroutine< T >::control_block::destroy( control_block * cb) noexcept {
    boost::context::continuation c = std::move( cb->c);
    // destroy control structure
    cb->~control_block();
    // destroy coroutine's stack
    cb->state |= state_t::destroy;
    c.resume();
}

template< typename T >
template< typename StackAllocator, typename Fn >
push_coroutine< T >::control_block::control_block( context::preallocated palloc, StackAllocator salloc,
                                                   Fn && fn) :
    c{},
    other{ nullptr },
    state{ state_t::unwind },
    except{} {
#if defined(BOOST_NO_CXX14_GENERIC_LAMBDAS)
    c = boost::context::callcc(
            std::allocator_arg, palloc, salloc,
            wrap( [this](typename std::decay< Fn >::type & fn_,boost::context::continuation && c) mutable {
                    // create synthesized pull_coroutine< T >
                    typename pull_coroutine< T >::control_block synthesized_cb{ this, c };
                    pull_coroutine< T > synthesized{ & synthesized_cb };
                    other = & synthesized_cb;
                    other->c = other->c.resume();
                    // set transferred value
                    if ( other->c.data_available() ) {
                        synthesized_cb.set( other->c.template get_data< T >() );
                    } else {
                        synthesized_cb.reset();
                    }
                    if ( state_t::none == ( state & state_t::destroy) ) {
                        try {
                            auto fn = std::move( fn_);
                            // call coroutine-fn with synthesized pull_coroutine as argument
                            fn( synthesized);
                        } catch ( boost::context::detail::forced_unwind const&) {
                            throw;
                        } catch (...) {
                            // store other exceptions in exception-pointer
                            except = std::current_exception();
                        }
                    }
                    // set termination flags
                    state |= state_t::complete;
                    // jump back
                    return other->c.resume();
                 },
                 std::forward< Fn >( fn) ) );
#else
    c = boost::context::callcc(
            std::allocator_arg, palloc, salloc,
            [this,fn_=std::forward< Fn >( fn)](boost::context::continuation && c) mutable {
               // create synthesized pull_coroutine< T >
               typename pull_coroutine< T >::control_block synthesized_cb{ this, c };
               pull_coroutine< T > synthesized{ & synthesized_cb };
               other = & synthesized_cb;
               other->c = other->c.resume();
               // set transferred value
               if ( other->c.data_available() ) {
                   synthesized_cb.set( other->c.template get_data< T >() );
               } else {
                   synthesized_cb.reset();
               }
               if ( state_t::none == ( state & state_t::destroy) ) {
                   try {
                       auto fn = std::move( fn_);
                       // call coroutine-fn with synthesized pull_coroutine as argument
                       fn( synthesized);
                   } catch ( boost::context::detail::forced_unwind const&) {
                       throw;
                   } catch (...) {
                       // store other exceptions in exception-pointer
                       except = std::current_exception();
                   }
               }
               // set termination flags
               state |= state_t::complete;
               // jump back
               return other->c.resume();
            });
#endif
}

template< typename T >
push_coroutine< T >::control_block::control_block( typename pull_coroutine< T >::control_block * cb,
                                                   boost::context::continuation & c_) noexcept :
    c{ std::move( c_) },
    other{ cb },
    state{ state_t::none },
    except{} {
}

template< typename T >
void
push_coroutine< T >::control_block::deallocate() noexcept {
    if ( state_t::none != ( state & state_t::unwind) ) {
        destroy( this);
    }
}

template< typename T >
void
push_coroutine< T >::control_block::resume( T const& data) {
    // pass an pointer to other context
    c = c.resume( data);
    if ( except) {
        std::rethrow_exception( except);
    }
}

template< typename T >
void
push_coroutine< T >::control_block::resume( T && data) {
    // pass an pointer to other context
    c = c.resume( std::move( data) );
    if ( except) {
        std::rethrow_exception( except);
    }
}

template< typename T >
bool
push_coroutine< T >::control_block::valid() const noexcept {
    return state_t::none == ( state & state_t::complete );
}


// push_coroutine< T & >

template< typename T >
void
push_coroutine< T & >::control_block::destroy( control_block * cb) noexcept {
    boost::context::continuation c = std::move( cb->c);
    // destroy control structure
    cb->~control_block();
    // destroy coroutine's stack
    cb->state |= state_t::destroy;
   c.resume();
}

template< typename T >
template< typename StackAllocator, typename Fn >
push_coroutine< T & >::control_block::control_block( context::preallocated palloc, StackAllocator salloc,
                                                     Fn && fn) :
    c{},
    other{ nullptr },
    state{ state_t::unwind },
    except{} {
#if defined(BOOST_NO_CXX14_GENERIC_LAMBDAS)
    c = boost::context::callcc(
            std::allocator_arg, palloc, salloc,
            wrap( [this](typename std::decay< Fn >::type & fn_,boost::context::continuation && c) mutable {
                    // create synthesized pull_coroutine< T & >
                    typename pull_coroutine< T & >::control_block synthesized_cb{ this, c };
                    pull_coroutine< T & > synthesized{ & synthesized_cb };
                    other = & synthesized_cb;
                    other->c = other->c.resume();
                    // set transferred value
                    if ( other->c.data_available() ) {
                        synthesized_cb.set( other->c.template get_data< T & >() );
                    } else {
                        synthesized_cb.reset();
                    }
                    if ( state_t::none == ( state & state_t::destroy) ) {
                        try {
                            auto fn = std::move( fn_);
                            // call coroutine-fn with synthesized pull_coroutine as argument
                            fn( synthesized);
                        } catch ( boost::context::detail::forced_unwind const&) {
                            throw;
                        } catch (...) {
                            // store other exceptions in exception-pointer
                            except = std::current_exception();
                        }
                    }
                    // set termination flags
                    state |= state_t::complete;
                    // jump back
                    return other->c.resume();
                 },
                 std::forward< Fn >( fn) ) );
#else
    c = boost::context::callcc(
            std::allocator_arg, palloc, salloc,
            [this,fn_=std::forward< Fn >( fn)](boost::context::continuation && c) mutable {
               // create synthesized pull_coroutine< T & >
               typename pull_coroutine< T & >::control_block synthesized_cb{ this, c };
               pull_coroutine< T & > synthesized{ & synthesized_cb };
               other = & synthesized_cb;
               other->c = other->c.resume();
               // set transferred value
               if ( other->c.data_available() ) {
                   synthesized_cb.set( other->c.template get_data< T & >() );
               } else {
                   synthesized_cb.reset();
               }
               if ( state_t::none == ( state & state_t::destroy) ) {
                   try {
                       auto fn = std::move( fn_);
                       // call coroutine-fn with synthesized pull_coroutine as argument
                       fn( synthesized);
                   } catch ( boost::context::detail::forced_unwind const&) {
                       throw;
                   } catch (...) {
                       // store other exceptions in exception-pointer
                       except = std::current_exception();
                   }
               }
               // set termination flags
               state |= state_t::complete;
               // jump back
               return other->c.resume();
            });
#endif
}

template< typename T >
push_coroutine< T & >::control_block::control_block( typename pull_coroutine< T & >::control_block * cb,
                                                     boost::context::continuation & c_) noexcept :
    c{ std::move( c_) },
    other{ cb },
    state{ state_t::none },
    except{} {
}

template< typename T >
void
push_coroutine< T & >::control_block::deallocate() noexcept {
    if ( state_t::none != ( state & state_t::unwind) ) {
        destroy( this);
    }
}

template< typename T >
void
push_coroutine< T & >::control_block::resume( T & t) {
    // pass an pointer to other context
    c = c.resume( std::ref( t) );
    if ( except) {
        std::rethrow_exception( except);
    }
}

template< typename T >
bool
push_coroutine< T & >::control_block::valid() const noexcept {
    return state_t::none == ( state & state_t::complete );
}


// push_coroutine< void >

inline
void
push_coroutine< void >::control_block::destroy( control_block * cb) noexcept {
    boost::context::continuation c = std::move( cb->c);
    // destroy control structure
    cb->~control_block();
    // destroy coroutine's stack
    cb->state |= state_t::destroy;
    c.resume();
}

template< typename StackAllocator, typename Fn >
push_coroutine< void >::control_block::control_block( context::preallocated palloc, StackAllocator salloc, Fn && fn) :
    c{},
    other{ nullptr },
    state{ state_t::unwind },
    except{} {
#if defined(BOOST_NO_CXX14_GENERIC_LAMBDAS)
    c = boost::context::callcc(
            std::allocator_arg, palloc, salloc,
            wrap( [this](typename std::decay< Fn >::type & fn_,boost::context::continuation && c) mutable {
                    // create synthesized pull_coroutine< void >
                    typename pull_coroutine< void >::control_block synthesized_cb{ this, c };
                    pull_coroutine< void > synthesized{ & synthesized_cb };
                    other = & synthesized_cb;
                    other->c =  other->c.resume();
                    if ( state_t::none == ( state & state_t::destroy) ) {
                        try {
                            auto fn = std::move( fn_);
                            // call coroutine-fn with synthesized pull_coroutine as argument
                            fn( synthesized);
                        } catch ( boost::context::detail::forced_unwind const&) {
                            throw;
                        } catch (...) {
                            // store other exceptions in exception-pointer
                            except = std::current_exception();
                        }
                    }
                    // set termination flags
                    state |= state_t::complete;
                    // jump back
                    return other->c.resume();
                 },
                 std::forward< Fn >( fn) ) );
#else
    c = boost::context::callcc(
            std::allocator_arg, palloc, salloc,
            [this,fn_=std::forward< Fn >( fn)](boost::context::continuation && c) mutable {
               // create synthesized pull_coroutine< void >
               typename pull_coroutine< void >::control_block synthesized_cb{ this, c};
               pull_coroutine< void > synthesized{ & synthesized_cb };
               other = & synthesized_cb;
               other->c = other->c.resume();
               if ( state_t::none == ( state & state_t::destroy) ) {
                   try {
                       auto fn = std::move( fn_);
                       // call coroutine-fn with synthesized pull_coroutine as argument
                       fn( synthesized);
                   } catch ( boost::context::detail::forced_unwind const&) {
                       throw;
                   } catch (...) {
                       // store other exceptions in exception-pointer
                       except = std::current_exception();
                   }
               }
               // set termination flags
               state |= state_t::complete;
               // jump back
               return other->c.resume();
            });
#endif
}

inline
push_coroutine< void >::control_block::control_block( pull_coroutine< void >::control_block * cb,
                                                      boost::context::continuation & c_) noexcept :
    c{ std::move( c_) },
    other{ cb },
    state{ state_t::none },
    except{} {
}

inline
void
push_coroutine< void >::control_block::deallocate() noexcept {
    if ( state_t::none != ( state & state_t::unwind) ) {
        destroy( this);
    }
}

inline
void
push_coroutine< void >::control_block::resume() {
    c = c.resume();
    if ( except) {
        std::rethrow_exception( except);
    }
}

inline
bool
push_coroutine< void >::control_block::valid() const noexcept {
    return state_t::none == ( state & state_t::complete );
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES2_DETAIL_PUSH_CONTROL_BLOCK_IPP
