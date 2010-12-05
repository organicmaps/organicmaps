///////////////////////////////////////////////////////////////////////////////
// funop.hpp
// Contains definition of funop[n]\<\> class template.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PP_IS_ITERATING
#error Do not include this file directly
#endif

#define M0(Z, N, DATA)                                                                          \
    typename proto::result_of::as_child<BOOST_PP_CAT(A, N), Domain>::type                       \
    /**/

#define M1(Z, N, DATA)                                                                          \
    proto::as_child<Domain>(BOOST_PP_CAT(a, N))                                                 \
    /**/

    /// \brief A helper metafunction for computing the
    /// return type of \c proto::expr\<\>::operator().
    template<typename Expr, typename Domain BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(), typename A)>
    struct BOOST_PP_CAT(funop, BOOST_PP_ITERATION())
    {
        typedef proto::expr<
            tag::function
          , BOOST_PP_CAT(list, BOOST_PP_INC(BOOST_PP_ITERATION()))<
                Expr &
                BOOST_PP_ENUM_TRAILING(BOOST_PP_ITERATION(), M0, ~)
            >
          , BOOST_PP_INC(BOOST_PP_ITERATION())
        > type;

        static type const call(
            Expr &e
            BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(BOOST_PP_ITERATION(), A, &a)
        )
        {
            type that = {
                e
                BOOST_PP_ENUM_TRAILING(BOOST_PP_ITERATION(), M1, ~)
            };
            return that;
        }
    };

    /// \brief A helper metafunction for computing the
    /// return type of \c proto::expr\<\>::operator().
    template<typename Expr BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(), typename A), typename This, typename Domain>
    struct funop<Expr(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A)), This, Domain>
      : BOOST_PP_CAT(funop, BOOST_PP_ITERATION())<
            typename detail::same_cv<Expr, This>::type
          , Domain
            BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(
                BOOST_PP_ITERATION()
              , typename remove_reference<A
              , >::type BOOST_PP_INTERCEPT
            )
        >
    {};

#undef M0
#undef M1
