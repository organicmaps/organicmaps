//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_CONTAINER_CONTAINER_VECTOR_HPP
#define BOOST_CONTAINER_CONTAINER_VECTOR_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/container/detail/config_begin.hpp>
#include <boost/container/detail/workaround.hpp>
#include <boost/container/container_fwd.hpp>

#include <cstddef>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <iterator>
#include <utility>
#include <boost/detail/no_exceptions_support.hpp>
#include <boost/type_traits/has_trivial_destructor.hpp>
#include <boost/type_traits/has_trivial_copy.hpp>
#include <boost/type_traits/has_trivial_assign.hpp>
#include <boost/type_traits/has_nothrow_copy.hpp>
#include <boost/type_traits/has_nothrow_assign.hpp>
#include <boost/type_traits/has_nothrow_constructor.hpp>
#include <boost/container/detail/version_type.hpp>
#include <boost/container/detail/allocation_type.hpp>
#include <boost/container/detail/utilities.hpp>
#include <boost/container/detail/iterators.hpp>
#include <boost/container/detail/algorithms.hpp>
#include <boost/container/detail/destroyers.hpp>
#include <boost/container/allocator_traits.hpp>
#include <boost/container/container_fwd.hpp>
#include <boost/move/move.hpp>
#include <boost/move/move_helpers.hpp>
#include <boost/intrusive/pointer_traits.hpp>
#include <boost/container/detail/mpl.hpp>
#include <boost/container/detail/type_traits.hpp>
#include <boost/container/detail/advanced_insert_int.hpp>
#include <boost/assert.hpp>

namespace boost {
namespace container {

/// @cond

namespace container_detail {

//! Const vector_iterator used to iterate through a vector.
template <class Pointer>
class vector_const_iterator
{
   public:
	typedef std::random_access_iterator_tag                                          iterator_category;
   typedef typename boost::intrusive::pointer_traits<Pointer>::element_type         value_type;
   typedef typename boost::intrusive::pointer_traits<Pointer>::difference_type      difference_type;
   typedef typename boost::intrusive::pointer_traits<Pointer>::template
                                 rebind_pointer<const value_type>::type             pointer;
   typedef  const value_type&                                                       reference;

   /// @cond
   protected:
   Pointer m_ptr;

   public:
   Pointer get_ptr() const    {  return   m_ptr;  }
   explicit vector_const_iterator(Pointer ptr)  : m_ptr(ptr){}
   /// @endcond

   public:

   //Constructors
   vector_const_iterator() : m_ptr(0){}

   //Pointer like operators
   reference operator*()   const 
   {  return *m_ptr;  }

   const value_type * operator->()  const 
   {  return container_detail::to_raw_pointer(m_ptr);  }

   reference operator[](difference_type off) const
   {  return m_ptr[off];   }

   //Increment / Decrement
   vector_const_iterator& operator++()      
   { ++m_ptr;  return *this; }

   vector_const_iterator operator++(int)     
   {  Pointer tmp = m_ptr; ++*this; return vector_const_iterator(tmp);  }

   vector_const_iterator& operator--()
   {  --m_ptr; return *this;  }

   vector_const_iterator operator--(int)
   {  Pointer tmp = m_ptr; --*this; return vector_const_iterator(tmp); }

   //Arithmetic
   vector_const_iterator& operator+=(difference_type off)
   {  m_ptr += off; return *this;   }

   vector_const_iterator operator+(difference_type off) const
   {  return vector_const_iterator(m_ptr+off);  }

   friend vector_const_iterator operator+(difference_type off, const vector_const_iterator& right)
   {  return vector_const_iterator(off + right.m_ptr); }

   vector_const_iterator& operator-=(difference_type off)
   {  m_ptr -= off; return *this;   }

   vector_const_iterator operator-(difference_type off) const
   {  return vector_const_iterator(m_ptr-off);  }

   difference_type operator-(const vector_const_iterator& right) const
   {  return m_ptr - right.m_ptr;   }

   //Comparison operators
   bool operator==   (const vector_const_iterator& r)  const
   {  return m_ptr == r.m_ptr;  }

   bool operator!=   (const vector_const_iterator& r)  const
   {  return m_ptr != r.m_ptr;  }

   bool operator<    (const vector_const_iterator& r)  const
   {  return m_ptr < r.m_ptr;  }

   bool operator<=   (const vector_const_iterator& r)  const
   {  return m_ptr <= r.m_ptr;  }

   bool operator>    (const vector_const_iterator& r)  const
   {  return m_ptr > r.m_ptr;  }

   bool operator>=   (const vector_const_iterator& r)  const
   {  return m_ptr >= r.m_ptr;  }
};

//! Iterator used to iterate through a vector
template <class Pointer>
class vector_iterator
   :  public vector_const_iterator<Pointer>
{
   public:
   explicit vector_iterator(Pointer ptr)
      : vector_const_iterator<Pointer>(ptr)
   {}

   public:
	typedef std::random_access_iterator_tag                                       iterator_category;
   typedef typename boost::intrusive::pointer_traits<Pointer>::element_type      value_type;
   typedef typename boost::intrusive::pointer_traits<Pointer>::difference_type   difference_type;
   typedef Pointer                                                               pointer;
   typedef value_type&                                                           reference;

   //Constructors
   vector_iterator()
   {}

   //Pointer like operators
   reference operator*()  const 
   {  return *this->m_ptr;  }

   value_type* operator->() const 
   {  return  container_detail::to_raw_pointer(this->m_ptr);  }

   reference operator[](difference_type off) const
   {  return this->m_ptr[off];   }

   //Increment / Decrement
   vector_iterator& operator++() 
   {  ++this->m_ptr; return *this;  }

   vector_iterator operator++(int)
   {  pointer tmp = this->m_ptr; ++*this; return vector_iterator(tmp);  }
  
   vector_iterator& operator--()
   {  --this->m_ptr; return *this;  }

   vector_iterator operator--(int)
   {  vector_iterator tmp = *this; --*this; return vector_iterator(tmp); }

   // Arithmetic
   vector_iterator& operator+=(difference_type off)
   {  this->m_ptr += off;  return *this;  }

   vector_iterator operator+(difference_type off) const
   {  return vector_iterator(this->m_ptr+off);  }

   friend vector_iterator operator+(difference_type off, const vector_iterator& right)
   {  return vector_iterator(off + right.m_ptr); }

   vector_iterator& operator-=(difference_type off)
   {  this->m_ptr -= off; return *this;   }

   vector_iterator operator-(difference_type off) const
   {  return vector_iterator(this->m_ptr-off);  }

   difference_type operator-(const vector_const_iterator<Pointer>& right) const
   {  return static_cast<const vector_const_iterator<Pointer>&>(*this) - right;   }
};

template <class T, class Allocator>
struct vector_value_traits
{
   typedef T value_type;
   typedef Allocator allocator_type;
   static const bool trivial_dctr = boost::has_trivial_destructor<value_type>::value;
   static const bool trivial_dctr_after_move = trivial_dctr;
      //::boost::has_trivial_destructor_after_move<value_type>::value || trivial_dctr;
   //static const bool trivial_copy = has_trivial_copy<value_type>::value;
   //static const bool nothrow_copy = has_nothrow_copy<value_type>::value;
   //static const bool trivial_assign = has_trivial_assign<value_type>::value;
   //static const bool nothrow_assign = has_nothrow_assign<value_type>::value;

   static const bool trivial_copy = has_trivial_copy<value_type>::value;
   static const bool nothrow_copy = has_nothrow_copy<value_type>::value;
   static const bool trivial_assign = has_trivial_assign<value_type>::value;
   static const bool nothrow_assign = false;

   //This is the anti-exception array destructor
   //to deallocate values already constructed
   typedef typename container_detail::if_c
      <trivial_dctr
      ,container_detail::null_scoped_destructor_n<Allocator>
      ,container_detail::scoped_destructor_n<Allocator>
      >::type   OldArrayDestructor;
   //This is the anti-exception array destructor
   //to destroy objects created with copy construction
   typedef typename container_detail::if_c
      <nothrow_copy
      ,container_detail::null_scoped_destructor_n<Allocator>
      ,container_detail::scoped_destructor_n<Allocator>
      >::type   ArrayDestructor;
   //This is the anti-exception array deallocator
   typedef typename container_detail::if_c
      <nothrow_copy
      ,container_detail::null_scoped_array_deallocator<Allocator>
      ,container_detail::scoped_array_deallocator<Allocator>
      >::type   ArrayDeallocator;
};

//!This struct deallocates and allocated memory
template <class Allocator>
struct vector_alloc_holder
{
   typedef boost::container::allocator_traits<Allocator> allocator_traits_type;
   typedef typename allocator_traits_type::pointer       pointer;
   typedef typename allocator_traits_type::size_type     size_type;
   typedef typename allocator_traits_type::value_type    value_type;
   typedef vector_value_traits<value_type, Allocator>    value_traits;

   //Constructor, does not throw
   vector_alloc_holder()
      BOOST_CONTAINER_NOEXCEPT_IF(::boost::has_nothrow_default_constructor<Allocator>::value)
      : members_()
   {}

   //Constructor, does not throw
   template<class AllocConvertible>
   explicit vector_alloc_holder(BOOST_FWD_REF(AllocConvertible) a) BOOST_CONTAINER_NOEXCEPT
      : members_(boost::forward<AllocConvertible>(a))
   {}

