//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_MANAGED_MULTI_SHARED_MEMORY_HPP
#define BOOST_INTERPROCESS_MANAGED_MULTI_SHARED_MEMORY_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/detail/managed_memory_impl.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/detail/no_exceptions_support.hpp>
#include <boost/interprocess/detail/multi_segment_services.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/containers/list.hpp>//list
#include <boost/interprocess/mapped_region.hpp> //mapped_region
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/detail/managed_open_or_create_impl.hpp> //managed_open_or_create_impl
#include <new>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/streams/vectorstream.hpp>
#include <memory>

//!\file
//!Describes a named shared memory object allocation user class.

namespace boost {

namespace interprocess {

//!A basic shared memory named object creation class. Initializes the 
//!shared memory segment. Inherits all basic functionality from 
//!basic_managed_memory_impl<CharType, MemoryAlgorithm, IndexType>
template
      <
         class CharType, 
         class MemoryAlgorithm, 
         template<class IndexConfig> class IndexType
      >
class basic_managed_multi_shared_memory 
   :  public detail::basic_managed_memory_impl
         <CharType, MemoryAlgorithm, IndexType>
{

   typedef basic_managed_multi_shared_memory
               <CharType, MemoryAlgorithm, IndexType>    self_t;
   typedef typename MemoryAlgorithm::void_pointer        void_pointer;
   typedef typename detail::
      managed_open_or_create_impl<shared_memory_object>  managed_impl;
   typedef typename void_pointer::segment_group_id       segment_group_id;
   ////////////////////////////////////////////////////////////////////////
   //
   //               Some internal helper structs/functors
   //
   ////////////////////////////////////////////////////////////////////////
   //!This class defines an operator() that creates a shared memory
   //!of the requested size. The rest of the parameters are
   //!passed in the constructor. The class a template parameter
   //!to be used with create_from_file/create_from_istream functions
   //!of basic_named_object classes

//   class segment_creator
//   {
//      public:
//      segment_creator(shared_memory &shmem,
//                      const char *mem_name, 
//                      const void *addr)
//      : m_shmem(shmem), m_mem_name(mem_name), m_addr(addr){}
//
//      void *operator()(std::size_t size)
//      {
//         if(!m_shmem.create(m_mem_name, size, m_addr))
//            return 0;
//         return m_shmem.get_address(); 
//      }      
//      private:
//      shared_memory &m_shmem;
//      const char *m_mem_name;
//      const void *m_addr;
//   };

   class group_services
      :  public multi_segment_services
   {
      public:
      typedef std::pair<void *, std::size_t>                result_type;
      typedef basic_managed_multi_shared_memory             frontend_t;
      typedef typename 
         basic_managed_multi_shared_memory::void_pointer    void_pointer;
      typedef typename void_pointer::segment_group_id       segment_group_id;
      group_services(frontend_t *const frontend)
         :  mp_frontend(frontend), m_group(0), m_min_segment_size(0){}

      virtual std::pair<void *, std::size_t> create_new_segment(std::size_t alloc_size)
      {  
         //We should allocate an extra byte so that the
         //[base_addr + alloc_size] byte belongs to this segment
         alloc_size += 1;

         //If requested size is less than minimum, update that
         alloc_size = (m_min_segment_size > alloc_size) ? 
                       m_min_segment_size : alloc_size;
         if(mp_frontend->priv_new_segment(create_open_func::DoCreate,
                                          alloc_size, 0)){
            shmem_list_t::value_type &m_impl = *mp_frontend->m_shmem_list.rbegin();
            return result_type(m_impl.get_real_address(), m_impl.get_real_size()-1);
         }
         return result_type(static_cast<void *>(0), 0);  
      }

      virtual bool update_segments ()
      {  return true;   }

      virtual ~group_services(){}

      void set_group(segment_group_id group)
         {  m_group = group;  }

      segment_group_id get_group() const
         {  return m_group;  }

      void set_min_segment_size(std::size_t min_segment_size)
         {  m_min_segment_size = min_segment_size;  }

      std::size_t get_min_segment_size() const
         {  return m_min_segment_size;  }

      private:

      frontend_t * const   mp_frontend;
      segment_group_id     m_group;
      std::size_t          m_min_segment_size;
   };

   //!Functor to execute atomically when opening or creating a shared memory
   //!segment.
   struct create_open_func
   {
      enum type_t {  DoCreate, DoOpen, DoOpenOrCreate  };
      typedef typename 
         basic_managed_multi_shared_memory::void_pointer   void_pointer;

      create_open_func(self_t * const    frontend,
                       type_t type, std::size_t segment_number)
         : mp_frontend(frontend), m_type(type), m_segment_number(segment_number){}

      bool operator()(void *addr, std::size_t size, bool created) const
      {  
         if(((m_type == DoOpen)   &&  created) || 
            ((m_type == DoCreate) && !created))
            return false;
         segment_group_id group = mp_frontend->m_group_services.get_group();
         bool mapped       = false;
         bool impl_done    = false;

         //Associate this newly created segment as the
         //segment id = 0 of this group
         void_pointer::insert_mapping
            ( group
            , static_cast<char*>(addr) - managed_impl::ManagedOpenOrCreateUserOffset
            , size + managed_impl::ManagedOpenOrCreateUserOffset);
         //Check if this is the master segment
         if(!m_segment_number){
            //Create or open the Interprocess machinery
            if((impl_done = created ? 
               mp_frontend->create_impl(addr, size) : mp_frontend->open_impl(addr, size))){
               return true;
            }
         }
         else{
            return true;
         }

         //This is the cleanup part
         //---------------
         if(impl_done){
            mp_frontend->close_impl();
         }
         if(mapped){
            bool ret = void_pointer::erase_last_mapping(group);
            assert(ret);(void)ret;
         }
         return false;
      }
      self_t * const    mp_frontend;
      type_t            m_type;
      std::size_t       m_segment_number;
   };

   //!Functor to execute atomically when closing a shared memory segment.
   struct close_func
   {
      typedef typename 
         basic_managed_multi_shared_memory::void_pointer   void_pointer;

      close_func(self_t * const frontend)
         : mp_frontend(frontend){}

      void operator()(const mapped_region &region, bool last) const
      {  
         if(last) mp_frontend->destroy_impl();
         else     mp_frontend->close_impl();
      }
      self_t * const    mp_frontend;
   };

   typedef detail::basic_managed_memory_impl 
      <CharType, MemoryAlgorithm, IndexType>                   base_t;

   //Friend declarations
   friend struct basic_managed_multi_shared_memory::create_open_func;
   friend struct basic_managed_multi_shared_memory::close_func;
   friend class basic_managed_multi_shared_memory::group_services;

   typedef list<managed_impl> shmem_list_t;

   basic_managed_multi_shared_memory *get_this_pointer()
      {  return this;   }

 public:

   basic_managed_multi_shared_memory(create_only_t,
                                     const char *name,
                                     std::size_t size)
      :  m_group_services(get_this_pointer())
   {
      priv_open_or_create(create_open_func::DoCreate,name, size);  
   }

   basic_managed_multi_shared_memory(open_or_create_t,
                                     const char *name,
                                     std::size_t size)
      :  m_group_services(get_this_pointer())
   {
      priv_open_or_create(create_open_func::DoOpenOrCreate, name, size);
   }

   basic_managed_multi_shared_memory(open_only_t, const char *name)
      :  m_group_services(get_this_pointer())
   {
      priv_open_or_create(create_open_func::DoOpen, name, 0);
   }

   ~basic_managed_multi_shared_memory()
      {  this->priv_close(); }

   private:
   bool  priv_open_or_create(typename create_open_func::type_t type, 
                             const char *name,
                             std::size_t size)
   {
      if(!m_shmem_list.empty())
         return false;
      typename void_pointer::segment_group_id group = 0;
      BOOST_TRY{
         m_root_name = name;
         //Insert multi segment services and get a group identifier
         group = void_pointer::new_segment_group(&m_group_services);
         size = void_pointer::round_size(size);
         m_group_services.set_group(group);
         m_group_services.set_min_segment_size(size);

         if(group){
            if(this->priv_new_segment(type, size, 0)){
               return true;
            }
         }
      }
      BOOST_CATCH(const std::bad_alloc&){
      }
      BOOST_CATCH_END
      if(group){
         void_pointer::delete_group(group);
      }
      return false;      
   }

   bool  priv_new_segment(typename create_open_func::type_t type,
                          std::size_t size,
                          const void *addr)
   {
      BOOST_TRY{
         //Get the number of groups of this multi_segment group
         std::size_t segment_id  = m_shmem_list.size();
         //Format the name of the shared memory: append segment number. 
         boost::interprocess::basic_ovectorstream<boost::interprocess::string> formatter;
         //Pre-reserve string size
         std::size_t str_size = m_root_name.length()+10;
         if(formatter.vector().size() < str_size){
            //This can throw.
            formatter.reserve(str_size);
         }
         //Format segment's name
         formatter << m_root_name
                   << static_cast<unsigned int>(segment_id) << std::ends;
         //This functor will be executed when constructing
         create_open_func func(this, type, segment_id);
         const char *name = formatter.vector().c_str();
         //This can throw.
         managed_impl mshm;

         switch(type){
            case create_open_func::DoCreate:
            {
               managed_impl shm(create_only, name, size, read_write, addr, func);
               mshm = boost::interprocess::move(shm);
            }
            break;

            case create_open_func::DoOpen:
            {
               managed_impl shm(open_only, name,read_write, addr, func);
               mshm = boost::interprocess::move(shm);
            }
            break;

            case create_open_func::DoOpenOrCreate:
            {
               managed_impl shm(open_or_create, name, size, read_write, addr, func);
               mshm = boost::interprocess::move(shm);
            }
            break;

            default:
               return false;
            break;
         }

         //This can throw.
         m_shmem_list.push_back(boost::interprocess::move(mshm));
         return true;
      }
      BOOST_CATCH(const std::bad_alloc&){
      }
      BOOST_CATCH_END
      return false;
   }

   //!Frees resources. Never throws.
   void priv_close()
   {  
      if(!m_shmem_list.empty()){
         bool ret;
         //Obtain group identifier
         segment_group_id group = m_group_services.get_group();
         //Erase main segment and its resources
         shmem_list_t::iterator  itbeg = m_shmem_list.begin(),
                                 itend = m_shmem_list.end(),
                                 it    = itbeg;
         //(*itbeg)->close_with_func(close_func(this));
         //Delete group. All mappings are erased too.
         ret = void_pointer::delete_group(group);
         assert(ret);
         m_shmem_list.clear();
      }
   }
   
   private:
   shmem_list_t   m_shmem_list;
   group_services m_group_services;
   std::string    m_root_name;
};

typedef basic_managed_multi_shared_memory
   < char
   , rbtree_best_fit<mutex_family, intersegment_ptr<void> >
   , iset_index>
   managed_multi_shared_memory;

}  //namespace interprocess {

}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_MANAGED_MULTI_SHARED_MEMORY_HPP

