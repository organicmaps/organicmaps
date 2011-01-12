//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2008-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_CONTAINERS_ADVANCED_INSERT_INT_HPP
#define BOOST_CONTAINERS_ADVANCED_INSERT_INT_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include "config_begin.hpp"
#include INCLUDE_BOOST_CONTAINER_DETAIL_WORKAROUND_HPP
#include INCLUDE_BOOST_CONTAINER_MOVE_HPP
#include <iterator>  //std::iterator_traits
#include <new>       //placement new
#include <boost/assert.hpp>

namespace boost { namespace container { namespace containers_detail {

//This class will be interface for operations dependent on FwdIt types used advanced_insert_aux_impl
template<class T, class Iterator>
struct advanced_insert_aux_int
{
   typedef typename std::iterator_traits<Iterator>::difference_type difference_type;
   virtual void copy_all_to(Iterator p) = 0;
   virtual void uninitialized_copy_all_to(Iterator p) = 0;
   virtual void uninitialized_copy_some_and_update(Iterator pos, difference_type division_count, bool first) = 0;
   virtual void copy_some_and_update(Iterator pos, difference_type division_count, bool first) = 0;
   virtual ~advanced_insert_aux_int() {}
};

//This class template will adapt each FwIt types to advanced_insert_aux_int
template<class T, class FwdIt, class Iterator>
struct advanced_insert_aux_proxy
   :  public advanced_insert_aux_int<T, Iterator>
{
   typedef typename advanced_insert_aux_int<T, Iterator>::difference_type difference_type;
   advanced_insert_aux_proxy(FwdIt first, FwdIt last)
      :  first_(first), last_(last)
   {}

   virtual ~advanced_insert_aux_proxy()
   {}

   virtual void copy_all_to(Iterator p)
   {  ::BOOST_CONTAINER_MOVE_NAMESPACE::copy_or_move(first_, last_, p);  }

   virtual void uninitialized_copy_all_to(Iterator p)
   {  ::BOOST_CONTAINER_MOVE_NAMESPACE::uninitialized_copy_or_move(first_, last_, p);  }

   virtual void uninitialized_copy_some_and_update(Iterator pos, difference_type division_count, bool first_n)
   {
      FwdIt mid = first_;
      std::advance(mid, division_count);
      if(first_n){
         ::BOOST_CONTAINER_MOVE_NAMESPACE::uninitialized_copy_or_move(first_, mid, pos);
         first_ = mid;
      }
      else{
         ::BOOST_CONTAINER_MOVE_NAMESPACE::uninitialized_copy_or_move(mid, last_, pos);
         last_ = mid;
      }
   }

   virtual void copy_some_and_update(Iterator pos, difference_type division_count, bool first_n)
   {
      FwdIt mid = first_;
      std::advance(mid, division_count);
      if(first_n){
         ::BOOST_CONTAINER_MOVE_NAMESPACE::copy_or_move(first_, mid, pos);
         first_ = mid;
      }
      else{
         ::BOOST_CONTAINER_MOVE_NAMESPACE::copy_or_move(mid, last_, pos);
         last_ = mid;
      }
   }

   FwdIt first_, last_;
};

//This class template will adapt each FwIt types to advanced_insert_aux_int
template<class T, class Iterator, class SizeType>
struct default_construct_aux_proxy
   :  public advanced_insert_aux_int<T, Iterator>
{
   typedef typename advanced_insert_aux_int<T, Iterator>::difference_type difference_type;
   default_construct_aux_proxy(SizeType count)
      :  count_(count)
   {}

   void uninitialized_copy_impl(Iterator p, const SizeType n)
   {
      BOOST_ASSERT(n <= count_);
      Iterator orig_p = p;
      SizeType i = 0;
      try{
         for(; i < n; ++i, ++p){
            new(containers_detail::get_pointer(&*p))T();
         }
      }
      catch(...){
         while(i--){
            containers_detail::get_pointer(&*orig_p++)->~T();
         }
         throw;
      }
      count_ -= n;
   }

   virtual ~default_construct_aux_proxy()
   {}

   virtual void copy_all_to(Iterator)
   {  //This should never be called with any count
      BOOST_ASSERT(count_ == 0);
   }

   virtual void uninitialized_copy_all_to(Iterator p)
   {  this->uninitialized_copy_impl(p, count_); }

   virtual void uninitialized_copy_some_and_update(Iterator pos, difference_type division_count, bool first_n)
   {
      SizeType new_count;
      if(first_n){
         new_count = division_count;
      }
      else{
         BOOST_ASSERT(difference_type(count_)>= division_count);
         new_count = count_ - division_count;
      }
      this->uninitialized_copy_impl(pos, new_count);
   }

   virtual void copy_some_and_update(Iterator , difference_type division_count, bool first_n)
   {
      BOOST_ASSERT(count_ == 0);
      SizeType new_count;
      if(first_n){
         new_count = division_count;
      }
      else{
         BOOST_ASSERT(difference_type(count_)>= division_count);
         new_count = count_ - division_count;
      }
      //This function should never called with a count different to zero
      BOOST_ASSERT(new_count == 0);
      (void)new_count;
   }

   SizeType count_;
};

}}}   //namespace boost { namespace container { namespace containers_detail {

