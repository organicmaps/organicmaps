//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SEGMENT_MANAGER_HPP
#define BOOST_INTERPROCESS_SEGMENT_MANAGER_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/detail/no_exceptions_support.hpp>
#include <boost/interprocess/detail/type_traits.hpp>

#include <boost/interprocess/detail/transform_iterator.hpp>

#include <boost/interprocess/detail/mpl.hpp>
#include <boost/interprocess/detail/segment_manager_helper.hpp>
#include <boost/interprocess/detail/named_proxy.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/indexes/iset_index.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/smart_ptr/deleter.hpp>
#include <boost/interprocess/detail/move.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <cstddef>   //std::size_t
#include <string>    //char_traits
#include <new>       //std::nothrow
#include <utility>   //std::pair
#ifndef BOOST_NO_EXCEPTIONS
#include <exception>
#endif

//!\file
//!Describes the object placed in a memory segment that provides
//!named object allocation capabilities for single-segment and
//!multi-segment allocations.

namespace boost{
namespace interprocess{

//!This object is the public base class of segment manager.
//!This class only depends on the memory allocation algorithm
//!and implements all the allocation features not related
//!to named or unique objects.
//!
//!Storing a reference to segment_manager forces
//!the holder class to be dependent on index types and character types.
//!When such dependence is not desirable and only anonymous and raw
//!allocations are needed, segment_manager_base is the correct answer.
template<class MemoryAlgorithm>
class segment_manager_base
   :  private MemoryAlgorithm
{
   public:
   typedef segment_manager_base<MemoryAlgorithm> segment_manager_base_type;
   typedef typename MemoryAlgorithm::void_pointer  void_pointer;
   typedef typename MemoryAlgorithm::mutex_family  mutex_family;
   typedef MemoryAlgorithm memory_algorithm;
   
   /// @cond
   
   //Experimental. Don't use
   typedef typename MemoryAlgorithm::multiallocation_chain    multiallocation_chain;

   /// @endcond

   //!This constant indicates the payload size
   //!associated with each allocation of the memory algorithm
   static const std::size_t PayloadPerAllocation = MemoryAlgorithm::PayloadPerAllocation;

   //!Constructor of the segment_manager_base
   //!
   //!"size" is the size of the memory segment where
   //!the basic segment manager is being constructed.
   //!
   //!"reserved_bytes" is the number of bytes 
   //!after the end of the memory algorithm object itself
   //!that the memory algorithm will exclude from
   //!dynamic allocation
   //!
   //!Can throw
   segment_manager_base(std::size_t size, std::size_t reserved_bytes)
      :  MemoryAlgorithm(size, reserved_bytes)
   {
      assert((sizeof(segment_manager_base<MemoryAlgorithm>) == sizeof(MemoryAlgorithm)));
   }

   //!Returns the size of the memory
   //!segment
   std::size_t get_size() const
   {  return MemoryAlgorithm::get_size();  }

   //!Returns the number of free bytes of the memory
   //!segment
   std::size_t get_free_memory() const
   {  return MemoryAlgorithm::get_free_memory();  }

   //!Obtains the minimum size needed by
   //!the segment manager
   static std::size_t get_min_size (std::size_t size)
   {  return MemoryAlgorithm::get_min_size(size);  }

   //!Allocates nbytes bytes. This function is only used in 
   //!single-segment management. Never throws
   void * allocate (std::size_t nbytes, std::nothrow_t)
   {  return MemoryAlgorithm::allocate(nbytes);   }

   /// @cond

   //Experimental. Dont' use.
   //!Allocates n_elements of
   //!elem_size bytes. Throws bad_alloc on failure.
   multiallocation_chain allocate_many(std::size_t elem_bytes, std::size_t num_elements)
   {
      multiallocation_chain mem(MemoryAlgorithm::allocate_many(elem_bytes, num_elements));
      if(mem.empty()) throw bad_alloc();
      return boost::interprocess::move(mem);
   }

   //!Allocates n_elements, each one of
   //!element_lenghts[i]*sizeof_element bytes. Throws bad_alloc on failure.
   multiallocation_chain allocate_many
      (const std::size_t *element_lenghts, std::size_t n_elements, std::size_t sizeof_element = 1)
   {
      multiallocation_chain mem(MemoryAlgorithm::allocate_many(element_lenghts, n_elements, sizeof_element));
      if(mem.empty()) throw bad_alloc();
      return boost::interprocess::move(mem);
   }

   //!Allocates n_elements of
   //!elem_size bytes. Returns a default constructed iterator on failure.
   multiallocation_chain allocate_many
      (std::size_t elem_bytes, std::size_t num_elements, std::nothrow_t)
   {  return MemoryAlgorithm::allocate_many(elem_bytes, num_elements); }

   //!Allocates n_elements, each one of
   //!element_lenghts[i]*sizeof_element bytes.
   //!Returns a default constructed iterator on failure.
   multiallocation_chain allocate_many
      (const std::size_t *elem_sizes, std::size_t n_elements, std::size_t sizeof_element, std::nothrow_t)
   {  return MemoryAlgorithm::allocate_many(elem_sizes, n_elements, sizeof_element); }

   //!Deallocates elements pointed by the
   //!multiallocation iterator range.
   void deallocate_many(multiallocation_chain chain)
   {  MemoryAlgorithm::deallocate_many(boost::interprocess::move(chain)); }

   /// @endcond

   //!Allocates nbytes bytes. Throws boost::interprocess::bad_alloc
   //!on failure
   void * allocate(std::size_t nbytes)
   {  
      void * ret = MemoryAlgorithm::allocate(nbytes);
      if(!ret)
         throw bad_alloc();
      return ret;
   }

   //!Allocates nbytes bytes. This function is only used in 
   //!single-segment management. Never throws
   void * allocate_aligned (std::size_t nbytes, std::size_t alignment, std::nothrow_t)
   {  return MemoryAlgorithm::allocate_aligned(nbytes, alignment);   }

   //!Allocates nbytes bytes. This function is only used in 
   //!single-segment management. Throws bad_alloc when fails
   void * allocate_aligned(std::size_t nbytes, std::size_t alignment)
   {  
      void * ret = MemoryAlgorithm::allocate_aligned(nbytes, alignment);
      if(!ret)
         throw bad_alloc();
      return ret;
   }

   template<class T>
   std::pair<T *, bool>
      allocation_command  (boost::interprocess::allocation_type command,   std::size_t limit_size,
                           std::size_t preferred_size,std::size_t &received_size,
                           T *reuse_ptr = 0)
   {
      std::pair<T *, bool> ret = MemoryAlgorithm::allocation_command
         ( command | boost::interprocess::nothrow_allocation, limit_size, preferred_size, received_size
         , reuse_ptr);
      if(!(command & boost::interprocess::nothrow_allocation) && !ret.first)
         throw bad_alloc();
      return ret;
   }

   std::pair<void *, bool>
      raw_allocation_command  (boost::interprocess::allocation_type command,   std::size_t limit_objects,
                           std::size_t preferred_objects,std::size_t &received_objects,
                           void *reuse_ptr = 0, std::size_t sizeof_object = 1)
   {
      std::pair<void *, bool> ret = MemoryAlgorithm::raw_allocation_command
         ( command | boost::interprocess::nothrow_allocation, limit_objects, preferred_objects, received_objects
         , reuse_ptr, sizeof_object);
      if(!(command & boost::interprocess::nothrow_allocation) && !ret.first)
         throw bad_alloc();
      return ret;
   }

   //!Deallocates the bytes allocated with allocate/allocate_many()
   //!pointed by addr
   void   deallocate          (void *addr)
   {  MemoryAlgorithm::deallocate(addr);   }

   //!Increases managed memory in extra_size bytes more. This only works
   //!with single-segment management.
   void grow(std::size_t extra_size)
   {  MemoryAlgorithm::grow(extra_size);   }

   //!Decreases managed memory to the minimum. This only works
   //!with single-segment management.
   void shrink_to_fit()
   {  MemoryAlgorithm::shrink_to_fit();   }

   //!Returns the result of "all_memory_deallocated()" function
   //!of the used memory algorithm
   bool all_memory_deallocated()
   {   return MemoryAlgorithm::all_memory_deallocated(); }

   //!Returns the result of "check_sanity()" function
   //!of the used memory algorithm
   bool check_sanity()
   {   return MemoryAlgorithm::check_sanity(); }

   //!Writes to zero free memory (memory not yet allocated)
   //!of the memory algorithm
   void zero_free_memory()
   {   MemoryAlgorithm::zero_free_memory(); }

   //!Returns the size of the buffer previously allocated pointed by ptr
   std::size_t size(const void *ptr) const
   {   return MemoryAlgorithm::size(ptr); }

   /// @cond
   protected:
   void * prot_anonymous_construct
      (std::size_t num, bool dothrow, detail::in_place_interface &table)
   {
      typedef detail::block_header block_header_t;
      block_header_t block_info (  table.size*num
                                 , table.alignment
                                 , anonymous_type
                                 , 1
                                 , 0);

      //Allocate memory
      void *ptr_struct = this->allocate(block_info.total_size(), std::nothrow_t());

      //Check if there is enough memory
      if(!ptr_struct){
         if(dothrow){
            throw bad_alloc();
         }
         else{
            return 0; 
         }
      }

      //Build scoped ptr to avoid leaks with constructor exception
      detail::mem_algo_deallocator<MemoryAlgorithm> mem(ptr_struct, *this);

      //Now construct the header
      block_header_t * hdr = new(ptr_struct) block_header_t(block_info);
      void *ptr = 0; //avoid gcc warning
      ptr = hdr->value();

      //Now call constructors
      detail::array_construct(ptr, num, table);

      //All constructors successful, we don't want erase memory
      mem.release();
      return ptr;
   }

   //!Calls the destructor and makes an anonymous deallocate
   void prot_anonymous_destroy(const void *object, detail::in_place_interface &table)
   {

      //Get control data from associated with this object    
      typedef detail::block_header block_header_t;
      block_header_t *ctrl_data = block_header_t::block_header_from_value(object, table.size, table.alignment);

      //-------------------------------
      //scoped_lock<rmutex> guard(m_header);
      //-------------------------------

      if(ctrl_data->alloc_type() != anonymous_type){
         //This is not an anonymous object, the pointer is wrong!
         assert(0);
      }

      //Call destructors and free memory
      //Build scoped ptr to avoid leaks with destructor exception
      std::size_t destroyed = 0;
      table.destroy_n(const_cast<void*>(object), ctrl_data->m_value_bytes/table.size, destroyed);
      this->deallocate(ctrl_data);
   }
   /// @endcond
};

//!This object is placed in the beginning of memory segment and
//!implements the allocation (named or anonymous) of portions
//!of the segment. This object contains two indexes that
//!maintain an association between a name and a portion of the segment. 
//!
//!The first index contains the mappings for normal named objects using the 
//!char type specified in the template parameter.
//!
//!The second index contains the association for unique instances. The key will
//!be the const char * returned from type_info.name() function for the unique
//!type to be constructed.
//!
//!segment_manager<CharType, MemoryAlgorithm, IndexType> inherits publicly
//!from segment_manager_base<MemoryAlgorithm> and inherits from it
//!many public functions related to anonymous object and raw memory allocation.
//!See segment_manager_base reference to know about those functions.
template<class CharType
        ,class MemoryAlgorithm
        ,template<class IndexConfig> class IndexType>
class segment_manager
   :  public segment_manager_base<MemoryAlgorithm>
{ 
   /// @cond
   //Non-copyable
   segment_manager();
   segment_manager(const segment_manager &);
   segment_manager &operator=(const segment_manager &);
   typedef segment_manager_base<MemoryAlgorithm> Base;
   typedef detail::block_header block_header_t;
   /// @endcond

   public:
   typedef MemoryAlgorithm                memory_algorithm;
   typedef typename Base::void_pointer    void_pointer;
   typedef CharType                       char_type;

   typedef segment_manager_base<MemoryAlgorithm>   segment_manager_base_type;

   static const std::size_t PayloadPerAllocation = Base::PayloadPerAllocation;

   /// @cond
   private:
   typedef detail::index_config<CharType, MemoryAlgorithm>  index_config_named;
   typedef detail::index_config<char, MemoryAlgorithm>      index_config_unique;
   typedef IndexType<index_config_named>                    index_type;
   typedef detail::bool_<is_intrusive_index<index_type>::value >    is_intrusive_t;
   typedef detail::bool_<is_node_index<index_type>::value>          is_node_index_t;

   public:
   typedef IndexType<index_config_named>                    named_index_t;
   typedef IndexType<index_config_unique>                   unique_index_t;
   typedef detail::char_ptr_holder<CharType>                char_ptr_holder_t;
   typedef detail::segment_manager_iterator_transform
      <typename named_index_t::const_iterator
      ,is_intrusive_index<index_type>::value>   named_transform;

   typedef detail::segment_manager_iterator_transform
      <typename unique_index_t::const_iterator
      ,is_intrusive_index<index_type>::value>   unique_transform;
   /// @endcond

   typedef typename Base::mutex_family       mutex_family;

   typedef transform_iterator
      <typename named_index_t::const_iterator, named_transform> const_named_iterator;
   typedef transform_iterator
      <typename unique_index_t::const_iterator, unique_transform> const_unique_iterator;

   /// @cond

   //!Constructor proxy object definition helper class
   template<class T>
   struct construct_proxy
   {
      typedef detail::named_proxy<segment_manager, T, false>   type;
   };

   //!Constructor proxy object definition helper class
   template<class T>
   struct construct_iter_proxy
   {
      typedef detail::named_proxy<segment_manager, T, true>   type;
   };

   /// @endcond

   //!Constructor of the segment manager
   //!"size" is the size of the memory segment where
   //!the segment manager is being constructed.
   //!Can throw
   segment_manager(std::size_t size)
      :  Base(size, priv_get_reserved_bytes())
      ,  m_header(static_cast<Base*>(get_this_pointer()))
   {
      (void) anonymous_instance;   (void) unique_instance;
      assert(static_cast<const void*>(this) == static_cast<const void*>(static_cast<Base*>(this)));
   }

   //!Tries to find a previous named allocation. Returns the address
   //!and the object count. On failure the first member of the
   //!returned pair is 0.
   template <class T>
   std::pair<T*, std::size_t> find  (const CharType* name)
   {  return this->priv_find_impl<T>(name, true);  }

   //!Tries to find a previous unique allocation. Returns the address
   //!and the object count. On failure the first member of the
   //!returned pair is 0.
   template <class T>
   std::pair<T*, std::size_t> find (const detail::unique_instance_t* name)
   {  return this->priv_find_impl<T>(name, true);  }

   //!Tries to find a previous named allocation. Returns the address
   //!and the object count. On failure the first member of the
   //!returned pair is 0. This search is not mutex-protected!
   template <class T>
   std::pair<T*, std::size_t> find_no_lock  (const CharType* name)
   {  return this->priv_find_impl<T>(name, false);  }

   //!Tries to find a previous unique allocation. Returns the address
   //!and the object count. On failure the first member of the
   //!returned pair is 0. This search is not mutex-protected!
   template <class T>
   std::pair<T*, std::size_t> find_no_lock (const detail::unique_instance_t* name)
   {  return this->priv_find_impl<T>(name, false);  }

   //!Returns throwing "construct" proxy
   //!object
   template <class T>
   typename construct_proxy<T>::type      
      construct(char_ptr_holder_t name)
   {  return typename construct_proxy<T>::type (this, name, false, true);  }

   //!Returns throwing "search or construct" proxy
   //!object
   template <class T>
   typename construct_proxy<T>::type find_or_construct(char_ptr_holder_t name)
   {  return typename construct_proxy<T>::type (this, name, true, true);  }

   //!Returns no throwing "construct" proxy
   //!object
   template <class T>
   typename construct_proxy<T>::type
      construct(char_ptr_holder_t name, std::nothrow_t)
   {  return typename construct_proxy<T>::type (this, name, false, false);  }

   //!Returns no throwing "search or construct"
   //!proxy object
   template <class T>
   typename construct_proxy<T>::type   
      find_or_construct(char_ptr_holder_t name, std::nothrow_t)
   {  return typename construct_proxy<T>::type (this, name, true, false);  }

   //!Returns throwing "construct from iterators" proxy object
   template <class T>
   typename construct_iter_proxy<T>::type     
      construct_it(char_ptr_holder_t name)
   {  return typename construct_iter_proxy<T>::type (this, name, false, true);  }

   //!Returns throwing "search or construct from iterators"
   //!proxy object
   template <class T>
   typename construct_iter_proxy<T>::type   
      find_or_construct_it(char_ptr_holder_t name)
   {  return typename construct_iter_proxy<T>::type (this, name, true, true);  }

   //!Returns no throwing "construct from iterators"
   //!proxy object
   template <class T>
   typename construct_iter_proxy<T>::type   
      construct_it(char_ptr_holder_t name, std::nothrow_t)
   {  return typename construct_iter_proxy<T>::type (this, name, false, false);  }

   //!Returns no throwing "search or construct from iterators"
   //!proxy object
   template <class T>
   typename construct_iter_proxy<T>::type 
      find_or_construct_it(char_ptr_holder_t name, std::nothrow_t)
   {  return typename construct_iter_proxy<T>::type (this, name, true, false);  }

   //!Calls object function blocking recursive interprocess_mutex and guarantees that 
   //!no new named_alloc or destroy will be executed by any process while 
   //!executing the object function call*/
   template <class Func>
   void atomic_func(Func &f)
   {  scoped_lock<rmutex> guard(m_header);  f();  }

   //!Destroys a previously created unique instance.
   //!Returns false if the object was not present.
   template <class T>
   bool destroy(const detail::unique_instance_t *)
   {
      detail::placement_destroy<T> dtor;
      return this->priv_generic_named_destroy<char>
         (typeid(T).name(), m_header.m_unique_index, dtor, is_intrusive_t());
   }

   //!Destroys the named object with
   //!the given name. Returns false if that object can't be found.
   template <class T>
   bool destroy(const CharType *name)
   {
      detail::placement_destroy<T> dtor;
      return this->priv_generic_named_destroy<CharType>
               (name, m_header.m_named_index, dtor, is_intrusive_t());
   }

   //!Destroys an anonymous, unique or named object
   //!using it's address
   template <class T>
   void destroy_ptr(const T *p)
   {
      //If T is void transform it to char
      typedef typename detail::char_if_void<T>::type data_t;
      detail::placement_destroy<data_t> dtor;
      priv_destroy_ptr(p, dtor);
   }

   //!Returns the name of an object created with construct/find_or_construct
   //!functions. Does not throw
   template<class T>
   static const CharType *get_instance_name(const T *ptr)
   { return priv_get_instance_name(block_header_t::block_header_from_value(ptr));  }

   //!Returns the length of an object created with construct/find_or_construct
   //!functions. Does not throw.
   template<class T>
   static std::size_t get_instance_length(const T *ptr)
   {  return priv_get_instance_length(block_header_t::block_header_from_value(ptr), sizeof(T));  }

   //!Returns is the the name of an object created with construct/find_or_construct
   //!functions. Does not throw
   template<class T>
   static instance_type get_instance_type(const T *ptr)
   {  return priv_get_instance_type(block_header_t::block_header_from_value(ptr));  }

   //!Preallocates needed index resources to optimize the 
   //!creation of "num" named objects in the managed memory segment.
   //!Can throw boost::interprocess::bad_alloc if there is no enough memory.
   void reserve_named_objects(std::size_t num)
   {  
      //-------------------------------
      scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      m_header.m_named_index.reserve(num);  
   }

   //!Preallocates needed index resources to optimize the 
   //!creation of "num" unique objects in the managed memory segment.
   //!Can throw boost::interprocess::bad_alloc if there is no enough memory.
   void reserve_unique_objects(std::size_t num)
   {  
      //-------------------------------
      scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      m_header.m_unique_index.reserve(num);
   }

   //!Calls shrink_to_fit in both named and unique object indexes
   //!to try to free unused memory from those indexes.
   void shrink_to_fit_indexes()
   {  
      //-------------------------------
      scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      m_header.m_named_index.shrink_to_fit();  
      m_header.m_unique_index.shrink_to_fit();  
   }

   //!Returns the number of named objects stored in
   //!the segment.
   std::size_t get_num_named_objects()
   {  
      //-------------------------------
      scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      return m_header.m_named_index.size();  
   }

   //!Returns the number of unique objects stored in
   //!the segment.
   std::size_t get_num_unique_objects()
   {  
      //-------------------------------
      scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      return m_header.m_unique_index.size();  
   }

   //!Obtains the minimum size needed by the
   //!segment manager
   static std::size_t get_min_size()
   {  return Base::get_min_size(priv_get_reserved_bytes());  }

   //!Returns a constant iterator to the beginning of the information about
   //!the named allocations performed in this segment manager
   const_named_iterator named_begin() const
   {
      return make_transform_iterator
         (m_header.m_named_index.begin(), named_transform());
   }

   //!Returns a constant iterator to the end of the information about
   //!the named allocations performed in this segment manager
   const_named_iterator named_end() const
   {
      return make_transform_iterator
         (m_header.m_named_index.end(), named_transform());
   }

   //!Returns a constant iterator to the beginning of the information about
   //!the unique allocations performed in this segment manager
   const_unique_iterator unique_begin() const
   {
      return make_transform_iterator
         (m_header.m_unique_index.begin(), unique_transform());
   }

   //!Returns a constant iterator to the end of the information about
   //!the unique allocations performed in this segment manager
   const_unique_iterator unique_end() const
   {
      return make_transform_iterator
         (m_header.m_unique_index.end(), unique_transform());
   }

   //!This is the default allocator to allocate types T
   //!from this managed segment
   template<class T>
   struct allocator
   {
      typedef boost::interprocess::allocator<T, segment_manager> type;
   };

   //!Returns an instance of the default allocator for type T
   //!initialized that allocates memory from this segment manager.
   template<class T>
   typename allocator<T>::type
      get_allocator()
   {   return typename allocator<T>::type(this); }

   //!This is the default deleter to delete types T
   //!from this managed segment.
   template<class T>
   struct deleter
   {
      typedef boost::interprocess::deleter<T, segment_manager> type;
   };

   //!Returns an instance of the default allocator for type T
   //!initialized that allocates memory from this segment manager.
   template<class T>
   typename deleter<T>::type
      get_deleter()
   {   return typename deleter<T>::type(this); }

   /// @cond

   //!Generic named/anonymous new function. Offers all the possibilities, 
   //!such as throwing, search before creating, and the constructor is 
   //!encapsulated in an object function.
   template<class T>
   T *generic_construct(const CharType *name, 
                         std::size_t num, 
                         bool try2find, 
                         bool dothrow,
                         detail::in_place_interface &table)
   {
      return static_cast<T*>
         (priv_generic_construct(name, num, try2find, dothrow, table));
   }

   private:
   //!Tries to find a previous named allocation. Returns the address
   //!and the object count. On failure the first member of the
   //!returned pair is 0.
   template <class T>
   std::pair<T*, std::size_t> priv_find_impl (const CharType* name, bool lock)
   {  
      //The name can't be null, no anonymous object can be found by name
      assert(name != 0);
      detail::placement_destroy<T> table;
      std::size_t size;
      void *ret;

      if(name == reinterpret_cast<const CharType*>(-1)){
         ret = priv_generic_find<char> (typeid(T).name(), m_header.m_unique_index, table, size, is_intrusive_t(), lock);
      }
      else{
         ret = priv_generic_find<CharType> (name, m_header.m_named_index, table, size, is_intrusive_t(), lock);
      }
      return std::pair<T*, std::size_t>(static_cast<T*>(ret), size);
   }

   //!Tries to find a previous unique allocation. Returns the address
   //!and the object count. On failure the first member of the
   //!returned pair is 0.
   template <class T>
   std::pair<T*, std::size_t> priv_find__impl (const detail::unique_instance_t* name, bool lock)
   {
      detail::placement_destroy<T> table;
      std::size_t size;
      void *ret = priv_generic_find<char>(name, m_header.m_unique_index, table, size, is_intrusive_t(), lock); 
      return std::pair<T*, std::size_t>(static_cast<T*>(ret), size);
   }

   void *priv_generic_construct(const CharType *name, 
                         std::size_t num, 
                         bool try2find, 
                         bool dothrow,
                         detail::in_place_interface &table)
   {
      void *ret;
      //Security overflow check
      if(num > ((std::size_t)-1)/table.size){
         if(dothrow)
            throw bad_alloc();
         else
            return 0;
      }
      if(name == 0){
         ret = this->prot_anonymous_construct(num, dothrow, table);
      }
      else if(name == reinterpret_cast<const CharType*>(-1)){
         ret = this->priv_generic_named_construct<char>
            (unique_type, table.type_name, num, try2find, dothrow, table, m_header.m_unique_index, is_intrusive_t());
      }
      else{
         ret = this->priv_generic_named_construct<CharType>
            (named_type, name, num, try2find, dothrow, table, m_header.m_named_index, is_intrusive_t());
      }
      return ret;
   }

   void priv_destroy_ptr(const void *ptr, detail::in_place_interface &dtor)
   {
      block_header_t *ctrl_data = block_header_t::block_header_from_value(ptr, dtor.size, dtor.alignment);
      switch(ctrl_data->alloc_type()){
         case anonymous_type:
            this->prot_anonymous_destroy(ptr, dtor);
         break;

         case named_type:
            this->priv_generic_named_destroy<CharType>
               (ctrl_data, m_header.m_named_index, dtor, is_node_index_t());
         break;

         case unique_type:
            this->priv_generic_named_destroy<char>
               (ctrl_data, m_header.m_unique_index, dtor, is_node_index_t());
         break;

         default:
            //This type is unknown, bad pointer passed to this function!
            assert(0);
         break;
      }
   }

   //!Returns the name of an object created with construct/find_or_construct
   //!functions. Does not throw
   static const CharType *priv_get_instance_name(block_header_t *ctrl_data)
   {
      boost::interprocess::allocation_type type = ctrl_data->alloc_type();
      if(type != named_type){
         assert((type == anonymous_type && ctrl_data->m_num_char == 0) ||
                (type == unique_type    && ctrl_data->m_num_char != 0) );
         return 0;
      }
      CharType *name = static_cast<CharType*>(ctrl_data->template name<CharType>());
   
      //Sanity checks
      assert(ctrl_data->sizeof_char() == sizeof(CharType));
      assert(ctrl_data->m_num_char == std::char_traits<CharType>::length(name));
      return name;
   }

   static std::size_t priv_get_instance_length(block_header_t *ctrl_data, std::size_t sizeofvalue)
   {
      //Get header
      assert((ctrl_data->value_bytes() %sizeofvalue) == 0);
      return ctrl_data->value_bytes()/sizeofvalue;
   }

   //!Returns is the the name of an object created with construct/find_or_construct
   //!functions. Does not throw
   static instance_type priv_get_instance_type(block_header_t *ctrl_data)
   {
      //Get header
      assert((instance_type)ctrl_data->alloc_type() < max_allocation_type);
      return (instance_type)ctrl_data->alloc_type();
   }

   static std::size_t priv_get_reserved_bytes()
   {
      //Get the number of bytes until the end of (*this)
      //beginning in the end of the Base base.
      return sizeof(segment_manager) - sizeof(Base);
   }

   template <class CharT>
   void *priv_generic_find
      (const CharT* name, 
       IndexType<detail::index_config<CharT, MemoryAlgorithm> > &index,
       detail::in_place_interface &table,
       std::size_t &length,
       detail::true_ is_intrusive,
       bool use_lock)
   {
      (void)is_intrusive;
      typedef IndexType<detail::index_config<CharT, MemoryAlgorithm> >         index_type;
      typedef detail::index_key<CharT, void_pointer>  index_key_t;
      typedef typename index_type::iterator           index_it;

      //-------------------------------
      scoped_lock<rmutex> guard(priv_get_lock(use_lock));
      //-------------------------------
      //Find name in index
      detail::intrusive_compare_key<CharT> key
         (name, std::char_traits<CharT>::length(name));
      index_it it = index.find(key);

      //Initialize return values
      void *ret_ptr  = 0;
      length         = 0;

      //If found, assign values
      if(it != index.end()){
         //Get header
         block_header_t *ctrl_data = it->get_block_header();

         //Sanity check
         assert((ctrl_data->m_value_bytes % table.size) == 0);
         assert(ctrl_data->sizeof_char() == sizeof(CharT));
         ret_ptr  = ctrl_data->value();
         length  = ctrl_data->m_value_bytes/table.size;
      }
      return ret_ptr;
   }

   template <class CharT>
   void *priv_generic_find
      (const CharT* name, 
       IndexType<detail::index_config<CharT, MemoryAlgorithm> > &index,
       detail::in_place_interface &table,
       std::size_t &length,
       detail::false_ is_intrusive,
       bool use_lock)
   {
      (void)is_intrusive;
      typedef IndexType<detail::index_config<CharT, MemoryAlgorithm> >      index_type;
      typedef typename index_type::key_type        key_type;
      typedef typename index_type::iterator        index_it;

      //-------------------------------
      scoped_lock<rmutex> guard(priv_get_lock(use_lock));
      //-------------------------------
      //Find name in index
      index_it it = index.find(key_type(name, std::char_traits<CharT>::length(name)));

      //Initialize return values
      void *ret_ptr  = 0;
      length         = 0;

      //If found, assign values
      if(it != index.end()){
         //Get header
         block_header_t *ctrl_data = reinterpret_cast<block_header_t*>
                                    (detail::get_pointer(it->second.m_ptr));

         //Sanity check
         assert((ctrl_data->m_value_bytes % table.size) == 0);
         assert(ctrl_data->sizeof_char() == sizeof(CharT));
         ret_ptr  = ctrl_data->value();
         length  = ctrl_data->m_value_bytes/table.size;
      }
      return ret_ptr;
   }

   template <class CharT>
   bool priv_generic_named_destroy
     (block_header_t *block_header,
      IndexType<detail::index_config<CharT, MemoryAlgorithm> > &index,
      detail::in_place_interface &table,
      detail::true_ is_node_index)
   {
      (void)is_node_index;
      typedef typename IndexType<detail::index_config<CharT, MemoryAlgorithm> >::iterator index_it;

      index_it *ihdr = block_header_t::to_first_header<index_it>(block_header);
      return this->priv_generic_named_destroy_impl<CharT>(*ihdr, index, table);
   }

   template <class CharT>
   bool priv_generic_named_destroy
     (block_header_t *block_header,
      IndexType<detail::index_config<CharT, MemoryAlgorithm> > &index,
      detail::in_place_interface &table,
      detail::false_ is_node_index)
   {
      (void)is_node_index;
      CharT *name = static_cast<CharT*>(block_header->template name<CharT>());
      return this->priv_generic_named_destroy<CharT>(name, index, table, is_intrusive_t());
   }

   template <class CharT>
   bool priv_generic_named_destroy(const CharT *name, 
                                   IndexType<detail::index_config<CharT, MemoryAlgorithm> > &index,
                                   detail::in_place_interface &table,
                                   detail::true_ is_intrusive_index)
   {
      (void)is_intrusive_index;
      typedef IndexType<detail::index_config<CharT, MemoryAlgorithm> >         index_type;
      typedef detail::index_key<CharT, void_pointer>  index_key_t;
      typedef typename index_type::iterator           index_it;
      typedef typename index_type::value_type         intrusive_value_type;
      
      //-------------------------------
      scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      //Find name in index
      detail::intrusive_compare_key<CharT> key
         (name, std::char_traits<CharT>::length(name));
      index_it it = index.find(key);

      //If not found, return false
      if(it == index.end()){
         //This name is not present in the index, wrong pointer or name!
         //assert(0);
         return false;
      }

      block_header_t *ctrl_data = it->get_block_header();
      intrusive_value_type *iv = intrusive_value_type::get_intrusive_value_type(ctrl_data);
      void *memory = iv;
      void *values = ctrl_data->value();
      std::size_t num = ctrl_data->m_value_bytes/table.size;
      
      //Sanity check
      assert((ctrl_data->m_value_bytes % table.size) == 0);
      assert(sizeof(CharT) == ctrl_data->sizeof_char());

      //Erase node from index
      index.erase(it);

      //Destroy the headers
      ctrl_data->~block_header_t();
      iv->~intrusive_value_type();

      //Call destructors and free memory
      std::size_t destroyed;
      table.destroy_n(values, num, destroyed);
      this->deallocate(memory);
      return true;
   }

   template <class CharT>
   bool priv_generic_named_destroy(const CharT *name, 
                                   IndexType<detail::index_config<CharT, MemoryAlgorithm> > &index,
                                   detail::in_place_interface &table,
                                   detail::false_ is_intrusive_index)
   {
      (void)is_intrusive_index;
      typedef IndexType<detail::index_config<CharT, MemoryAlgorithm> >            index_type;
      typedef typename index_type::iterator              index_it;
      typedef typename index_type::key_type              key_type;

      //-------------------------------
      scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      //Try to find the name in the index
      index_it it = index.find(key_type (name, 
                                     std::char_traits<CharT>::length(name)));

      //If not found, return false
      if(it == index.end()){
         //This name is not present in the index, wrong pointer or name!
         //assert(0);
         return false;
      }
      return this->priv_generic_named_destroy_impl<CharT>(it, index, table);
   }

   template <class CharT>
   bool priv_generic_named_destroy_impl
      (const typename IndexType<detail::index_config<CharT, MemoryAlgorithm> >::iterator &it,
      IndexType<detail::index_config<CharT, MemoryAlgorithm> > &index,
      detail::in_place_interface &table)
   {
      typedef IndexType<detail::index_config<CharT, MemoryAlgorithm> >      index_type;
      typedef typename index_type::iterator        index_it;

      //Get allocation parameters
      block_header_t *ctrl_data = reinterpret_cast<block_header_t*>
                                 (detail::get_pointer(it->second.m_ptr));
      char *stored_name       = static_cast<char*>(static_cast<void*>(const_cast<CharT*>(it->first.name())));
      (void)stored_name;

      //Check if the distance between the name pointer and the memory pointer 
      //is correct (this can detect incorrect type in destruction)
      std::size_t num = ctrl_data->m_value_bytes/table.size;
      void *values = ctrl_data->value();

      //Sanity check
      assert((ctrl_data->m_value_bytes % table.size) == 0);
      assert(static_cast<void*>(stored_name) == static_cast<void*>(ctrl_data->template name<CharT>()));
      assert(sizeof(CharT) == ctrl_data->sizeof_char());

      //Erase node from index
      index.erase(it);

      //Destroy the header
      ctrl_data->~block_header_t();

      void *memory;
      if(is_node_index_t::value){
         index_it *ihdr = block_header_t::
            to_first_header<index_it>(ctrl_data);
         ihdr->~index_it();
         memory = ihdr;
      }
      else{
         memory = ctrl_data;
      }

      //Call destructors and free memory
      std::size_t destroyed;
      table.destroy_n(values, num, destroyed);
      this->deallocate(memory);
      return true;
   }

   template<class CharT>
   void * priv_generic_named_construct(std::size_t type,
                               const CharT *name,
                               std::size_t num, 
                               bool try2find, 
                               bool dothrow,
                               detail::in_place_interface &table,
                               IndexType<detail::index_config<CharT, MemoryAlgorithm> > &index,
                               detail::true_ is_intrusive)
   {
      (void)is_intrusive;
      std::size_t namelen  = std::char_traits<CharT>::length(name);

      block_header_t block_info ( table.size*num
                                 , table.alignment
                                 , type
                                 , sizeof(CharT)
                                 , namelen);

      typedef IndexType<detail::index_config<CharT, MemoryAlgorithm> >            index_type;
      typedef typename index_type::iterator              index_it;
      typedef std::pair<index_it, bool>                  index_ib;

      //-------------------------------
      scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      //Insert the node. This can throw.
      //First, we want to know if the key is already present before
      //we allocate any memory, and if the key is not present, we 
      //want to allocate all memory in a single buffer that will
      //contain the name and the user buffer.
      //
      //Since equal_range(key) + insert(hint, value) approach is
      //quite inefficient in container implementations 
      //(they re-test if the position is correct), I've chosen
      //to insert the node, do an ugly un-const cast and modify
      //the key (which is a smart pointer) to an equivalent one
      index_ib insert_ret;

      typename index_type::insert_commit_data   commit_data;
      typedef typename index_type::value_type   intrusive_value_type;

      BOOST_TRY{
         detail::intrusive_compare_key<CharT> key(name, namelen);
         insert_ret = index.insert_check(key, commit_data);
      }
      //Ignore exceptions
      BOOST_CATCH(...){
         if(dothrow)
            BOOST_RETHROW
         return 0;
      }
      BOOST_CATCH_END

      index_it it = insert_ret.first;

      //If found and this is find or construct, return data
      //else return null
      if(!insert_ret.second){
         if(try2find){
            return it->get_block_header()->value();
         }
         if(dothrow){
            throw interprocess_exception(already_exists_error);
         }
         else{
            return 0;
         }
      }

      //Allocates buffer for name + data, this can throw (it hurts)
      void *buffer_ptr; 

      //Check if there is enough memory
      if(dothrow){
         buffer_ptr = this->allocate
            (block_info.total_size_with_header<intrusive_value_type>());
      }
      else{
         buffer_ptr = this->allocate
            (block_info.total_size_with_header<intrusive_value_type>(), std::nothrow_t());
         if(!buffer_ptr)
            return 0; 
      }

      //Now construct the intrusive hook plus the header
      intrusive_value_type * intrusive_hdr = new(buffer_ptr) intrusive_value_type();
      block_header_t * hdr = new(intrusive_hdr->get_block_header())block_header_t(block_info);
      void *ptr = 0; //avoid gcc warning
      ptr = hdr->value();

      //Copy name to memory segment and insert data
      CharT *name_ptr = static_cast<CharT *>(hdr->template name<CharT>());
      std::char_traits<CharT>::copy(name_ptr, name, namelen+1);

      BOOST_TRY{
         //Now commit the insertion using previous context data
         it = index.insert_commit(*intrusive_hdr, commit_data);
      }
      //Ignore exceptions
      BOOST_CATCH(...){
         if(dothrow)
            BOOST_RETHROW
         return 0;
      }
      BOOST_CATCH_END

      //Avoid constructions if constructor is trivial
      //Build scoped ptr to avoid leaks with constructor exception
      detail::mem_algo_deallocator<segment_manager_base_type> mem
         (buffer_ptr, *static_cast<segment_manager_base_type*>(this));

      //Initialize the node value_eraser to erase inserted node
      //if something goes wrong. This will be executed *before*
      //the memory allocation as the intrusive value is built in that
      //memory
      value_eraser<index_type> v_eraser(index, it);
      
      //Construct array, this can throw
      detail::array_construct(ptr, num, table);

      //Release rollbacks since construction was successful
      v_eraser.release();
      mem.release();
      return ptr;
   }

   //!Generic named new function for
   //!named functions
   template<class CharT>
   void * priv_generic_named_construct(std::size_t type,  
                               const CharT *name,
                               std::size_t num, 
                               bool try2find, 
                               bool dothrow,
                               detail::in_place_interface &table,
                               IndexType<detail::index_config<CharT, MemoryAlgorithm> > &index,
                               detail::false_ is_intrusive)
   {
      (void)is_intrusive;
      std::size_t namelen  = std::char_traits<CharT>::length(name);

      block_header_t block_info ( table.size*num
                                 , table.alignment
                                 , type
                                 , sizeof(CharT)
                                 , namelen);

      typedef IndexType<detail::index_config<CharT, MemoryAlgorithm> >            index_type;
      typedef typename index_type::key_type              key_type;
      typedef typename index_type::mapped_type           mapped_type;
      typedef typename index_type::value_type            value_type;
      typedef typename index_type::iterator              index_it;
      typedef std::pair<index_it, bool>                  index_ib;

      //-------------------------------
      scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      //Insert the node. This can throw.
      //First, we want to know if the key is already present before
      //we allocate any memory, and if the key is not present, we 
      //want to allocate all memory in a single buffer that will
      //contain the name and the user buffer.
      //
      //Since equal_range(key) + insert(hint, value) approach is
      //quite inefficient in container implementations 
      //(they re-test if the position is correct), I've chosen
      //to insert the node, do an ugly un-const cast and modify
      //the key (which is a smart pointer) to an equivalent one
      index_ib insert_ret;
      BOOST_TRY{
         insert_ret = index.insert(value_type(key_type (name, namelen), mapped_type(0)));
      }
      //Ignore exceptions
      BOOST_CATCH(...){
         if(dothrow)
            BOOST_RETHROW;
         return 0;
      }
      BOOST_CATCH_END

      index_it it = insert_ret.first;

      //If found and this is find or construct, return data
      //else return null
      if(!insert_ret.second){
         if(try2find){
            block_header_t *hdr = static_cast<block_header_t*>
               (detail::get_pointer(it->second.m_ptr));
            return hdr->value();
         }
         return 0;
      }
      //Initialize the node value_eraser to erase inserted node
      //if something goes wrong
      value_eraser<index_type> v_eraser(index, it);

      //Allocates buffer for name + data, this can throw (it hurts)
      void *buffer_ptr; 
      block_header_t * hdr;

      //Allocate and construct the headers
      if(is_node_index_t::value){
         std::size_t total_size = block_info.total_size_with_header<index_it>();
         if(dothrow){
            buffer_ptr = this->allocate(total_size);
         }
         else{
            buffer_ptr = this->allocate(total_size, std::nothrow_t());
            if(!buffer_ptr)
               return 0; 
         }
         index_it *idr = new(buffer_ptr) index_it(it);
         hdr = block_header_t::from_first_header<index_it>(idr);
      }
      else{
         if(dothrow){
            buffer_ptr = this->allocate(block_info.total_size());
         }
         else{
            buffer_ptr = this->allocate(block_info.total_size(), std::nothrow_t());
            if(!buffer_ptr)
               return 0; 
         }
         hdr = static_cast<block_header_t*>(buffer_ptr);
      }

      hdr = new(hdr)block_header_t(block_info);
      void *ptr = 0; //avoid gcc warning
      ptr = hdr->value();

      //Copy name to memory segment and insert data
      CharT *name_ptr = static_cast<CharT *>(hdr->template name<CharT>());
      std::char_traits<CharT>::copy(name_ptr, name, namelen+1);

      //Do the ugly cast, please mama, forgive me!
      //This new key points to an identical string, so it must have the 
      //same position than the overwritten key according to the predicate
      const_cast<key_type &>(it->first).name(name_ptr);
      it->second.m_ptr  = hdr;

      //Build scoped ptr to avoid leaks with constructor exception
      detail::mem_algo_deallocator<segment_manager_base_type> mem
         (buffer_ptr, *static_cast<segment_manager_base_type*>(this));

      //Construct array, this can throw
      detail::array_construct(ptr, num, table);

      //All constructors successful, we don't want to release memory
      mem.release();

      //Release node v_eraser since construction was successful
      v_eraser.release();
      return ptr;
   }

   private:
   //!Returns the this pointer
   segment_manager *get_this_pointer()
   {  return this;  }

   typedef typename MemoryAlgorithm::mutex_family::recursive_mutex_type   rmutex;

   scoped_lock<rmutex> priv_get_lock(bool use_lock)
   {
      scoped_lock<rmutex> local(m_header, defer_lock);
      if(use_lock){
         local.lock();
      }
      return scoped_lock<rmutex>(boost::interprocess::move(local));
   }

   //!This struct includes needed data and derives from
   //!rmutex to allow EBO when using null interprocess_mutex
   struct header_t
      :  public rmutex
   {
      named_index_t           m_named_index;
      unique_index_t          m_unique_index;
   
      header_t(Base *restricted_segment_mngr)
         :  m_named_index (restricted_segment_mngr)
         ,  m_unique_index(restricted_segment_mngr)
      {}
   }  m_header;

   /// @endcond
};


}} //namespace boost { namespace interprocess

#include <boost/interprocess/detail/config_end.hpp>

#endif //#ifndef BOOST_INTERPROCESS_SEGMENT_MANAGER_HPP

