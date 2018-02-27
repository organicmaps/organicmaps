
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBER_DETAIL_WRAP_H
#define BOOST_FIBER_DETAIL_WRAP_H

#include <type_traits>

#include <boost/config.hpp>
#if defined(BOOST_NO_CXX17_STD_INVOKE)
#include <boost/context/detail/invoke.hpp>
#endif
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
namespace fibers {
namespace detail {

#if (BOOST_EXECUTION_CONTEXT==1)
template< typename Fn1, typename Fn2, typename Tpl  >
class wrapper {
private:
    typename std::decay< Fn1 >::type    fn1_;
    typename std::decay< Fn2 >::type    fn2_;
    typename std::decay< Tpl >::type    tpl_;
    boost::context::execution_context   ctx_;

public:
    wrapper( Fn1 && fn1, Fn2 && fn2, Tpl && tpl,
             boost::context::execution_context const& ctx) :
        fn1_{ std::move( fn1) },
        fn2_{ std::move( fn2) },
        tpl_{ std::move( tpl) },
        ctx_{ ctx } {
    }

    wrapper( wrapper const&) = delete;
    wrapper & operator=( wrapper const&) = delete;

    wrapper( wrapper && other) = default;
    wrapper & operator=( wrapper && other) = default;

    void operator()( void * vp) {
#if defined(BOOST_NO_CXX17_STD_INVOKE)
        boost::context::detail::invoke( std::move( fn1_), fn2_, tpl_, ctx_, vp);
#else
        std::invoke( std::move( fn1_), fn2_, tpl_, ctx_, vp);
#endif
    }
};

template< typename Fn1, typename Fn2, typename Tpl  >
wrapper< Fn1, Fn2, Tpl >
wrap( Fn1 && fn1, Fn2 && fn2, Tpl && tpl,
      boost::context::execution_context const& ctx) {
    return wrapper< Fn1, Fn2, Tpl >{
            std::forward< Fn1 >( fn1),
            std::forward< Fn2 >( fn2),
            std::forward< Tpl >( tpl),
            ctx };
}
#else
template< typename Fn1, typename Fn2, typename Tpl  >
class wrapper {
private:
    typename std::decay< Fn1 >::type    fn1_;
    typename std::decay< Fn2 >::type    fn2_;
    typename std::decay< Tpl >::type    tpl_;

public:
    wrapper( Fn1 && fn1, Fn2 && fn2, Tpl && tpl) :
        fn1_{ std::move( fn1) },
        fn2_{ std::move( fn2) },
        tpl_{ std::move( tpl) } {
    }

    wrapper( wrapper const&) = delete;
    wrapper & operator=( wrapper const&) = delete;

    wrapper( wrapper && other) = default;
    wrapper & operator=( wrapper && other) = default;

    boost::context::continuation
    operator()( boost::context::continuation && c) {
#if defined(BOOST_NO_CXX17_STD_INVOKE)
        return boost::context::detail::invoke(
                std::move( fn1_),
                fn2_,
                tpl_,
                std::forward< boost::context::continuation >( c) );
#else
        return std::invoke(
                std::move( fn1_),
                fn2_,
                tpl_,
                std::forward< boost::context::continuation >( c) );
#endif
    }
};

template< typename Fn1, typename Fn2, typename Tpl  >
wrapper< Fn1, Fn2, Tpl >
wrap( Fn1 && fn1, Fn2 && fn2, Tpl && tpl) {
    return wrapper< Fn1, Fn2, Tpl >{
            std::forward< Fn1 >( fn1),
            std::forward< Fn2 >( fn2),
            std::forward< Tpl >( tpl) };
}
#endif

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBER_DETAIL_WRAP_H