#ifdef BOOST_CONTAINERS_PERFECT_FORWARDING

#include INCLUDE_BOOST_CONTAINER_DETAIL_VARIADIC_TEMPLATES_TOOLS_HPP
#include INCLUDE_BOOST_CONTAINER_DETAIL_STORED_REF_HPP
#include INCLUDE_BOOST_CONTAINER_MOVE_HPP
#include <typeinfo>
//#include <iostream> //For debugging purposes

namespace boost {
namespace container { 
namespace containers_detail {

//This class template will adapt each FwIt types to advanced_insert_aux_int
template<class T, class Iterator, class ...Args>
struct advanced_insert_aux_emplace
   :  public advanced_insert_aux_int<T, Iterator>
{
   typedef typename advanced_insert_aux_int<T, Iterator>::difference_type difference_type;
   typedef typename build_number_seq<sizeof...(Args)>::type             index_tuple_t;

   explicit advanced_insert_aux_emplace(Args&&... args)
      : args_(args...)
      , used_(false)
   {}

   ~advanced_insert_aux_emplace()
   {}

   virtual void copy_all_to(Iterator p)
   {  this->priv_copy_all_to(index_tuple_t(), p);   }

   virtual void uninitialized_copy_all_to(Iterator p)
   {  this->priv_uninitialized_copy_all_to(index_tuple_t(), p);   }

   virtual void uninitialized_copy_some_and_update(Iterator p, difference_type division_count, bool first_n)
   {  this->priv_uninitialized_copy_some_and_update(index_tuple_t(), p, division_count, first_n);  }

   virtual void copy_some_and_update(Iterator p, difference_type division_count, bool first_n)
   {  this->priv_copy_some_and_update(index_tuple_t(), p, division_count, first_n);  }

   private:
   template<int ...IdxPack>
   void priv_copy_all_to(const index_tuple<IdxPack...>&, Iterator p)
   {
      if(!used_){
         *p = BOOST_CONTAINER_MOVE_NAMESPACE::move(T (::boost::container::containers_detail::stored_ref<Args>::forward(get<IdxPack>(args_))...));
         used_ = true;
      }
   }

   template<int ...IdxPack>
   void priv_uninitialized_copy_all_to(const index_tuple<IdxPack...>&, Iterator p)
   {
      if(!used_){
         new(containers_detail::get_pointer(&*p))T(::boost::container::containers_detail::stored_ref<Args>::forward(get<IdxPack>(args_))...);
         used_ = true;
      }
   }

   template<int ...IdxPack>
   void priv_uninitialized_copy_some_and_update(const index_tuple<IdxPack...>&, Iterator p, difference_type division_count, bool first_n)
   {
      BOOST_ASSERT(division_count <=1);
      if((first_n && division_count == 1) || (!first_n && division_count == 0)){
         if(!used_){
            new(containers_detail::get_pointer(&*p))T(::boost::container::containers_detail::stored_ref<Args>::forward(get<IdxPack>(args_))...);
            used_ = true;
         }
      }
   }

   template<int ...IdxPack>
   void priv_copy_some_and_update(const index_tuple<IdxPack...>&, Iterator p, difference_type division_count, bool first_n)
   {
      BOOST_ASSERT(division_count <=1);
      if((first_n && division_count == 1) || (!first_n && division_count == 0)){
         if(!used_){
            *p = BOOST_CONTAINER_MOVE_NAMESPACE::move(T(::boost::container::containers_detail::stored_ref<Args>::forward(get<IdxPack>(args_))...));
            used_ = true;
         }
      }
   }
   tuple<Args&...> args_;
   bool used_;
};

}}}   //namespace boost { namespace container { namespace containers_detail {

#else //#ifdef BOOST_CONTAINERS_PERFECT_FORWARDING

#include INCLUDE_BOOST_CONTAINER_DETAIL_PREPROCESSOR_HPP 
#include INCLUDE_BOOST_CONTAINER_DETAIL_VALUE_INIT_HPP

