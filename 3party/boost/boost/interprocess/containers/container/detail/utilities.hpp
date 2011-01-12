//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_CONTAINERS_DETAIL_UTILITIES_HPP
#define BOOST_CONTAINERS_DETAIL_UTILITIES_HPP

#include "config_begin.hpp"
#include <cstdio>
#include <boost/type_traits/is_fundamental.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/is_member_pointer.hpp>
#include INCLUDE_BOOST_CONTAINER_MOVE_HPP
#include INCLUDE_BOOST_CONTAINER_DETAIL_MPL_HPP
#include INCLUDE_BOOST_CONTAINER_DETAIL_TYPE_TRAITS_HPP
#include <algorithm>

namespace boost {
namespace container {
namespace containers_detail {

template<class T>
const T &max_value(const T &a, const T &b)
{  return a > b ? a : b;   }

template<class T>
const T &min_value(const T &a, const T &b)
{  return a < b ? a : b;   }

template <class SizeType>
SizeType
   get_next_capacity(const SizeType max_size
                    ,const SizeType capacity
                    ,const SizeType n)
{
//   if (n > max_size - capacity)
//      throw std::length_error("get_next_capacity");

   const SizeType m3 = max_size/3;

   if (capacity < m3)
      return capacity + max_value(3*(capacity+1)/5, n);

   if (capacity < m3*2)
      return capacity + max_value((capacity+1)/2, n);

   return max_size;
}

template<class SmartPtr>
struct smart_ptr_type
{
   typedef typename SmartPtr::value_type value_type;
   typedef value_type *pointer;
   static pointer get (const SmartPtr &smartptr)
   {  return smartptr.get();}
};

template<class T>
struct smart_ptr_type<T*>
{
   typedef T value_type;
   typedef value_type *pointer;
   static pointer get (pointer ptr)
   {  return ptr;}
};

//!Overload for smart pointers to avoid ADL problems with get_pointer
template<class Ptr>
inline typename smart_ptr_type<Ptr>::pointer
get_pointer(const Ptr &ptr)
{  return smart_ptr_type<Ptr>::get(ptr);   }

//!To avoid ADL problems with swap
template <class T>
inline void do_swap(T& x, T& y)
{
   using std::swap;
   swap(x, y);
}

//Rounds "orig_size" by excess to round_to bytes
inline std::size_t get_rounded_size(std::size_t orig_size, std::size_t round_to)
{
   return ((orig_size-1)/round_to+1)*round_to;
}

template <std::size_t OrigSize, std::size_t RoundTo>
struct ct_rounded_size
{
   enum { value = ((OrigSize-1)/RoundTo+1)*RoundTo };
};

template <class _TypeT>
struct __rw_is_enum
{
struct _C_no { };
struct _C_yes { int _C_dummy [2]; };

struct _C_indirect {
// prevent classes with user-defined conversions from matching

// use double to prevent float->int gcc conversion warnings
_C_indirect (double);
};

// nested struct gets rid of bogus gcc errors
struct _C_nest {
// supply first argument to prevent HP aCC warnings
static _C_no _C_is (int, ...);
static _C_yes _C_is (int, _C_indirect);

static _TypeT _C_make_T ();
};

enum {
_C_val = sizeof (_C_yes)
== sizeof (_C_nest::_C_is (0, _C_nest::_C_make_T ()))
&& !::boost::is_fundamental<_TypeT>::value
};

}; 

template<class T>
struct move_const_ref_type
   : if_c
   < ::boost::is_fundamental<T>::value || ::boost::is_pointer<T>::value ||
     ::boost::is_member_pointer<T>::value || ::boost::is_enum<T>::value
   ,const T &
   ,BOOST_MOVE_MACRO_CATCH_CONST_RLVALUE(T)
   >
{};

}  //namespace containers_detail {
}  //namespace container {
}  //namespace boost {


#include INCLUDE_BOOST_CONTAINER_DETAIL_CONFIG_END_HPP

#endif   //#ifndef BOOST_CONTAINERS_DETAIL_UTILITIES_HPP
