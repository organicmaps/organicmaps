#ifndef BOOST_INTERPROCESS_DETAIL_SP_COUNTED_IMPL_HPP_INCLUDED
#define BOOST_INTERPROCESS_DETAIL_SP_COUNTED_IMPL_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//
//  This file is the adaptation for shared memory memory mapped
//  files of boost/detail/sp_counted_impl.hpp
//
//  Copyright (c) 2001, 2002, 2003 Peter Dimov and Multi Media Ltd.
//  Copyright 2004-2005 Peter Dimov
//  Copyright 2006      Ion Gaztanaga
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/smart_ptr/detail/sp_counted_base.hpp>
#include <boost/interprocess/smart_ptr/scoped_ptr.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/pointer_to_other.hpp>

namespace boost {

namespace interprocess {

namespace detail {

//!A deleter for scoped_ptr that deallocates the memory
//!allocated for an object using a STL allocator.
template <class Allocator>
struct scoped_ptr_dealloc_functor
{
   typedef typename Allocator::pointer pointer;
   typedef detail::integral_constant<unsigned,
      boost::interprocess::version<Allocator>::value>                   alloc_version;
   typedef detail::integral_constant<unsigned, 1>     allocator_v1;
   typedef detail::integral_constant<unsigned, 2>     allocator_v2;

   private:
   void priv_deallocate(const typename Allocator::pointer &p, allocator_v1)
   {  m_alloc.deallocate(p, 1); }

   void priv_deallocate(const typename Allocator::pointer &p, allocator_v2)
   {  m_alloc.deallocate_one(p); }

   public:
   Allocator& m_alloc;

   scoped_ptr_dealloc_functor(Allocator& a)
      : m_alloc(a) {}

   void operator()(pointer ptr)
   {  if (ptr) priv_deallocate(ptr, alloc_version());  }
};

template<class A, class D>
class sp_counted_impl_pd 
   :  public sp_counted_base
   ,  A::template rebind< sp_counted_impl_pd<A, D> >::other
   ,  D  // copy constructor must not throw
{
   private:
   typedef sp_counted_impl_pd<A, D>          this_type;
   typedef typename A::template rebind
      <this_type>::other                     this_allocator;
   typedef typename A::template rebind
      <const this_type>::other               const_this_allocator;
   typedef typename this_allocator::pointer  this_pointer;

   sp_counted_impl_pd( sp_counted_impl_pd const & );
   sp_counted_impl_pd & operator= ( sp_counted_impl_pd const & );

   typedef typename boost::pointer_to_other
            <typename A::pointer, const D>::type   const_deleter_pointer;

   typedef typename boost::pointer_to_other
            <typename A::pointer, const A>::type   const_allocator_pointer;

   typedef typename D::pointer   pointer;
   pointer m_ptr;

   public:
   // pre: d(p) must not throw
   template<class Ptr>
   sp_counted_impl_pd(const Ptr & p, const A &a, const D &d )
      :  this_allocator(a), D(d), m_ptr(p)
   {}

   const_deleter_pointer get_deleter() const
   {  return const_deleter_pointer(&static_cast<const D&>(*this)); }

   const_allocator_pointer get_allocator() const
   {  return const_allocator_pointer(&static_cast<const A&>(*this)); }

   void dispose() // nothrow
   {  static_cast<D&>(*this)(m_ptr);   }

   void destroy() // nothrow
   {
      //Self destruction, so get a copy of the allocator
      //(in the future we could move it)
      this_allocator a_copy(*this);
      BOOST_ASSERT(a_copy == *this);
      this_pointer this_ptr (this);
      //Do it now!
      scoped_ptr< this_type, scoped_ptr_dealloc_functor<this_allocator> >
         deleter(this_ptr, a_copy);
      typedef typename this_allocator::value_type value_type;
      detail::get_pointer(this_ptr)->~value_type();
   }

   void release() // nothrow
   {
      if(this->ref_release()){
         this->dispose();
         this->weak_release();
      }
   }

   void weak_release() // nothrow
   {
      if(sp_counted_base::weak_release()){
         this->destroy();
      }
   }
};


} // namespace detail

} // namespace interprocess

} // namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif  // #ifndef BOOST_INTERPROCESS_DETAIL_SP_COUNTED_IMPL_HPP_INCLUDED
