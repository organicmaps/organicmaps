/*
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Hewlett-Packard Company makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 *
 * Copyright (c) 1996
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 */
//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////
//
// This file comes from SGI's stl_deque.h and stl_uninitialized.h files. 
// Modified by Ion Gaztanaga 2005.
// Renaming, isolating and porting to generic algorithms. Pointer typedef 
// set to allocator::pointer to allow placing it in shared memory.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_CONTAINERS_DEQUE_HPP
#define BOOST_CONTAINERS_DEQUE_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/containers/container/detail/config_begin.hpp>
#include <boost/interprocess/containers/container/detail/workaround.hpp>

#include <boost/interprocess/containers/container/detail/utilities.hpp>
#include <boost/interprocess/containers/container/detail/iterators.hpp>
#include <boost/interprocess/containers/container/detail/algorithms.hpp>
#include <boost/interprocess/containers/container/detail/mpl.hpp>
#include <boost/interprocess/containers/container/container_fwd.hpp>
#include <cstddef>
#include <iterator>
#include <cassert>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <boost/detail/no_exceptions_support.hpp>
#include <boost/type_traits/has_trivial_destructor.hpp>
#include <boost/type_traits/has_trivial_copy.hpp>
#include <boost/type_traits/has_trivial_assign.hpp>
#include <boost/type_traits/has_nothrow_copy.hpp>
#include <boost/type_traits/has_nothrow_assign.hpp>
#include <boost/interprocess/detail/move.hpp>
#include <boost/interprocess/containers/container/detail/advanced_insert_int.hpp>

#ifdef BOOST_CONTAINER_DOXYGEN_INVOKED
namespace boost {
namespace container {
#else
namespace boost {
namespace container {
#endif

/// @cond
template <class T, class Alloc>
class deque;

template <class T, class A>
struct deque_value_traits
{
   typedef T value_type;
   typedef A allocator_type;
   static const bool trivial_dctr = boost::has_trivial_destructor<value_type>::value;
   static const bool trivial_dctr_after_move = false;
      //::boost::has_trivial_destructor_after_move<value_type>::value || trivial_dctr;
   static const bool trivial_copy = has_trivial_copy<value_type>::value;
   static const bool nothrow_copy = has_nothrow_copy<value_type>::value;
   static const bool trivial_assign = has_trivial_assign<value_type>::value;
   static const bool nothrow_assign = has_nothrow_assign<value_type>::value;

};

// Note: this function is simply a kludge to work around several compilers'
//  bugs in handling constant expressions.
inline std::size_t deque_buf_size(std::size_t size) 
   {  return size < 512 ? std::size_t(512 / size) : std::size_t(1);  }

// Deque base class.  It has two purposes.  First, its constructor
//  and destructor allocate (but don't initialize) storage.  This makes
//  exception safety easier.
template <class T, class Alloc>
class deque_base
{
   public:
   typedef typename Alloc::value_type              val_alloc_val;
   typedef typename Alloc::pointer                 val_alloc_ptr;
   typedef typename Alloc::const_pointer           val_alloc_cptr;
   typedef typename Alloc::reference               val_alloc_ref;
   typedef typename Alloc::const_reference         val_alloc_cref;
   typedef typename Alloc::value_type              val_alloc_diff;
   typedef typename Alloc::template rebind
      <typename Alloc::pointer>::other             ptr_alloc_t;
   typedef typename ptr_alloc_t::value_type        ptr_alloc_val;
   typedef typename ptr_alloc_t::pointer           ptr_alloc_ptr;
   typedef typename ptr_alloc_t::const_pointer     ptr_alloc_cptr;
   typedef typename ptr_alloc_t::reference         ptr_alloc_ref;
   typedef typename ptr_alloc_t::const_reference   ptr_alloc_cref;
   typedef typename Alloc::template
      rebind<T>::other                             allocator_type;
   typedef allocator_type                          stored_allocator_type;

   protected:

   typedef deque_value_traits<T, Alloc>            traits_t;
   typedef typename Alloc::template
      rebind<typename Alloc::pointer>::other map_allocator_type;

   static std::size_t s_buffer_size() { return deque_buf_size(sizeof(T)); }

   val_alloc_ptr priv_allocate_node() 
      {  return this->alloc().allocate(s_buffer_size());  }

   void priv_deallocate_node(val_alloc_ptr p) 
      {  this->alloc().deallocate(p, s_buffer_size());  }

   ptr_alloc_ptr priv_allocate_map(std::size_t n) 
      { return this->ptr_alloc().allocate(n); }

   void priv_deallocate_map(ptr_alloc_ptr p, std::size_t n) 
      { this->ptr_alloc().deallocate(p, n); }