namespace boost {
namespace container { 
namespace containers_detail {

//This class template will adapt each FwIt types to advanced_insert_aux_int
template<class T, class Iterator>
struct advanced_insert_aux_emplace
   :  public advanced_insert_aux_int<T, Iterator>
{
   typedef typename advanced_insert_aux_int<T, Iterator>::difference_type difference_type;
   advanced_insert_aux_emplace()
      :  used_(false)
   {}

   ~advanced_insert_aux_emplace()
   {}

   virtual void copy_all_to(Iterator p)
   {
      if(!used_){
         value_init<T>v;
         *p = BOOST_CONTAINER_MOVE_NAMESPACE::move(v.m_t);
         used_ = true;
      }
   }

   virtual void uninitialized_copy_all_to(Iterator p)
   {
      if(!used_){
         new(containers_detail::get_pointer(&*p))T();
         used_ = true;
      }
   }

   virtual void uninitialized_copy_some_and_update(Iterator p, difference_type division_count, bool first_n)
   {
      BOOST_ASSERT(division_count <=1);
      if((first_n && division_count == 1) || (!first_n && division_count == 0)){
         if(!used_){
            new(containers_detail::get_pointer(&*p))T();
            used_ = true;
         }
      }
   }

   virtual void copy_some_and_update(Iterator p, difference_type division_count, bool first_n)
   {
      BOOST_ASSERT(division_count <=1);
      if((first_n && division_count == 1) || (!first_n && division_count == 0)){
         if(!used_){
            value_init<T>v;
            *p = BOOST_CONTAINER_MOVE_NAMESPACE::move(v.m_t);
            used_ = true;
         }
      }
   }
   private:
   bool used_;
};

   #define BOOST_PP_LOCAL_MACRO(n)                                                     \
   template<class T, class Iterator, BOOST_PP_ENUM_PARAMS(n, class P) >                \
   struct BOOST_PP_CAT(BOOST_PP_CAT(advanced_insert_aux_emplace, n), arg)                \
      :  public advanced_insert_aux_int<T, Iterator>                                     \
   {                                                                                   \
      typedef typename advanced_insert_aux_int<T, Iterator>::difference_type difference_type;  \
                                                                                       \
      BOOST_PP_CAT(BOOST_PP_CAT(advanced_insert_aux_emplace, n), arg)                    \
         ( BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_LIST, _) )          \
         : used_(false), BOOST_PP_ENUM(n, BOOST_CONTAINERS_AUX_PARAM_INIT, _) {}     \
                                                                                       \
      virtual void copy_all_to(Iterator p)                                             \
      {                                                                                \
         if(!used_){                                                                   \
            T v(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_MEMBER_FORWARD, _));            \
            *p = BOOST_CONTAINER_MOVE_NAMESPACE::move(v);                                                 \
            used_ = true;                                                              \
         }                                                                             \
      }                                                                                \
                                                                                       \
      virtual void uninitialized_copy_all_to(Iterator p)                               \
      {                                                                                \
         if(!used_){                                                                   \
            new(containers_detail::get_pointer(&*p))T                                             \
               (BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_MEMBER_FORWARD, _));            \
            used_ = true;                                                              \
         }                                                                             \
      }                                                                                \
                                                                                       \
      virtual void uninitialized_copy_some_and_update                                  \
         (Iterator p, difference_type division_count, bool first_n)                    \
      {                                                                                \
         BOOST_ASSERT(division_count <=1);                                                   \
         if((first_n && division_count == 1) || (!first_n && division_count == 0)){    \
            if(!used_){                                                                \
               new(containers_detail::get_pointer(&*p))T                                          \
                  (BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_MEMBER_FORWARD, _));         \
               used_ = true;                                                           \
            }                                                                          \
         }                                                                             \
      }                                                                                \
                                                                                       \
      virtual void copy_some_and_update                                                \
         (Iterator p, difference_type division_count, bool first_n)                    \
      {                                                                                \
         BOOST_ASSERT(division_count <=1);                                                   \
         if((first_n && division_count == 1) || (!first_n && division_count == 0)){    \
            if(!used_){                                                                \
               T v(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_MEMBER_FORWARD, _));         \
               *p = BOOST_CONTAINER_MOVE_NAMESPACE::move(v);                                              \
               used_ = true;                                                           \
            }                                                                          \
         }                                                                             \
      }                                                                                \
                                                                                       \
      bool used_;                                                                      \
      BOOST_PP_REPEAT(n, BOOST_CONTAINERS_AUX_PARAM_DEFINE, _)                       \
   };                                                                                  \
//!

#define BOOST_PP_LOCAL_LIMITS (1, BOOST_CONTAINERS_MAX_CONSTRUCTOR_PARAMETERS)
#include BOOST_PP_LOCAL_ITERATE()

}}}   //namespace boost { namespace container { namespace containers_detail {

#endif   //#ifdef BOOST_CONTAINERS_PERFECT_FORWARDING

#include INCLUDE_BOOST_CONTAINER_DETAIL_CONFIG_END_HPP

#endif //#ifndef BOOST_CONTAINERS_ADVANCED_INSERT_INT_HPP
