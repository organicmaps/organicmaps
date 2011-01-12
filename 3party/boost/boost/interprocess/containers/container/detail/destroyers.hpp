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

#ifndef BOOST_CONTAINERS_DESTROYERS_HPP
#define BOOST_CONTAINERS_DESTROYERS_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include "config_begin.hpp"
#include INCLUDE_BOOST_CONTAINER_DETAIL_WORKAROUND_HPP
#include INCLUDE_BOOST_CONTAINER_DETAIL_VERSION_TYPE_HPP
#include INCLUDE_BOOST_CONTAINER_DETAIL_UTILITIES_HPP

namespace boost {
namespace container { 
namespace containers_detail {

//!A deleter for scoped_ptr that deallocates the memory
//!allocated for an array of objects using a STL allocator.
template <class Allocator>
struct scoped_array_deallocator
{
   typedef typename Allocator::pointer    pointer;
   typedef typename Allocator::size_type  size_type;

   scoped_array_deallocator(pointer p, Allocator& a, size_type length)
      : m_ptr(p), m_alloc(a), m_length(length) {}

   ~scoped_array_deallocator()
   {  if (m_ptr) m_alloc.deallocate(m_ptr, m_length);  }

   void release()
   {  m_ptr = 0; }

   private:
   pointer     m_ptr;
   Allocator&  m_alloc;
   size_type   m_length;
};

template <class Allocator>
struct null_scoped_array_deallocator
{
   typedef typename Allocator::pointer    pointer;
   typedef typename Allocator::size_type  size_type;

   null_scoped_array_deallocator(pointer, Allocator&, size_type)
   {}

   void release()
   {}
};


//!A deleter for scoped_ptr that destroys
//!an object using a STL allocator.
template <class Allocator>
struct scoped_destructor_n
{
   typedef typename Allocator::pointer    pointer;
   typedef typename Allocator::value_type value_type;
   typedef typename Allocator::size_type  size_type;

   pointer     m_p;
   size_type   m_n;

   scoped_destructor_n(pointer p, size_type n)
      : m_p(p), m_n(n)
   {}

   void release()
   {  m_p = 0; }

   void increment_size(size_type inc)
   {  m_n += inc;   }
   
   ~scoped_destructor_n()
   {
      if(!m_p) return;
      value_type *raw_ptr = containers_detail::get_pointer(m_p);
      for(std::size_t i = 0; i < m_n; ++i, ++raw_ptr)
         raw_ptr->~value_type();
   }
};

//!A deleter for scoped_ptr that destroys
//!an object using a STL allocator.
template <class Allocator>
struct null_scoped_destructor_n
{
   typedef typename Allocator::pointer pointer;
   typedef typename Allocator::size_type size_type;

   null_scoped_destructor_n(pointer, size_type)
   {}

   void increment_size(size_type)
   {}

   void release()
   {}
};

template <class A>
class allocator_destroyer
{
   typedef typename A::value_type value_type;
   typedef containers_detail::integral_constant<unsigned,
      boost::container::containers_detail::
         version<A>::value>                           alloc_version;
   typedef containers_detail::integral_constant<unsigned, 1>     allocator_v1;
   typedef containers_detail::integral_constant<unsigned, 2>     allocator_v2;

   private:
   A & a_;

   private:
   void priv_deallocate(const typename A::pointer &p, allocator_v1)
   {  a_.deallocate(p, 1); }

   void priv_deallocate(const typename A::pointer &p, allocator_v2)
   {  a_.deallocate_one(p); }

   public:
   allocator_destroyer(A &a)
      :  a_(a)
   {}

   void operator()(const typename A::pointer &p)
   {  
      containers_detail::get_pointer(p)->~value_type();
      priv_deallocate(p, alloc_version());
   }
};


}  //namespace containers_detail { 
}  //namespace container { 
}  //namespace boost {

#include INCLUDE_BOOST_CONTAINER_DETAIL_CONFIG_END_HPP

#endif   //#ifndef BOOST_CONTAINERS_DESTROYERS_HPP