 public:
   // Class invariants:
   //  For any nonsingular iterator i:
   //    i.node is the address of an element in the map array.  The
   //      contents of i.node is a pointer to the beginning of a node.
   //    i.first == //(i.node) 
   //    i.last  == i.first + node_size
   //    i.cur is a pointer in the range [i.first, i.last).  NOTE:
   //      the implication of this is that i.cur is always a dereferenceable
   //      pointer, even if i is a past-the-end iterator.
   //  Start and Finish are always nonsingular iterators.  NOTE: this means
   //    that an empty deque must have one node, and that a deque
   //    with N elements, where N is the buffer size, must have two nodes.
   //  For every node other than start.node and finish.node, every element
   //    in the node is an initialized object.  If start.node == finish.node,
   //    then [start.cur, finish.cur) are initialized objects, and
   //    the elements outside that range are uninitialized storage.  Otherwise,
   //    [start.cur, start.last) and [finish.first, finish.cur) are initialized
   //    objects, and [start.first, start.cur) and [finish.cur, finish.last)
   //    are uninitialized storage.
   //  [map, map + map_size) is a valid, non-empty range.  
   //  [start.node, finish.node] is a valid range contained within 
   //    [map, map + map_size).  
   //  A pointer in the range [map, map + map_size) points to an allocated node
   //    if and only if the pointer is in the range [start.node, finish.node].
   class const_iterator 
      : public std::iterator<std::random_access_iterator_tag, 
                              val_alloc_val,  val_alloc_diff, 
                              val_alloc_cptr, val_alloc_cref>
   {
      public:
      static std::size_t s_buffer_size() { return deque_base<T, Alloc>::s_buffer_size(); }

      typedef std::random_access_iterator_tag   iterator_category;
      typedef val_alloc_val                     value_type;
      typedef val_alloc_cptr                    pointer;
      typedef val_alloc_cref                    reference;
      typedef std::size_t                       size_type;
      typedef std::ptrdiff_t                    difference_type;

      typedef ptr_alloc_ptr                     index_pointer;
      typedef const_iterator                    self_t;

      friend class deque<T, Alloc>;
      friend class deque_base<T, Alloc>;

      protected: 
      val_alloc_ptr  m_cur;
      val_alloc_ptr  m_first;
      val_alloc_ptr  m_last;
      index_pointer  m_node;

      public: 
      const_iterator(val_alloc_ptr x, index_pointer y) 
         : m_cur(x), m_first(*y),
           m_last(*y + s_buffer_size()), m_node(y) {}

      const_iterator() : m_cur(0), m_first(0), m_last(0), m_node(0) {}

      const_iterator(const const_iterator& x)
         : m_cur(x.m_cur),   m_first(x.m_first), 
           m_last(x.m_last), m_node(x.m_node) {}

      reference operator*() const 
         { return *this->m_cur; }

      pointer operator->() const 
         { return this->m_cur; }

      difference_type operator-(const self_t& x) const 
      {
         if(!this->m_cur && !x.m_cur){
            return 0;
         }
         return difference_type(this->s_buffer_size()) * (this->m_node - x.m_node - 1) +
            (this->m_cur - this->m_first) + (x.m_last - x.m_cur);
      }

      self_t& operator++() 
      {
         ++this->m_cur;
         if (this->m_cur == this->m_last) {
            this->priv_set_node(this->m_node + 1);
            this->m_cur = this->m_first;
         }
         return *this; 
      }

      self_t operator++(int)  
      {
         self_t tmp = *this;
         ++*this;
         return tmp;
      }

      self_t& operator--() 
      {
         if (this->m_cur == this->m_first) {
            this->priv_set_node(this->m_node - 1);
            this->m_cur = this->m_last;
         }
         --this->m_cur;
         return *this;
      }

      self_t operator--(int) 
      {
         self_t tmp = *this;
         --*this;
         return tmp;
      }

      self_t& operator+=(difference_type n)
      {
         difference_type offset = n + (this->m_cur - this->m_first);
         if (offset >= 0 && offset < difference_type(this->s_buffer_size()))
            this->m_cur += n;
         else {
            difference_type node_offset =
            offset > 0 ? offset / difference_type(this->s_buffer_size())
                        : -difference_type((-offset - 1) / this->s_buffer_size()) - 1;
            this->priv_set_node(this->m_node + node_offset);
            this->m_cur = this->m_first + 
            (offset - node_offset * difference_type(this->s_buffer_size()));
         }
         return *this;
      }

      self_t operator+(difference_type n) const
         {  self_t tmp = *this; return tmp += n;  }

      self_t& operator-=(difference_type n) 
         { return *this += -n; }
       
      self_t operator-(difference_type n) const 
         {  self_t tmp = *this; return tmp -= n;  }

      reference operator[](difference_type n) const 
         { return *(*this + n); }

      bool operator==(const self_t& x) const 
         { return this->m_cur == x.m_cur; }

      bool operator!=(const self_t& x) const 
         { return !(*this == x); }

      bool operator<(const self_t& x) const 
      {
         return (this->m_node == x.m_node) ? 
            (this->m_cur < x.m_cur) : (this->m_node < x.m_node);
      }

      bool operator>(const self_t& x) const  
         { return x < *this; }

      bool operator<=(const self_t& x) const 
         { return !(x < *this); }

      bool operator>=(const self_t& x) const 
         { return !(*this < x); }

      void priv_set_node(index_pointer new_node) 
      {
         this->m_node = new_node;
         this->m_first = *new_node;
         this->m_last = this->m_first + difference_type(this->s_buffer_size());
      }

      friend const_iterator operator+(std::ptrdiff_t n, const const_iterator& x)
         {  return x + n;  }
   };

   //Deque iterator
   class iterator : public const_iterator
   {
      public:
      typedef std::random_access_iterator_tag   iterator_category;
      typedef val_alloc_val                     value_type;
      typedef val_alloc_ptr                     pointer;
      typedef val_alloc_ref                     reference;
      typedef std::size_t                       size_type;
      typedef std::ptrdiff_t                    difference_type;
      typedef ptr_alloc_ptr                     index_pointer;
      typedef const_iterator                    self_t;

      friend class deque<T, Alloc>;
      friend class deque_base<T, Alloc>;

      private:
      explicit iterator(const const_iterator& x) : const_iterator(x){}

      public:
      //Constructors
      iterator(val_alloc_ptr x, index_pointer y) : const_iterator(x, y){}
      iterator() : const_iterator(){}
      //iterator(const const_iterator &cit) : const_iterator(cit){}
      iterator(const iterator& x) : const_iterator(x){}

      //Pointer like operators
      reference operator*() const { return *this->m_cur; }
      pointer operator->() const { return this->m_cur; }

      reference operator[](difference_type n) const { return *(*this + n); }

      //Increment / Decrement
      iterator& operator++()  
         { this->const_iterator::operator++(); return *this;  }

      iterator operator++(int)
         { iterator tmp = *this; ++*this; return tmp; }
      
      iterator& operator--()
         {  this->const_iterator::operator--(); return *this;  }

      iterator operator--(int)
         {  iterator tmp = *this; --*this; return tmp; }

      // Arithmetic
      iterator& operator+=(difference_type off)
         {  this->const_iterator::operator+=(off); return *this;  }

      iterator operator+(difference_type off) const
         {  return iterator(this->const_iterator::operator+(off));  }

      friend iterator operator+(difference_type off, const iterator& right)
         {  return iterator(off+static_cast<const const_iterator &>(right)); }

      iterator& operator-=(difference_type off)
         {  this->const_iterator::operator-=(off); return *this;   }

      iterator operator-(difference_type off) const
         {  return iterator(this->const_iterator::operator-(off));  }

      difference_type operator-(const const_iterator& right) const
         {  return static_cast<const const_iterator&>(*this) - right;   }
   };

   deque_base(const allocator_type& a, std::size_t num_elements)
      :  members_(a)
   { this->priv_initialize_map(num_elements); }

   deque_base(const allocator_type& a) 
      :  members_(a)
   {}

   ~deque_base()
   {
      if (this->members_.m_map) {
         this->priv_destroy_nodes(this->members_.m_start.m_node, this->members_.m_finish.m_node + 1);
         this->priv_deallocate_map(this->members_.m_map, this->members_.m_map_size);
      }
   }

   private:
   deque_base(const deque_base&);
  
   protected:

   void priv_initialize_map(std::size_t num_elements)
   {
//      if(num_elements){
         std::size_t num_nodes = num_elements / s_buffer_size() + 1;

         this->members_.m_map_size = containers_detail::max_value((std::size_t) InitialMapSize, num_nodes + 2);
         this->members_.m_map = this->priv_allocate_map(this->members_.m_map_size);

         ptr_alloc_ptr nstart = this->members_.m_map + (this->members_.m_map_size - num_nodes) / 2;
         ptr_alloc_ptr nfinish = nstart + num_nodes;
             
         BOOST_TRY {
            this->priv_create_nodes(nstart, nfinish);
         }
         BOOST_CATCH(...){
            this->priv_deallocate_map(this->members_.m_map, this->members_.m_map_size);
            this->members_.m_map = 0;
            this->members_.m_map_size = 0;
            BOOST_RETHROW
         }
         BOOST_CATCH_END

         this->members_.m_start.priv_set_node(nstart);
         this->members_.m_finish.priv_set_node(nfinish - 1);
         this->members_.m_start.m_cur = this->members_.m_start.m_first;
         this->members_.m_finish.m_cur = this->members_.m_finish.m_first +
                        num_elements % s_buffer_size();
//      }
   }

   void priv_create_nodes(ptr_alloc_ptr nstart, ptr_alloc_ptr nfinish)
   {
      ptr_alloc_ptr cur;
      BOOST_TRY {
         for (cur = nstart; cur < nfinish; ++cur)
            *cur = this->priv_allocate_node();
      }
      BOOST_CATCH(...){
         this->priv_destroy_nodes(nstart, cur);
         BOOST_RETHROW
      }
      BOOST_CATCH_END
   }

   void priv_destroy_nodes(ptr_alloc_ptr nstart, ptr_alloc_ptr nfinish)
   {
      for (ptr_alloc_ptr n = nstart; n < nfinish; ++n)
         this->priv_deallocate_node(*n);
   }

   enum { InitialMapSize = 8 };

   protected:
   struct members_holder
      :  public ptr_alloc_t
      ,  public allocator_type
   {
      members_holder(const allocator_type &a)
         :  map_allocator_type(a), allocator_type(a)
         ,  m_map(0), m_map_size(0)
         ,  m_start(), m_finish(m_start)
      {}

      ptr_alloc_ptr   m_map;
      std::size_t     m_map_size;
      iterator        m_start;
      iterator        m_finish;
   } members_;

   ptr_alloc_t &ptr_alloc() 
   {  return members_;  }
   
   const ptr_alloc_t &ptr_alloc() const 
   {  return members_;  }

   allocator_type &alloc() 
   {  return members_;  }
   
   const allocator_type &alloc() const 
   {  return members_;  }
};
/// @endcond

//! Deque class
//!
template <class T, class Alloc>
class deque : protected deque_base<T, Alloc>
{
   /// @cond
   typedef typename containers_detail::
      move_const_ref_type<T>::type                    insert_const_ref_type;
  typedef deque_base<T, Alloc> Base;

   public:                         // Basic types
   typedef typename Alloc::value_type           val_alloc_val;
   typedef typename Alloc::pointer              val_alloc_ptr;
   typedef typename Alloc::const_pointer        val_alloc_cptr;
   typedef typename Alloc::reference            val_alloc_ref;
   typedef typename Alloc::const_reference      val_alloc_cref;
   typedef typename Alloc::template
      rebind<val_alloc_ptr>::other                ptr_alloc_t;
   typedef typename ptr_alloc_t::value_type       ptr_alloc_val;
   typedef typename ptr_alloc_t::pointer          ptr_alloc_ptr;
   typedef typename ptr_alloc_t::const_pointer    ptr_alloc_cptr;
   typedef typename ptr_alloc_t::reference        ptr_alloc_ref;
   typedef typename ptr_alloc_t::const_reference  ptr_alloc_cref;
   /// @endcond

   typedef T                                    value_type;
   typedef val_alloc_ptr                        pointer;
   typedef val_alloc_cptr                       const_pointer;
   typedef val_alloc_ref                        reference;
   typedef val_alloc_cref                       const_reference;
   typedef std::size_t                          size_type;
   typedef std::ptrdiff_t                       difference_type;

   typedef typename Base::allocator_type        allocator_type;

   public:                                // Iterators
   typedef typename Base::iterator              iterator;
   typedef typename Base::const_iterator        const_iterator;

   typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
   typedef std::reverse_iterator<iterator>      reverse_iterator;

   /// @cond
   private:                      // Internal typedefs
   BOOST_COPYABLE_AND_MOVABLE(deque)
   typedef ptr_alloc_ptr index_pointer;
   static std::size_t s_buffer_size() 
      { return Base::s_buffer_size(); }
   typedef containers_detail::advanced_insert_aux_int<value_type, iterator> advanced_insert_aux_int_t;
   typedef repeat_iterator<T, difference_type>  r_iterator;
   typedef boost::interprocess::move_iterator<r_iterator>    move_it;

   /// @endcond

   allocator_type get_allocator() const { return Base::alloc(); }

   public:                         // Basic accessors

   iterator begin() 
      { return this->members_.m_start; }

   iterator end() 
      { return this->members_.m_finish; }

   const_iterator begin() const 
      { return this->members_.m_start; }

   const_iterator end() const 
      { return this->members_.m_finish; }

   reverse_iterator rbegin() 
      { return reverse_iterator(this->members_.m_finish); }

   reverse_iterator rend() 
      { return reverse_iterator(this->members_.m_start); }

   const_reverse_iterator rbegin() const 
      { return const_reverse_iterator(this->members_.m_finish); }

   const_reverse_iterator rend() const 
      { return const_reverse_iterator(this->members_.m_start); }

   const_iterator cbegin() const 
      { return this->members_.m_start; }

   const_iterator cend() const 
      { return this->members_.m_finish; }

   const_reverse_iterator crbegin() const 
      { return const_reverse_iterator(this->members_.m_finish); }

   const_reverse_iterator crend() const 
      { return const_reverse_iterator(this->members_.m_start); }

   reference operator[](size_type n)
      { return this->members_.m_start[difference_type(n)]; }

   const_reference operator[](size_type n) const 
      { return this->members_.m_start[difference_type(n)]; }

   void priv_range_check(size_type n) const 
      {  if (n >= this->size())  BOOST_RETHROW std::out_of_range("deque");   }

   reference at(size_type n)
      { this->priv_range_check(n); return (*this)[n]; }

   const_reference at(size_type n) const
      { this->priv_range_check(n); return (*this)[n]; }

   reference front() { return *this->members_.m_start; }

   reference back()  {  return *(end()-1); }

   const_reference front() const 
      { return *this->members_.m_start; }

   const_reference back() const  {  return *(cend()-1);  }

   size_type size() const 
      { return this->members_.m_finish - this->members_.m_start; }

   size_type max_size() const 
      { return this->alloc().max_size(); }

   bool empty() const 
   { return this->members_.m_finish == this->members_.m_start; }

