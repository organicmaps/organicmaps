//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2008-2011. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_PREPROCESSOR_HPP
#define BOOST_INTERPROCESS_DETAIL_PREPROCESSOR_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>

#ifdef BOOST_INTERPROCESS_PERFECT_FORWARDING
#error "This file is not needed when perfect forwarding is available"
#endif

#include <boost/preprocessor/iteration/local.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>

#define BOOST_INTERPROCESS_MAX_CONSTRUCTOR_PARAMETERS 10

//Note:
//We define template parameters as const references to
//be able to bind temporaries. After that we will un-const them.
//This cast is ugly but it is necessary until "perfect forwarding"
//is achieved in C++0x. Meanwhile, if we want to be able to
//bind rvalues with non-const references, we have to be ugly
#ifndef BOOST_NO_RVALUE_REFERENCES
   #define BOOST_INTERPROCESS_PP_PARAM_LIST(z, n, data) \
   BOOST_PP_CAT(P, n) && BOOST_PP_CAT(p, n) \
   //!
#else
   #define BOOST_INTERPROCESS_PP_PARAM_LIST(z, n, data) \
   const BOOST_PP_CAT(P, n) & BOOST_PP_CAT(p, n) \
   //!
#endif

#ifndef BOOST_NO_RVALUE_REFERENCES
   #define BOOST_INTERPROCESS_PP_PARAM(U, u) \
   U && u \
   //!
#else
   #define BOOST_INTERPROCESS_PP_PARAM(U, u) \
   const U & u \
   //!
#endif

#ifndef BOOST_NO_RVALUE_REFERENCES

   #if defined(BOOST_MOVE_MSVC_10_MEMBER_RVALUE_REF_BUG)

   #define BOOST_INTERPROCESS_PP_PARAM_INIT(z, n, data)  \
      BOOST_PP_CAT(m_p, n) (BOOST_PP_CAT(p, n))          \
   //!


   #else //#if defined(BOOST_MOVE_MSVC_10_MEMBER_RVALUE_REF_BUG)

   #define BOOST_INTERPROCESS_PP_PARAM_INIT(z, n, data) \
     BOOST_PP_CAT(m_p, n) (::boost::forward< BOOST_PP_CAT(P, n) >( BOOST_PP_CAT(p, n) ))  \
   //!

   #endif   //#if defined(BOOST_MOVE_MSVC_10_MEMBER_RVALUE_REF_BUG)

#else //#ifndef BOOST_NO_RVALUE_REFERENCES

   #define BOOST_INTERPROCESS_PP_PARAM_INIT(z, n, data) \
     BOOST_PP_CAT(m_p, n) (const_cast<BOOST_PP_CAT(P, n) &>(BOOST_PP_CAT(p, n))) \
   //!
#endif

#define BOOST_INTERPROCESS_PP_PARAM_INC(z, n, data)   \
   BOOST_PP_CAT(++m_p, n)                        \
//!

#ifndef BOOST_NO_RVALUE_REFERENCES

#if defined(BOOST_MOVE_MSVC_10_MEMBER_RVALUE_REF_BUG)

#define BOOST_INTERPROCESS_PP_PARAM_DEFINE(z, n, data)  \
  BOOST_PP_CAT(P, n) & BOOST_PP_CAT(m_p, n);            \
//!

#else

#define BOOST_INTERPROCESS_PP_PARAM_DEFINE(z, n, data)  \
  BOOST_PP_CAT(P, n) && BOOST_PP_CAT(m_p, n);            \
//!

#endif //defined(BOOST_MOVE_MSVC_10_MEMBER_RVALUE_REF_BUG)


#else
#define BOOST_INTERPROCESS_PP_PARAM_DEFINE(z, n, data)  \
  BOOST_PP_CAT(P, n) & BOOST_PP_CAT(m_p, n);             \
//!
#endif

#define BOOST_INTERPROCESS_PP_PARAM_FORWARD(z, n, data) \
::boost::forward< BOOST_PP_CAT(P, n) >( BOOST_PP_CAT(p, n) ) \
//!

#if !defined(BOOST_NO_RVALUE_REFERENCES) && defined(BOOST_MOVE_MSVC_10_MEMBER_RVALUE_REF_BUG)

#include <boost/container/detail/stored_ref.hpp>

#define BOOST_INTERPROCESS_PP_MEMBER_FORWARD(z, n, data) \
::boost::container::container_detail::stored_ref< BOOST_PP_CAT(P, n) >::forward( BOOST_PP_CAT(m_p, n) ) \
//!

#else

#define BOOST_INTERPROCESS_PP_MEMBER_FORWARD(z, n, data) \
::boost::forward< BOOST_PP_CAT(P, n) >( BOOST_PP_CAT(m_p, n) ) \
//!

#endif   //!defined(BOOST_NO_RVALUE_REFERENCES) && defined(BOOST_MOVE_MSVC_10_MEMBER_RVALUE_REF_BUG)

#define BOOST_INTERPROCESS_PP_MEMBER_IT_FORWARD(z, n, data) \
BOOST_PP_CAT(*m_p, n) \
//!

#include <boost/interprocess/detail/config_end.hpp>

#else
#ifdef BOOST_INTERPROCESS_PERFECT_FORWARDING
#error "This file is not needed when perfect forwarding is available"
#endif
#endif //#ifndef BOOST_INTERPROCESS_DETAIL_PREPROCESSOR_HPP
