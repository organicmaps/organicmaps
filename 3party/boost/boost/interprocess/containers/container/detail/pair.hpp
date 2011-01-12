//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_CONTAINERS_CONTAINERS_DETAIL_PAIR_HPP
#define BOOST_CONTAINERS_CONTAINERS_DETAIL_PAIR_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include "config_begin.hpp"
#include INCLUDE_BOOST_CONTAINER_DETAIL_WORKAROUND_HPP

#include INCLUDE_BOOST_CONTAINER_DETAIL_MPL_HPP
#include INCLUDE_BOOST_CONTAINER_DETAIL_TYPE_TRAITS_HPP

#include <utility>   //std::pair

#include INCLUDE_BOOST_CONTAINER_MOVE_HPP

#ifndef BOOST_CONTAINERS_PERFECT_FORWARDING
#include INCLUDE_BOOST_CONTAINER_DETAIL_PREPROCESSOR_HPP
#endif

namespace boost {
namespace container { 
namespace containers_detail {

template <class T1, class T2>
struct pair
{
   private:
   BOOST_MOVE_MACRO_COPYABLE_AND_MOVABLE(pair)

   public:
   typedef T1 first_type;
   typedef T2 second_type;

   T1 first;
   T2 second;

   //std::pair compatibility
   template <class D, class S>
   pair(const std::pair<D, S>& p)
      : first(p.first), second(p.second)
   {}

   //To resolve ambiguity with the variadic constructor of 1 argument
   //and the previous constructor
   pair(std::pair<T1, T2>& x)
      : first(x.first), second(x.second)
   {}

   template <class D, class S>
   pair(BOOST_MOVE_MACRO_RV_REF_2_TEMPL_ARGS(std::pair, D, S) p)
      : first(BOOST_CONTAINER_MOVE_NAMESPACE::move(p.first)), second(BOOST_CONTAINER_MOVE_NAMESPACE::move(p.second))
   {}

   pair()
      : first(), second()
   {}

   pair(const pair<T1, T2>& x)
      : first(x.first), second(x.second)
   {}

   //To resolve ambiguity with the variadic constructor of 1 argument
   //and the copy constructor
   pair(pair<T1, T2>& x)
      : first(x.first), second(x.second)
   {}

   pair(BOOST_MOVE_MACRO_RV_REF(pair) p)
      : first(BOOST_CONTAINER_MOVE_NAMESPACE::move(p.first)), second(BOOST_CONTAINER_MOVE_NAMESPACE::move(p.second))
   {}

   template <class D, class S>
   pair(BOOST_MOVE_MACRO_RV_REF_2_TEMPL_ARGS(pair, D, S) p)
      : first(BOOST_CONTAINER_MOVE_NAMESPACE::move(p.first)), second(BOOST_CONTAINER_MOVE_NAMESPACE::move(p.second))
   {}

   #ifdef BOOST_CONTAINERS_PERFECT_FORWARDING

   template<class U, class ...Args>
   pair(U &&u, Args &&... args)
      : first(BOOST_CONTAINER_MOVE_NAMESPACE::forward<U>(u))
      , second(BOOST_CONTAINER_MOVE_NAMESPACE::forward<Args>(args)...)
   {}

   #else

   template<class U>
   pair( BOOST_CONTAINERS_PARAM(U, u)
       #ifdef BOOST_NO_RVALUE_REFERENCES
       , typename containers_detail::disable_if
          < containers_detail::is_same<U, ::BOOST_CONTAINER_MOVE_NAMESPACE::rv<pair> > >::type* = 0
       #endif
      )
      : first(BOOST_CONTAINER_MOVE_NAMESPACE::forward<U>(const_cast<U&>(u)))
   {}

