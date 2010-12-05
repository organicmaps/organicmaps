/*=============================================================================
    Copyright (c) 2001-2006 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_PP_IS_ITERATING)
#if !defined(BOOST_FUSION_SEQUENCE_DEQUE_DETAIL_DEQUE_KEYED_VALUES_CALL_04122006_2211)
#define BOOST_FUSION_SEQUENCE_DEQUE_DETAIL_DEQUE_KEYED_VALUES_CALL_04122006_2211

#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum_shifted_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>

#define BOOST_PP_FILENAME_1 \
    <boost/fusion/container/deque/detail/deque_keyed_values_call.hpp>
#define BOOST_PP_ITERATION_LIMITS (1, FUSION_MAX_DEQUE_SIZE)
#include BOOST_PP_ITERATE()

#endif
#else

#define N BOOST_PP_ITERATION()

static type call(BOOST_PP_ENUM_BINARY_PARAMS(N, typename add_reference<typename add_const<T, >::type>::type t))
{
    return type(t0, 
                deque_keyed_values_impl<
                next_index
#if N > 1
                , BOOST_PP_ENUM_SHIFTED_PARAMS(N, T)
#endif
                >::call(BOOST_PP_ENUM_SHIFTED_PARAMS(N, t)));
}

#undef N
#endif
