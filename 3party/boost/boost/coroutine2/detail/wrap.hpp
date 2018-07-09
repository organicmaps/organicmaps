
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINE2_DETAIL_WRAP_H
#define BOOST_COROUTINE2_DETAIL_WRAP_H

#include <type_traits>

#include <boost/config.hpp>
#include <boost/context/detail/invoke.hpp>
#if (BOOST_EXECUTION_CONTEXT==1)
# include <boost/context/execution_context.hpp>
#else
# include <boost/context/continuation.hpp>
#endif

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/data.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines2 {
namespace detail {

#if (BOOST_EXECUTION_CONTEXT==1)
template< typename Fn1, typename Fn2  >
class wrapper {
private:
    typename std::decay< Fn1 >::type    fn1_;
    typename std::decay< Fn2 >::type    fn2_;
    boost::context::execution_context   ctx_;

public:
    wrapper( Fn1 && fn1, Fn2 && fn2,
             boost::context::execution_context const& ctx) :
        fn1_( std::move( fn1) ),
        fn2_( std::move( fn2) ),
        ctx_{ ctx } {
    }

    wrapper( wrapper const&) = delete;
    wrapper & operator=( wrapper const&) = delete;

    wrapper( wrapper && other) = default;
    wrapper & operator=( wrapper && other) = default;

    void operator()( void * vp) {
        boost::context::detail::invoke(
                std::move( fn1_),
                fn2_, ctx_, vp);
    }
};

template< typename Fn1, typename Fn2  >
wrapper< Fn1, Fn2 >
wrap( Fn1 && fn1, Fn2 && fn2,
      boost::context::execution_context const& ctx) {
    return wrapper< Fn1, Fn2 >(
            std::forward< Fn1 >( fn1),
            std::forward< Fn2 >( fn2),
            ctx);
}
#else
template< typename Fn1, typename Fn2  >
class wrapper {
private:
    typename std::decay< Fn1 >::type    fn1_;
    typename std::decay< Fn2 >::type    fn2_;

public:
    wrapper( Fn1 && fn1, Fn2 && fn2) :
        fn1_( std::move( fn1) ),
        fn2_( std::move( fn2) ) {
    }

    wrapper( wrapper const&) = delete;
    wrapper & operator=( wrapper const&) = delete;

    wrapper( wrapper && other) = default;
    wrapper & operator=( wrapper && other) = default;

    boost::context::continuation
    operator()( boost::context::continuation && c) {
        return boost::context::detail::invoke(
                std::move( fn1_),
                fn2_,
                std::forward< boost::context::continuation >( c) );
    }
};

template< typename Fn1, typename Fn2 >
wrapper< Fn1, Fn2 >
wrap( Fn1 && fn1, Fn2 && fn2) {
    return wrapper< Fn1, Fn2 >(
            std::forward< Fn1 >( fn1),
            std::forward< Fn2 >( fn2) );
}
#endif

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINE2_DETAIL_WRAP_H
