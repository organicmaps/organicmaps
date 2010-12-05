//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_NAMED_MUTEX_HPP
#define BOOST_INTERPROCESS_NAMED_MUTEX_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/sync/emulation/named_creation_functor.hpp>
#include <boost/interprocess/detail/interprocess_tester.hpp>

#if defined(BOOST_INTERPROCESS_NAMED_MUTEX_USES_POSIX_SEMAPHORES)
   #include <boost/interprocess/sync/posix/semaphore_wrapper.hpp>
#else
   #include <boost/interprocess/shared_memory_object.hpp>
   #include <boost/interprocess/sync/interprocess_mutex.hpp>
   #include <boost/interprocess/detail/managed_open_or_create_impl.hpp>
   #include <boost/interprocess/detail/posix_time_types_wrk.hpp>
#endif

//!\file
//!Describes a named mutex class for inter-process synchronization

namespace boost {
namespace interprocess {

class named_condition;

//!A mutex with a global name, so it can be found from different 
//!processes. This mutex can't be placed in shared memory, and
//!each process should have it's own named_mutex.
class named_mutex
{
   /// @cond

   //Non-copyable
   named_mutex();
   named_mutex(const named_mutex &);
   named_mutex &operator=(const named_mutex &);
   friend class named_condition;
   /// @endcond

   public:
   //!Creates a global interprocess_mutex with a name.
   //!Throws interprocess_exception on error.
   named_mutex(create_only_t create_only, const char *name);

   //!Opens or creates a global mutex with a name. 
   //!If the mutex is created, this call is equivalent to
   //!named_mutex(create_only_t, ... )
   //!If the mutex is already created, this call is equivalent
   //!named_mutex(open_only_t, ... )
   //!Does not throw
   named_mutex(open_or_create_t open_or_create, const char *name);

   //!Opens a global mutex with a name if that mutex is previously
   //!created. If it is not previously created this function throws
   //!interprocess_exception.
   named_mutex(open_only_t open_only, const char *name);

   //!Destroys *this and indicates that the calling process is finished using
   //!the resource. The destructor function will deallocate
   //!any system resources allocated by the system for use by this process for
   //!this resource. The resource can still be opened again calling
   //!the open constructor overload. To erase the resource from the system
   //!use remove().
   ~named_mutex();

   //!Unlocks a previously locked
   //!interprocess_mutex.
   void unlock();

   //!Locks interprocess_mutex, sleeps when interprocess_mutex is already locked.
   //!Throws interprocess_exception if a severe error is found
   void lock();

   //!Tries to lock the interprocess_mutex, returns false when interprocess_mutex 
   //!is already locked, returns true when success.
   //!Throws interprocess_exception if a severe error is found
   bool try_lock();

   //!Tries to lock the interprocess_mutex until time abs_time,
   //!Returns false when timeout expires, returns true when locks.
   //!Throws interprocess_exception if a severe error is found
   bool timed_lock(const boost::posix_time::ptime &abs_time);

   //!Erases a named mutex from the system.
   //!Returns false on error. Never throws.
   static bool remove(const char *name);

   /// @cond
   private:
   friend class detail::interprocess_tester;
   void dont_close_on_destruction();

   #if defined(BOOST_INTERPROCESS_NAMED_MUTEX_USES_POSIX_SEMAPHORES)
   detail::named_semaphore_wrapper m_sem;
   #else
   interprocess_mutex *mutex() const
   {  return static_cast<interprocess_mutex*>(m_shmem.get_user_address()); }

   detail::managed_open_or_create_impl<shared_memory_object> m_shmem;
   typedef detail::named_creation_functor<interprocess_mutex> construct_func_t;
   #endif
   /// @endcond
};

/// @cond

#if defined(BOOST_INTERPROCESS_NAMED_MUTEX_USES_POSIX_SEMAPHORES)

inline named_mutex::named_mutex(create_only_t, const char *name)
   :  m_sem(detail::DoCreate, name, read_write, 1)
{}

inline named_mutex::named_mutex(open_or_create_t, const char *name)
   :  m_sem(detail::DoOpenOrCreate, name, read_write, 1)
{}

inline named_mutex::named_mutex(open_only_t, const char *name)
   :  m_sem(detail::DoOpen, name, read_write, 1)
{}

inline void named_mutex::dont_close_on_destruction()
{  detail::interprocess_tester::dont_close_on_destruction(m_sem);  }

inline named_mutex::~named_mutex()
{}

inline void named_mutex::lock()
{  m_sem.wait();  }

inline void named_mutex::unlock()
{  m_sem.post();  }

inline bool named_mutex::try_lock()
{  return m_sem.try_wait();  }

inline bool named_mutex::timed_lock(const boost::posix_time::ptime &abs_time)
{
   if(abs_time == boost::posix_time::pos_infin){
      this->lock();
      return true;
   }
   return m_sem.timed_wait(abs_time);
}

inline bool named_mutex::remove(const char *name)
{  return detail::named_semaphore_wrapper::remove(name);   }

#else

inline void named_mutex::dont_close_on_destruction()
{  detail::interprocess_tester::dont_close_on_destruction(m_shmem);  }

inline named_mutex::~named_mutex()
{}

inline named_mutex::named_mutex(create_only_t, const char *name)
   :  m_shmem  (create_only
               ,name
               ,sizeof(interprocess_mutex) +
                  detail::managed_open_or_create_impl<shared_memory_object>::
                     ManagedOpenOrCreateUserOffset
               ,read_write
               ,0
               ,construct_func_t(detail::DoCreate))
{}

inline named_mutex::named_mutex(open_or_create_t, const char *name)
   :  m_shmem  (open_or_create
               ,name
               ,sizeof(interprocess_mutex) +
                  detail::managed_open_or_create_impl<shared_memory_object>::
                     ManagedOpenOrCreateUserOffset
               ,read_write
               ,0
               ,construct_func_t(detail::DoOpenOrCreate))
{}

inline named_mutex::named_mutex(open_only_t, const char *name)
   :  m_shmem  (open_only
               ,name
               ,read_write
               ,0
               ,construct_func_t(detail::DoOpen))
{}

inline void named_mutex::lock()
{  this->mutex()->lock();  }

inline void named_mutex::unlock()
{  this->mutex()->unlock();  }

inline bool named_mutex::try_lock()
{  return this->mutex()->try_lock();  }

inline bool named_mutex::timed_lock(const boost::posix_time::ptime &abs_time)
{
   if(abs_time == boost::posix_time::pos_infin){
      this->lock();
      return true;
   }
   return this->mutex()->timed_lock(abs_time);
}

inline bool named_mutex::remove(const char *name)
{  return shared_memory_object::remove(name); }

#endif

/// @endcond

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_NAMED_MUTEX_HPP
