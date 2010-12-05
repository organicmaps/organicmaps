//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009.
// (C) Copyright Gennaro Prota 2003 - 2004.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_CONTAINERS_DETAIL_ITERATORS_HPP
#define BOOST_CONTAINERS_DETAIL_ITERATORS_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/containers/container/detail/config_begin.hpp>
#include <boost/interprocess/containers/container/detail/workaround.hpp>
#include <boost/interprocess/detail/move.hpp>

#ifdef BOOST_CONTAINERS_PERFECT_FORWARDING
#include <boost/interprocess/containers/container/detail/variadic_templates_tools.hpp>
#else
#include <boost/interprocess/containers/container/detail/preprocessor.hpp>
#endif

#include <iterator>

namespace boost {
namespace container { 

template <class T, class Difference = std::ptrdiff_t>
class constant_iterator
  : public std::iterator
      <std::random_access_iterator_tag, T, Difference, const T*, const T &>
{
   typedef  constant_iterator<T, Difference> this_type;

   public:
   explicit constant_iterator(const T &ref, Difference range_size)
      :  m_ptr(&ref), m_num(range_size){}

   //Constructors
   constant_iterator()
      :  m_ptr(0), m_num(0){}

   constant_iterator& operator++() 
   { increment();   return *this;   }
   
   constant_iterator operator++(int)
   {
      constant_iterator result (*this);
      increment();
      return result;
   }

   friend bool operator== (const constant_iterator& i, const constant_iterator& i2)
   { return i.equal(i2); }

   friend bool operator!= (const constant_iterator& i, const constant_iterator& i2)
   { return !(i == i2); }

   friend bool operator< (const constant_iterator& i, const constant_iterator& i2)
   { return i.less(i2); }

   friend bool operator> (const constant_iterator& i, const constant_iterator& i2)
   { return i2 < i; }

   friend bool operator<= (const constant_iterator& i, const constant_iterator& i2)
   { return !(i > i2); }

   friend bool operator>= (const constant_iterator& i, const constant_iterator& i2)
   { return !(i < i2); }

   friend Difference operator- (const constant_iterator& i, const constant_iterator& i2)
   { return i2.distance_to(i); }

   //Arithmetic
   constant_iterator& operator+=(Difference off)
   {  this->advance(off); return *this;   }

   constant_iterator operator+(Difference off) const
   {
      constant_iterator other(*this);
      other.advance(off);
      return other;
   }

   friend constant_iterator operator+(Difference off, const constant_iterator& right)
   {  return right + off; }

   constant_iterator& operator-=(Difference off)
   {  this->advance(-off); return *this;   }

   constant_iterator operator-(Difference off) const
   {  return *this + (-off);  }

   const T& operator*() const
   { return dereference(); }

   const T* operator->() const
   { return &(dereference()); }

   private:
   const T *   m_ptr;
   Difference  m_num;

   void increment()
   { --m_num; }

   void decrement()
   { ++m_num; }

   bool equal(const this_type &other) const
   {  return m_num == other.m_num;   }

   bool less(const this_type &other) const
   {  return other.m_num < m_num;   }

   const T & dereference() const
   { return *m_ptr; }

   void advance(Difference n)
   {  m_num -= n; }

   Difference distance_to(const this_type &other)const
   {  return m_num - other.m_num;   }
};

template <class T, class Difference = std::ptrdiff_t>
class default_construct_iterator
  : public std::iterator
      <std::random_access_iterator_tag, T, Difference, const T*, const T &>
{
   typedef  default_construct_iterator<T, Difference> this_type;

   public:
   explicit default_construct_iterator(Difference range_size)
      :  m_num(range_size){}

   //Constructors
   default_construct_iterator()
      :  m_num(0){}

   default_construct_iterator& operator++() 
   { increment();   return *this;   }
   
   default_construct_iterator operator++(int)
   {
      default_construct_iterator result (*this);
      increment();
      return result;
   }

   friend bool operator== (const default_construct_iterator& i, const default_construct_iterator& i2)
   { return i.equal(i2); }

   friend bool operator!= (const default_construct_iterator& i, const default_construct_iterator& i2)
   { return !(i == i2); }

   friend bool operator< (const default_construct_iterator& i, const default_construct_iterator& i2)
   { return i.less(i2); }

   friend bool operator> (const default_construct_iterator& i, const default_construct_iterator& i2)
   { return i2 < i; }

   friend bool operator<= (const default_construct_iterator& i, const default_construct_iterator& i2)
   { return !(i > i2); }

   friend bool operator>= (const default_construct_iterator& i, const default_construct_iterator& i2)
   { return !(i < i2); }

   friend Difference operator- (const default_construct_iterator& i, const default_construct_iterator& i2)
   { return i2.distance_to(i); }

   //Arithmetic
   default_construct_iterator& operator+=(Difference off)
   {  this->advance(off); return *this;   }

   default_construct_iterator operator+(Difference off) const
   {
      default_construct_iterator other(*this);
      other.advance(off);
      return other;
   }

   friend default_construct_iterator operator+(Difference off, const default_construct_iterator& right)
   {  return right + off; }

   default_construct_iterator& operator-=(Difference off)
   {  this->advance(-off); return *this;   }

   default_construct_iterator operator-(Difference off) const
   {  return *this + (-off);  }

   const T& operator*() const
   { return dereference(); }

   const T* operator->() const
   { return &(dereference()); }

   private:
   Difference  m_num;

   void increment()
   { --m_num; }

   void decrement()
   { ++m_num; }

   bool equal(const this_type &other) const
   {  return m_num == other.m_num;   }

   bool less(const this_type &other) const
   {  return other.m_num < m_num;   }

   const T & dereference() const
   { 
      static T dummy;
      return dummy;
   }

   void advance(Difference n)
   {  m_num -= n; }

   Difference distance_to(const this_type &other)const
   {  return m_num - other.m_num;   }
};

template <class T, class Difference = std::ptrdiff_t>
class repeat_iterator
  : public std::iterator
      <std::random_access_iterator_tag, T, Difference>
{
   typedef repeat_iterator<T, Difference> this_type;
   public:
   explicit repeat_iterator(T &ref, Difference range_size)
      :  m_ptr(&ref), m_num(range_size){}

   //Constructors
   repeat_iterator()
      :  m_ptr(0), m_num(0){}

   this_type& operator++() 
   { increment();   return *this;   }
   
   this_type operator++(int)
   {
      this_type result (*this);
      increment();
      return result;
   }

   friend bool operator== (const this_type& i, const this_type& i2)
   { return i.equal(i2); }

   friend bool operator!= (const this_type& i, const this_type& i2)
   { return !(i == i2); }

   friend bool operator< (const this_type& i, const this_type& i2)
   { return i.less(i2); }

   friend bool operator> (const this_type& i, const this_type& i2)
   { return i2 < i; }

   friend bool operator<= (const this_type& i, const this_type& i2)
   { return !(i > i2); }

   friend bool operator>= (const this_type& i, const this_type& i2)
   { return !(i < i2); }

   friend Difference operator- (const this_type& i, const this_type& i2)
   { return i2.distance_to(i); }

   //Arithmetic
   this_type& operator+=(Difference off)
   {  this->advance(off); return *this;   }

   this_type operator+(Difference off) const
   {
      this_type other(*this);
      other.advance(off);
      return other;
   }

   friend this_type operator+(Difference off, const this_type& right)
   {  return right + off; }

   this_type& operator-=(Difference off)
   {  this->advance(-off); return *this;   }

   this_type operator-(Difference off) const
   {  return *this + (-off);  }

   T& operator*() const
   { return dereference(); }

   T *operator->() const
   { return &(dereference()); }

   private:
   T *         m_ptr;
   Difference  m_num;

   void increment()
   { --m_num; }

   void decrement()
   { ++m_num; }

   bool equal(const this_type &other) const
   {  return m_num == other.m_num;   }

   bool less(const this_type &other) const
   {  return other.m_num < m_num;   }

   T & dereference() const
   { return *m_ptr; }

   void advance(Difference n)
   {  m_num -= n; }

   Difference distance_to(const this_type &other)const
   {  return m_num - other.m_num;   }
};

template <class T, class E>
class emplace_iterator
  : public std::iterator
      <std::random_access_iterator_tag, T, std::ptrdiff_t, const T*, const T &>
{
   typedef emplace_iterator this_type;

   public:
   explicit emplace_iterator(E&e)
      :  m_num(1), m_pe(&e){}

   emplace_iterator()
      :  m_num(0), m_pe(0){}

   this_type& operator++() 
   { increment();   return *this;   }
   
   this_type operator++(int)
   {
      this_type result (*this);
      increment();
      return result;
   }

   friend bool operator== (const this_type& i, const this_type& i2)
   { return i.equal(i2); }

   friend bool operator!= (const this_type& i, const this_type& i2)
   { return !(i == i2); }

   friend bool operator< (const this_type& i, const this_type& i2)
   { return i.less(i2); }

   friend bool operator> (const this_type& i, const this_type& i2)
   { return i2 < i; }

   friend bool operator<= (const this_type& i, const this_type& i2)
   { return !(i > i2); }

   friend bool operator>= (const this_type& i, const this_type& i2)
   { return !(i < i2); }

   friend std::ptrdiff_t operator- (const this_type& i, const this_type& i2)
   { return i2.distance_to(i); }

   //Arithmetic
   this_type& operator+=(std::ptrdiff_t off)
   {  this->advance(off); return *this;   }

   this_type operator+(std::ptrdiff_t off) const
   {
      this_type other(*this);
      other.advance(off);
      return other;
   }

   friend this_type operator+(std::ptrdiff_t off, const this_type& right)
   {  return right + off; }

   this_type& operator-=(std::ptrdiff_t off)
   {  this->advance(-off); return *this;   }

   this_type operator-(std::ptrdiff_t off) const
   {  return *this + (-off);  }

   const T& operator*() const
   { return dereference(); }

   const T* operator->() const
   { return &(dereference()); }

   void construct_in_place(T* ptr)
   {  (*m_pe)(ptr);  }

   private:
   std::ptrdiff_t m_num;
   E *            m_pe;

   void increment()
   { --m_num; }

   void decrement()
   { ++m_num; }

   bool equal(const this_type &other) const
   {  return m_num == other.m_num;   }

   bool less(const this_type &other) const
   {  return other.m_num < m_num;   }

   const T & dereference() const
   { 
      static T dummy;
      return dummy;
   }

   void advance(std::ptrdiff_t n)
   {  m_num -= n; }

   std::ptrdiff_t distance_to(const this_type &other)const
   {  return m_num - other.m_num;   }
};

#ifdef BOOST_CONTAINERS_PERFECT_FORWARDING

template<class T, class ...Args>
struct emplace_functor
{
   typedef typename containers_detail::build_number_seq<sizeof...(Args)>::type index_tuple_t;