   explicit deque(const allocator_type& a = allocator_type()) 
      : Base(a)
   {}

   deque(const deque& x)
      :  Base(x.alloc()) 
   {
      if(x.size()){
         this->priv_initialize_map(x.size());
         std::uninitialized_copy(x.begin(), x.end(), this->members_.m_start);
      }
   }

   deque(BOOST_INTERPROCESS_RV_REF(deque) mx) 
      :  Base(mx.alloc())
   {  this->swap(mx);   }

   deque(size_type n, const value_type& value,
         const allocator_type& a = allocator_type()) : Base(a, n)
   { this->priv_fill_initialize(value); }

   explicit deque(size_type n) : Base(allocator_type(), n)
   {  this->resize(n); }

   // Check whether it's an integral type.  If so, it's not an iterator.
   template <class InpIt>
   deque(InpIt first, InpIt last, const allocator_type& a = allocator_type())
      : Base(a) 
   {
      //Dispatch depending on integer/iterator
      const bool aux_boolean = containers_detail::is_convertible<InpIt, std::size_t>::value;
      typedef containers_detail::bool_<aux_boolean> Result;
      this->priv_initialize_dispatch(first, last, Result());
   }

   ~deque() 
   {
      priv_destroy_range(this->members_.m_start, this->members_.m_finish);
   }

   deque& operator= (BOOST_INTERPROCESS_COPY_ASSIGN_REF(deque) x) 
   {
      const size_type len = size();
      if (&x != this) {
         if (len >= x.size())
            this->erase(std::copy(x.begin(), x.end(), this->members_.m_start), this->members_.m_finish);
         else {
            const_iterator mid = x.begin() + difference_type(len);
            std::copy(x.begin(), mid, this->members_.m_start);
            this->insert(this->members_.m_finish, mid, x.end());
         }
      }
      return *this;
   }        

   deque& operator= (BOOST_INTERPROCESS_RV_REF(deque) x)
   {
      this->clear();
      this->swap(x);
      return *this;
   }

   void swap(deque &x)
   {
      std::swap(this->members_.m_start, x.members_.m_start);
      std::swap(this->members_.m_finish, x.members_.m_finish);
      std::swap(this->members_.m_map, x.members_.m_map);
      std::swap(this->members_.m_map_size, x.members_.m_map_size);
   }

   void assign(size_type n, const T& val)
   {  this->priv_fill_assign(n, val);  }

   template <class InpIt>
   void assign(InpIt first, InpIt last)
   {
      //Dispatch depending on integer/iterator
      const bool aux_boolean = containers_detail::is_convertible<InpIt, std::size_t>::value;
      typedef containers_detail::bool_<aux_boolean> Result;
      this->priv_assign_dispatch(first, last, Result());
   }

   #if !defined(BOOST_HAS_RVALUE_REFS) && !defined(BOOST_MOVE_DOXYGEN_INVOKED)
   void push_back(T &x) { push_back(const_cast<const T &>(x)); }

   template<class U>
   void push_back(const U &u, typename containers_detail::enable_if_c<containers_detail::is_same<T, U>::value && !::boost::interprocess::is_movable<U>::value >::type* =0)
   { return priv_push_back(u); }
   #endif

   void push_back(insert_const_ref_type t)
   {  return priv_push_back(t);  }

   void push_back(BOOST_INTERPROCESS_RV_REF(value_type) t) 
   {
      if(this->priv_push_back_simple_available()){
         new(this->priv_push_back_simple_pos())value_type(boost::interprocess::move(t));
         this->priv_push_back_simple_commit();
      }
      else{
         this->priv_insert_aux(cend(), move_it(r_iterator(t, 1)), move_it(r_iterator()));
      }
   }

   #if !defined(BOOST_HAS_RVALUE_REFS) && !defined(BOOST_MOVE_DOXYGEN_INVOKED)
   void push_front(T &x) { push_front(const_cast<const T &>(x)); }

   template<class U>
   void push_front(const U &u, typename containers_detail::enable_if_c<containers_detail::is_same<T, U>::value && !::boost::interprocess::is_movable<U>::value >::type* =0)
   { return priv_push_front(u); }
   #endif

   void push_front(insert_const_ref_type t)
   { return priv_push_front(t); }

   void push_front(BOOST_INTERPROCESS_RV_REF(value_type) t)
   {
      if(this->priv_push_front_simple_available()){
         new(this->priv_push_front_simple_pos())value_type(boost::interprocess::move(t));
         this->priv_push_front_simple_commit();
      }
      else{
         this->priv_insert_aux(cbegin(), move_it(r_iterator(t, 1)), move_it(r_iterator()));
      }
   }

   void pop_back() 
   {
      if (this->members_.m_finish.m_cur != this->members_.m_finish.m_first) {
         --this->members_.m_finish.m_cur;
         containers_detail::get_pointer(this->members_.m_finish.m_cur)->~value_type();
      }
      else
         this->priv_pop_back_aux();
   }

   void pop_front() 
   {
      if (this->members_.m_start.m_cur != this->members_.m_start.m_last - 1) {
         containers_detail::get_pointer(this->members_.m_start.m_cur)->~value_type();
         ++this->members_.m_start.m_cur;
      }
      else 
         this->priv_pop_front_aux();
   }

   #if !defined(BOOST_HAS_RVALUE_REFS) && !defined(BOOST_MOVE_DOXYGEN_INVOKED)
   iterator insert(const_iterator position, T &x)
   { return this->insert(position, const_cast<const T &>(x)); }

   template<class U>
   iterator insert(const_iterator position, const U &u, typename containers_detail::enable_if_c<containers_detail::is_same<T, U>::value && !::boost::interprocess::is_movable<U>::value >::type* =0)
   {  return this->priv_insert(position, u); }
   #endif

   iterator insert(const_iterator position, insert_const_ref_type x) 
   {  return this->priv_insert(position, x); }

   iterator insert(const_iterator position, BOOST_INTERPROCESS_RV_REF(value_type) mx) 
   {
      if (position == cbegin()) {
         this->push_front(boost::interprocess::move(mx));
         return begin();
      }
      else if (position == cend()) {
         this->push_back(boost::interprocess::move(mx));
         return(end()-1);
      }
      else {
         //Just call more general insert(pos, size, value) and return iterator
         size_type n = position - begin();
         this->priv_insert_aux(position, move_it(r_iterator(mx, 1)), move_it(r_iterator()));
         return iterator(this->begin() + n);
      }
   }

   void insert(const_iterator pos, size_type n, const value_type& x)
   { this->priv_fill_insert(pos, n, x); }