   //Destructor
   ~vector_alloc_holder()
   {
      this->prot_destroy_all();
      this->prot_deallocate();
   }

   typedef container_detail::integral_constant<unsigned, 1>      allocator_v1;
   typedef container_detail::integral_constant<unsigned, 2>      allocator_v2;
   typedef container_detail::integral_constant<unsigned,
      boost::container::container_detail::version<Allocator>::value> alloc_version;
   std::pair<pointer, bool>
      allocation_command(allocation_type command,
                         size_type limit_size,
                         size_type preferred_size,
                         size_type &received_size, const pointer &reuse = 0)
   {
      return allocation_command(command, limit_size, preferred_size,
                               received_size, reuse, alloc_version());
   }

   std::pair<pointer, bool>
      allocation_command(allocation_type command,
                         size_type limit_size,
                         size_type preferred_size,
                         size_type &received_size,
                         const pointer &reuse,
                         allocator_v1)
   {
      (void)limit_size;
      (void)reuse;
      if(!(command & allocate_new))
         return std::pair<pointer, bool>(pointer(0), false);
      received_size = preferred_size;
      return std::make_pair(this->alloc().allocate(received_size), false);
   }

   std::pair<pointer, bool>
      allocation_command(allocation_type command,
                         size_type limit_size,
                         size_type preferred_size,
                         size_type &received_size,
                         const pointer &reuse,
                         allocator_v2)
   {
      return this->alloc().allocation_command
         (command, limit_size, preferred_size, received_size, reuse);
   }

   size_type next_capacity(size_type additional_objects) const
   {
      return get_next_capacity( allocator_traits_type::max_size(this->alloc())
                              , this->members_.m_capacity, additional_objects);
   }

   struct members_holder
      : public Allocator
   {
      private:
      members_holder(const members_holder&);

      public:
      template<class Alloc>
      explicit members_holder(BOOST_FWD_REF(Alloc) alloc)
         :  Allocator(boost::forward<Alloc>(alloc)), m_start(0), m_size(0), m_capacity(0)
      {}

      members_holder()
         :  Allocator(), m_start(0), m_size(0), m_capacity(0)
      {}

      pointer     m_start;
      size_type   m_size;
      size_type   m_capacity;
   } members_;

   void swap_members(vector_alloc_holder &x)
   {
      container_detail::do_swap(this->members_.m_start, x.members_.m_start);
      container_detail::do_swap(this->members_.m_size, x.members_.m_size);
      container_detail::do_swap(this->members_.m_capacity, x.members_.m_capacity);
   }

   Allocator &alloc()
   {  return members_;  }

   const Allocator &alloc() const
   {  return members_;  }

   protected:
   void prot_deallocate()
   {
      if(!this->members_.m_capacity)   return;
      this->alloc().deallocate(this->members_.m_start, this->members_.m_capacity);
      this->members_.m_start     = 0;
      this->members_.m_size      = 0;
      this->members_.m_capacity  = 0;
   }

   void destroy(value_type* p)
   {
      if(!value_traits::trivial_dctr)
         allocator_traits_type::destroy(this->alloc(), p);
   }

   void destroy_n(value_type* p, size_type n)
   {
      if(!value_traits::trivial_dctr){
         for(; n--; ++p){
            allocator_traits_type::destroy(this->alloc(), p);
         }
      }
   }

   void prot_destroy_all()
   {
      this->destroy_n(container_detail::to_raw_pointer(this->members_.m_start), this->members_.m_size);
      this->members_.m_size = 0;
   }
};

}  //namespace container_detail {
/// @endcond

//! \class vector
//! A vector is a sequence that supports random access to elements, constant
//! time insertion and removal of elements at the end, and linear time insertion
//! and removal of elements at the beginning or in the middle. The number of
//! elements in a vector may vary dynamically; memory management is automatic.
//! boost::container::vector is similar to std::vector but it's compatible
//! with shared memory and memory mapped files.
#ifdef BOOST_CONTAINER_DOXYGEN_INVOKED
template <class T, class Allocator = std::allocator<T> >
#else
template <class T, class Allocator>
#endif
class vector : private container_detail::vector_alloc_holder<Allocator>
{
   /// @cond
   typedef container_detail::vector_alloc_holder<Allocator> base_t;
   typedef allocator_traits<Allocator>            allocator_traits_type;
   /// @endcond
   public:
   //////////////////////////////////////////////
   //
   //                    types
   //
   //////////////////////////////////////////////

   typedef T                                                                           value_type;
   typedef typename ::boost::container::allocator_traits<Allocator>::pointer           pointer;
   typedef typename ::boost::container::allocator_traits<Allocator>::const_pointer     const_pointer;
   typedef typename ::boost::container::allocator_traits<Allocator>::reference         reference;
   typedef typename ::boost::container::allocator_traits<Allocator>::const_reference   const_reference;
   typedef typename ::boost::container::allocator_traits<Allocator>::size_type         size_type;
   typedef typename ::boost::container::allocator_traits<Allocator>::difference_type   difference_type;
   typedef Allocator                                                                   allocator_type;
   typedef Allocator                                                                   stored_allocator_type;
   typedef BOOST_CONTAINER_IMPDEF(container_detail::vector_iterator<pointer>)          iterator;
   typedef BOOST_CONTAINER_IMPDEF(container_detail::vector_const_iterator<pointer>)    const_iterator;
   typedef BOOST_CONTAINER_IMPDEF(std::reverse_iterator<iterator>)                     reverse_iterator;
   typedef BOOST_CONTAINER_IMPDEF(std::reverse_iterator<const_iterator>)               const_reverse_iterator;

   /// @cond
   private:
   BOOST_COPYABLE_AND_MOVABLE(vector)
   typedef container_detail::advanced_insert_aux_int<T*>    advanced_insert_aux_int_t;
   typedef container_detail::vector_value_traits<value_type, Allocator> value_traits;

   typedef typename base_t::allocator_v1           allocator_v1;
   typedef typename base_t::allocator_v2           allocator_v2;
   typedef typename base_t::alloc_version          alloc_version;

   typedef constant_iterator<T, difference_type>   cvalue_iterator;
   typedef repeat_iterator<T, difference_type>     repeat_it;
   typedef boost::move_iterator<repeat_it>         repeat_move_it;
   /// @endcond

   public:
   //////////////////////////////////////////////
   //
   //          construct/copy/destroy
   //
   //////////////////////////////////////////////

   //! <b>Effects</b>: Constructs a vector taking the allocator as parameter.
   //!
   //! <b>Throws</b>: If allocator_type's default constructor throws.
   //!
   //! <b>Complexity</b>: Constant.
   vector()
      BOOST_CONTAINER_NOEXCEPT_IF(::boost::has_nothrow_default_constructor<Allocator>::value)
      : base_t()
   {}

   //! <b>Effects</b>: Constructs a vector taking the allocator as parameter.
   //!
   //! <b>Throws</b>: Nothing
   //!
   //! <b>Complexity</b>: Constant.
   explicit vector(const Allocator& a) BOOST_CONTAINER_NOEXCEPT
      : base_t(a)
   {}

   //! <b>Effects</b>: Constructs a vector that will use a copy of allocator a
   //!   and inserts n default contructed values.
   //!
   //! <b>Throws</b>: If allocator_type's default constructor or allocation
   //!   throws or T's default constructor throws.
   //!
   //! <b>Complexity</b>: Linear to n.
   explicit vector(size_type n)
      :  base_t()
   {  this->resize(n);  }

   //! <b>Effects</b>: Constructs a vector that will use a copy of allocator a
   //!   and inserts n copies of value.
   //!
   //! <b>Throws</b>: If allocator_type's default constructor or allocation
   //!   throws or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to n.
   vector(size_type n, const T& value, const allocator_type& a = allocator_type())
      :  base_t(a)
   {  this->resize(n, value); }

   //! <b>Effects</b>: Constructs a vector that will use a copy of allocator a
   //!   and inserts a copy of the range [first, last) in the vector.
   //!
   //! <b>Throws</b>: If allocator_type's default constructor or allocation
   //!   throws or T's constructor taking an dereferenced InIt throws.
   //!
   //! <b>Complexity</b>: Linear to the range [first, last).
   template <class InIt>
   vector(InIt first, InIt last, const allocator_type& a = allocator_type())
      :  base_t(a)
   {  this->assign(first, last); }

   //! <b>Effects</b>: Copy constructs a vector.
   //!
   //! <b>Postcondition</b>: x == *this.
   //!
   //! <b>Throws</b>: If allocator_type's default constructor or allocation
   //!   throws or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to the elements x contains.
   vector(const vector &x)
      :  base_t(allocator_traits_type::select_on_container_copy_construction(x.alloc()))
   {
      this->assign( container_detail::to_raw_pointer(x.members_.m_start)
                  , container_detail::to_raw_pointer(x.members_.m_start + x.members_.m_size));
   }

   //! <b>Effects</b>: Move constructor. Moves mx's resources to *this.
   //!
   //! <b>Throws</b>: Nothing
   //!
   //! <b>Complexity</b>: Constant.
   vector(BOOST_RV_REF(vector) mx) BOOST_CONTAINER_NOEXCEPT
      :  base_t(boost::move(mx.alloc()))
   {  this->swap_members(mx);   }

