///////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_ALLOCATOR_HPP
#define BOOST_INTERPROCESS_ALLOCATOR_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/pointer_to_other.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/containers/allocation_type.hpp>
#include <boost/interprocess/containers/container/detail/multiallocation_chain.hpp>
#include <boost/interprocess/allocators/detail/allocator_common.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/containers/version_type.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/assert.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/interprocess/detail/type_traits.hpp>

#include <memory>
#include <algorithm>
#include <cstddef>
#include <stdexcept>

//!\file
//!Describes an allocator that allocates portions of fixed size
//!memory buffer (shared memory, mapped file...)

namespace boost {
namespace interprocess {


//!An STL compatible allocator that uses a segment manager as 
//!memory source. The internal pointer type will of the same type (raw, smart) as
//!"typename SegmentManager::void_pointer" type. This allows
//!placing the allocator in shared memory, memory mapped-files, etc...
template<class T, class SegmentManager>
class allocator 
{
   public:
   //Segment manager
   typedef SegmentManager                                segment_manager;
   typedef typename SegmentManager::void_pointer         void_pointer;

   /// @cond
   private:

   //Self type
   typedef allocator<T, SegmentManager>   self_t;

   //Pointer to void
   typedef typename segment_manager::void_pointer  aux_pointer_t;

   //Typedef to const void pointer
   typedef typename 
      boost::pointer_to_other
         <aux_pointer_t, const void>::type   cvoid_ptr;

   //Pointer to the allocator
   typedef typename boost::pointer_to_other
      <cvoid_ptr, segment_manager>::type     alloc_ptr_t;

   //Not assignable from related allocator
   template<class T2, class SegmentManager2>
   allocator& operator=(const allocator<T2, SegmentManager2>&);

   //Not assignable from other allocator
   allocator& operator=(const allocator&);

   //Pointer to the allocator
   alloc_ptr_t mp_mngr;
   /// @endcond

   public:
   typedef T                                    value_type;
   typedef typename boost::pointer_to_other
      <cvoid_ptr, T>::type                      pointer;
   typedef typename boost::
      pointer_to_other<pointer, const T>::type  const_pointer;
   typedef typename detail::add_reference
                     <value_type>::type         reference;
   typedef typename detail::add_reference
                     <const value_type>::type   const_reference;
   typedef std::size_t                          size_type;
   typedef std::ptrdiff_t                       difference_type;

   typedef boost::interprocess::version_type<allocator, 2>   version;

   /// @cond

   //Experimental. Don't use.
   typedef boost::container::containers_detail::transform_multiallocation_chain
      <typename SegmentManager::multiallocation_chain, T>multiallocation_chain;
   /// @endcond

   //!Obtains an allocator that allocates
   //!objects of type T2
   template<class T2>
   struct rebind
   {   
      typedef allocator<T2, SegmentManager>     other;
   };

   //!Returns the segment manager.
   //!Never throws
   segment_manager* get_segment_manager()const
   {  return detail::get_pointer(mp_mngr);   }

   //!Constructor from the segment manager.
   //!Never throws
   allocator(segment_manager *segment_mngr) 
      : mp_mngr(segment_mngr) { }

   //!Constructor from other allocator.
   //!Never throws
   allocator(const allocator &other) 
      : mp_mngr(other.get_segment_manager()){ }

   //!Constructor from related allocator.
   //!Never throws
   template<class T2>
   allocator(const allocator<T2, SegmentManager> &other) 
      : mp_mngr(other.get_segment_manager()){}

   //!Allocates memory for an array of count elements. 
   //!Throws boost::interprocess::bad_alloc if there is no enough memory
   pointer allocate(size_type count, cvoid_ptr hint = 0)
   {
      (void)hint;
      if(count > this->max_size())
         throw bad_alloc();
      return pointer(static_cast<value_type*>(mp_mngr->allocate(count*sizeof(T))));
   }

   //!Deallocates memory previously allocated.
   //!Never throws
   void deallocate(const pointer &ptr, size_type)
   {  mp_mngr->deallocate((void*)detail::get_pointer(ptr));  }

   //!Returns the number of elements that could be allocated.
   //!Never throws
   size_type max_size() const
   {  return mp_mngr->get_size()/sizeof(T);   }

   //!Swap segment manager. Does not throw. If each allocator is placed in
   //!different memory segments, the result is undefined.
   friend void swap(self_t &alloc1, self_t &alloc2)
   {  detail::do_swap(alloc1.mp_mngr, alloc2.mp_mngr);   }

   //!Returns maximum the number of objects the previously allocated memory
   //!pointed by p can hold. This size only works for memory allocated with
   //!allocate, allocation_command and allocate_many.
   size_type size(const pointer &p) const
   {  
      return (size_type)mp_mngr->size(detail::get_pointer(p))/sizeof(T);
   }