   emplace_functor(Args&&... args)
      : args_(args...)
   {}

   void operator()(T *ptr)
   {  emplace_functor::inplace_impl(ptr, index_tuple_t());  }

   template<int ...IdxPack>
   void inplace_impl(T* ptr, const containers_detail::index_tuple<IdxPack...>&)
   {  ::new(ptr) T(boost::interprocess::forward<Args>(containers_detail::get<IdxPack>(args_))...); }

   containers_detail::tuple<Args&&...> args_;
};

#else

template<class T>
struct emplace_functor
{
   emplace_functor()
   {}
   void operator()(T *ptr)
   {  new(ptr) T();  }
};

#define BOOST_PP_LOCAL_MACRO(n)                                                        \
   template <class T, BOOST_PP_ENUM_PARAMS(n, class P) >                               \
   struct BOOST_PP_CAT(BOOST_PP_CAT(emplace_functor, n), arg)                          \
   {                                                                                   \
      BOOST_PP_CAT(BOOST_PP_CAT(emplace_functor, n), arg)                              \
         ( BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_LIST, _) )                       \
      :  BOOST_PP_ENUM(n, BOOST_CONTAINERS_AUX_PARAM_INIT, _) {}                       \
                                                                                       \
      void operator()(T *ptr)                                                          \
      {                                                                                \
         new(ptr)T (BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_MEMBER_FORWARD, _));          \
      }                                                                                \
      BOOST_PP_REPEAT(n, BOOST_CONTAINERS_AUX_PARAM_DEFINE, _)                         \
   };                                                                                  \
   //!
#define BOOST_PP_LOCAL_LIMITS (1, BOOST_CONTAINERS_MAX_CONSTRUCTOR_PARAMETERS)
#include BOOST_PP_LOCAL_ITERATE()

#endif

}  //namespace container { 
}  //namespace boost {

#include <boost/interprocess/containers/container/detail/config_end.hpp>

#endif   //#ifndef BOOST_CONTAINERS_DETAIL_ITERATORS_HPP