   //! <b>Effects</b>: Copy constructs a vector using the specified allocator.
   //!
   //! <b>Postcondition</b>: x == *this.
   //!
   //! <b>Throws</b>: If allocation
   //!   throws or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to the elements x contains.
   vector(const vector &x, const allocator_type &a)
      :  base_t(a)
   {
      this->assign( container_detail::to_raw_pointer(x.members_.m_start)
                  , container_detail::to_raw_pointer(x.members_.m_start + x.members_.m_size));
   }

   //! <b>Effects</b>: Move constructor using the specified allocator.
   //!                 Moves mx's resources to *this if a == allocator_type().
   //!                 Otherwise copies values from x to *this.
   //!
   //! <b>Throws</b>: If allocation or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Constant if a == mx.get_allocator(), linear otherwise.
   vector(BOOST_RV_REF(vector) mx, const allocator_type &a)
      :  base_t(a)
   {
      if(mx.alloc() == a){
         this->swap_members(mx);
      }
      else{
         this->assign( container_detail::to_raw_pointer(mx.members_.m_start)
                     , container_detail::to_raw_pointer(mx.members_.m_start) + mx.members_.m_size);
      }
   }

   //! <b>Effects</b>: Destroys the vector. All stored values are destroyed
   //!   and used memory is deallocated.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements.
   ~vector() BOOST_CONTAINER_NOEXCEPT
   {} //vector_alloc_holder clears the data

   //! <b>Effects</b>: Makes *this contain the same elements as x.
   //!
   //! <b>Postcondition</b>: this->size() == x.size(). *this contains a copy
   //! of each of x's elements.
   //!
   //! <b>Throws</b>: If memory allocation throws or T's copy/move constructor/assignment throws.
   //!
   //! <b>Complexity</b>: Linear to the number of elements in x.
   vector& operator=(BOOST_COPY_ASSIGN_REF(vector) x)
   {
      if (&x != this){
         allocator_type &this_alloc     = this->alloc();
         const allocator_type &x_alloc  = x.alloc();
         container_detail::bool_<allocator_traits_type::
            propagate_on_container_copy_assignment::value> flag;
         if(flag && this_alloc != x_alloc){
            this->clear();
            this->shrink_to_fit();
         }
         container_detail::assign_alloc(this_alloc, x_alloc, flag);
         this->assign( container_detail::to_raw_pointer(x.members_.m_start)
                     , container_detail::to_raw_pointer(x.members_.m_start + x.members_.m_size));
      }
      return *this;
   }

   //! <b>Effects</b>: Move assignment. All mx's values are transferred to *this.
   //!
   //! <b>Postcondition</b>: x.empty(). *this contains a the elements x had
   //!   before the function.
   //!
   //! <b>Throws</b>: Nothing
   //!
   //! <b>Complexity</b>: Linear.
   vector& operator=(BOOST_RV_REF(vector) x)
      //iG BOOST_CONTAINER_NOEXCEPT_IF(!allocator_type::propagate_on_container_move_assignment::value || is_nothrow_move_assignable<allocator_type>::value);)
      BOOST_CONTAINER_NOEXCEPT
   {
      if (&x != this){
         allocator_type &this_alloc = this->alloc();
         allocator_type &x_alloc    = x.alloc();
         //If allocators are equal we can just swap pointers
         if(this_alloc == x_alloc){
            //Destroy objects but retain memory in case x reuses it in the future
            this->clear();
            this->swap_members(x);
            //Move allocator if needed
            container_detail::bool_<allocator_traits_type::
               propagate_on_container_move_assignment::value> flag;
            container_detail::move_alloc(this_alloc, x_alloc, flag);
         }
         //If unequal allocators, then do a one by one move
         else{
            this->assign( boost::make_move_iterator(container_detail::to_raw_pointer(x.members_.m_start))
                        , boost::make_move_iterator(container_detail::to_raw_pointer(x.members_.m_start + x.members_.m_size)));
         }
      }
      return *this;
   }

   //! <b>Effects</b>: Assigns the the range [first, last) to *this.
   //!
   //! <b>Throws</b>: If memory allocation throws or T's copy/move constructor/assignment or
   //!   T's constructor/assignment from dereferencing InpIt throws.
   //!
   //! <b>Complexity</b>: Linear to n.
   template <class InIt>
   void assign(InIt first, InIt last
      #if !defined(BOOST_CONTAINER_DOXYGEN_INVOKED)
      , typename container_detail::enable_if_c
         < !container_detail::is_convertible<InIt, size_type>::value
            //&& container_detail::is_input_iterator<InIt>::value
         >::type * = 0
      #endif
      )
   {
      //Overwrite all elements we can from [first, last)
      iterator cur = this->begin();
      for ( ; first != last && cur != end(); ++cur, ++first){
         *cur = *first;
      }

      if (first == last){
         //There are no more elements in the sequence, erase remaining
         this->erase(cur, this->cend());
      }
      else{
         //There are more elements in the range, insert the remaining ones
         this->insert(this->cend(), first, last);
      }
   }

   //! <b>Effects</b>: Assigns the n copies of val to *this.
   //!
   //! <b>Throws</b>: If memory allocation throws or
   //!   T's copy/move constructor/assignment throws.
   //!
   //! <b>Complexity</b>: Linear to n.
   void assign(size_type n, const value_type& val)
   {  this->assign(cvalue_iterator(val, n), cvalue_iterator());   }

   //! <b>Effects</b>: Returns a copy of the internal allocator.
   //!
   //! <b>Throws</b>: If allocator's copy constructor throws.
   //!
   //! <b>Complexity</b>: Constant.
   allocator_type get_allocator() const BOOST_CONTAINER_NOEXCEPT
   { return this->alloc();  }


   //! <b>Effects</b>: Returns a reference to the internal allocator.
   //!
   //! <b>Throws</b>: Nothing
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Note</b>: Non-standard extension.
   stored_allocator_type &get_stored_allocator() BOOST_CONTAINER_NOEXCEPT
   {  return this->alloc(); }

   //! <b>Effects</b>: Returns a reference to the internal allocator.
   //!
   //! <b>Throws</b>: Nothing
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Note</b>: Non-standard extension.
   const stored_allocator_type &get_stored_allocator() const BOOST_CONTAINER_NOEXCEPT
   {  return this->alloc(); }

   //////////////////////////////////////////////
   //
   //                iterators
   //
   //////////////////////////////////////////////

   //! <b>Effects</b>: Returns an iterator to the first element contained in the vector.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   iterator begin() BOOST_CONTAINER_NOEXCEPT
   { return iterator(this->members_.m_start); }

   //! <b>Effects</b>: Returns a const_iterator to the first element contained in the vector.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   const_iterator begin() const BOOST_CONTAINER_NOEXCEPT
   { return const_iterator(this->members_.m_start); }

   //! <b>Effects</b>: Returns an iterator to the end of the vector.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   iterator end() BOOST_CONTAINER_NOEXCEPT
   { return iterator(this->members_.m_start + this->members_.m_size); }

   //! <b>Effects</b>: Returns a const_iterator to the end of the vector.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   const_iterator end() const BOOST_CONTAINER_NOEXCEPT
   { return this->cend(); }

   //! <b>Effects</b>: Returns a reverse_iterator pointing to the beginning
   //! of the reversed vector.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   reverse_iterator rbegin() BOOST_CONTAINER_NOEXCEPT
   { return reverse_iterator(this->end());      }

   //! <b>Effects</b>: Returns a const_reverse_iterator pointing to the beginning
   //! of the reversed vector.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   const_reverse_iterator rbegin() const BOOST_CONTAINER_NOEXCEPT
   { return this->crbegin(); }

   //! <b>Effects</b>: Returns a reverse_iterator pointing to the end
   //! of the reversed vector.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   reverse_iterator rend() BOOST_CONTAINER_NOEXCEPT
   { return reverse_iterator(this->begin());       }

   //! <b>Effects</b>: Returns a const_reverse_iterator pointing to the end
   //! of the reversed vector.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   const_reverse_iterator rend() const BOOST_CONTAINER_NOEXCEPT
   { return this->crend(); }

   //! <b>Effects</b>: Returns a const_iterator to the first element contained in the vector.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   const_iterator cbegin() const BOOST_CONTAINER_NOEXCEPT
   { return const_iterator(this->members_.m_start); }

   //! <b>Effects</b>: Returns a const_iterator to the end of the vector.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   const_iterator cend() const BOOST_CONTAINER_NOEXCEPT
   { return const_iterator(this->members_.m_start + this->members_.m_size); }

   //! <b>Effects</b>: Returns a const_reverse_iterator pointing to the beginning
   //! of the reversed vector.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   const_reverse_iterator crbegin() const BOOST_CONTAINER_NOEXCEPT
   { return const_reverse_iterator(this->end());}

   //! <b>Effects</b>: Returns a const_reverse_iterator pointing to the end
   //! of the reversed vector.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   const_reverse_iterator crend() const BOOST_CONTAINER_NOEXCEPT
   { return const_reverse_iterator(this->begin()); }

   //////////////////////////////////////////////
   //
   //                capacity
   //
   //////////////////////////////////////////////

   //! <b>Effects</b>: Returns true if the vector contains no elements.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   bool empty() const BOOST_CONTAINER_NOEXCEPT
   { return !this->members_.m_size; }