   // Check whether it's an integral type.  If so, it's not an iterator.
   template <class InpIt>
   void insert(const_iterator pos, InpIt first, InpIt last) 
   {
      //Dispatch depending on integer/iterator
      const bool aux_boolean = containers_detail::is_convertible<InpIt, std::size_t>::value;
      typedef containers_detail::bool_<aux_boolean> Result;
      this->priv_insert_dispatch(pos, first, last, Result());
   }

   #if defined(BOOST_CONTAINERS_PERFECT_FORWARDING) || defined(BOOST_CONTAINER_DOXYGEN_INVOKED)

   template <class... Args>
   void emplace_back(Args&&... args)
   {
      if(this->priv_push_back_simple_available()){
         new(this->priv_push_back_simple_pos())value_type(boost::interprocess::forward<Args>(args)...);
         this->priv_push_back_simple_commit();
      }
      else{
         typedef containers_detail::advanced_insert_aux_emplace<T, iterator, Args...> type;
         type &&proxy = type(boost::interprocess::forward<Args>(args)...);
         this->priv_insert_aux_impl(this->cend(), 1, proxy);
      }
   }

   template <class... Args>
   void emplace_front(Args&&... args)
   {
      if(this->priv_push_front_simple_available()){
         new(this->priv_push_front_simple_pos())value_type(boost::interprocess::forward<Args>(args)...);
         this->priv_push_front_simple_commit();
      }
      else{
         typedef containers_detail::advanced_insert_aux_emplace<T, iterator, Args...> type;
         type &&proxy = type(boost::interprocess::forward<Args>(args)...);
         this->priv_insert_aux_impl(this->cbegin(), 1, proxy);
      }
   }

   template <class... Args>
   iterator emplace(const_iterator p, Args&&... args)
   {
      if(p == this->cbegin()){
         this->emplace_front(boost::interprocess::forward<Args>(args)...);
         return this->begin();
      }
      else if(p == this->cend()){
         this->emplace_back(boost::interprocess::forward<Args>(args)...);
         return (this->end()-1);
      }
      else{
         size_type n = p - this->cbegin();
         typedef containers_detail::advanced_insert_aux_emplace<T, iterator, Args...> type;
         type &&proxy = type(boost::interprocess::forward<Args>(args)...);
         this->priv_insert_aux_impl(p, 1, proxy);
         return iterator(this->begin() + n);
      }
   }

   #else //#ifdef BOOST_CONTAINERS_PERFECT_FORWARDING

   //0 args
   void emplace_back()
   {
      if(priv_push_front_simple_available()){
         new(priv_push_front_simple_pos())value_type();
         priv_push_front_simple_commit();
      }
      else{
         containers_detail::advanced_insert_aux_emplace<T, iterator> proxy;
         priv_insert_aux_impl(cend(), 1, proxy);
      }
   }

   void emplace_front()
   {
      if(priv_push_front_simple_available()){
         new(priv_push_front_simple_pos())value_type();
         priv_push_front_simple_commit();
      }
      else{
         containers_detail::advanced_insert_aux_emplace<T, iterator> proxy;
         priv_insert_aux_impl(cbegin(), 1, proxy);
      }
   }

   iterator emplace(const_iterator p)
   {
      if(p == cbegin()){
         emplace_front();
         return begin();
      }
      else if(p == cend()){
         emplace_back();
         return (end()-1);
      }
      else{
         size_type n = p - cbegin();
         containers_detail::advanced_insert_aux_emplace<T, iterator> proxy;
         priv_insert_aux_impl(p, 1, proxy);
         return iterator(this->begin() + n);
      }
   }

