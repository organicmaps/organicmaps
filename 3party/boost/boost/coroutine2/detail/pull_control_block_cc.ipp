
//          Copyright Oliver Kowalke 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES2_DETAIL_PULL_CONTROL_BLOCK_IPP
#define BOOST_COROUTINES2_DETAIL_PULL_CONTROL_BLOCK_IPP

#include <algorithm>
#include <exception>
#include <memory>
#include <tuple>

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

// pull_coroutine< T >

template< typename T >
void
pull_coroutine< T >::control_block::destroy( control_block * cb) noexcept {
    boost::context::continuation c = std::move( cb->c);
    // destroy control structure
    cb->~control_block();
    // destroy coroutine's stack
    cb->state |= state_t::destroy;
    c.resume();
}

template< typename T >
template< typename StackAllocator, typename Fn >
pull_coroutine< T >::control_block::control_block( context::preallocated palloc, StackAllocator salloc,
                                                   Fn && fn) :
    c{},
    other{ nullptr },
    state{ state_t::unwind },
    except{},
    bvalid{ false },
    storage{} {
#if defined(BOOST_NO_CXX14_GENERIC_LAMBDAS)
    c = boost::context::callcc(
            std::allocator_arg, palloc, salloc,
            wrap( [this](typename std::decay< Fn >::type & fn_,boost::context::continuation && c) mutable {
                    // create synthesized push_coroutine< T >
                    typename push_coroutine< T >::control_block synthesized_cb{ this, c };
                    push_coroutine< T > synthesized{ & synthesized_cb };
                    other = & synthesized_cb;
                    if ( state_t::none == ( state & state_t::destroy) ) {
                        try {
                            auto fn = std::move( fn_);
                            // call coroutine-fn with synthesized push_coroutine as argument
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
               // create synthesized push_coroutine< T >
               typename push_coroutine< T >::control_block synthesized_cb{ this, c };
               push_coroutine< T > synthesized{ & synthesized_cb };
               other = & synthesized_cb;
               if ( state_t::none == ( state & state_t::destroy) ) {
                   try {
                       auto fn = std::move( fn_);
                       // call coroutine-fn with synthesized push_coroutine as argument
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
    if ( c.data_available() ) {
        set( c.get_data< T >() );
    }
}

template< typename T >
pull_coroutine< T >::control_block::control_block( typename push_coroutine< T >::control_block * cb,
                                                   boost::context::continuation & c_) noexcept :
    c{ std::move( c_) },
    other{ cb },
    state{ state_t::none },
    except{},
    bvalid{ false },
    storage{} {
}

template< typename T >
pull_coroutine< T >::control_block::~control_block() {
    // destroy data if set
    if ( bvalid) {
        reinterpret_cast< T * >( std::addressof( storage) )->~T();
    }
}

template< typename T >
void
pull_coroutine< T >::control_block::deallocate() noexcept {
    if ( state_t::none != ( state & state_t::unwind) ) {
        destroy( this);
    }
}

template< typename T >
void
pull_coroutine< T >::control_block::resume() {
    c = c.resume();
    if ( c.data_available() ) {
        set( c.get_data< T >() );
    } else {
        reset();
    }
    if ( except) {
        std::rethrow_exception( except);
    }
}

template< typename T >
void
pull_coroutine< T >::control_block::set( T const& t) {
    // destroy data if set
    if ( bvalid) {
        reinterpret_cast< T * >( std::addressof( storage) )->~T();
    }
    ::new ( static_cast< void * >( std::addressof( storage) ) ) T( t);
    bvalid = true;
}

template< typename T >
void
pull_coroutine< T >::control_block::set( T && t) {
    // destroy data if set
    if ( bvalid) {
        reinterpret_cast< T * >( std::addressof( storage) )->~T();
    }
    ::new ( static_cast< void * >( std::addressof( storage) ) ) T( std::move( t) );
    bvalid = true;
}

template< typename T >
void
pull_coroutine< T >::control_block::reset() {
    // destroy data if set
    if ( bvalid) {
        reinterpret_cast< T * >( std::addressof( storage) )->~T();
    }
    bvalid = false;
}

template< typename T >
T &
pull_coroutine< T >::control_block::get() noexcept {
    return * reinterpret_cast< T * >( std::addressof( storage) );
}

template< typename T >
bool
pull_coroutine< T >::control_block::valid() const noexcept {
    return nullptr != other && state_t::none == ( state & state_t::complete) && bvalid;
}


// pull_coroutine< T & >

template< typename T >
void
pull_coroutine< T & >::control_block::destroy( control_block * cb) noexcept {
    boost::context::continuation c = std::move( cb->c);
    // destroy control structure
    cb->~control_block();
    // destroy coroutine's stack
    cb->state |= state_t::destroy;
    c.resume();
}

template< typename T >
template< typename StackAllocator, typename Fn >
pull_coroutine< T & >::control_block::control_block( context::preallocated palloc, StackAllocator salloc,
                                                     Fn && fn) :
    c{},
    other{ nullptr },
    state{ state_t::unwind },
    except{},
    bvalid{ false },
    storage{} {
#if defined(BOOST_NO_CXX14_GENERIC_LAMBDAS)
    c = boost::context::callcc(
            std::allocator_arg, palloc, salloc,
            wrap( [this](typename std::decay< Fn >::type & fn_,boost::context::continuation && c) mutable {
                    // create synthesized push_coroutine< T & >
                    typename push_coroutine< T & >::control_block synthesized_cb{ this, c };
                    push_coroutine< T & > synthesized{ & synthesized_cb };
                    other = & synthesized_cb;
                    if ( state_t::none == ( state & state_t::destroy) ) {
                        try {
                            auto fn = std::move( fn_);
                            // call coroutine-fn with synthesized push_coroutine as argument
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
               // create synthesized push_coroutine< T & >
               typename push_coroutine< T & >::control_block synthesized_cb{ this, c };
               push_coroutine< T & > synthesized{ & synthesized_cb };
               other = & synthesized_cb;
               if ( state_t::none == ( state & state_t::destroy) ) {
                   try {
                       auto fn = std::move( fn_);
                       // call coroutine-fn with synthesized push_coroutine as argument
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
    if ( c.data_available() ) {
        set( c.get_data< T & >() );
    }
}

template< typename T >
pull_coroutine< T & >::control_block::control_block( typename push_coroutine< T & >::control_block * cb,
                                                     boost::context::continuation & c_) noexcept :
    c{ std::move( c_) },
    other{ cb },
    state{ state_t::none },
    except{},
    bvalid{ false },
    storage{} {
}

template< typename T >
void
pull_coroutine< T & >::control_block::deallocate() noexcept {
    if ( state_t::none != ( state & state_t::unwind) ) {
        destroy( this);
    }
}

template< typename T >
void
pull_coroutine< T & >::control_block::resume() {
    c = c.resume();
    if ( c.data_available() ) {
        set( c.get_data< T & >() );
    } else {
        reset();
    }
    if ( except) {
        std::rethrow_exception( except);
    }
}

template< typename T >
void
pull_coroutine< T & >::control_block::set( T & t) {
    ::new ( static_cast< void * >( std::addressof( storage) ) ) holder{ t };
    bvalid = true;
}

template< typename T >
void
pull_coroutine< T & >::control_block::reset() {
    if ( bvalid) {
        reinterpret_cast< holder * >( std::addressof( storage) )->~holder();
    }
    bvalid = false;
}

template< typename T >
T &
pull_coroutine< T & >::control_block::get() noexcept {
    return reinterpret_cast< holder * >( std::addressof( storage) )->t;
}

template< typename T >
bool
pull_coroutine< T & >::control_block::valid() const noexcept {
    return nullptr != other && state_t::none == ( state & state_t::complete) && bvalid;
}


// pull_coroutine< void >

inline
void
pull_coroutine< void >::control_block::destroy( control_block * cb) noexcept {
    boost::context::continuation c = std::move( cb->c);
    // destroy control structure
    cb->~control_block();
    // destroy coroutine's stack
    cb->state |= state_t::destroy;
    c.resume();
}

template< typename StackAllocator, typename Fn >
pull_coroutine< void >::control_block::control_block( context::preallocated palloc, StackAllocator salloc,
                                                      Fn && fn) :
    c{},
    other{ nullptr },
    state{ state_t::unwind },
    except{} {
#if defined(BOOST_NO_CXX14_GENERIC_LAMBDAS)
    c = boost::context::callcc(
            std::allocator_arg, palloc, salloc,
            wrap( [this](typename std::decay< Fn >::type & fn_,boost::context::continuation && c) mutable {
                    // create synthesized push_coroutine< void >
                    typename push_coroutine< void >::control_block synthesized_cb{ this, c };
                    push_coroutine< void > synthesized{ & synthesized_cb };
                    other = & synthesized_cb;
                    if ( state_t::none == ( state & state_t::destroy) ) {
                        try {
                            auto fn = std::move( fn_);
                            // call coroutine-fn with synthesized push_coroutine as argument
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
            [this,fn_=std::forward< Fn >( fn)]( boost::context::continuation && c) mutable {
               // create synthesized push_coroutine< void >
               typename push_coroutine< void >::control_block synthesized_cb{ this, c };
               push_coroutine< void > synthesized{ & synthesized_cb };
               other = & synthesized_cb;
               if ( state_t::none == ( state & state_t::destroy) ) {
                   try {
                       auto fn = std::move( fn_);
                       // call coroutine-fn with synthesized push_coroutine as argument
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
               // jump back to ctx
               return other->c.resume();
            });
#endif
}

inline
pull_coroutine< void >::control_block::control_block( push_coroutine< void >::control_block * cb,
                                                      boost::context::continuation & c_) noexcept :
    c{ std::move( c_) },
    other{ cb },
    state{ state_t::none },
    except{} {
}

inline
void
pull_coroutine< void >::control_block::deallocate() noexcept {
    if ( state_t::none != ( state & state_t::unwind) ) {
        destroy( this);
    }
}

inline
void
pull_coroutine< void >::control_block::resume() {
    c = c.resume();
    if ( except) {
        std::rethrow_exception( except);
    }
}

inline
bool
pull_coroutine< void >::control_block::valid() const noexcept {
    return nullptr != other && state_t::none == ( state & state_t::complete);
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES2_DETAIL_PULL_CONTROL_BLOCK_IPP