   //! <b>Effects</b>: Returns the number of the elements contained in the vector.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   size_type size() const BOOST_CONTAINER_NOEXCEPT
   { return this->members_.m_size; }

   //! <b>Effects</b>: Returns the largest possible size of the vector.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   size_type max_size() const BOOST_CONTAINER_NOEXCEPT
   { return allocator_traits_type::max_size(this->alloc()); }

   //! <b>Effects</b>: Inserts or erases elements at the end such that
   //!   the size becomes n. New elements are default constructed.
   //!
   //! <b>Throws</b>: If memory allocation throws, or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to the difference between size() and new_size.
   void resize(size_type new_size)
   {
      if (new_size < this->size()){
         //Destroy last elements
         this->erase(const_iterator(this->members_.m_start + new_size), this->end());
      }
      else{
         const size_type n = new_size - this->size();
         this->reserve(new_size);
         container_detail::default_construct_aux_proxy<Allocator, T*> proxy(this->alloc(), n);
         this->priv_forward_range_insert(this->cend().get_ptr(), n, proxy);
      }
   }

   //! <b>Effects</b>: Inserts or erases elements at the end such that
   //!   the size becomes n. New elements are copy constructed from x.
   //!
   //! <b>Throws</b>: If memory allocation throws, or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to the difference between size() and new_size.
   void resize(size_type new_size, const T& x)
   {
      pointer finish = this->members_.m_start + this->members_.m_size;
      if (new_size < size()){
         //Destroy last elements
         this->erase(const_iterator(this->members_.m_start + new_size), this->end());
      }
      else{
         //Insert new elements at the end
         this->insert(const_iterator(finish), new_size - this->size(), x);
      }
   }

   //! <b>Effects</b>: Number of elements for which memory has been allocated.
   //!   capacity() is always greater than or equal to size().
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   size_type capacity() const BOOST_CONTAINER_NOEXCEPT
   { return this->members_.m_capacity; }

   //! <b>Effects</b>: If n is less than or equal to capacity(), this call has no
   //!   effect. Otherwise, it is a request for allocation of additional memory.
   //!   If the request is successful, then capacity() is greater than or equal to
   //!   n; otherwise, capacity() is unchanged. In either case, size() is unchanged.
   //!
   //! <b>Throws</b>: If memory allocation allocation throws or T's copy/move constructor throws.
   void reserve(size_type new_cap)
   {
      if (this->capacity() < new_cap){
         //There is not enough memory, allocate a new
         //buffer or expand the old one.
         bool same_buffer_start;
         size_type real_cap = 0;
         std::pair<pointer, bool> ret =
            this->allocation_command
               (allocate_new | expand_fwd | expand_bwd,
                  new_cap, new_cap, real_cap, this->members_.m_start);

         //Check for forward expansion
         same_buffer_start = ret.second && this->members_.m_start == ret.first;
         if(same_buffer_start){
            #ifdef BOOST_CONTAINER_VECTOR_ALLOC_STATS
            ++this->num_expand_fwd;
            #endif
            this->members_.m_capacity  = real_cap;
         }

         //If there is no forward expansion, move objects
         else{
            //We will reuse insert code, so create a dummy input iterator
            T *dummy_it(container_detail::to_raw_pointer(this->members_.m_start));
            container_detail::advanced_insert_aux_proxy<Allocator, boost::move_iterator<T*>, T*>
               proxy(this->alloc(), ::boost::make_move_iterator(dummy_it), ::boost::make_move_iterator(dummy_it));
            //Backwards (and possibly forward) expansion
            if(ret.second){
               #ifdef BOOST_CONTAINER_VECTOR_ALLOC_STATS
               ++this->num_expand_bwd;
               #endif
               this->priv_range_insert_expand_backwards
                  ( container_detail::to_raw_pointer(ret.first)
                  , real_cap
                  , container_detail::to_raw_pointer(this->members_.m_start)
                  , 0
                  , proxy);
            }
            //New buffer
            else{
               #ifdef BOOST_CONTAINER_VECTOR_ALLOC_STATS
               ++this->num_alloc;
               #endif
               this->priv_range_insert_new_allocation
                  ( container_detail::to_raw_pointer(ret.first)
                  , real_cap
                  , container_detail::to_raw_pointer(this->members_.m_start)
                  , 0
                  , proxy);
            }
         }
      }
   }

   //! <b>Effects</b>: Tries to deallocate the excess of memory created
   //!   with previous allocations. The size of the vector is unchanged
   //!
   //! <b>Throws</b>: If memory allocation throws, or T's copy/move constructor throws.
   //!
   //! <b>Complexity</b>: Linear to size().
   void shrink_to_fit()
   {  this->priv_shrink_to_fit(alloc_version());   }

   //////////////////////////////////////////////
   //
   //               element access
   //
   //////////////////////////////////////////////

   //! <b>Requires</b>: !empty()
   //!
   //! <b>Effects</b>: Returns a reference to the first
   //!   element of the container.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   reference         front() BOOST_CONTAINER_NOEXCEPT
   { return *this->members_.m_start; }

   //! <b>Requires</b>: !empty()
   //!
   //! <b>Effects</b>: Returns a const reference to the first
   //!   element of the container.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   const_reference   front() const BOOST_CONTAINER_NOEXCEPT
   { return *this->members_.m_start; }

   //! <b>Requires</b>: !empty()
   //!
   //! <b>Effects</b>: Returns a reference to the last
   //!   element of the container.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   reference         back() BOOST_CONTAINER_NOEXCEPT
   { return this->members_.m_start[this->members_.m_size - 1]; }

   //! <b>Requires</b>: !empty()
   //!
   //! <b>Effects</b>: Returns a const reference to the last
   //!   element of the container.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   const_reference   back()  const BOOST_CONTAINER_NOEXCEPT
   { return this->members_.m_start[this->members_.m_size - 1]; }

   //! <b>Requires</b>: size() > n.
   //!
   //! <b>Effects</b>: Returns a reference to the nth element
   //!   from the beginning of the container.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   reference operator[](size_type n)        
   { return this->members_.m_start[n]; }

   //! <b>Requires</b>: size() > n.
   //!
   //! <b>Effects</b>: Returns a const reference to the nth element
   //!   from the beginning of the container.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   const_reference operator[](size_type n) const BOOST_CONTAINER_NOEXCEPT
   { return this->members_.m_start[n]; }

   //! <b>Requires</b>: size() > n.
   //!
   //! <b>Effects</b>: Returns a reference to the nth element
   //!   from the beginning of the container.
   //!
   //! <b>Throws</b>: std::range_error if n >= size()
   //!
   //! <b>Complexity</b>: Constant.
   reference at(size_type n)
   { this->priv_check_range(n); return this->members_.m_start[n]; }

   //! <b>Requires</b>: size() > n.
   //!
   //! <b>Effects</b>: Returns a const reference to the nth element
   //!   from the beginning of the container.
   //!
   //! <b>Throws</b>: std::range_error if n >= size()
   //!
   //! <b>Complexity</b>: Constant.
   const_reference at(size_type n) const
   { this->priv_check_range(n); return this->members_.m_start[n]; }

   //////////////////////////////////////////////
   //
   //                 data access
   //
   //////////////////////////////////////////////

   //! <b>Returns</b>: Allocator pointer such that [data(),data() + size()) is a valid range.
   //!   For a non-empty vector, data() == &front().
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   T* data() BOOST_CONTAINER_NOEXCEPT
   { return container_detail::to_raw_pointer(this->members_.m_start); }

   //! <b>Returns</b>: Allocator pointer such that [data(),data() + size()) is a valid range.
   //!   For a non-empty vector, data() == &front().
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   const T * data()  const BOOST_CONTAINER_NOEXCEPT
   { return container_detail::to_raw_pointer(this->members_.m_start); }

   //////////////////////////////////////////////
   //
   //                modifiers
   //
   //////////////////////////////////////////////

   #if defined(BOOST_CONTAINER_PERFECT_FORWARDING) || defined(BOOST_CONTAINER_DOXYGEN_INVOKED)
   //! <b>Effects</b>: Inserts an object of type T constructed with
   //!   std::forward<Args>(args)... in the end of the vector.
   //!
   //! <b>Throws</b>: If memory allocation throws or the in-place constructor throws or
   //!   T's move constructor throws.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   template<class ...Args>
   void emplace_back(Args &&...args)
   {
      T* back_pos = container_detail::to_raw_pointer(this->members_.m_start) + this->members_.m_size;
      if (this->members_.m_size < this->members_.m_capacity){
         //There is more memory, just construct a new object at the end
         allocator_traits_type::construct(this->alloc(), back_pos, ::boost::forward<Args>(args)...);
         ++this->members_.m_size;
      }
      else{
         typedef container_detail::advanced_insert_aux_emplace<Allocator, T*, Args...> type;
         type &&proxy = type(this->alloc(), ::boost::forward<Args>(args)...);
         this->priv_forward_range_insert(back_pos, 1, proxy);
      }
   }

   //! <b>Requires</b>: position must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Inserts an object of type T constructed with
   //!   std::forward<Args>(args)... before position
   //!
   //! <b>Throws</b>: If memory allocation throws or the in-place constructor throws or
   //!   T's move constructor/assignment throws.
   //!
   //! <b>Complexity</b>: If position is end(), amortized constant time
   //!   Linear time otherwise.
   template<class ...Args>
   iterator emplace(const_iterator position, Args && ...args)
   {
      //Just call more general insert(pos, size, value) and return iterator
      size_type pos_n = position - cbegin();
      typedef container_detail::advanced_insert_aux_emplace<Allocator, T*, Args...> type;
      type &&proxy = type(this->alloc(), ::boost::forward<Args>(args)...);
      this->priv_forward_range_insert(position.get_ptr(), 1, proxy);
      return iterator(this->members_.m_start + pos_n);
   }

