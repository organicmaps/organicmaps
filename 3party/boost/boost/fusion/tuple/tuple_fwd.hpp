/*=============================================================================
    Copyright (c) 2005 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_TUPLE_FORWARD_10032005_0956)
#define FUSION_TUPLE_FORWARD_10032005_0956

#include <boost/fusion/container/vector/limits.hpp>
#include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>

namespace boost { namespace fusion
{
    struct void_;

    template <
        BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(
            FUSION_MAX_VECTOR_SIZE, typename T, void_)
    >
    struct tuple;
}}

#endif
