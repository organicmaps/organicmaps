//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2011. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_MAPPED_REGION_HPP
#define BOOST_INTERPROCESS_MAPPED_REGION_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/move/move.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <string>
#include <boost/cstdint.hpp>

#if defined (BOOST_INTERPROCESS_WINDOWS)
#  include <boost/interprocess/detail/win32_api.hpp>
#  include <boost/interprocess/sync/windows/sync_utils.hpp>
#else
#  ifdef BOOST_HAS_UNISTD_H
#    include <fcntl.h>
#    include <sys/mman.h>     //mmap
#    include <unistd.h>
#    include <sys/stat.h>
#    include <sys/types.h>
#    if defined(BOOST_INTERPROCESS_XSI_SHARED_MEMORY_OBJECTS)
#      include <sys/shm.h>      //System V shared memory...
#    endif
#    include <boost/assert.hpp>
#  else
#    error Unknown platform
#  endif

#endif   //#if (defined BOOST_INTERPROCESS_WINDOWS)

//!\file
//!Describes mapped region class

namespace boost {
namespace interprocess {

/// @cond
namespace ipcdetail{ class interprocess_tester; }
namespace ipcdetail{ class raw_mapped_region_creator; }

/// @endcond

//!The mapped_region class represents a portion or region created from a
//!memory_mappable object.
//!
//!The OS can map a region bigger than the requested one, as region must
//!be multiple of the page size, but mapped_region will always refer to
//!the region specified by the user.
class mapped_region
{
   /// @cond
   //Non-copyable
   BOOST_MOVABLE_BUT_NOT_COPYABLE(mapped_region)
   /// @endcond

   public:

   //!Creates a mapping region of the mapped memory "mapping", starting in
   //!offset "offset", and the mapping's size will be "size". The mapping
   //!can be opened for read only, read-write or copy-on-write.
   //!
   //!If an address is specified, both the offset and the address must be
   //!multiples of the page size.
   //!
   //!The OS could allocate more pages than size/page_size(), but get_address()
   //!will always return the address passed in this function (if not null) and
   //!get_size() will return the specified size.
   template<class MemoryMappable>
   mapped_region(const MemoryMappable& mapping
                ,mode_t mode
                ,offset_t offset = 0
                ,std::size_t size = 0
                ,const void *address = 0);

   //!Default constructor. Address will be 0 (nullptr).
   //!Size will be 0.
   //!Does not throw
   mapped_region();

   //!Move constructor. *this will be constructed taking ownership of "other"'s
   //!region and "other" will be left in default constructor state.
   mapped_region(BOOST_RV_REF(mapped_region) other)
   #if defined (BOOST_INTERPROCESS_WINDOWS)
   :  m_base(0), m_size(0)
   ,  m_page_offset(0)
   ,  m_mode(read_only)
   ,  m_file_or_mapping_hnd(ipcdetail::invalid_file())
   #else
   :  m_base(0), m_size(0), m_page_offset(0), m_mode(read_only), m_is_xsi(false)
   #endif
   {  this->swap(other);   }

   //!Destroys the mapped region.
   //!Does not throw
   ~mapped_region();

   //!Move assignment. If *this owns a memory mapped region, it will be
   //!destroyed and it will take ownership of "other"'s memory mapped region.
   mapped_region &operator=(BOOST_RV_REF(mapped_region) other)
   {
      mapped_region tmp(boost::move(other));
      this->swap(tmp);
      return *this;
   }

   //!Returns the size of the mapping. Never throws.
   std::size_t get_size() const;

   //!Returns the base address of the mapping.
   //!Never throws.
   void*       get_address() const;

   //!Returns the mode of the mapping used to construct the mapped region.
   //!Never throws.
   mode_t get_mode() const;

   //!Flushes to the disk a byte range within the mapped memory.
   //!If 'async' is true, the function will return before flushing operation is completed
   //!If 'async' is false, function will return once data has been written into the underlying
   //!device (i.e., in mapped files OS cached information is written to disk).
   //!Never throws. Returns false if operation could not be performed.
   bool flush(std::size_t mapping_offset = 0, std::size_t numbytes = 0, bool async = true);

   //!Swaps the mapped_region with another
   //!mapped region
   void swap(mapped_region &other);