   #else

   #define BOOST_PP_LOCAL_MACRO(n)                                                              \
   BOOST_PP_EXPR_IF(n, template<) BOOST_PP_ENUM_PARAMS(n, class P) BOOST_PP_EXPR_IF(n, >)       \
   void emplace_back(BOOST_PP_ENUM(n, BOOST_CONTAINER_PP_PARAM_LIST, _))                        \
   {                                                                                            \
      T* back_pos = container_detail::to_raw_pointer                                            \
         (this->members_.m_start) + this->members_.m_size;                                      \
      if (this->members_.m_size < this->members_.m_capacity){                                   \
         allocator_traits_type::construct (this->alloc()                                        \
            , back_pos BOOST_PP_ENUM_TRAILING(n, BOOST_CONTAINER_PP_PARAM_FORWARD, _) );        \
         ++this->members_.m_size;                                                               \
      }                                                                                         \
      else{                                                                                     \
         container_detail::BOOST_PP_CAT(BOOST_PP_CAT(advanced_insert_aux_emplace, n), arg)      \
            <Allocator, T* BOOST_PP_ENUM_TRAILING_PARAMS(n, P)> proxy                                   \
            (this->alloc() BOOST_PP_ENUM_TRAILING(n, BOOST_CONTAINER_PP_PARAM_FORWARD, _));     \
         this->priv_forward_range_insert(back_pos, 1, proxy);                                   \
      }                                                                                         \
   }                                                                                            \
                                                                                                \
   BOOST_PP_EXPR_IF(n, template<) BOOST_PP_ENUM_PARAMS(n, class P) BOOST_PP_EXPR_IF(n, >)       \
   iterator emplace(const_iterator pos                                                          \
                    BOOST_PP_ENUM_TRAILING(n, BOOST_CONTAINER_PP_PARAM_LIST, _))                \
   {                                                                                            \
      size_type pos_n = pos - cbegin();                                                         \
         container_detail::BOOST_PP_CAT(BOOST_PP_CAT(advanced_insert_aux_emplace, n), arg)      \
            <Allocator, T* BOOST_PP_ENUM_TRAILING_PARAMS(n, P)> proxy                                   \
            (this->alloc() BOOST_PP_ENUM_TRAILING(n, BOOST_CONTAINER_PP_PARAM_FORWARD, _));     \
      this->priv_forward_range_insert(container_detail::to_raw_pointer(pos.get_ptr()), 1,proxy);\
      return iterator(this->members_.m_start + pos_n);                                          \
   }                                                                                            \
   //!
   #define BOOST_PP_LOCAL_LIMITS (0, BOOST_CONTAINER_MAX_CONSTRUCTOR_PARAMETERS)
   #include BOOST_PP_LOCAL_ITERATE()

   #endif   //#ifdef BOOST_CONTAINER_PERFECT_FORWARDING

   #if defined(BOOST_CONTAINER_DOXYGEN_INVOKED)
   //! <b>Effects</b>: Inserts a copy of x at the end of the vector.
   //!
   //! <b>Throws</b>: If memory allocation throws or
   //!   T's copy/move constructor throws.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   void push_back(const T &x);

   //! <b>Effects</b>: Constructs a new element in the end of the vector
   //!   and moves the resources of mx to this new element.
   //!
   //! <b>Throws</b>: If memory allocation throws or
   //!   T's move constructor throws.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   void push_back(T &&x);
   #else
   BOOST_MOVE_CONVERSION_AWARE_CATCH(push_back, T, void, priv_push_back)
   #endif
 
   #if defined(BOOST_CONTAINER_DOXYGEN_INVOKED)
   //! <b>Requires</b>: position must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Insert a copy of x before position.
   //!
   //! <b>Throws</b>: If memory allocation throws or T's copy/move constructor/assignment throws.
   //!
   //! <b>Complexity</b>: If position is end(), amortized constant time
   //!   Linear time otherwise.
   iterator insert(const_iterator position, const T &x);

   //! <b>Requires</b>: position must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Insert a new element before position with mx's resources.
   //!
   //! <b>Throws</b>: If memory allocation throws.
   //!
   //! <b>Complexity</b>: If position is end(), amortized constant time
   //!   Linear time otherwise.
   iterator insert(const_iterator position, T &&x);
   #else
   BOOST_MOVE_CONVERSION_AWARE_CATCH_1ARG(insert, T, iterator, priv_insert, const_iterator)
   #endif

   //! <b>Requires</b>: p must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Insert n copies of x before pos.
   //!
   //! <b>Returns</b>: an iterator to the first inserted element or p if n is 0.
   //!
   //! <b>Throws</b>: If memory allocation throws or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to n.
   iterator insert(const_iterator p, size_type n, const T& x)
   {  return this->insert(p, cvalue_iterator(x, n), cvalue_iterator());  }

   //! <b>Requires</b>: p must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Insert a copy of the [first, last) range before pos.
   //!
   //! <b>Returns</b>: an iterator to the first inserted element or pos if first == last.
   //!
   //! <b>Throws</b>: If memory allocation throws, T's constructor from a
   //!   dereferenced InpIt throws or T's copy/move constructor/assignment throws.
   //!
   //! <b>Complexity</b>: Linear to std::distance [first, last).
   template <class InIt>
   iterator insert(const_iterator pos, InIt first, InIt last
      #if !defined(BOOST_CONTAINER_DOXYGEN_INVOKED)
      , typename container_detail::enable_if_c
         < !container_detail::is_convertible<InIt, size_type>::value
            && container_detail::is_input_iterator<InIt>::value
         >::type * = 0
      #endif
      )
   {
      const size_type n_pos = pos - this->cbegin();
      iterator it(pos.get_ptr());
      for(;first != last; ++first){
         it = this->emplace(it, *first);
         ++it;
      }
      return this->begin() + n_pos;
   }

   #if !defined(BOOST_CONTAINER_DOXYGEN_INVOKED)
   template <class FwdIt>
   iterator insert(const_iterator pos, FwdIt first, FwdIt last
      , typename container_detail::enable_if_c
         < !container_detail::is_convertible<FwdIt, size_type>::value
            && !container_detail::is_input_iterator<FwdIt>::value
         >::type * = 0
      )
   {
      const size_type n_pos = pos - this->cbegin();
      const size_type n = std::distance(first, last);
      container_detail::advanced_insert_aux_proxy<Allocator, FwdIt, T*> proxy(this->alloc(), first, last);
      this->priv_forward_range_insert(pos.get_ptr(), n, proxy);
      return this->begin() + n_pos;
   }
   #endif

   //! <b>Effects</b>: Removes the last element from the vector.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant time.
   void pop_back()
   {
      //Destroy last element
      --this->members_.m_size;
      this->destroy(container_detail::to_raw_pointer(this->members_.m_start) + this->members_.m_size);
   }

   //! <b>Effects</b>: Erases the element at position pos.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the elements between pos and the
   //!   last element. Constant if pos is the last element.
   iterator erase(const_iterator position)
   {
      T *pos = container_detail::to_raw_pointer(position.get_ptr());
      T *beg = container_detail::to_raw_pointer(this->members_.m_start);
      ::boost::move(pos + 1, beg + this->members_.m_size, pos);
      --this->members_.m_size;
      //Destroy last element
      base_t::destroy(container_detail::to_raw_pointer(this->members_.m_start) + this->members_.m_size);
      return iterator(position.get_ptr());
   }

   //! <b>Effects</b>: Erases the elements pointed by [first, last).
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the distance between first and last
   //!   plus linear to the elements between pos and the last element.
   iterator erase(const_iterator first, const_iterator last)
   {
      if (first != last){   // worth doing, copy down over hole
         T* end_pos = container_detail::to_raw_pointer(this->members_.m_start) + this->members_.m_size;
         T* ptr = container_detail::to_raw_pointer(boost::move
            (container_detail::to_raw_pointer(last.get_ptr())
            ,end_pos
            ,container_detail::to_raw_pointer(first.get_ptr())
            ));
         size_type destroyed = (end_pos - ptr);
         this->destroy_n(ptr, destroyed);
         this->members_.m_size -= destroyed;
      }
      return iterator(first.get_ptr());
   }

   //! <b>Effects</b>: Swaps the contents of *this and x.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   void swap(vector& x)
   {
      //Just swap internals
      this->swap_members(x);
      //And now the allocator
      container_detail::bool_<allocator_traits_type::propagate_on_container_swap::value> flag;
      container_detail::swap_alloc(this->alloc(), x.alloc(), flag);
   }

   //! <b>Effects</b>: Erases all the elements of the vector.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements in the vector.
   void clear() BOOST_CONTAINER_NOEXCEPT
   {  this->prot_destroy_all();  }

   /// @cond

   //Absolutely experimental. This function might change, disappear or simply crash!
   template<class BiDirPosConstIt, class BiDirValueIt>
   void insert_ordered_at(size_type element_count, BiDirPosConstIt last_position_it, BiDirValueIt last_value_it)
   {
      const size_type *dummy = 0;
      this->priv_insert_ordered_at(element_count, last_position_it, false, &dummy[0], last_value_it);
   }

