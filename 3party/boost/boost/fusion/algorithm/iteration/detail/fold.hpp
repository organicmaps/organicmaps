/*=============================================================================
    Copyright (c) 2001-2006 Joel de Guzman
    Copyright (c) 2006 Dan Marsden
    Copyright (c) 2009-2010 Christopher Schmidt

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_ALGORITHM_ITERATION_DETAIL_FOLD_HPP
#define BOOST_FUSION_ALGORITHM_ITERATION_DETAIL_FOLD_HPP

#include <boost/config.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/end.hpp>
#include <boost/fusion/sequence/intrinsic/empty.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/iterator/equal_to.hpp>
#include <boost/fusion/iterator/deref.hpp>
#include <boost/fusion/iterator/value_of.hpp>
#include <boost/fusion/iterator/prior.hpp>
#include <boost/fusion/iterator/next.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/add_reference.hpp>

#endif

#ifdef BOOST_FUSION_REVERSE_FOLD
#   ifdef BOOST_FUSION_ITER_FOLD
#       define BOOST_FUSION_FOLD_NAME reverse_iter_fold
#   else
#       define BOOST_FUSION_FOLD_NAME reverse_fold
#   endif

#   define BOOST_FUSION_FOLD_IMPL_FIRST_IT_FUNCTION end
#   define BOOST_FUSION_FOLD_IMPL_NEXT_IT_FUNCTION prior
#   define BOOST_FUSION_FOLD_IMPL_FIRST_IT_META_TRANSFORM(IT)                   \
        typename fusion::result_of::prior<IT>::type
#   define BOOST_FUSION_FOLD_IMPL_FIRST_IT_TRANSFORM(IT) fusion::prior(IT)
#else
#   ifdef BOOST_FUSION_ITER_FOLD
#       define BOOST_FUSION_FOLD_NAME iter_fold
#   else
#       define BOOST_FUSION_FOLD_NAME fold
#   endif

#   define BOOST_FUSION_FOLD_IMPL_FIRST_IT_FUNCTION begin
#   define BOOST_FUSION_FOLD_IMPL_NEXT_IT_FUNCTION next
#   define BOOST_FUSION_FOLD_IMPL_FIRST_IT_META_TRANSFORM(IT) IT
#   define BOOST_FUSION_FOLD_IMPL_FIRST_IT_TRANSFORM(IT) IT
#endif
#ifdef BOOST_FUSION_ITER_FOLD
#   define BOOST_FUSION_FOLD_IMPL_INVOKE_IT_META_TRANSFORM(IT) IT&
#   define BOOST_FUSION_FOLD_IMPL_INVOKE_IT_TRANSFORM(IT) IT
#else
#   define BOOST_FUSION_FOLD_IMPL_INVOKE_IT_META_TRANSFORM(IT)                  \
        typename fusion::result_of::deref<IT>::type
#   define BOOST_FUSION_FOLD_IMPL_INVOKE_IT_TRANSFORM(IT) fusion::deref(IT)
#endif

namespace boost { namespace fusion
{
    namespace detail
    {
        template<typename State, typename It, typename F>
        struct BOOST_PP_CAT(BOOST_FUSION_FOLD_NAME, _rvalue_state)
          : boost::result_of<
                F(
                typename add_reference<typename add_const<State>::type>::type,
                BOOST_FUSION_FOLD_IMPL_INVOKE_IT_META_TRANSFORM(It))
            >
        {};

        template<typename Result,int N>
        struct BOOST_PP_CAT(unrolled_,BOOST_FUSION_FOLD_NAME)
        {
            template<typename State, typename It0, typename F>
            static Result
            call(State const& state,It0 const& it0,F f)
            {
                typedef typename
                    result_of::BOOST_FUSION_FOLD_IMPL_NEXT_IT_FUNCTION<
                        It0 const
                    >::type
                It1;
                It1 it1 = fusion::BOOST_FUSION_FOLD_IMPL_NEXT_IT_FUNCTION(it0);
                typedef typename
                    result_of::BOOST_FUSION_FOLD_IMPL_NEXT_IT_FUNCTION<
                        It1
                    >::type
                It2;
                It2 it2 = fusion::BOOST_FUSION_FOLD_IMPL_NEXT_IT_FUNCTION(it1);
                typedef typename
                    result_of::BOOST_FUSION_FOLD_IMPL_NEXT_IT_FUNCTION<
                        It2
                    >::type
                It3;
                It3 it3 = fusion::BOOST_FUSION_FOLD_IMPL_NEXT_IT_FUNCTION(it2);

                return BOOST_PP_CAT(unrolled_,BOOST_FUSION_FOLD_NAME)<
                    Result
                  , N-4
                >::call(
                    f(
                        f(
                            f(
                                f(
                                    state,
                                    BOOST_FUSION_FOLD_IMPL_INVOKE_IT_TRANSFORM(
                                        it0)
                                ),
                                BOOST_FUSION_FOLD_IMPL_INVOKE_IT_TRANSFORM(it1)
                            ),
                            BOOST_FUSION_FOLD_IMPL_INVOKE_IT_TRANSFORM(it2)
                        ),
                        BOOST_FUSION_FOLD_IMPL_INVOKE_IT_TRANSFORM(it3)
                    ),
                    fusion::BOOST_FUSION_FOLD_IMPL_NEXT_IT_FUNCTION(it3),
                    f);
            }
        };

        template<typename Result>
        struct BOOST_PP_CAT(unrolled_,BOOST_FUSION_FOLD_NAME)<Result,3>
        {
            template<typename State, typename It0, typename F>
            static Result
            call(State const& state,It0 const& it0,F f)
            {
                typedef typename
                    result_of::BOOST_FUSION_FOLD_IMPL_NEXT_IT_FUNCTION<
                        It0 const
                    >::type
                It1;
                It1 it1 = fusion::BOOST_FUSION_FOLD_IMPL_NEXT_IT_FUNCTION(it0);
                typedef typename
                    result_of::BOOST_FUSION_FOLD_IMPL_NEXT_IT_FUNCTION<
                        It1
                    >::type
                It2;
                It2 it2 = fusion::BOOST_FUSION_FOLD_IMPL_NEXT_IT_FUNCTION(it1);

                return f(
                        f(
                            f(
                                state,
                                BOOST_FUSION_FOLD_IMPL_INVOKE_IT_TRANSFORM(it0)
                            ),
                            BOOST_FUSION_FOLD_IMPL_INVOKE_IT_TRANSFORM(it1)
                        ),
                        BOOST_FUSION_FOLD_IMPL_INVOKE_IT_TRANSFORM(
                            fusion::BOOST_FUSION_FOLD_IMPL_NEXT_IT_FUNCTION(it1)
                        ));
            }
        };

        template<typename Result>
        struct BOOST_PP_CAT(unrolled_,BOOST_FUSION_FOLD_NAME)<Result,2>
        {
            template<typename State, typename It0, typename F>
            static Result
            call(State const& state,It0 const& it0,F f)
            {
                return f(
                    f(state,
                        BOOST_FUSION_FOLD_IMPL_INVOKE_IT_TRANSFORM(it0)),
                    BOOST_FUSION_FOLD_IMPL_INVOKE_IT_TRANSFORM(
                        fusion::BOOST_FUSION_FOLD_IMPL_NEXT_IT_FUNCTION(it0)));
            }
        };

        template<typename Result>
        struct BOOST_PP_CAT(unrolled_,BOOST_FUSION_FOLD_NAME)<Result,1>
        {
            template<typename State, typename It0, typename F>
            static Result
            call(State const& state,It0 const& it0,F f)
            {
                return f(state,
                    BOOST_FUSION_FOLD_IMPL_INVOKE_IT_TRANSFORM(it0));
            }
        };

        template<typename Result>
        struct BOOST_PP_CAT(unrolled_,BOOST_FUSION_FOLD_NAME)<Result,0>
        {
            template<typename State, typename It0, typename F>
            static Result
            call(State const& state,It0 const& it0,F f)
            {
                return static_cast<Result>(state);
            }
        };

        template<typename StateRef, typename It0, typename F, int N>
        struct BOOST_PP_CAT(result_of_unrolled_,BOOST_FUSION_FOLD_NAME)
        {
            typedef typename
                BOOST_PP_CAT(BOOST_FUSION_FOLD_NAME, _rvalue_state)<
                    StateRef
                  , It0 const
                  , F
                >::type
            rest1;
            typedef typename
                result_of::BOOST_FUSION_FOLD_IMPL_NEXT_IT_FUNCTION<
                    It0 const
                >::type
            it1;
            typedef typename
                BOOST_PP_CAT(BOOST_FUSION_FOLD_NAME, _rvalue_state)<
                    rest1
                  , it1
                  , F
                >::type
            rest2;
            typedef typename
                result_of::BOOST_FUSION_FOLD_IMPL_NEXT_IT_FUNCTION<it1>::type
            it2;
            typedef typename
                BOOST_PP_CAT(BOOST_FUSION_FOLD_NAME, _rvalue_state)<
                    rest2
                  , it2
                  , F
                >::type
            rest3;
            typedef typename
                result_of::BOOST_FUSION_FOLD_IMPL_NEXT_IT_FUNCTION<it2>::type
            it3;

            typedef typename
                BOOST_PP_CAT(result_of_unrolled_,BOOST_FUSION_FOLD_NAME)<
                    typename BOOST_PP_CAT(
                        BOOST_FUSION_FOLD_NAME, _rvalue_state)<
                        rest3
                      , it3
                      , F
                    >::type
                  , typename result_of::BOOST_FUSION_FOLD_IMPL_NEXT_IT_FUNCTION<
                        it3
                    >::type
                  , F
                  , N-4
                >::type
            type;
        };

        template<typename StateRef, typename It0, typename F>
        struct BOOST_PP_CAT(result_of_unrolled_,BOOST_FUSION_FOLD_NAME)<
            StateRef
          , It0
          , F
          , 3
        >
        {
            typedef typename
                BOOST_PP_CAT(BOOST_FUSION_FOLD_NAME, _rvalue_state)<
                    StateRef
                  , It0 const
                  , F
                >::type
            rest1;
            typedef typename
                result_of::BOOST_FUSION_FOLD_IMPL_NEXT_IT_FUNCTION<
                    It0 const
                >::type
            it1;

            typedef typename
                BOOST_PP_CAT(BOOST_FUSION_FOLD_NAME, _rvalue_state)<
                    typename BOOST_PP_CAT(
                        BOOST_FUSION_FOLD_NAME, _rvalue_state)<
                        rest1
                      , it1
                      , F
                    >::type
                  , typename result_of::BOOST_FUSION_FOLD_IMPL_NEXT_IT_FUNCTION<
                        it1 const
                    >::type const
                  , F
                >::type
            type;
        };

        template<typename StateRef, typename It0, typename F>
        struct BOOST_PP_CAT(result_of_unrolled_,BOOST_FUSION_FOLD_NAME)<
            StateRef
          , It0
          , F
          , 2
        >
          : BOOST_PP_CAT(BOOST_FUSION_FOLD_NAME, _rvalue_state)<
                typename BOOST_PP_CAT(BOOST_FUSION_FOLD_NAME, _rvalue_state)<
                    StateRef
                  , It0 const
                  , F
                >::type
              , typename result_of::BOOST_FUSION_FOLD_IMPL_NEXT_IT_FUNCTION<
                    It0 const
                >::type const
              , F
            >
        {};

        template<typename StateRef, typename It0, typename F>
        struct BOOST_PP_CAT(result_of_unrolled_,BOOST_FUSION_FOLD_NAME)<
            StateRef
          , It0
          , F
          , 1
        >
          : BOOST_PP_CAT(BOOST_FUSION_FOLD_NAME, _rvalue_state)<
                StateRef
              , It0 const
              , F
            >
        {};

        template<typename StateRef, typename It0, typename F>
        struct BOOST_PP_CAT(result_of_unrolled_,BOOST_FUSION_FOLD_NAME)<
            StateRef
          , It0
          , F
          , 0
        >
        {
            typedef StateRef type;
        };

        template<typename StateRef, typename It0, typename F, int SeqSize>
        struct BOOST_PP_CAT(result_of_first_unrolled,BOOST_FUSION_FOLD_NAME)
        {
            typedef typename
                BOOST_PP_CAT(result_of_unrolled_,BOOST_FUSION_FOLD_NAME)<
                    typename boost::result_of<
                        F(
                            StateRef,
                            BOOST_FUSION_FOLD_IMPL_INVOKE_IT_META_TRANSFORM(
                                It0 const)
                        )
                    >::type
                  , typename result_of::BOOST_FUSION_FOLD_IMPL_NEXT_IT_FUNCTION<
                        It0 const
                    >::type
                  , F
                  , SeqSize-1
                >::type
            type;
        };

        template<int SeqSize, typename StateRef, typename It0, typename F>
        struct BOOST_PP_CAT(BOOST_FUSION_FOLD_NAME,_impl)
        {
            typedef typename
                BOOST_PP_CAT(
                    result_of_first_unrolled,BOOST_FUSION_FOLD_NAME)<
                    StateRef
                  , BOOST_FUSION_FOLD_IMPL_FIRST_IT_META_TRANSFORM(It0)
                  , F
                  , SeqSize
                >::type
            type;

            static type
            call(StateRef state, It0 const& it0, F f)
            {
                return BOOST_PP_CAT(unrolled_,BOOST_FUSION_FOLD_NAME)<
                    type
                  , SeqSize
                >::call(state,BOOST_FUSION_FOLD_IMPL_FIRST_IT_TRANSFORM(it0),f);
            }
        };

        template<typename StateRef, typename It0, typename F>
        struct BOOST_PP_CAT(BOOST_FUSION_FOLD_NAME,_impl)<0,StateRef,It0,F>
        {
            typedef StateRef type;

            static StateRef
            call(StateRef state, It0 const&, F)
            {
                return static_cast<StateRef>(state);
            }
        };
    }

    namespace result_of
    {
        template<typename Seq, typename State, typename F>
        struct BOOST_FUSION_FOLD_NAME
          : detail::BOOST_PP_CAT(BOOST_FUSION_FOLD_NAME,_impl)<
                size<Seq>::value
              , typename add_reference<
                    typename add_const<State>::type
                >::type
              , typename BOOST_FUSION_FOLD_IMPL_FIRST_IT_FUNCTION<Seq>::type
              , F
            >
        {};
    }

    template<typename Seq, typename State, typename F>
    inline typename result_of::fold<Seq,State const,F>::type
    BOOST_FUSION_FOLD_NAME(Seq& seq,State const& state,F f)
    {
        return result_of::BOOST_FUSION_FOLD_NAME<Seq,State const,F>::call(
            state,
            fusion::BOOST_FUSION_FOLD_IMPL_FIRST_IT_FUNCTION(seq),
            f);
    }

    template<typename Seq, typename State, typename F>
    inline typename result_of::fold<Seq const,State const,F>::type
    BOOST_FUSION_FOLD_NAME(Seq const& seq,State const& state,F f)
    {
        return result_of::BOOST_FUSION_FOLD_NAME<Seq const,State const,F>::call(
            state,
            fusion::BOOST_FUSION_FOLD_IMPL_FIRST_IT_FUNCTION(seq),
            f);
    }
}}

#undef BOOST_FUSION_FOLD_NAME
#undef BOOST_FUSION_FOLD_IMPL_FIRST_IT_FUNCTION
#undef BOOST_FUSION_FOLD_IMPL_NEXT_IT_FUNCTION
#undef BOOST_FUSION_FOLD_IMPL_FIRST_IT_META_TRANSFORM
#undef BOOST_FUSION_FOLD_IMPL_FIRST_IT_TRANSFORM
#undef BOOST_FUSION_FOLD_IMPL_INVOKE_IT_META_TRANSFORM
#undef BOOST_FUSION_FOLD_IMPL_INVOKE_IT_TRANSFORM
