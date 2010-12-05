/*=============================================================================
    Copyright (c) 2001-2006 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_PP_IS_ITERATING)
#if !defined(BOOST_FUSION_SEQUENCE_DEQUE_DETAIL_DEQUE_FORWARD_CTOR_04122006_2212)
#define BOOST_FUSION_SEQUENCE_DEQUE_DETAIL_DEQUE_FORWARD_CTOR_04122006_2212

#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum_shifted_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>

#define BOOST_PP_FILENAME_1 \
    <boost/fusion/container/deque/detail/deque_forward_ctor.hpp>
#define BOOST_PP_ITERATION_LIMITS (2, FUSION_MAX_DEQUE_SIZE)
#include BOOST_PP_ITERATE()

#endif
#else

#define N BOOST_PP_ITERATION()

deque(BOOST_PP_ENUM_BINARY_PARAMS(N, typename add_reference<typename add_const<T, >::type>::type t))
    : base(detail::deque_keyed_values<BOOST_PP_ENUM_PARAMS(N, T)>::call(BOOST_PP_ENUM_PARAMS(N, t)))
{}

#undef N
#endif
