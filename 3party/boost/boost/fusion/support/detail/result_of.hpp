/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_RESULT_OF_10272014_0654)
#define FUSION_RESULT_OF_10272014_0654

#include <boost/config.hpp>
#include <boost/utility/result_of.hpp>

#if !defined(BOOST_RESULT_OF_USE_DECLTYPE) || defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
#define BOOST_FUSION_NO_DECLTYPE_BASED_RESULT_OF
#endif

#if !defined(BOOST_FUSION_NO_DECLTYPE_BASED_RESULT_OF)
#include <boost/mpl/if.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/has_xxx.hpp>
#endif

namespace boost { namespace fusion { namespace detail
{
    // This is a temporary workaround for result_of before we make fusion fully
    // sfinae result_of friendy, which will require some heavy lifting for some
    // low level code. So far this is used only in the fold algorithm. This will
    // be removed once we overhaul fold.

#if defined(BOOST_FUSION_NO_DECLTYPE_BASED_RESULT_OF)

    template <typename Sig>
    struct result_of_with_decltype : boost::tr1_result_of<Sig> {};

#else // defined(BOOST_FUSION_NO_DECLTYPE_BASED_RESULT_OF)

    BOOST_MPL_HAS_XXX_TRAIT_DEF(result_type)
    BOOST_MPL_HAS_XXX_TEMPLATE_DEF(result)

    template <typename Sig>
    struct result_of_with_decltype;

    template <typename F, typename... Args>
    struct result_of_with_decltype<F(Args...)>
        : mpl::if_<mpl::or_<has_result_type<F>, detail::has_result<F> >,
            boost::tr1_result_of<F(Args...)>,
            boost::detail::cpp0x_result_of<F(Args...)> >::type {};

#endif // defined(BOOST_FUSION_NO_DECLTYPE_BASED_RESULT_OF)

}}}

#endif
