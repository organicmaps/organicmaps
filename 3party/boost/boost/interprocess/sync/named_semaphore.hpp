 //////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_NAMED_SEMAPHORE_HPP
#define BOOST_INTERPROCESS_NAMED_SEMAPHORE_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/interprocess_tester.hpp>
#include <boost/interprocess/detail/posix_time_types_wrk.hpp>

#if defined(BOOST_INTERPROCESS_NAMED_SEMAPHORE_USES_POSIX_SEMAPHORES)
#include <boost/interprocess/sync/posix/semaphore_wrapper.hpp>
#else
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/detail/managed_open_or_create_impl.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/sync/emulation/named_creation_functor.hpp>
#endif

//!\file
//!Describes a named semaphore class for inter-process synchronization

namespace boost {
namespace interprocess {

//!A semaphore with a global name, so it can be found from different 
//!processes. Allows several resource sharing patterns and efficient 
//!acknowledgment mechanisms.
class named_semaphore
{
   /// @cond

   //Non-copyable
   named_semaphore();
   named_semaphore(const named_semaphore &);
   named_semaphore &operator=(const named_semaphore &);
   /// @endcond

   public:
   //!Creates a global semaphore with a name, and an initial count. 
   //!If the semaphore can't be created throws interprocess_exception
   named_semaphore(create_only_t, const char *name, unsigned int initialCount);

   //!Opens or creates a global semaphore with a name, and an initial count. 
   //!If the semaphore is created, this call is equivalent to
   //!named_semaphore(create_only_t, ...)
   //!If the semaphore is already created, this call is equivalent to
   //!named_semaphore(open_only_t, ... )
   //!and initialCount is ignored.
   named_semaphore(open_or_create_t, const char *name, unsigned int initialCount);

   //!Opens a global semaphore with a name if that semaphore is previously.
   //!created. If it is not previously created this function throws
   //!interprocess_exception.
   named_semaphore(open_only_t, const char *name);

   //!Destroys *this and indicates that the calling process is finished using
   //!the resource. The destructor function will deallocate
   //!any system resources allocated by the system for use by this process for
   //!this resource. The resource can still be opened again calling
   //!the open constructor overload. To erase the resource from the system
   //!use remove().
   ~named_semaphore();

   //!Increments the semaphore count. If there are processes/threads blocked waiting
   //!for the semaphore, then one of these processes will return successfully from
   //!its wait function. If there is an error an interprocess_exception exception is thrown.
   void post();

   //!Decrements the semaphore. If the semaphore value is not greater than zero,
   //!then the calling process/thread blocks until it can decrement the counter. 
   //!If there is an error an interprocess_exception exception is thrown.
   void wait();

   //!Decrements the semaphore if the semaphore's value is greater than zero
   //!and returns true. If the value is not greater than zero returns false.
   //!If there is an error an interprocess_exception exception is thrown.
   bool try_wait();

   //!Decrements the semaphore if the semaphore's value is greater
   //!than zero and returns true. Otherwise, waits for the semaphore
   //!to the posted or the timeout expires. If the timeout expires, the
   //!function returns false. If the semaphore is posted the function
   //!returns true. If there is an error throws sem_exception
   bool timed_wait(const boost::posix_time::ptime &abs_time);

   //!Erases a named semaphore from the system.
   //!Returns false on error. Never throws.
   static bool remove(const char *name);

   /// @cond
   private:
   friend class detail::interprocess_tester;
   void dont_close_on_destruction();

   #if defined(BOOST_INTERPROCESS_NAMED_SEMAPHORE_USES_POSIX_SEMAPHORES)
   detail::named_semaphore_wrapper m_sem;
   #else
   interprocess_semaphore *semaphore() const
   {  return static_cast<interprocess_semaphore*>(m_shmem.get_user_address()); }

   detail::managed_open_or_create_impl<shared_memory_object> m_shmem;
   typedef detail::named_creation_functor<interprocess_semaphore, int> construct_func_t;
   #endif
   /// @endcond
};

/// @cond

#if defined(BOOST_INTERPROCESS_NAMED_SEMAPHORE_USES_POSIX_SEMAPHORES)

inline named_semaphore::named_semaphore
   (create_only_t, const char *name, unsigned int initialCount)
   :  m_sem(detail::DoCreate, name, read_write, initialCount)
{}

inline named_semaphore::named_semaphore
   (open_or_create_t, const char *name, unsigned int initialCount)
   :  m_sem(detail::DoOpenOrCreate, name, read_write, initialCount)
{}

inline named_semaphore::named_semaphore
   (open_only_t, const char *name)
   :  m_sem(detail::DoOpen, name, read_write, 1)
{}

inline named_semaphore::~named_semaphore()
{}

inline void named_semaphore::dont_close_on_destruction()
{  detail::interprocess_tester::dont_close_on_destruction(m_sem);  }

inline void named_semaphore::wait()
{  m_sem.wait();  }

inline void named_semaphore::post()
{  m_sem.post();  }

inline bool named_semaphore::try_wait()
{  return m_sem.try_wait();  }

inline bool named_semaphore::timed_wait(const boost::posix_time::ptime &abs_time)
{
   if(abs_time == boost::posix_time::pos_infin){
      this->wait();
      return true;
   }
   return m_sem.timed_wait(abs_time);
}

inline bool named_semaphore::remove(const char *name)
{  return detail::named_semaphore_wrapper::remove(name);   }

#else

inline named_semaphore::~named_semaphore()
{}

inline void named_semaphore::dont_close_on_destruction()
{  detail::interprocess_tester::dont_close_on_destruction(m_shmem);  }

inline named_semaphore::named_semaphore
   (create_only_t, const char *name, unsigned int initialCount)
   :  m_shmem  (create_only
               ,name
               ,sizeof(interprocess_semaphore) +
                  detail::managed_open_or_create_impl<shared_memory_object>::
                     ManagedOpenOrCreateUserOffset
               ,read_write
               ,0
               ,construct_func_t(detail::DoCreate, initialCount))
{}

inline named_semaphore::named_semaphore
   (open_or_create_t, const char *name, unsigned int initialCount)
   :  m_shmem  (open_or_create
               ,name
               ,sizeof(interprocess_semaphore) +
                  detail::managed_open_or_create_impl<shared_memory_object>::
                     ManagedOpenOrCreateUserOffset
               ,read_write
               ,0
               ,construct_func_t(detail::DoOpenOrCreate, initialCount))
{}

inline named_semaphore::named_semaphore
   (open_only_t, const char *name)
   :  m_shmem  (open_only
               ,name
               ,read_write
               ,0
               ,construct_func_t(detail::DoOpen, 0))
{}

inline void named_semaphore::post()
{  semaphore()->post();   }

inline void named_semaphore::wait()
{  semaphore()->wait();   }

inline bool named_semaphore::try_wait()
{  return semaphore()->try_wait();   }

inline bool named_semaphore::timed_wait(const boost::posix_time::ptime &abs_time)
{
   if(abs_time == boost::posix_time::pos_infin){
      this->wait();
      return true;
   }
   return semaphore()->timed_wait(abs_time);
}

inline bool named_semaphore::remove(const char *name)
{  return shared_memory_object::remove(name); }

#endif

/// @endcond

}  //namespace interprocess {
}  //namespace boost {


#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_NAMED_SEMAPHORE_HPP
