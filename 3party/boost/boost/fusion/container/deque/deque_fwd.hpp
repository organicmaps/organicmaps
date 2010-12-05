/*=============================================================================
    Copyright (c) 2005-2007 Joel de Guzman
    Copyright (c) 2005-2007 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_DEQUE_FORWARD_02092007_0749)
#define FUSION_DEQUE_FORWARD_02092007_0749

#include <boost/fusion/container/deque/limits.hpp>
#include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>

namespace boost { namespace fusion
{
    struct void_;

    template<
        BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(
            FUSION_MAX_DEQUE_SIZE, typename T, void_)>
    struct deque;
}}

#endif
