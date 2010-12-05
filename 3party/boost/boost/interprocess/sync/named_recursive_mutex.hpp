//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_NAMED_RECURSIVE_MUTEX_HPP
#define BOOST_INTERPROCESS_NAMED_RECURSIVE_MUTEX_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/posix_time_types_wrk.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/detail/managed_open_or_create_impl.hpp>
#include <boost/interprocess/sync/interprocess_recursive_mutex.hpp>
#include <boost/interprocess/sync/emulation/named_creation_functor.hpp>

//!\file
//!Describes a named named_recursive_mutex class for inter-process synchronization

namespace boost {
namespace interprocess {

/// @cond
namespace detail{ class interprocess_tester; }
/// @endcond

//!A recursive mutex with a global name, so it can be found from different 
//!processes. This mutex can't be placed in shared memory, and
//!each process should have it's own named_recursive_mutex.
class named_recursive_mutex
{
   /// @cond
   //Non-copyable
   named_recursive_mutex();
   named_recursive_mutex(const named_recursive_mutex &);
   named_recursive_mutex &operator=(const named_recursive_mutex &);
   /// @endcond
   public:

   //!Creates a global recursive_mutex with a name.
   //!If the recursive_mutex can't be created throws interprocess_exception
   named_recursive_mutex(create_only_t create_only, const char *name);

   //!Opens or creates a global recursive_mutex with a name. 
   //!If the recursive_mutex is created, this call is equivalent to
   //!named_recursive_mutex(create_only_t, ... )
   //!If the recursive_mutex is already created, this call is equivalent
   //!named_recursive_mutex(open_only_t, ... )
   //!Does not throw
   named_recursive_mutex(open_or_create_t open_or_create, const char *name);

   //!Opens a global recursive_mutex with a name if that recursive_mutex is previously
   //!created. If it is not previously created this function throws
   //!interprocess_exception.
   named_recursive_mutex(open_only_t open_only, const char *name);

   //!Destroys *this and indicates that the calling process is finished using
   //!the resource. The destructor function will deallocate
   //!any system resources allocated by the system for use by this process for
   //!this resource. The resource can still be opened again calling
   //!the open constructor overload. To erase the resource from the system
   //!use remove().
   ~named_recursive_mutex();

   //!Unlocks a previously locked
   //!named_recursive_mutex.
   void unlock();

   //!Locks named_recursive_mutex, sleeps when named_recursive_mutex is already locked.
   //!Throws interprocess_exception if a severe error is found.
   void lock();

   //!Tries to lock the named_recursive_mutex, returns false when named_recursive_mutex 
   //!is already locked, returns true when success.
   //!Throws interprocess_exception if a severe error is found.
   bool try_lock();

   //!Tries to lock the named_recursive_mutex until time abs_time,
   //!Returns false when timeout expires, returns true when locks.
   //!Throws interprocess_exception if a severe error is found
   bool timed_lock(const boost::posix_time::ptime &abs_time);

   //!Erases a named recursive mutex
   //!from the system
   static bool remove(const char *name);

   /// @cond
   private:
   friend class detail::interprocess_tester;
   void dont_close_on_destruction();

   interprocess_recursive_mutex *mutex() const
   {  return static_cast<interprocess_recursive_mutex*>(m_shmem.get_user_address()); }

   detail::managed_open_or_create_impl<shared_memory_object> m_shmem;
   typedef detail::named_creation_functor<interprocess_recursive_mutex> construct_func_t;
   /// @endcond
};

/// @cond

inline named_recursive_mutex::~named_recursive_mutex()
{}

inline void named_recursive_mutex::dont_close_on_destruction()
{  detail::interprocess_tester::dont_close_on_destruction(m_shmem);  }

inline named_recursive_mutex::named_recursive_mutex(create_only_t, const char *name)
   :  m_shmem  (create_only
               ,name
               ,sizeof(interprocess_recursive_mutex) +
                  detail::managed_open_or_create_impl<shared_memory_object>::
                     ManagedOpenOrCreateUserOffset
               ,read_write
               ,0
               ,construct_func_t(detail::DoCreate))
{}

inline named_recursive_mutex::named_recursive_mutex(open_or_create_t, const char *name)
   :  m_shmem  (open_or_create
               ,name
               ,sizeof(interprocess_recursive_mutex) +
                  detail::managed_open_or_create_impl<shared_memory_object>::
                     ManagedOpenOrCreateUserOffset
               ,read_write
               ,0
               ,construct_func_t(detail::DoOpenOrCreate))
{}

inline named_recursive_mutex::named_recursive_mutex(open_only_t, const char *name)
   :  m_shmem  (open_only
               ,name
               ,read_write
               ,0
               ,construct_func_t(detail::DoOpen))
{}

inline void named_recursive_mutex::lock()
{  this->mutex()->lock();  }

inline void named_recursive_mutex::unlock()
{  this->mutex()->unlock();  }

inline bool named_recursive_mutex::try_lock()
{  return this->mutex()->try_lock();  }

inline bool named_recursive_mutex::timed_lock(const boost::posix_time::ptime &abs_time)
{
   if(abs_time == boost::posix_time::pos_infin){
      this->lock();
      return true;
   }
   return this->mutex()->timed_lock(abs_time);
}

inline bool named_recursive_mutex::remove(const char *name)
{  return shared_memory_object::remove(name); }

/// @endcond

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_NAMED_RECURSIVE_MUTEX_HPP