   //Absolutely experimental. This function might change, disappear or simply crash!
   template<class BiDirPosConstIt, class BiDirSkipConstIt, class BiDirValueIt>
   void insert_ordered_at(size_type element_count, BiDirPosConstIt last_position_it, BiDirSkipConstIt last_skip_it, BiDirValueIt last_value_it)
   {
      this->priv_insert_ordered_at(element_count, last_position_it, true, last_skip_it, last_value_it);
   }

   private:
   iterator priv_insert(const_iterator position, const T &x)
   {
      //Just call more general insert(pos, size, value) and return iterator
      size_type pos_n = position - cbegin();
      this->insert(position, (size_type)1, x);
      return iterator(this->members_.m_start + pos_n);
   }

   iterator priv_insert(const_iterator position, BOOST_RV_REF(T) x)
   {
      //Just call more general insert(pos, size, value) and return iterator
      size_type pos_n = position - cbegin();
      this->insert(position
                  ,repeat_move_it(repeat_it(x, 1))
                  ,repeat_move_it(repeat_it()));
      return iterator(this->members_.m_start + pos_n);
   }

   template <class U>
   void priv_push_back(BOOST_MOVE_CATCH_FWD(U) x)
   {
      if (this->members_.m_size < this->members_.m_capacity){
         //There is more memory, just construct a new object at the end
         allocator_traits_type::construct
            ( this->alloc()
            , container_detail::to_raw_pointer(this->members_.m_start + this->members_.m_size)
            , ::boost::forward<U>(x) );
         ++this->members_.m_size;
      }
      else{
         this->insert(this->cend(), ::boost::forward<U>(x));
      }
   }

   void priv_shrink_to_fit(allocator_v1)
   {
      if(this->members_.m_capacity){
         if(!this->size()){
            this->prot_deallocate();
         }
         else{
            //Allocate a new buffer.
            size_type real_cap = 0;
            std::pair<pointer, bool> ret =
               this->allocation_command
                  (allocate_new, this->size(), this->size(), real_cap, this->members_.m_start);
            if(real_cap < this->capacity()){
               //We will reuse insert code, so create a dummy input iterator
               T *dummy_it(container_detail::to_raw_pointer(this->members_.m_start));
               container_detail::advanced_insert_aux_proxy<Allocator, boost::move_iterator<T*>, T*>
                  proxy(this->alloc(), ::boost::make_move_iterator(dummy_it), ::boost::make_move_iterator(dummy_it));
               #ifdef BOOST_CONTAINER_VECTOR_ALLOC_STATS
               ++this->num_alloc;
               #endif
               this->priv_range_insert_new_allocation
                  ( container_detail::to_raw_pointer(ret.first)
                  , real_cap
                  , container_detail::to_raw_pointer(this->members_.m_start)
                  , 0
                  , proxy);
            }
            else{
               this->alloc().deallocate(ret.first, real_cap);
            }
         }
      }
   }

   void priv_shrink_to_fit(allocator_v2)
   {
      if(this->members_.m_capacity){
         if(!size()){
            this->prot_deallocate();
         }
         else{
            size_type received_size;
            if(this->allocation_command
               ( shrink_in_place | nothrow_allocation
               , this->capacity(), this->size()
               , received_size,   this->members_.m_start).first){
               this->members_.m_capacity = received_size;
               #ifdef BOOST_CONTAINER_VECTOR_ALLOC_STATS
               ++this->num_shrink;
               #endif
            }
         }
      }
   }

   void priv_forward_range_insert(pointer pos, const size_type n, advanced_insert_aux_int_t &interf)
   {
      //Check if we have enough memory or try to expand current memory
      size_type remaining = this->members_.m_capacity - this->members_.m_size;
      bool same_buffer_start;
      std::pair<pointer, bool> ret;
      size_type real_cap = this->members_.m_capacity;

      //Check if we already have room
      if (n <= remaining){
         same_buffer_start = true;
      }
      else{
         //There is not enough memory, allocate a new
         //buffer or expand the old one.
         size_type new_cap = this->next_capacity(n);
         ret = this->allocation_command
               (allocate_new | expand_fwd | expand_bwd,
                  this->members_.m_size + n, new_cap, real_cap, this->members_.m_start);

         //Check for forward expansion
         same_buffer_start = ret.second && this->members_.m_start == ret.first;
         if(same_buffer_start){
            this->members_.m_capacity  = real_cap;
         }
      }
     
      //If we had room or we have expanded forward
      if (same_buffer_start){
         #ifdef BOOST_CONTAINER_VECTOR_ALLOC_STATS
         ++this->num_expand_fwd;
         #endif
         this->priv_range_insert_expand_forward
            (container_detail::to_raw_pointer(pos), n, interf);
      }
      //Backwards (and possibly forward) expansion
      else if(ret.second){
         #ifdef BOOST_CONTAINER_VECTOR_ALLOC_STATS
         ++this->num_expand_bwd;
         #endif
         this->priv_range_insert_expand_backwards
            ( container_detail::to_raw_pointer(ret.first)
            , real_cap
            , container_detail::to_raw_pointer(pos)
            , n
            , interf);
      }
      //New buffer
      else{
         #ifdef BOOST_CONTAINER_VECTOR_ALLOC_STATS
         ++this->num_alloc;
         #endif
         this->priv_range_insert_new_allocation
            ( container_detail::to_raw_pointer(ret.first)
            , real_cap
            , container_detail::to_raw_pointer(pos)
            , n
            , interf);
      }
   }

   //Absolutely experimental. This function might change, disappear or simply crash!
   template<class BiDirPosConstIt, class BiDirSkipConstIt, class BiDirValueIt>
   void priv_insert_ordered_at( size_type element_count, BiDirPosConstIt last_position_it
                              , bool do_skip, BiDirSkipConstIt last_skip_it, BiDirValueIt last_value_it)
   {
      const size_type old_size_pos = this->size();
      this->reserve(old_size_pos + element_count);
      T* const begin_ptr = container_detail::to_raw_pointer(this->members_.m_start);
      size_type insertions_left = element_count;
      size_type next_pos = old_size_pos;
      size_type hole_size = element_count;

      //Exception rollback. If any copy throws before the hole is filled, values
      //already inserted/copied at the end of the buffer will be destroyed.
      typename value_traits::ArrayDestructor past_hole_values_destroyer
         (begin_ptr + old_size_pos + element_count, this->alloc(), size_type(0u));
      //Loop for each insertion backwards, first moving the elements after the insertion point,
      //then inserting the element.
      while(insertions_left){
         if(do_skip){
            size_type n = *(--last_skip_it);
            std::advance(last_value_it, -difference_type(n));
         }
         const size_type pos = static_cast<size_type>(*(--last_position_it));
         BOOST_ASSERT(pos <= old_size_pos);
         //If needed shift the range after the insertion point and the previous insertion point.
         //Function will take care if the shift crosses the size() boundary, using copy/move
         //or uninitialized copy/move if necessary.
         size_type new_hole_size = (pos != next_pos)
            ? priv_insert_ordered_at_shift_range(pos, next_pos, this->size(), insertions_left)
            : hole_size
            ;
         if(new_hole_size > 0){
            //The hole was reduced by priv_insert_ordered_at_shift_range so expand exception rollback range backwards
            past_hole_values_destroyer.increment_size_backwards(next_pos - pos);
            //Insert the new value in the hole
            allocator_traits_type::construct(this->alloc(), begin_ptr + pos + insertions_left - 1, *(--last_value_it));
            --new_hole_size;
            if(new_hole_size == 0){
               //Hole was just filled, disable exception rollback and change vector size
               past_hole_values_destroyer.release();
               this->members_.m_size += element_count;
            }
            else{
               //The hole was reduced by the new insertion by one
               past_hole_values_destroyer.increment_size_backwards(size_type(1u));
            }
         }
         else{
            if(hole_size){
               //Hole was just filled by priv_insert_ordered_at_shift_range, disable exception rollback and change vector size
               past_hole_values_destroyer.release();
               this->members_.m_size += element_count;
            }
            //Insert the new value in the already constructed range
            begin_ptr[pos + insertions_left - 1] = *(--last_value_it);
         }
         --insertions_left;
         hole_size = new_hole_size;
         next_pos = pos;
      }
   }

