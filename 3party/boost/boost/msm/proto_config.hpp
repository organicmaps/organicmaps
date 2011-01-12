// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_PROTO_CONFIG_H
#define BOOST_MSM_PROTO_CONFIG_H

#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS

#ifdef BOOST_MSVC 
  #if BOOST_PROTO_MAX_ARITY <= 7
    #ifdef BOOST_MPL_LIMIT_METAFUNCTION_ARITY
    #undef BOOST_MPL_LIMIT_METAFUNCTION_ARITY
    #endif
    #define BOOST_MPL_LIMIT_METAFUNCTION_ARITY 7
    #define BOOST_PROTO_MAX_ARITY 7
 #endif
#else
 #if BOOST_PROTO_MAX_ARITY <= 6
    #ifdef BOOST_MPL_LIMIT_METAFUNCTION_ARITY
    #undef BOOST_MPL_LIMIT_METAFUNCTION_ARITY
    #endif
    #define BOOST_MPL_LIMIT_METAFUNCTION_ARITY 6
    #define BOOST_PROTO_MAX_ARITY 6
 #endif
#endif

#endif //BOOST_MSM_PROTO_CONFIG_H
