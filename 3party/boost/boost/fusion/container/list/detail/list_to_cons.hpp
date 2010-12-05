/*=============================================================================
    Copyright (c) 2005 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_LIST_TO_CONS_07172005_1041)
#define FUSION_LIST_TO_CONS_07172005_1041

#include <boost/fusion/container/list/cons.hpp>
#include <boost/fusion/container/list/limits.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_shifted_params.hpp>
#include <boost/preprocessor/arithmetic/dec.hpp>

#define FUSION_VOID(z, n, _) void_

namespace boost { namespace fusion
{
    struct nil;
    struct void_;
}}

namespace boost { namespace fusion { namespace detail
{
    template <BOOST_PP_ENUM_PARAMS(FUSION_MAX_LIST_SIZE, typename T)>
    struct list_to_cons
    {
        typedef T0 head_type;
        typedef list_to_cons<
            BOOST_PP_ENUM_SHIFTED_PARAMS(FUSION_MAX_LIST_SIZE, T), void_>
        tail_list_to_cons;
        typedef typename tail_list_to_cons::type tail_type;
        
        typedef cons<head_type, tail_type> type;
        
        #include <boost/fusion/container/list/detail/list_to_cons_call.hpp>
    };

    template <>
    struct list_to_cons<BOOST_PP_ENUM(FUSION_MAX_LIST_SIZE, FUSION_VOID, _)>
    {
        typedef nil type;
    };    
}}}

#undef FUSION_VOID
#endif
