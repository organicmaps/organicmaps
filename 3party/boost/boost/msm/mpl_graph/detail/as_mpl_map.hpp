// Copyright 2008-2010 Gordon Woodhull
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// this is an experiment at implementing a metafunction that's
// present in fusion but not in mpl
// based on fusion/container/map/detail/as_map.hpp
/*=============================================================================
    Copyright (c) 2001-2006 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PP_IS_ITERATING
#if !defined(AS_MPL_MAP_HPP)
#define AS_MPL_MAP_HPP

#define AS_MPL_MAP_SIZE 20

#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/inc.hpp>
#include <boost/preprocessor/dec.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/next.hpp>

namespace boost { namespace mpl { 
    
    namespace detail {
        template <int size>
        struct as_map;
        
    }
    template<typename Seq>
    struct as_map : detail::as_map<mpl::size<Seq>::value>::template 
                                   apply<typename mpl::begin<Seq>::type> {};
    
    namespace detail
{
    template <int size>
    struct as_map;

    template <>
    struct as_map<0>
    {
        template <typename Iterator>
        struct apply
        {
            typedef map<> type;
        };

    };

#define BOOST_AS_MPL_MAP_NEXT_ITERATOR(z, n, data)                                  \
    typedef typename mpl::next<BOOST_PP_CAT(I, n)>::type          \
        BOOST_PP_CAT(I, BOOST_PP_INC(n));

#define BOOST_AS_MPL_MAP_DEREF_ITERATOR(z, n, data)                              \
    typedef typename mpl::deref<BOOST_PP_CAT(I, n)>::type      \
        BOOST_PP_CAT(T, n);

#define BOOST_PP_FILENAME_1 <boost/msm/mpl_graph/detail/as_mpl_map.hpp>
#define BOOST_PP_ITERATION_LIMITS (1, AS_MPL_MAP_SIZE)
#include BOOST_PP_ITERATE()

#undef BOOST_AS_MPL_MAP_NEXT_ITERATOR
#undef BOOST_AS_MPL_MAP_DEREF_ITERATOR

}}}

#endif
#else // defined(BOOST_PP_IS_ITERATING)
///////////////////////////////////////////////////////////////////////////////
//
//  Preprocessor vertical repetition code
//
///////////////////////////////////////////////////////////////////////////////

#define N BOOST_PP_ITERATION()

    template <>
    struct as_map<N>
    {
        template <typename I0>
        struct apply
        {
            BOOST_PP_REPEAT(N, BOOST_AS_MPL_MAP_NEXT_ITERATOR, _)
            BOOST_PP_REPEAT(N, BOOST_AS_MPL_MAP_DEREF_ITERATOR, _)
            typedef map<BOOST_PP_ENUM_PARAMS(N, T)> type;
        };

    };

#undef N
#endif // defined(BOOST_PP_IS_ITERATING)