   #define BOOST_PP_LOCAL_MACRO(n)                                                            \
   template<class U, BOOST_PP_ENUM_PARAMS(n, class P)>                                        \
   pair(BOOST_CONTAINERS_PARAM(U, u)                                                          \
       ,BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_LIST, _))                                  \
      : first(BOOST_CONTAINER_MOVE_NAMESPACE::forward<U>(const_cast<U&>(u)))                             \
      , second(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_FORWARD, _))                        \
   {}                                                                                         \
   //!
   #define BOOST_PP_LOCAL_LIMITS (1, BOOST_CONTAINERS_MAX_CONSTRUCTOR_PARAMETERS)
   #include BOOST_PP_LOCAL_ITERATE()
   #endif

   pair& operator=(BOOST_MOVE_MACRO_COPY_ASSIGN_REF(pair) p)
   {
      first  = p.first;
      second = p.second;
      return *this;
   }

   pair& operator=(BOOST_MOVE_MACRO_RV_REF(pair) p)
   {
      first  = BOOST_CONTAINER_MOVE_NAMESPACE::move(p.first);
      second = BOOST_CONTAINER_MOVE_NAMESPACE::move(p.second);
      return *this;
   }

   pair& operator=(BOOST_MOVE_MACRO_RV_REF_2_TEMPL_ARGS(std::pair, T1, T2) p)
   {
      first  = BOOST_CONTAINER_MOVE_NAMESPACE::move(p.first);
      second = BOOST_CONTAINER_MOVE_NAMESPACE::move(p.second);
      return *this;
   }

   template <class D, class S>
   pair& operator=(BOOST_MOVE_MACRO_RV_REF_2_TEMPL_ARGS(std::pair, D, S) p)
   {
      first  = BOOST_CONTAINER_MOVE_NAMESPACE::move(p.first);
      second = BOOST_CONTAINER_MOVE_NAMESPACE::move(p.second);
      return *this;
   }

   void swap(pair& p)
   {  std::swap(*this, p); }
};

template <class T1, class T2>
inline bool operator==(const pair<T1,T2>& x, const pair<T1,T2>& y)
{  return static_cast<bool>(x.first == y.first && x.second == y.second);  }

template <class T1, class T2>
inline bool operator< (const pair<T1,T2>& x, const pair<T1,T2>& y)
{  return static_cast<bool>(x.first < y.first ||
                         (!(y.first < x.first) && x.second < y.second)); }

template <class T1, class T2>
inline bool operator!=(const pair<T1,T2>& x, const pair<T1,T2>& y)
{  return static_cast<bool>(!(x == y));  }

template <class T1, class T2>
inline bool operator> (const pair<T1,T2>& x, const pair<T1,T2>& y)
{  return y < x;  }

template <class T1, class T2>
inline bool operator>=(const pair<T1,T2>& x, const pair<T1,T2>& y)
{  return static_cast<bool>(!(x < y)); }

template <class T1, class T2>
inline bool operator<=(const pair<T1,T2>& x, const pair<T1,T2>& y)
{  return static_cast<bool>(!(y < x)); }

template <class T1, class T2>
inline pair<T1, T2> make_pair(T1 x, T2 y)
{  return pair<T1, T2>(x, y); }

template <class T1, class T2>
inline void swap(pair<T1, T2>& x, pair<T1, T2>& y)
{
   swap(x.first, y.first);
   swap(x.second, y.second);
}

}  //namespace containers_detail { 
}  //namespace container { 


//Without this specialization recursive flat_(multi)map instantiation fails
//because is_enum needs to instantiate the recursive pair, leading to a compilation error).
//This breaks the cycle clearly stating that pair is not an enum avoiding any instantiation.
template<class T>
struct is_enum;

template<class T, class U>
struct is_enum< ::boost::container::containers_detail::pair<T, U> >
{
   static const bool value = false;
};

}  //namespace boost {

#include INCLUDE_BOOST_CONTAINER_DETAIL_CONFIG_END_HPP

#endif   //#ifndef BOOST_CONTAINERS_DETAIL_PAIR_HPP