   //advanced_insert_int.hpp includes all necessary preprocessor machinery...
   #define BOOST_PP_LOCAL_MACRO(n)                                                           \
   template<BOOST_PP_ENUM_PARAMS(n, class P)>                                                \
   void emplace_back(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_LIST, _))                  \
   {                                                                                         \
      if(priv_push_back_simple_available()){                                                 \
         new(priv_push_back_simple_pos())value_type                                          \
         (BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_FORWARD, _));                         \
         priv_push_back_simple_commit();                                                     \
      }                                                                                      \
      else{                                                                                  \
         containers_detail::BOOST_PP_CAT(BOOST_PP_CAT(advanced_insert_aux_emplace, n), arg)             \
            <value_type, iterator, BOOST_PP_ENUM_PARAMS(n, P)>                               \
               proxy(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_FORWARD, _));              \
         priv_insert_aux_impl(cend(), 1, proxy);                                             \
      }                                                                                      \
   }                                                                                         \
                                                                                             \
   template<BOOST_PP_ENUM_PARAMS(n, class P)>                                                \
   void emplace_front(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_LIST, _))                 \
   {                                                                                         \
      if(priv_push_front_simple_available()){                                                \
         new(priv_push_front_simple_pos())value_type                                         \
            (BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_FORWARD, _));                      \
         priv_push_front_simple_commit();                                                    \
      }                                                                                      \
      else{                                                                                  \
         containers_detail::BOOST_PP_CAT(BOOST_PP_CAT(advanced_insert_aux_emplace, n), arg)             \
            <value_type, iterator, BOOST_PP_ENUM_PARAMS(n, P)>                               \
               proxy(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_FORWARD, _));              \
         priv_insert_aux_impl(cbegin(), 1, proxy);                                           \
      }                                                                                      \
   }                                                                                         \
                                                                                             \
   template<BOOST_PP_ENUM_PARAMS(n, class P)>                                                \
   iterator emplace(const_iterator p, BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_LIST, _)) \
   {                                                                                         \
      if(p == this->cbegin()){                                                               \
         this->emplace_front(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_FORWARD, _));      \
         return this->begin();                                                               \
      }                                                                                      \
      else if(p == cend()){                                                                  \
         this->emplace_back(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_FORWARD, _));       \
         return (this->end()-1);                                                             \
      }                                                                                      \
      else{                                                                                  \
         size_type pos_num = p - this->cbegin();                                             \
         containers_detail::BOOST_PP_CAT(BOOST_PP_CAT(advanced_insert_aux_emplace, n), arg)             \
            <value_type, iterator, BOOST_PP_ENUM_PARAMS(n, P)>                               \
               proxy(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_FORWARD, _));              \
         this->priv_insert_aux_impl(p, 1, proxy);                                            \
         return iterator(this->begin() + pos_num);                                           \
      }                                                                                      \
   }                                                                                         \
   //!
   #define BOOST_PP_LOCAL_LIMITS (1, BOOST_CONTAINERS_MAX_CONSTRUCTOR_PARAMETERS)
   #include BOOST_PP_LOCAL_ITERATE()

   #endif   //#ifdef BOOST_CONTAINERS_PERFECT_FORWARDING

   void resize(size_type new_size, const value_type& x) 
   {
      const size_type len = size();
      if (new_size < len) 
         this->erase(this->members_.m_start + new_size, this->members_.m_finish);
      else
         this->insert(this->members_.m_finish, new_size - len, x);
   }

   void resize(size_type new_size) 
   {
      const size_type len = size();
      if (new_size < len) 
         this->erase(this->members_.m_start + new_size, this->members_.m_finish);
      else{
         size_type n = new_size - this->size();
         containers_detail::default_construct_aux_proxy<T, iterator, size_type> proxy(n);
         priv_insert_aux_impl(this->cend(), n, proxy);
      }
   }

   iterator erase(const_iterator pos) 
   {
      const_iterator next = pos;
      ++next;
      difference_type index = pos - this->members_.m_start;
      if (size_type(index) < (this->size() >> 1)) {
         boost::interprocess::move_backward(begin(), iterator(pos), iterator(next));
         pop_front();
      }
      else {
         boost::interprocess::move(iterator(next), end(), iterator(pos));
         pop_back();
      }
      return this->members_.m_start + index;
   }

   iterator erase(const_iterator first, const_iterator last)
   {
      if (first == this->members_.m_start && last == this->members_.m_finish) {
         this->clear();
         return this->members_.m_finish;
      }
      else {
         difference_type n = last - first;
         difference_type elems_before = first - this->members_.m_start;
         if (elems_before < static_cast<difference_type>(this->size() - n) - elems_before) {
            boost::interprocess::move_backward(begin(), iterator(first), iterator(last));
            iterator new_start = this->members_.m_start + n;
            if(!Base::traits_t::trivial_dctr_after_move)
               this->priv_destroy_range(this->members_.m_start, new_start);
            this->priv_destroy_nodes(new_start.m_node, this->members_.m_start.m_node);
            this->members_.m_start = new_start;
         }
         else {
            boost::interprocess::move(iterator(last), end(), iterator(first));
            iterator new_finish = this->members_.m_finish - n;
            if(!Base::traits_t::trivial_dctr_after_move)
               this->priv_destroy_range(new_finish, this->members_.m_finish);
            this->priv_destroy_nodes(new_finish.m_node + 1, this->members_.m_finish.m_node + 1);
            this->members_.m_finish = new_finish;
         }
         return this->members_.m_start + elems_before;
      }
   }

   void clear()
   {
      for (index_pointer node = this->members_.m_start.m_node + 1;
            node < this->members_.m_finish.m_node;
            ++node) {
         this->priv_destroy_range(*node, *node + this->s_buffer_size());
         this->priv_deallocate_node(*node);
      }

      if (this->members_.m_start.m_node != this->members_.m_finish.m_node) {
         this->priv_destroy_range(this->members_.m_start.m_cur, this->members_.m_start.m_last);
         this->priv_destroy_range(this->members_.m_finish.m_first, this->members_.m_finish.m_cur);
         this->priv_deallocate_node(this->members_.m_finish.m_first);
      }
      else
         this->priv_destroy_range(this->members_.m_start.m_cur, this->members_.m_finish.m_cur);

      this->members_.m_finish = this->members_.m_start;
   }

   /// @cond
   private:

   iterator priv_insert(const_iterator position, const value_type &x) 
   {
      if (position == cbegin()){
         this->push_front(x);
         return begin();
      }
      else if (position == cend()){
         this->push_back(x);
         return (end()-1);
      }
      else {
         size_type n = position - cbegin();
         this->priv_insert_aux(position, size_type(1), x);
         return iterator(this->begin() + n);
      }
   }

   void priv_push_front(const value_type &t)
   {
      if(this->priv_push_front_simple_available()){
         new(this->priv_push_front_simple_pos())value_type(t);
         this->priv_push_front_simple_commit();
      }
      else{
         this->priv_insert_aux(cbegin(), size_type(1), t);
      }
   }

   void priv_push_back(const value_type &t)
   {
      if(this->priv_push_back_simple_available()){
         new(this->priv_push_back_simple_pos())value_type(t);
         this->priv_push_back_simple_commit();
      }
      else{
         this->priv_insert_aux(cend(), size_type(1), t);
      }
   }


   bool priv_push_back_simple_available() const
   {
      return this->members_.m_map &&
         (this->members_.m_finish.m_cur != (this->members_.m_finish.m_last - 1));
   }

   void *priv_push_back_simple_pos() const
   {
      return static_cast<void*>(containers_detail::get_pointer(this->members_.m_finish.m_cur));
   }

   void priv_push_back_simple_commit()
   {
      ++this->members_.m_finish.m_cur;
   }

   bool priv_push_front_simple_available() const
   {
      return this->members_.m_map &&
         (this->members_.m_start.m_cur != this->members_.m_start.m_first);
   }

   void *priv_push_front_simple_pos() const
   {  return static_cast<void*>(containers_detail::get_pointer(this->members_.m_start.m_cur) - 1);  }

   void priv_push_front_simple_commit()
   {  --this->members_.m_start.m_cur;   }

   template <class InpIt>
   void priv_insert_aux(const_iterator pos, InpIt first, InpIt last, std::input_iterator_tag)
   {
      for(;first != last; ++first){
         this->insert(pos, boost::interprocess::move(value_type(*first)));
      }
   }

   template <class FwdIt>
   void priv_insert_aux(const_iterator pos, FwdIt first, FwdIt last, std::forward_iterator_tag) 
   {  this->priv_insert_aux(pos, first, last);  }

  // assign(), a generalized assignment member function.  Two
  // versions: one that takes a count, and one that takes a range.
  // The range version is a member template, so we dispatch on whether
  // or not the type is an integer.
   void priv_fill_assign(size_type n, const T& val)
   {
      if (n > size()) {
         std::fill(begin(), end(), val);
         this->insert(cend(), n - size(), val);
      }
      else {
         this->erase(cbegin() + n, cend());
         std::fill(begin(), end(), val);
      }
   }

   template <class Integer>
   void priv_initialize_dispatch(Integer n, Integer x, containers_detail::true_) 
   {
      this->priv_initialize_map(n);
      this->priv_fill_initialize(x);
   }

   template <class InpIt>
   void priv_initialize_dispatch(InpIt first, InpIt last, containers_detail::false_) 
   {
      typedef typename std::iterator_traits<InpIt>::iterator_category ItCat;
      this->priv_range_initialize(first, last, ItCat());
   }

   void priv_destroy_range(iterator p, iterator p2)
   {
      for(;p != p2; ++p)
         containers_detail::get_pointer(&*p)->~value_type();
   }

   void priv_destroy_range(pointer p, pointer p2)
   {
      for(;p != p2; ++p)
         containers_detail::get_pointer(&*p)->~value_type();
   }

   template <class Integer>
   void priv_assign_dispatch(Integer n, Integer val, containers_detail::true_)
      { this->priv_fill_assign((size_type) n, (T) val); }

   template <class InpIt>
   void priv_assign_dispatch(InpIt first, InpIt last, containers_detail::false_) 
   {
      typedef typename std::iterator_traits<InpIt>::iterator_category ItCat;
      this->priv_assign_aux(first, last, ItCat());
   }

   template <class InpIt>
   void priv_assign_aux(InpIt first, InpIt last, std::input_iterator_tag)
   {
      iterator cur = begin();
      for ( ; first != last && cur != end(); ++cur, ++first)
         *cur = *first;
      if (first == last)
         this->erase(cur, cend());
      else
         this->insert(cend(), first, last);
   }

   template <class FwdIt>
   void priv_assign_aux(FwdIt first, FwdIt last, std::forward_iterator_tag)
   {
      size_type len = std::distance(first, last);
      if (len > size()) {
         FwdIt mid = first;
         std::advance(mid, size());
         std::copy(first, mid, begin());
         this->insert(cend(), mid, last);
      }
      else
         this->erase(std::copy(first, last, begin()), cend());
   }

   template <class Integer>
   void priv_insert_dispatch(const_iterator pos, Integer n, Integer x, containers_detail::true_) 
   {  this->priv_fill_insert(pos, (size_type) n, (value_type) x); }

   template <class InpIt>
   void priv_insert_dispatch(const_iterator pos,InpIt first, InpIt last, containers_detail::false_) 
   {
      typedef typename std::iterator_traits<InpIt>::iterator_category ItCat;
      this->priv_insert_aux(pos, first, last, ItCat());
   }

   void priv_insert_aux(const_iterator pos, size_type n, const value_type& x)
   {
      typedef constant_iterator<value_type, difference_type> c_it;
      this->priv_insert_aux(pos, c_it(x, n), c_it());
   }

   //Just forward all operations to priv_insert_aux_impl
   template <class FwdIt>
   void priv_insert_aux(const_iterator p, FwdIt first, FwdIt last)
   {
      containers_detail::advanced_insert_aux_proxy<T, FwdIt, iterator> proxy(first, last);
      priv_insert_aux_impl(p, (size_type)std::distance(first, last), proxy);
   }

   void priv_insert_aux_impl(const_iterator p, size_type n, advanced_insert_aux_int_t &interf)
   {
      iterator pos(p);
      if(!this->members_.m_map){
         this->priv_initialize_map(0);
         pos = this->begin();
      }

      const difference_type elemsbefore = pos - this->members_.m_start;
      size_type length = this->size();
      if (elemsbefore < static_cast<difference_type>(length / 2)) {
         iterator new_start = this->priv_reserve_elements_at_front(n);
         iterator old_start = this->members_.m_start;
         pos = this->members_.m_start + elemsbefore;
         if (elemsbefore >= difference_type(n)) {
            iterator start_n = this->members_.m_start + difference_type(n); 
            ::boost::interprocess::uninitialized_move(this->members_.m_start, start_n, new_start);
            this->members_.m_start = new_start;
            boost::interprocess::move(start_n, pos, old_start);
            interf.copy_all_to(pos - difference_type(n));
         }
         else {
            difference_type mid_count = (difference_type(n) - elemsbefore);
            iterator mid_start = old_start - mid_count;
            interf.uninitialized_copy_some_and_update(mid_start, mid_count, true);
            this->members_.m_start = mid_start;
            ::boost::interprocess::uninitialized_move(old_start, pos, new_start);
            this->members_.m_start = new_start;
            interf.copy_all_to(old_start);
         }
      }
      else {
         iterator new_finish = this->priv_reserve_elements_at_back(n);
         iterator old_finish = this->members_.m_finish;
         const difference_type elemsafter = 
            difference_type(length) - elemsbefore;
         pos = this->members_.m_finish - elemsafter;
         if (elemsafter >= difference_type(n)) {
            iterator finish_n = this->members_.m_finish - difference_type(n);
            ::boost::interprocess::uninitialized_move(finish_n, this->members_.m_finish, this->members_.m_finish);
            this->members_.m_finish = new_finish;
            boost::interprocess::move_backward(pos, finish_n, old_finish);
            interf.copy_all_to(pos);
         }
         else {
            interf.uninitialized_copy_some_and_update(old_finish, elemsafter, false);
            this->members_.m_finish += n-elemsafter;
            ::boost::interprocess::uninitialized_move(pos, old_finish, this->members_.m_finish);
            this->members_.m_finish = new_finish;
            interf.copy_all_to(pos);
         }
      }
   }

   void priv_fill_insert(const_iterator pos, size_type n, const value_type& x)
   {
      typedef constant_iterator<value_type, difference_type> c_it;
      this->insert(pos, c_it(x, n), c_it());
   }

   // Precondition: this->members_.m_start and this->members_.m_finish have already been initialized,
   // but none of the deque's elements have yet been constructed.
   void priv_fill_initialize(const value_type& value) 
   {
      index_pointer cur;
      BOOST_TRY {
         for (cur = this->members_.m_start.m_node; cur < this->members_.m_finish.m_node; ++cur){
            std::uninitialized_fill(*cur, *cur + this->s_buffer_size(), value);
         }
         std::uninitialized_fill(this->members_.m_finish.m_first, this->members_.m_finish.m_cur, value);
      }
      BOOST_CATCH(...){
         this->priv_destroy_range(this->members_.m_start, iterator(*cur, cur));
         BOOST_RETHROW
      }
      BOOST_CATCH_END
   }

   template <class InpIt>
   void priv_range_initialize(InpIt first, InpIt last, std::input_iterator_tag)
   {
      this->priv_initialize_map(0);
      BOOST_TRY {
         for ( ; first != last; ++first)
            this->push_back(*first);
      }
      BOOST_CATCH(...){
         this->clear();
         BOOST_RETHROW
      }
      BOOST_CATCH_END
   }

   template <class FwdIt>
   void priv_range_initialize(FwdIt first, FwdIt last, std::forward_iterator_tag)
   {
      size_type n = 0;
      n = std::distance(first, last);
      this->priv_initialize_map(n);

      index_pointer cur_node;
      BOOST_TRY {
         for (cur_node = this->members_.m_start.m_node; 
               cur_node < this->members_.m_finish.m_node; 
               ++cur_node) {
            FwdIt mid = first;
            std::advance(mid, this->s_buffer_size());
            ::boost::interprocess::uninitialized_copy_or_move(first, mid, *cur_node);
            first = mid;
         }
         ::boost::interprocess::uninitialized_copy_or_move(first, last, this->members_.m_finish.m_first);
      }
      BOOST_CATCH(...){
         this->priv_destroy_range(this->members_.m_start, iterator(*cur_node, cur_node));
         BOOST_RETHROW
      }
      BOOST_CATCH_END
   }

   // Called only if this->members_.m_finish.m_cur == this->members_.m_finish.m_first.
   void priv_pop_back_aux()
   {
      this->priv_deallocate_node(this->members_.m_finish.m_first);
      this->members_.m_finish.priv_set_node(this->members_.m_finish.m_node - 1);
      this->members_.m_finish.m_cur = this->members_.m_finish.m_last - 1;
      containers_detail::get_pointer(this->members_.m_finish.m_cur)->~value_type();
   }

   // Called only if this->members_.m_start.m_cur == this->members_.m_start.m_last - 1.  Note that 
   // if the deque has at least one element (a precondition for this member 
   // function), and if this->members_.m_start.m_cur == this->members_.m_start.m_last, then the deque 
   // must have at least two nodes.
   void priv_pop_front_aux()
   {
      containers_detail::get_pointer(this->members_.m_start.m_cur)->~value_type();
      this->priv_deallocate_node(this->members_.m_start.m_first);
      this->members_.m_start.priv_set_node(this->members_.m_start.m_node + 1);
      this->members_.m_start.m_cur = this->members_.m_start.m_first;
   }      

   iterator priv_reserve_elements_at_front(size_type n) 
   {
      size_type vacancies = this->members_.m_start.m_cur - this->members_.m_start.m_first;
      if (n > vacancies){
         size_type new_elems = n-vacancies;
         size_type new_nodes = (new_elems + this->s_buffer_size() - 1) / 
            this->s_buffer_size();
         size_type s = (size_type)(this->members_.m_start.m_node - this->members_.m_map);
         if (new_nodes > s){
            this->priv_reallocate_map(new_nodes, true);
         }
         size_type i = 1;
         BOOST_TRY {
            for (; i <= new_nodes; ++i)
               *(this->members_.m_start.m_node - i) = this->priv_allocate_node();
         }
         BOOST_CATCH(...) {
            for (size_type j = 1; j < i; ++j)
               this->priv_deallocate_node(*(this->members_.m_start.m_node - j));      
            BOOST_RETHROW
         }
         BOOST_CATCH_END
      }
      return this->members_.m_start - difference_type(n);
   }

   iterator priv_reserve_elements_at_back(size_type n) 
   {
      size_type vacancies = (this->members_.m_finish.m_last - this->members_.m_finish.m_cur) - 1;
      if (n > vacancies){
         size_type new_elems = n - vacancies;
         size_type new_nodes = (new_elems + this->s_buffer_size() - 1)/s_buffer_size();
         size_type s = (size_type)(this->members_.m_map_size - (this->members_.m_finish.m_node - this->members_.m_map));
         if (new_nodes + 1 > s){
            this->priv_reallocate_map(new_nodes, false);
         }
         size_type i;
         BOOST_TRY {
            for (i = 1; i <= new_nodes; ++i)
               *(this->members_.m_finish.m_node + i) = this->priv_allocate_node();
         }
         BOOST_CATCH(...) {
            for (size_type j = 1; j < i; ++j)
               this->priv_deallocate_node(*(this->members_.m_finish.m_node + j));      
            BOOST_RETHROW
         }
         BOOST_CATCH_END
      }
      return this->members_.m_finish + difference_type(n);
   }

   void priv_reallocate_map(size_type nodes_to_add, bool add_at_front)
   {
      size_type old_num_nodes = this->members_.m_finish.m_node - this->members_.m_start.m_node + 1;
      size_type new_num_nodes = old_num_nodes + nodes_to_add;

      index_pointer new_nstart;
      if (this->members_.m_map_size > 2 * new_num_nodes) {
         new_nstart = this->members_.m_map + (this->members_.m_map_size - new_num_nodes) / 2 
                           + (add_at_front ? nodes_to_add : 0);
         if (new_nstart < this->members_.m_start.m_node)
            boost::interprocess::move(this->members_.m_start.m_node, this->members_.m_finish.m_node + 1, new_nstart);
         else
            boost::interprocess::move_backward
               (this->members_.m_start.m_node, this->members_.m_finish.m_node + 1, new_nstart + old_num_nodes);
      }
      else {
         size_type new_map_size = 
            this->members_.m_map_size + containers_detail::max_value(this->members_.m_map_size, nodes_to_add) + 2;

         index_pointer new_map = this->priv_allocate_map(new_map_size);
         new_nstart = new_map + (new_map_size - new_num_nodes) / 2
                              + (add_at_front ? nodes_to_add : 0);
         boost::interprocess::move(this->members_.m_start.m_node, this->members_.m_finish.m_node + 1, new_nstart);
         this->priv_deallocate_map(this->members_.m_map, this->members_.m_map_size);

         this->members_.m_map = new_map;
         this->members_.m_map_size = new_map_size;
      }

      this->members_.m_start.priv_set_node(new_nstart);
      this->members_.m_finish.priv_set_node(new_nstart + old_num_nodes - 1);
   }
   /// @endcond
};