   //!Returns the size of the page. This size is the minimum memory that
   //!will be used by the system when mapping a memory mappable source and
   //!will restrict the address and the offset to map.
   static std::size_t get_page_size();

   /// @cond
   private:
   //!Closes a previously opened memory mapping. Never throws
   void priv_close();

   void* priv_map_address()  const;
   std::size_t priv_map_size()  const;
   bool priv_flush_param_check(std::size_t mapping_offset, void *&addr, std::size_t &numbytes) const;
   static void priv_size_from_mapping_size
      (offset_t mapping_size, offset_t offset, offset_t page_offset, std::size_t &size);
   static offset_t priv_page_offset_addr_fixup(offset_t page_offset, const void *&addr);

   template<int dummy>
   struct page_size_holder
   {
      static const std::size_t PageSize;
      static std::size_t get_page_size();
   };

   void*             m_base;
   std::size_t       m_size;
   std::size_t       m_page_offset;
   mode_t            m_mode;
   #if defined(BOOST_INTERPROCESS_WINDOWS)
   file_handle_t     m_file_or_mapping_hnd;
   #else
   bool              m_is_xsi;
   #endif

   friend class ipcdetail::interprocess_tester;
   friend class ipcdetail::raw_mapped_region_creator;
   void dont_close_on_destruction();
   #if defined(BOOST_INTERPROCESS_WINDOWS) && !defined(BOOST_INTERPROCESS_FORCE_GENERIC_EMULATION)
   template<int Dummy>
   static void destroy_syncs_in_range(const void *addr, std::size_t size);
   #endif
   /// @endcond
};

///@cond

inline void swap(mapped_region &x, mapped_region &y)
{  x.swap(y);  }

inline mapped_region::~mapped_region()
{  this->priv_close(); }

inline std::size_t mapped_region::get_size()  const
{  return m_size; }

inline mode_t mapped_region::get_mode()  const
{  return m_mode;   }

inline void*    mapped_region::get_address()  const
{  return m_base; }

inline void*    mapped_region::priv_map_address()  const
{  return static_cast<char*>(m_base) - m_page_offset; }

inline std::size_t mapped_region::priv_map_size()  const
{  return m_size + m_page_offset; }

inline bool mapped_region::priv_flush_param_check
   (std::size_t mapping_offset, void *&addr, std::size_t &numbytes) const
{
   //Check some errors
   if(m_base == 0)
      return false;

   if(mapping_offset >= m_size || (mapping_offset + numbytes) > m_size){
      return false;
   }

   //Update flush size if the user does not provide it
   if(numbytes == 0){
      numbytes = m_size - mapping_offset;
   }
   addr = (char*)this->priv_map_address() + mapping_offset;
   numbytes += m_page_offset;
   return true;
}

inline void mapped_region::priv_size_from_mapping_size
   (offset_t mapping_size, offset_t offset, offset_t page_offset, std::size_t &size)
{
   //Check if mapping size fits in the user address space
   //as offset_t is the maximum file size and its signed.
   if(mapping_size < offset ||
      boost::uintmax_t(mapping_size - (offset - page_offset)) >
         boost::uintmax_t(std::size_t(-1))){
      error_info err(size_error);
      throw interprocess_exception(err);
   }
   size = static_cast<std::size_t>(mapping_size - (offset - page_offset));
}

inline offset_t mapped_region::priv_page_offset_addr_fixup(offset_t offset, const void *&address)
{
   //We can't map any offset so we have to obtain system's
   //memory granularity
   const std::size_t page_size  = mapped_region::get_page_size();

   //We calculate the difference between demanded and valid offset
   //(always less than a page in std::size_t, thus, representable by std::size_t)
   const std::size_t page_offset =
      static_cast<std::size_t>(offset - (offset / page_size) * page_size);
   //Update the mapping address
   if(address){
      address = static_cast<const char*>(address) - page_offset;
   }
   return page_offset;
}

#if defined (BOOST_INTERPROCESS_WINDOWS)

inline mapped_region::mapped_region()
   :  m_base(0), m_size(0), m_page_offset(0), m_mode(read_only)
   ,  m_file_or_mapping_hnd(ipcdetail::invalid_file())
{}

template<int dummy>
inline std::size_t mapped_region::page_size_holder<dummy>::get_page_size()
{
   winapi::system_info info;
   get_system_info(&info);
   return std::size_t(info.dwAllocationGranularity);
}

template<class MemoryMappable>
inline mapped_region::mapped_region
   (const MemoryMappable &mapping
   ,mode_t mode
   ,offset_t offset
   ,std::size_t size
   ,const void *address)
   :  m_base(0), m_size(0), m_page_offset(0), m_mode(mode)
   ,  m_file_or_mapping_hnd(ipcdetail::invalid_file())
{
   mapping_handle_t mhandle = mapping.get_mapping_handle();
   {
      file_handle_t native_mapping_handle = 0;

      //Set accesses
      //For "create_file_mapping"
      unsigned long protection = 0;
      //For "mapviewoffile"
      unsigned long map_access = 0;

      switch(mode)
      {
         case read_only:
         case read_private:
            protection   |= winapi::page_readonly;
            map_access   |= winapi::file_map_read;
         break;
         case read_write:
            protection   |= winapi::page_readwrite;
            map_access   |= winapi::file_map_write;
         break;
         case copy_on_write:
            protection   |= winapi::page_writecopy;
            map_access   |= winapi::file_map_copy;
         break;
         default:
            {
               error_info err(mode_error);
               throw interprocess_exception(err);
            }
         break;
      }

      //For file mapping (including emulated shared memory through temporary files),
      //the device is a file handle so we need to obtain file's size and call create_file_mapping
      //to obtain the mapping handle.
      //For files we don't need the file mapping after mapping the memory, as the file is there
      //so we'll program the handle close
      void * handle_to_close = winapi::invalid_handle_value;
      if(!mhandle.is_shm){
         //Create mapping handle
         native_mapping_handle = winapi::create_file_mapping
            ( ipcdetail::file_handle_from_mapping_handle(mapping.get_mapping_handle())
            , protection, 0, 0, 0);

         //Check if all is correct
         if(!native_mapping_handle){
            error_info err = winapi::get_last_error();
            throw interprocess_exception(err);
         }
         handle_to_close = native_mapping_handle;
      }
      else{
         //For windows_shared_memory the device handle is already a mapping handle
         //and we need to maintain it
         native_mapping_handle = mhandle.handle;
      }
      //RAII handle close on scope exit
      const winapi::handle_closer close_handle(handle_to_close);
      (void)close_handle;

      const offset_t page_offset = priv_page_offset_addr_fixup(offset, address);

      //Obtain mapping size if user provides 0 size
      if(size == 0){
         offset_t mapping_size;
         if(!winapi::get_file_mapping_size(native_mapping_handle, mapping_size)){
            error_info err = winapi::get_last_error();
            throw interprocess_exception(err);
         }
         //This can throw
         priv_size_from_mapping_size(mapping_size, offset, page_offset, size);
      }


      //Map with new offsets and size
      void *base = winapi::map_view_of_file_ex
                                 (native_mapping_handle,
                                 map_access,
                                 offset - page_offset,
                                 static_cast<std::size_t>(page_offset + size),
                                 const_cast<void*>(address));
      //Check error
      if(!base){
         error_info err = winapi::get_last_error();
         throw interprocess_exception(err);
      }

      //Calculate new base for the user
      m_base = static_cast<char*>(base) + page_offset;
      m_page_offset = page_offset;
      m_size = size;
   }
   //Windows shared memory needs the duplication of the handle if we want to
   //make mapped_region independent from the mappable device
   //
   //For mapped files, we duplicate the file handle to be able to FlushFileBuffers
   if(!winapi::duplicate_current_process_handle(mhandle.handle, &m_file_or_mapping_hnd)){
      error_info err = winapi::get_last_error();
      this->priv_close();
      throw interprocess_exception(err);
   }
}

inline bool mapped_region::flush(std::size_t mapping_offset, std::size_t numbytes, bool async)
{
   void *addr;
   if(!this->priv_flush_param_check(mapping_offset, addr, numbytes)){
      return false;
   }
   //Flush it all
   if(!winapi::flush_view_of_file(addr, numbytes)){
      return false;
   }
   //m_file_or_mapping_hnd can be a file handle or a mapping handle.
   //so flushing file buffers has only sense for files...
   else if(async && m_file_or_mapping_hnd != winapi::invalid_handle_value &&
           winapi::get_file_type(m_file_or_mapping_hnd) == winapi::file_type_disk){
      return winapi::flush_file_buffers(m_file_or_mapping_hnd);
   }
   return true;
}

inline void mapped_region::priv_close()
{
   if(m_base){
      void *addr = this->priv_map_address();
      #if !defined(BOOST_INTERPROCESS_FORCE_GENERIC_EMULATION)
      mapped_region::destroy_syncs_in_range<0>(addr, m_size);
      #endif
      winapi::unmap_view_of_file(addr);
      m_base = 0;
   }
   if(m_file_or_mapping_hnd != ipcdetail::invalid_file()){
      winapi::close_handle(m_file_or_mapping_hnd);
      m_file_or_mapping_hnd = ipcdetail::invalid_file();
   }
}

inline void mapped_region::dont_close_on_destruction()
{}

#else    //#if (defined BOOST_INTERPROCESS_WINDOWS)

inline mapped_region::mapped_region()
   :  m_base(0), m_size(0), m_page_offset(0), m_mode(read_only), m_is_xsi(false)
{}

template<int dummy>
inline std::size_t mapped_region::page_size_holder<dummy>::get_page_size()
{  return std::size_t(sysconf(_SC_PAGESIZE)); }

template<class MemoryMappable>
inline mapped_region::mapped_region
   ( const MemoryMappable &mapping
   , mode_t mode
   , offset_t offset
   , std::size_t size
   , const void *address)
   : m_base(0), m_size(0), m_page_offset(0), m_mode(mode), m_is_xsi(false)
{
   mapping_handle_t map_hnd = mapping.get_mapping_handle();

   //Some systems dont' support XSI shared memory
   #ifdef BOOST_INTERPROCESS_XSI_SHARED_MEMORY_OBJECTS
   if(map_hnd.is_xsi){
      //Get the size
      ::shmid_ds xsi_ds;
      int ret = ::shmctl(map_hnd.handle, IPC_STAT, &xsi_ds);
      if(ret == -1){
         error_info err(system_error_code());
         throw interprocess_exception(err);
      }
      //Compare sizess
      if(size == 0){
         size = (std::size_t)xsi_ds.shm_segsz;
      }
      else if(size != (std::size_t)xsi_ds.shm_segsz){
         error_info err(size_error);
         throw interprocess_exception(err);
      }
      //Calculate flag
      int flag = 0;
      if(m_mode == read_only){
         flag |= SHM_RDONLY;
      }
      else if(m_mode != read_write){
         error_info err(mode_error);
         throw interprocess_exception(err);
      }
      //Attach memory
      void *base = ::shmat(map_hnd.handle, (void*)address, flag);
      if(base == (void*)-1){
         error_info err(system_error_code());
         throw interprocess_exception(err);
      }
      //Update members
      m_base   = base;
      m_size   = size;
      m_mode   = mode;
      m_page_offset = 0;
      m_is_xsi = true;
      return;
   }
   #endif   //ifdef BOOST_INTERPROCESS_XSI_SHARED_MEMORY_OBJECTS

   //We calculate the difference between demanded and valid offset
   const offset_t page_offset = priv_page_offset_addr_fixup(offset, address);

   if(size == 0){
      struct ::stat buf;
      if(0 != fstat(map_hnd.handle, &buf)){
         error_info err(system_error_code());
         throw interprocess_exception(err);
      }
      //This can throw
      priv_size_from_mapping_size(buf.st_size, offset, page_offset, size);
   }

   //Create new mapping
   int prot    = 0;
   int flags   = 0;

   switch(mode)
   {
      case read_only:
         prot  |= PROT_READ;
         flags |= MAP_SHARED;
      break;

      case read_private:
         prot  |= (PROT_READ);
         flags |= MAP_PRIVATE;
      break;

      case read_write:
         prot  |= (PROT_WRITE | PROT_READ);
         flags |= MAP_SHARED;
      break;

      case copy_on_write:
         prot  |= (PROT_WRITE | PROT_READ);
         flags |= MAP_PRIVATE;
      break;

      default:
         {
            error_info err(mode_error);
            throw interprocess_exception(err);
         }
      break;
   }

   //Map it to the address space
   void* base = mmap ( const_cast<void*>(address)
                     , static_cast<std::size_t>(page_offset + size)
                     , prot
                     , flags
                     , mapping.get_mapping_handle().handle
                     , offset - page_offset);

   //Check if mapping was successful
   if(base == MAP_FAILED){
      error_info err = system_error_code();
      throw interprocess_exception(err);
   }

   //Calculate new base for the user
   m_base = static_cast<char*>(base) + page_offset;
   m_page_offset = page_offset;
   m_size   = size;

   //Check for fixed mapping error
   if(address && (base != address)){
      error_info err(busy_error);
      this->priv_close();
      throw interprocess_exception(err);
   }
}

inline bool mapped_region::flush(std::size_t mapping_offset, std::size_t numbytes, bool async)
{
   void *addr;
   if(!this->priv_flush_param_check(mapping_offset, addr, numbytes)){
      return false;
   }
   //Flush it all
   return msync( addr, numbytes, async ? MS_ASYNC : MS_SYNC) == 0;
}

inline void mapped_region::priv_close()
{
   if(m_base != 0){
      #ifdef BOOST_INTERPROCESS_XSI_SHARED_MEMORY_OBJECTS
      if(m_is_xsi){
         int ret = ::shmdt(m_base);
         BOOST_ASSERT(ret == 0);
         (void)ret;
         return;
      }
      #endif //#ifdef BOOST_INTERPROCESS_XSI_SHARED_MEMORY_OBJECTS
      munmap(this->priv_map_address(), this->priv_map_size());
      m_base = 0;
   }
}

inline void mapped_region::dont_close_on_destruction()
{  m_base = 0;   }

#endif   //##if (defined BOOST_INTERPROCESS_WINDOWS)

template<int dummy>
const std::size_t mapped_region::page_size_holder<dummy>::PageSize
   = mapped_region::page_size_holder<dummy>::get_page_size();

inline std::size_t mapped_region::get_page_size()
{
   if(!page_size_holder<0>::PageSize)
      return page_size_holder<0>::get_page_size();
   else
      return page_size_holder<0>::PageSize;
}

inline void mapped_region::swap(mapped_region &other)
{
   ipcdetail::do_swap(this->m_base, other.m_base);
   ipcdetail::do_swap(this->m_size, other.m_size);
   ipcdetail::do_swap(this->m_page_offset, other.m_page_offset);
   ipcdetail::do_swap(this->m_mode,  other.m_mode);
   #if (defined BOOST_INTERPROCESS_WINDOWS)
   ipcdetail::do_swap(this->m_file_or_mapping_hnd, other.m_file_or_mapping_hnd);
   #else
   ipcdetail::do_swap(this->m_is_xsi, other.m_is_xsi);
   #endif
}

//!No-op functor
struct null_mapped_region_function
{
   bool operator()(void *, std::size_t , bool) const
      {   return true;   }
};

/// @endcond

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_MAPPED_REGION_HPP

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

#ifndef BOOST_INTERPROCESS_MAPPED_REGION_EXT_HPP
#define BOOST_INTERPROCESS_MAPPED_REGION_EXT_HPP

#if defined(BOOST_INTERPROCESS_WINDOWS) && !defined(BOOST_INTERPROCESS_FORCE_GENERIC_EMULATION)
#  include <boost/interprocess/sync/windows/sync_utils.hpp>
#  include <boost/interprocess/detail/windows_intermodule_singleton.hpp>

namespace boost {
namespace interprocess {

template<int Dummy>
inline void mapped_region::destroy_syncs_in_range(const void *addr, std::size_t size)
{
   ipcdetail::sync_handles &handles =
      ipcdetail::windows_intermodule_singleton<ipcdetail::sync_handles>::get();
   handles.destroy_syncs_in_range(addr, size);
}

}  //namespace interprocess {
}  //namespace boost {

#endif   //defined(BOOST_INTERPROCESS_WINDOWS) && !defined(BOOST_INTERPROCESS_FORCE_GENERIC_EMULATION)

#endif   //#ifdef BOOST_INTERPROCESS_MAPPED_REGION_EXT_HPP

#endif   //#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