   //Takes the range pointed by [first_pos, last_pos) and shifts it to the right
   //by 'shift_count'. 'limit_pos' marks the end of constructed elements.
   //
   //Precondition: first_pos <= last_pos <= limit_pos
   //
   //The shift operation might cross limit_pos so elements to moved beyond limit_pos
   //are uninitialized_moved with an allocator. Other elements are moved.
   //
   //The shift operation might left uninitialized elements after limit_pos
   //and the number of uninitialized elements is returned by the function.
   //
   //Old situation:
   //       first_pos   last_pos         old_limit
   //             |       |                  | 
   // ____________V_______V__________________V_____________
   //|   prefix   | range |     suffix       |raw_mem      ~
   //|____________|_______|__________________|_____________~
   //
   //New situation in Case Allocator (hole_size == 0):
   // range is moved through move assignments
   //
   //       first_pos   last_pos         limit_pos
   //             |       |                  | 
   // ____________V_______V__________________V_____________
   //|   prefix'  |       |  | range |suffix'|raw_mem      ~
   //|________________+______|___^___|_______|_____________~
   //                 |          |
   //                 |_>_>_>_>_>^                    
   //
   //
   //New situation in Case B (hole_size > 0):
   // range is moved through uninitialized moves
   //
   //       first_pos   last_pos         limit_pos
   //             |       |                  | 
   // ____________V_______V__________________V________________
   //|    prefix' |       |                  | [hole] | range |
   //|_______________________________________|________|___^___|
   //                 |                                   |
   //                 |_>_>_>_>_>_>_>_>_>_>_>_>_>_>_>_>_>_^
   //
   //New situation in Case C (hole_size == 0):
   // range is moved through move assignments and uninitialized moves
   //
   //       first_pos   last_pos         limit_pos
   //             |       |                  | 
   // ____________V_______V__________________V___
   //|   prefix'  |       |              | range |
   //|___________________________________|___^___|
   //                 |                      |
   //                 |_>_>_>_>_>_>_>_>_>_>_>^
   size_type priv_insert_ordered_at_shift_range
      (size_type first_pos, size_type last_pos, size_type limit_pos, size_type shift_count)
   {
      BOOST_ASSERT(first_pos <= last_pos);
      BOOST_ASSERT(last_pos <= limit_pos);
      //
      T* const begin_ptr = container_detail::to_raw_pointer(this->members_.m_start);
      T* const first_ptr = begin_ptr + first_pos;
      T* const last_ptr  = begin_ptr + last_pos;

      size_type hole_size = 0;
      //Case Allocator:
      if((last_pos + shift_count) <= limit_pos){
         //All move assigned
         boost::move_backward(first_ptr, last_ptr, last_ptr + shift_count);
      }
      //Case B:
      else if((first_pos + shift_count) >= limit_pos){
         //All uninitialized_moved
         ::boost::container::uninitialized_move_alloc(this->alloc(), first_ptr, last_ptr, first_ptr + shift_count);
         hole_size = last_pos + shift_count - limit_pos;
      }
      //Case C:
      else{
         //Some uninitialized_moved
         T* const limit_ptr    = begin_ptr + limit_pos;
         T* const boundary_ptr = limit_ptr - shift_count;
         ::boost::container::uninitialized_move_alloc(this->alloc(), boundary_ptr, last_ptr, limit_ptr);
         //The rest is move assigned
         boost::move_backward(first_ptr, boundary_ptr, limit_ptr);
      }
      return hole_size;
   }

   private:
   void priv_range_insert_expand_forward(T* pos, size_type n, advanced_insert_aux_int_t &interf)
   {
      //n can't be 0, because there is nothing to do in that case
      if(!n) return;
      //There is enough memory
      T* old_finish = container_detail::to_raw_pointer(this->members_.m_start) + this->members_.m_size;
      const size_type elems_after = old_finish - pos;

      if (elems_after >= n){
         //New elements can be just copied.
         //Move to uninitialized memory last objects
         ::boost::container::uninitialized_move_alloc
            (this->alloc(), old_finish - n, old_finish, old_finish);
         this->members_.m_size += n;
         //Copy previous to last objects to the initialized end
         boost::move_backward(pos, old_finish - n, old_finish);
         //Insert new objects in the pos
         interf.copy_remaining_to(pos);
      }
      else {
         //The new elements don't fit in the [pos, end()) range. Copy
         //to the beginning of the unallocated zone the last new elements.
         interf.uninitialized_copy_some_and_update(old_finish, elems_after, false);
         this->members_.m_size += n - elems_after;
         //Copy old [pos, end()) elements to the uninitialized memory
         ::boost::container::uninitialized_move_alloc
            (this->alloc(), pos, old_finish, container_detail::to_raw_pointer(this->members_.m_start) + this->members_.m_size);
         this->members_.m_size += elems_after;
         //Copy first new elements in pos
         interf.copy_remaining_to(pos);
      }
   }

   void priv_range_insert_new_allocation
      (T* new_start, size_type new_cap, T* pos, size_type n, advanced_insert_aux_int_t &interf)
   {
      //n can be zero, if we want to reallocate!
      T *new_finish = new_start;
      T *old_finish;
      //Anti-exception rollbacks
      typename value_traits::ArrayDeallocator scoped_alloc(new_start, this->alloc(), new_cap);
      typename value_traits::ArrayDestructor constructed_values_destroyer(new_start, this->alloc(), 0u);

      //Initialize with [begin(), pos) old buffer
      //the start of the new buffer
      T *old_buffer = container_detail::to_raw_pointer(this->members_.m_start);
      if(old_buffer){
         new_finish = ::boost::container::uninitialized_move_alloc
            (this->alloc(), container_detail::to_raw_pointer(this->members_.m_start), pos, old_finish = new_finish);
         constructed_values_destroyer.increment_size(new_finish - old_finish);
      }
      //Initialize new objects, starting from previous point
      interf.uninitialized_copy_remaining_to(old_finish = new_finish);
      new_finish += n;
      constructed_values_destroyer.increment_size(new_finish - old_finish);
      //Initialize from the rest of the old buffer,
      //starting from previous point
      if(old_buffer){
         new_finish = ::boost::container::uninitialized_move_alloc
            (this->alloc(), pos, old_buffer + this->members_.m_size, new_finish);
         //Destroy and deallocate old elements
         //If there is allocated memory, destroy and deallocate
         if(!value_traits::trivial_dctr_after_move)
            this->destroy_n(old_buffer, this->members_.m_size);
         this->alloc().deallocate(this->members_.m_start, this->members_.m_capacity);
      }
      this->members_.m_start     = new_start;
      this->members_.m_size      = new_finish - new_start;
      this->members_.m_capacity  = new_cap;
      //All construction successful, disable rollbacks
      constructed_values_destroyer.release();
      scoped_alloc.release();
   }