   std::pair<pointer, bool>
      allocation_command(boost::interprocess::allocation_type command,
                         size_type limit_size, 
                         size_type preferred_size,
                         size_type &received_size, const pointer &reuse = 0)
   {
      return mp_mngr->allocation_command
         (command, limit_size, preferred_size, received_size, detail::get_pointer(reuse));
   }

   //!Allocates many elements of size elem_size in a contiguous block
   //!of memory. The minimum number to be allocated is min_elements,
   //!the preferred and maximum number is
   //!preferred_elements. The number of actually allocated elements is
   //!will be assigned to received_size. The elements must be deallocated
   //!with deallocate(...)
   multiallocation_chain allocate_many
      (size_type elem_size, std::size_t num_elements)
   {
      return multiallocation_chain(mp_mngr->allocate_many(sizeof(T)*elem_size, num_elements));
   }

   //!Allocates n_elements elements, each one of size elem_sizes[i]in a
   //!contiguous block
   //!of memory. The elements must be deallocated
   multiallocation_chain allocate_many
      (const size_type *elem_sizes, size_type n_elements)
   {
      multiallocation_chain(mp_mngr->allocate_many(elem_sizes, n_elements, sizeof(T)));
   }

   //!Allocates many elements of size elem_size in a contiguous block
   //!of memory. The minimum number to be allocated is min_elements,
   //!the preferred and maximum number is
   //!preferred_elements. The number of actually allocated elements is
   //!will be assigned to received_size. The elements must be deallocated
   //!with deallocate(...)
   void deallocate_many(multiallocation_chain chain)
   {
      return mp_mngr->deallocate_many(chain.extract_multiallocation_chain());
   }

   //!Allocates just one object. Memory allocated with this function
   //!must be deallocated only with deallocate_one().
   //!Throws boost::interprocess::bad_alloc if there is no enough memory
   pointer allocate_one()
   {  return this->allocate(1);  }

   //!Allocates many elements of size == 1 in a contiguous block
   //!of memory. The minimum number to be allocated is min_elements,
   //!the preferred and maximum number is
   //!preferred_elements. The number of actually allocated elements is
   //!will be assigned to received_size. Memory allocated with this function
   //!must be deallocated only with deallocate_one().
   multiallocation_chain allocate_individual
      (std::size_t num_elements)
   {  return this->allocate_many(1, num_elements); }

   //!Deallocates memory previously allocated with allocate_one().
   //!You should never use deallocate_one to deallocate memory allocated
   //!with other functions different from allocate_one(). Never throws
   void deallocate_one(const pointer &p)
   {  return this->deallocate(p, 1);  }

   //!Allocates many elements of size == 1 in a contiguous block
   //!of memory. The minimum number to be allocated is min_elements,
   //!the preferred and maximum number is
   //!preferred_elements. The number of actually allocated elements is
   //!will be assigned to received_size. Memory allocated with this function
   //!must be deallocated only with deallocate_one().
   void deallocate_individual(multiallocation_chain chain)
   {  return this->deallocate_many(boost::interprocess::move(chain)); }

   //!Returns address of mutable object.
   //!Never throws
   pointer address(reference value) const
   {  return pointer(boost::addressof(value));  }

   //!Returns address of non mutable object.
   //!Never throws
   const_pointer address(const_reference value) const
   {  return const_pointer(boost::addressof(value));  }

   //!Copy construct an object
   //!Throws if T's copy constructor throws
   void construct(const pointer &ptr, const_reference v)
   {  new((void*)detail::get_pointer(ptr)) value_type(v);  }

   //!Default construct an object. 
   //!Throws if T's default constructor throws
   void construct(const pointer &ptr)
   {  new((void*)detail::get_pointer(ptr)) value_type;  }

   //!Destroys object. Throws if object's
   //!destructor throws
   void destroy(const pointer &ptr)
   {  BOOST_ASSERT(ptr != 0); (*ptr).~value_type();  }
};

//!Equality test for same type
//!of allocator
template<class T, class SegmentManager> inline
bool operator==(const allocator<T , SegmentManager>  &alloc1, 
                const allocator<T, SegmentManager>  &alloc2)
   {  return alloc1.get_segment_manager() == alloc2.get_segment_manager(); }

//!Inequality test for same type
//!of allocator
template<class T, class SegmentManager> inline
bool operator!=(const allocator<T, SegmentManager>  &alloc1, 
                const allocator<T, SegmentManager>  &alloc2)
   {  return alloc1.get_segment_manager() != alloc2.get_segment_manager(); }

}  //namespace interprocess {

/// @cond

template<class T>
struct has_trivial_destructor;

template<class T, class SegmentManager>
struct has_trivial_destructor
   <boost::interprocess::allocator <T, SegmentManager> >
{
   enum { value = true };
};
/// @endcond

}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_ALLOCATOR_HPP