// Nonmember functions.
template <class T, class Alloc>
inline bool operator==(const deque<T, Alloc>& x,
                       const deque<T, Alloc>& y)
{
   return x.size() == y.size() && equal(x.begin(), x.end(), y.begin());
}

template <class T, class Alloc>
inline bool operator<(const deque<T, Alloc>& x,
                      const deque<T, Alloc>& y) 
{
   return lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
}

template <class T, class Alloc>
inline bool operator!=(const deque<T, Alloc>& x,
                       const deque<T, Alloc>& y) 
   {  return !(x == y);   }

template <class T, class Alloc>
inline bool operator>(const deque<T, Alloc>& x,
                      const deque<T, Alloc>& y) 
   {  return y < x; }

template <class T, class Alloc>
inline bool operator<=(const deque<T, Alloc>& x,
                       const deque<T, Alloc>& y) 
   {  return !(y < x); }

template <class T, class Alloc>
inline bool operator>=(const deque<T, Alloc>& x,
                       const deque<T, Alloc>& y) 
   {  return !(x < y); }


template <class T, class A>
inline void swap(deque<T, A>& x, deque<T, A>& y)
{  x.swap(y);  }

}}

/// @cond

namespace boost {
/*
//!has_trivial_destructor_after_move<> == true_type
//!specialization for optimizations
template <class T, class A>
struct has_trivial_destructor_after_move<boost::container::deque<T, A> >
{
   enum {   value = has_trivial_destructor<A>::value  };
};
*/
}

/// @endcond

#include <boost/interprocess/containers/container/detail/config_end.hpp>

#endif //   #ifndef  BOOST_CONTAINERS_DEQUE_HPP