   void priv_range_insert_expand_backwards
         (T* new_start, size_type new_capacity,
          T* pos, const size_type n, advanced_insert_aux_int_t &interf)
   {
      //n can be zero to just expand capacity
      //Backup old data
      T* old_start  = container_detail::to_raw_pointer(this->members_.m_start);
      T* old_finish = old_start + this->members_.m_size;
      size_type old_size = this->members_.m_size;

      //We can have 8 possibilities:
      const size_type elemsbefore   = (size_type)(pos - old_start);
      const size_type s_before      = (size_type)(old_start - new_start);

      //Update the vector buffer information to a safe state
      this->members_.m_start      = new_start;
      this->members_.m_capacity   = new_capacity;
      this->members_.m_size = 0;

      //If anything goes wrong, this object will destroy
      //all the old objects to fulfill previous vector state
      typename value_traits::OldArrayDestructor old_values_destroyer(old_start, this->alloc(), old_size);
      //Check if s_before is big enough to hold the beginning of old data + new data
      if(difference_type(s_before) >= difference_type(elemsbefore + n)){
         //Copy first old values before pos, after that the new objects
         ::boost::container::uninitialized_move_alloc(this->alloc(), old_start, pos, new_start);
         this->members_.m_size = elemsbefore;
         interf.uninitialized_copy_remaining_to(new_start + elemsbefore);
         this->members_.m_size += n;
         //Check if s_before is so big that even copying the old data + new data
         //there is a gap between the new data and the old data
         if(s_before >= (old_size + n)){
            //Old situation:
            // _________________________________________________________
            //|            raw_mem                | old_begin | old_end |
            //| __________________________________|___________|_________|
            //
            //New situation:
            // _________________________________________________________
            //| old_begin |    new   | old_end |         raw_mem        |
            //|___________|__________|_________|________________________|
            //
            //Now initialize the rest of memory with the last old values
            ::boost::container::uninitialized_move_alloc
               (this->alloc(), pos, old_finish, new_start + elemsbefore + n);
            //All new elements correctly constructed, avoid new element destruction
            this->members_.m_size = old_size + n;
            //Old values destroyed automatically with "old_values_destroyer"
            //when "old_values_destroyer" goes out of scope unless the have trivial
            //destructor after move.
            if(value_traits::trivial_dctr_after_move)
               old_values_destroyer.release();
         }
         //s_before is so big that divides old_end
         else{
            //Old situation:
            // __________________________________________________
            //|            raw_mem         | old_begin | old_end |
            //| ___________________________|___________|_________|
            //
            //New situation:
            // __________________________________________________
            //| old_begin |   new    | old_end |  raw_mem        |
            //|___________|__________|_________|_________________|
            //
            //Now initialize the rest of memory with the last old values
            //All new elements correctly constructed, avoid new element destruction
            size_type raw_gap = s_before - (elemsbefore + n);
            //Now initialize the rest of s_before memory with the
            //first of elements after new values
            ::boost::container::uninitialized_move_alloc
               (this->alloc(), pos, pos + raw_gap, new_start + elemsbefore + n);
            //Update size since we have a contiguous buffer
            this->members_.m_size = old_size + s_before;
            //All new elements correctly constructed, avoid old element destruction
            old_values_destroyer.release();
            //Now copy remaining last objects in the old buffer begin
            T *to_destroy = ::boost::move(pos + raw_gap, old_finish, old_start);
            //Now destroy redundant elements except if they were moved and
            //they have trivial destructor after move
            size_type n_destroy =  old_finish - to_destroy;
            if(!value_traits::trivial_dctr_after_move)
               this->destroy_n(to_destroy, n_destroy);
            this->members_.m_size -= n_destroy;
         }
      }
      else{
         //Check if we have to do the insertion in two phases
         //since maybe s_before is not big enough and
         //the buffer was expanded both sides
         //
         //Old situation:
         // _________________________________________________
         //| raw_mem | old_begin + old_end |  raw_mem        |
         //|_________|_____________________|_________________|
         //
         //New situation with do_after:
         // _________________________________________________
         //|     old_begin + new + old_end     |  raw_mem    |
         //|___________________________________|_____________|
         //
         //New without do_after:
         // _________________________________________________
         //| old_begin + new + old_end  |  raw_mem           |
         //|____________________________|____________________|
         //
         bool do_after    = n > s_before;

         //Now we can have two situations: the raw_mem of the
         //beginning divides the old_begin, or the new elements:
         if (s_before <= elemsbefore) {
            //The raw memory divides the old_begin group:
            //
            //If we need two phase construction (do_after)
            //new group is divided in new = new_beg + new_end groups
            //In this phase only new_beg will be inserted
            //
            //Old situation:
            // _________________________________________________
            //| raw_mem | old_begin | old_end |  raw_mem        |
            //|_________|___________|_________|_________________|
            //
            //New situation with do_after(1):
            //This is not definitive situation, the second phase
            //will include
            // _________________________________________________
            //| old_begin | new_beg | old_end |  raw_mem        |
            //|___________|_________|_________|_________________|
            //
            //New situation without do_after:
            // _________________________________________________
            //| old_begin | new | old_end |  raw_mem            |
            //|___________|_____|_________|_____________________|
            //
            //Copy the first part of old_begin to raw_mem
            T *start_n = old_start + difference_type(s_before);
            ::boost::container::uninitialized_move_alloc
               (this->alloc(), old_start, start_n, new_start);
            //The buffer is all constructed until old_end,
            //release destroyer and update size
            old_values_destroyer.release();
            this->members_.m_size = old_size + s_before;
            //Now copy the second part of old_begin overwriting himself
            T* next = ::boost::move(start_n, pos, old_start);
            if(do_after){
               //Now copy the new_beg elements
               interf.copy_some_and_update(next, s_before, true);
            }
            else{
               //Now copy the all the new elements
               interf.copy_remaining_to(next);
               T* move_start = next + n;
               //Now displace old_end elements
               T* move_end   = ::boost::move(pos, old_finish, move_start);
               //Destroy remaining moved elements from old_end except if
               //they have trivial destructor after being moved
               difference_type n_destroy = s_before - n;
               if(!value_traits::trivial_dctr_after_move)
                  this->destroy_n(move_end, n_destroy);
               this->members_.m_size -= n_destroy;
            }
         }
         else {
            //If we have to expand both sides,
            //we will play if the first new values so
            //calculate the upper bound of new values

            //The raw memory divides the new elements
            //
            //If we need two phase construction (do_after)
            //new group is divided in new = new_beg + new_end groups
            //In this phase only new_beg will be inserted
            //
            //Old situation:
            // _______________________________________________________
            //|   raw_mem     | old_begin | old_end |  raw_mem        |
            //|_______________|___________|_________|_________________|
            //
            //New situation with do_after():
            // ____________________________________________________
            //| old_begin |    new_beg    | old_end |  raw_mem     |
            //|___________|_______________|_________|______________|
            //
            //New situation without do_after:
            // ______________________________________________________
            //| old_begin | new | old_end |  raw_mem                 |
            //|___________|_____|_________|__________________________|
            //
            //First copy whole old_begin and part of new to raw_mem
            ::boost::container::uninitialized_move_alloc
               (this->alloc(), old_start, pos, new_start);
            this->members_.m_size = elemsbefore;

            const size_type mid_n = difference_type(s_before) - elemsbefore;
            interf.uninitialized_copy_some_and_update(new_start + elemsbefore, mid_n, true);
            this->members_.m_size = old_size + s_before;
            //The buffer is all constructed until old_end,
            //release destroyer and update size
            old_values_destroyer.release();

            if(do_after){
               //Copy new_beg part
               interf.copy_some_and_update(old_start, s_before - mid_n, true);
            }
            else{
               //Copy all new elements
               interf.copy_remaining_to(old_start);
               T* move_start = old_start + (n-mid_n);
               //Displace old_end
               T* move_end = ::boost::move(pos, old_finish, move_start);
               //Destroy remaining moved elements from old_end except if they
               //have trivial destructor after being moved
               difference_type n_destroy = s_before - n;
               if(!value_traits::trivial_dctr_after_move)
                  this->destroy_n(move_end, n_destroy);
               this->members_.m_size -= n_destroy;
            }
         }

         //This is only executed if two phase construction is needed
         //This can be executed without exception handling since we
         //have to just copy and append in raw memory and
         //old_values_destroyer has been released in phase 1.
         if(do_after){
            //The raw memory divides the new elements
            //
            //Old situation:
            // ______________________________________________________
            //|   raw_mem    | old_begin |  old_end   |  raw_mem     |
            //|______________|___________|____________|______________|
            //
            //New situation with do_after(1):
            // _______________________________________________________
            //| old_begin   +   new_beg  | new_end |old_end | raw_mem |
            //|__________________________|_________|________|_________|
            //
            //New situation with do_after(2):
            // ______________________________________________________
            //| old_begin      +       new            | old_end |raw |
            //|_______________________________________|_________|____|
            //
            const size_type n_after  = n - s_before;
            const difference_type elemsafter = old_size - elemsbefore;

            //We can have two situations:
            if (elemsafter > difference_type(n_after)){
               //The raw_mem from end will divide displaced old_end
               //
               //Old situation:
               // ______________________________________________________
               //|   raw_mem    | old_begin |  old_end   |  raw_mem     |
               //|______________|___________|____________|______________|
               //
               //New situation with do_after(1):
               // _______________________________________________________
               //| old_begin   +   new_beg  | new_end |old_end | raw_mem |
               //|__________________________|_________|________|_________|
               //
               //First copy the part of old_end raw_mem
               T* finish_n = old_finish - difference_type(n_after);
               ::boost::container::uninitialized_move_alloc
                  (this->alloc(), finish_n, old_finish, old_finish);
               this->members_.m_size += n_after;
               //Displace the rest of old_end to the new position
               boost::move_backward(pos, finish_n, old_finish);
               //Now overwrite with new_end
               //The new_end part is [first + (n - n_after), last)
               interf.copy_remaining_to(pos);
            }
            else {
               //The raw_mem from end will divide new_end part
               //
               //Old situation:
               // _____________________________________________________________
               //|   raw_mem    | old_begin |  old_end   |  raw_mem            |
               //|______________|___________|____________|_____________________|
               //
               //New situation with do_after(2):
               // _____________________________________________________________
               //| old_begin   +   new_beg  |     new_end   |old_end | raw_mem |
               //|__________________________|_______________|________|_________|
               //
               size_type mid_last_dist = n_after - elemsafter;
               //First initialize data in raw memory
               //The new_end part is [first + (n - n_after), last)
               interf.uninitialized_copy_some_and_update(old_finish, elemsafter, false);
               this->members_.m_size += mid_last_dist;
               ::boost::container::uninitialized_move_alloc
                  (this->alloc(), pos, old_finish, old_finish + mid_last_dist);
               this->members_.m_size += n_after - mid_last_dist;
               //Now copy the part of new_end over constructed elements
               interf.copy_remaining_to(pos);
            }
         }
      }
   }

   void priv_check_range(size_type n) const
   {
      //If n is out of range, throw an out_of_range exception
      if (n >= this->size())
         throw std::out_of_range("vector::at");
   }

   #ifdef BOOST_CONTAINER_VECTOR_ALLOC_STATS
   public:
   unsigned int num_expand_fwd;
   unsigned int num_expand_bwd;
   unsigned int num_shrink;
   unsigned int num_alloc;
   void reset_alloc_stats()
   {  num_expand_fwd = num_expand_bwd = num_alloc = 0, num_shrink = 0;   }
   #endif
   /// @endcond
};

template <class T, class Allocator>
inline bool
operator==(const vector<T, Allocator>& x, const vector<T, Allocator>& y)
{
   //Check first size and each element if needed
   return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin());
}

template <class T, class Allocator>
inline bool
operator!=(const vector<T, Allocator>& x, const vector<T, Allocator>& y)
{
   //Check first size and each element if needed
  return x.size() != y.size() || !std::equal(x.begin(), x.end(), y.begin());
}

template <class T, class Allocator>
inline bool
operator<(const vector<T, Allocator>& x, const vector<T, Allocator>& y)
{
   return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
}

template <class T, class Allocator>
inline void swap(vector<T, Allocator>& x, vector<T, Allocator>& y)
{  x.swap(y);  }

}}

/// @cond

namespace boost {

/*

//!has_trivial_destructor_after_move<> == true_type
//!specialization for optimizations
template <class T, class Allocator>
struct has_trivial_destructor_after_move<boost::container::vector<T, Allocator> >
{
   static const bool value = has_trivial_destructor<Allocator>::value;
};

*/

}

/// @endcond

#include <boost/container/detail/config_end.hpp>

#endif //   #ifndef  BOOST_CONTAINER_CONTAINER_VECTOR_HPP

