//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2008-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_CONTAINERS_DETAIL_PREPROCESSOR_HPP
#define BOOST_CONTAINERS_DETAIL_PREPROCESSOR_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/containers/container/detail/config_begin.hpp>
#include <boost/interprocess/containers/container/detail/workaround.hpp>

#ifdef BOOST_CONTAINERS_PERFECT_FORWARDING
#error "This file is not needed when perfect forwarding is available"
#endif

#include <boost/preprocessor/iteration/local.hpp> 
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>

#define BOOST_CONTAINERS_MAX_CONSTRUCTOR_PARAMETERS 10

//Note:
//We define template parameters as const references to
//be able to bind temporaries. After that we will un-const them.
//This cast is ugly but it is necessary until "perfect forwarding"
//is achieved in C++0x. Meanwhile, if we want to be able to
//bind rvalues with non-const references, we have to be ugly
#ifdef BOOST_HAS_RVALUE_REFS
   #define BOOST_CONTAINERS_PP_PARAM_LIST(z, n, data) \
   BOOST_PP_CAT(P, n) && BOOST_PP_CAT(p, n) \
   //!
#else
   #define BOOST_CONTAINERS_PP_PARAM_LIST(z, n, data) \
   const BOOST_PP_CAT(P, n) & BOOST_PP_CAT(p, n) \
   //!
#endif

#ifdef BOOST_HAS_RVALUE_REFS
   #define BOOST_CONTAINERS_PARAM(U, u) \
   U && u \
   //!
#else
   #define BOOST_CONTAINERS_PARAM(U, u) \
   const U & u \
   //!
#endif

#ifdef BOOST_HAS_RVALUE_REFS
#define BOOST_CONTAINERS_AUX_PARAM_INIT(z, n, data) \
  BOOST_PP_CAT(m_p, n) (BOOST_PP_CAT(p, n))           \
//!
#else
#define BOOST_CONTAINERS_AUX_PARAM_INIT(z, n, data) \
  BOOST_PP_CAT(m_p, n) (const_cast<BOOST_PP_CAT(P, n) &>(BOOST_PP_CAT(p, n))) \
//!
#endif

#define BOOST_CONTAINERS_AUX_PARAM_INC(z, n, data)   \
  BOOST_PP_CAT(++m_p, n)                        \
//!

#ifdef BOOST_HAS_RVALUE_REFS
#define BOOST_CONTAINERS_AUX_PARAM_DEFINE(z, n, data)  \
  BOOST_PP_CAT(P, n) && BOOST_PP_CAT(m_p, n);            \
//!
#else
#define BOOST_CONTAINERS_AUX_PARAM_DEFINE(z, n, data)  \
  BOOST_PP_CAT(P, n) & BOOST_PP_CAT(m_p, n);             \
//!
#endif

#define BOOST_CONTAINERS_PP_PARAM_FORWARD(z, n, data) \
boost::interprocess::forward< BOOST_PP_CAT(P, n) >( BOOST_PP_CAT(p, n) ) \
//!

#define BOOST_CONTAINERS_PP_MEMBER_FORWARD(z, n, data) \
boost::interprocess::forward< BOOST_PP_CAT(P, n) >( BOOST_PP_CAT(m_p, n) ) \
//!

#define BOOST_CONTAINERS_PP_MEMBER_IT_FORWARD(z, n, data) \
BOOST_PP_CAT(*m_p, n) \
//!

#include <boost/interprocess/containers/container/detail/config_end.hpp>

#else
#ifdef BOOST_CONTAINERS_PERFECT_FORWARDING
#error "This file is not needed when perfect forwarding is available"
#endif
#endif //#ifndef BOOST_CONTAINERS_DETAIL_PREPROCESSOR_HPP
