//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
//
// Parts of the pthread code come from Boost Threads code.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_MUTEX_HPP
#define BOOST_INTERPROCESS_MUTEX_HPP

/// @cond

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/posix_time_types_wrk.hpp>
#include <boost/assert.hpp>

#if !defined(BOOST_INTERPROCESS_FORCE_GENERIC_EMULATION) && defined (BOOST_INTERPROCESS_POSIX_PROCESS_SHARED)
   #include <pthread.h>
   #include <errno.h>   
   #include <boost/interprocess/sync/posix/pthread_helpers.hpp>
   #define BOOST_INTERPROCESS_USE_POSIX
#else
   #include <boost/interprocess/sync/emulation/mutex.hpp>
   #define BOOST_INTERPROCESS_USE_GENERIC_EMULATION

namespace boost {
namespace interprocess {
namespace detail{
namespace robust_emulation_helpers {

template<class T>
class mutex_traits;

}}}}

#endif

/// @endcond

//!\file
//!Describes a mutex class that can be placed in memory shared by
//!several processes.

namespace boost {
namespace interprocess {

class interprocess_condition;

//!Wraps a interprocess_mutex that can be placed in shared memory and can be 
//!shared between processes. Allows timed lock tries
class interprocess_mutex
{
   /// @cond
   //Non-copyable
   interprocess_mutex(const interprocess_mutex &);
   interprocess_mutex &operator=(const interprocess_mutex &);
   friend class interprocess_condition;
   /// @endcond
   public:

   //!Constructor.
   //!Throws interprocess_exception on error.
   interprocess_mutex();

   //!Destructor. If any process uses the mutex after the destructor is called
   //!the result is undefined. Does not throw.
   ~interprocess_mutex();

   //!Effects: The calling thread tries to obtain ownership of the mutex, and
   //!   if another thread has ownership of the mutex, it waits until it can
   //!   obtain the ownership. If a thread takes ownership of the mutex the
   //!   mutex must be unlocked by the same mutex.
   //!Throws: interprocess_exception on error.
   void lock();

   //!Effects: The calling thread tries to obtain ownership of the mutex, and
   //!   if another thread has ownership of the mutex returns immediately.
   //!Returns: If the thread acquires ownership of the mutex, returns true, if
   //!   the another thread has ownership of the mutex, returns false.
   //!Throws: interprocess_exception on error.
   bool try_lock();

   //!Effects: The calling thread will try to obtain exclusive ownership of the
   //!   mutex if it can do so in until the specified time is reached. If the
   //!   mutex supports recursive locking, the mutex must be unlocked the same
   //!   number of times it is locked. 
   //!Returns: If the thread acquires ownership of the mutex, returns true, if
   //!   the timeout expires returns false. 
   //!Throws: interprocess_exception on error.
   bool timed_lock(const boost::posix_time::ptime &abs_time);

   //!Effects: The calling thread releases the exclusive ownership of the mutex.
   //!Throws: interprocess_exception on error.
   void unlock();
   /// @cond
   private:

   #if   defined(BOOST_INTERPROCESS_USE_GENERIC_EMULATION)
   friend class detail::robust_emulation_helpers::mutex_traits<interprocess_mutex>;
   void take_ownership(){ mutex.take_ownership(); }
   detail::emulation_mutex mutex;
   #elif defined(BOOST_INTERPROCESS_USE_POSIX)
      pthread_mutex_t   m_mut;
   #endif   //#if (defined BOOST_INTERPROCESS_WINDOWS)
   /// @endcond
};

}  //namespace interprocess {
}  //namespace boost {

#ifdef BOOST_INTERPROCESS_USE_GENERIC_EMULATION
#  undef BOOST_INTERPROCESS_USE_GENERIC_EMULATION

namespace boost {
namespace interprocess {

inline interprocess_mutex::interprocess_mutex(){}
inline interprocess_mutex::~interprocess_mutex(){}
inline void interprocess_mutex::lock(){ mutex.lock(); }
inline bool interprocess_mutex::try_lock(){ return mutex.try_lock(); }
inline bool interprocess_mutex::timed_lock(const boost::posix_time::ptime &abs_time){ return mutex.timed_lock(abs_time); }
inline void interprocess_mutex::unlock(){ mutex.unlock(); }

}  //namespace interprocess {
}  //namespace boost {

#endif

#ifdef BOOST_INTERPROCESS_USE_POSIX
#include <boost/interprocess/sync/posix/interprocess_mutex.hpp>
#  undef BOOST_INTERPROCESS_USE_POSIX
#endif

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_MUTEX_HPP
