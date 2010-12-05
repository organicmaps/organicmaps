/*=============================================================================
    Copyright (c) 2001-2006 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PP_IS_ITERATING
#if !defined(FUSION_MAKE_SET_09162005_1125)
#define FUSION_MAKE_SET_09162005_1125

#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/fusion/container/set/set.hpp>
#include <boost/fusion/support/detail/as_fusion_element.hpp>
#include <boost/fusion/support/pair.hpp>

namespace boost { namespace fusion
{
    struct void_;
        
    namespace result_of
    {
        template <
            BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(
                FUSION_MAX_VECTOR_SIZE, typename T, void_)
          , typename Extra = void_
        >
        struct make_set;
            
        template <>
        struct make_set<>
        {
            typedef set<> type;
        };
    }
    
    inline set<>
    make_set()
    {
        return set<>();
    }

#define BOOST_FUSION_AS_FUSION_ELEMENT(z, n, data)                               \
    typename detail::as_fusion_element<BOOST_PP_CAT(T, n)>::type

#define BOOST_PP_FILENAME_1 <boost/fusion/container/generation/make_set.hpp>
#define BOOST_PP_ITERATION_LIMITS (1, FUSION_MAX_VECTOR_SIZE)
#include BOOST_PP_ITERATE()

#undef BOOST_FUSION_ELEMENT
#undef BOOST_FUSION_AS_ELEMENT

}}

#endif
#else // defined(BOOST_PP_IS_ITERATING)
///////////////////////////////////////////////////////////////////////////////
//
//  Preprocessor vertical repetition code
//
///////////////////////////////////////////////////////////////////////////////

#define N BOOST_PP_ITERATION()

    namespace result_of
    {
        template <BOOST_PP_ENUM_PARAMS(N, typename T)>
#if defined(BOOST_NO_PARTIAL_SPECIALIZATION_IMPLICIT_DEFAULT_ARGS)
        #define TEXT(z, n, text) , text
        struct make_set< BOOST_PP_ENUM_PARAMS(N, T) BOOST_PP_REPEAT_FROM_TO(BOOST_PP_DEC(N), FUSION_MAX_SET_SIZE, TEXT, void_) >
        #undef TEXT
#else
        struct make_set<BOOST_PP_ENUM_PARAMS(N, T)>
#endif
        {
            typedef set<BOOST_PP_ENUM(N, BOOST_FUSION_AS_FUSION_ELEMENT, _)> type;
        };
    }

    template <BOOST_PP_ENUM_PARAMS(N, typename T)>
    inline set<BOOST_PP_ENUM(N, BOOST_FUSION_AS_FUSION_ELEMENT, _)>
    make_set(BOOST_PP_ENUM_BINARY_PARAMS(N, T, const& _))
    {
        return set<BOOST_PP_ENUM(N, BOOST_FUSION_AS_FUSION_ELEMENT, _)>(
            BOOST_PP_ENUM_PARAMS(N, _));
    }

#undef N
#endif // defined(BOOST_PP_IS_ITERATING)

