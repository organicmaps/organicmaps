//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_XSI_SHARED_MEMORY_HPP
#define BOOST_INTERPROCESS_XSI_SHARED_MEMORY_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/detail/workaround.hpp>

#if defined(BOOST_INTERPROCESS_WINDOWS)
#error "This header can't be used in Windows operating systems"
#endif

#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <sys/shm.h>
#include <cstddef>
#include <boost/cstdint.hpp>
#include <string>

//!\file
//!Describes a class representing a native xsi shared memory.

namespace boost {
namespace interprocess {

//!A class that wraps XSI (System V) shared memory.
//!Unlike shared_memory_object, xsi_shared_memory needs a valid
//!path and a 8 bit key to identify a shared memory create
//!when all processes destroy all their xsi_shared_memory
//!objects and mapped regions for the same shared memory
//!or the processes end/crash.
//!
//!Warning: XSI shared memory and interprocess portable
//!shared memory (boost::interprocess::shared_memory_object)
//!can't communicate between them.
class xsi_shared_memory
{
   /// @cond
   //Non-copyable and non-assignable
   xsi_shared_memory(xsi_shared_memory &);
   xsi_shared_memory &operator=(xsi_shared_memory &);
   /// @endcond

   public:
   BOOST_INTERPROCESS_ENABLE_MOVE_EMULATION(xsi_shared_memory)

   //!Default constructor.
   //!Represents an empty xsi_shared_memory.
   xsi_shared_memory();

   //!Creates a new XSI  shared memory with a key obtained from a call to ftok (with path
   //!"path" and id "id"), of size "size" and permissions "perm".
   //!If the shared memory previously exists, throws an error.
   xsi_shared_memory(create_only_t, const char *path, boost::uint8_t id, std::size_t size, int perm = 0666)
   {  this->priv_open_or_create(detail::DoCreate, path, id, perm, size);  }

   //!Tries to create a new XSI  shared memory with a key obtained from a call to ftok (with path
   //!"path" and id "id"), of size "size" and permissions "perm".
   //!If the shared memory previously exists, it tries to open it.
   //!Otherwise throws an error.
   xsi_shared_memory(open_or_create_t, const char *path, boost::uint8_t id, std::size_t size, int perm = 0666)
   {  this->priv_open_or_create(detail::DoOpenOrCreate, path, id, perm, size);  }

   //!Tries to open a XSI shared memory with a key obtained from a call to ftok (with path
   //!"path" and id "id") and permissions "perm".
   //!If the shared memory does not previously exist, it throws an error.
   xsi_shared_memory(open_only_t, const char *path, boost::uint8_t id, int perm = 0666)
   {  this->priv_open_or_create(detail::DoOpen, path, id, perm, 0);  }

   //!Moves the ownership of "moved"'s shared memory object to *this. 
   //!After the call, "moved" does not represent any shared memory object. 
   //!Does not throw
   xsi_shared_memory(BOOST_INTERPROCESS_RV_REF(xsi_shared_memory) moved)
   {  this->swap(moved);   }

   //!Moves the ownership of "moved"'s shared memory to *this.
   //!After the call, "moved" does not represent any shared memory. 
   //!Does not throw
   xsi_shared_memory &operator=(BOOST_INTERPROCESS_RV_REF(xsi_shared_memory) moved)
   {  
      xsi_shared_memory tmp(boost::interprocess::move(moved));
      this->swap(tmp);
      return *this;  
   }

   //!Swaps two xsi_shared_memorys. Does not throw
   void swap(xsi_shared_memory &other);

   //!Destroys *this. The shared memory won't be destroyed, just
   //!this connection to it. Use remove() to destroy the shared memory.
   ~xsi_shared_memory();

   //!Returns the path used to
   //!obtain the key.
   const char *get_path() const;

   //!Returns the shared memory ID that
   //!identifies the shared memory
   int get_shmid() const;

   //!Returns access
   //!permissions
   int get_permissions() const;

   //!Returns the mapping handle.
   //!Never throws
   mapping_handle_t get_mapping_handle() const;

   //!Erases the XSI shared memory object identified by shmid
   //!from the system.
   //!Returns false on error. Never throws
   static bool remove(int shmid);

   /// @cond
   private:

   //!Closes a previously opened file mapping. Never throws.
   bool priv_open_or_create( detail::create_enum_t type
                           , const char *filename
                           , boost::uint8_t id
                           , int perm
                           , std::size_t size);
   int            m_shmid;
   /// @endcond
};

/// @cond

inline xsi_shared_memory::xsi_shared_memory() 
   :  m_shmid(-1)
{}

inline xsi_shared_memory::~xsi_shared_memory() 
{}

inline int xsi_shared_memory::get_shmid() const
{  return m_shmid; }

inline void xsi_shared_memory::swap(xsi_shared_memory &other)
{
   std::swap(m_shmid, other.m_shmid);
}

inline mapping_handle_t xsi_shared_memory::get_mapping_handle() const
{  mapping_handle_t mhnd = { m_shmid, true};   return mhnd;   }

inline bool xsi_shared_memory::priv_open_or_create
   (detail::create_enum_t type, const char *filename, boost::uint8_t id, int perm, std::size_t size)
{
   key_t key;   
   if(filename){
      key  = ::ftok(filename, id);
      if(((key_t)-1) == key){
         error_info err = system_error_code();
         throw interprocess_exception(err);
      }
   }
   else{
      key = IPC_PRIVATE;
   }

   perm &= 0x01FF;
   int shmflg = perm;

   switch(type){
      case detail::DoOpen:
         shmflg |= 0;
      break;
      case detail::DoCreate:
         shmflg |= IPC_CREAT | IPC_EXCL;
      break;
      case detail::DoOpenOrCreate:
         shmflg |= IPC_CREAT;
      break;
      default:
         {
            error_info err = other_error;
            throw interprocess_exception(err);
         }
   }

   int ret = ::shmget(key, size, shmflg);
   int shmid = ret;
   if((type == detail::DoOpen) && (-1 != ret)){
      //Now get the size
      ::shmid_ds xsi_ds;
      ret = ::shmctl(ret, IPC_STAT, &xsi_ds);
      size = xsi_ds.shm_segsz;
   }
   if(-1 == ret){
      error_info err = system_error_code();
      throw interprocess_exception(err);
   }

   m_shmid = shmid;
   return true;
}

inline bool xsi_shared_memory::remove(int shmid)
{  return -1 != ::shmctl(shmid, IPC_RMID, 0); }

///@endcond

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_XSI_SHARED_MEMORY_HPP
