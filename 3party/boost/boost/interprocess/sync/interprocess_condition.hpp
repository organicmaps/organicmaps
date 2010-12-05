//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_CONDITION_HPP
#define BOOST_INTERPROCESS_CONDITION_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

/// @cond

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/detail/posix_time_types_wrk.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/limits.hpp>
#include <cassert>

#if !defined(BOOST_INTERPROCESS_FORCE_GENERIC_EMULATION) && defined(BOOST_INTERPROCESS_POSIX_PROCESS_SHARED)
   #include <pthread.h>
   #include <errno.h>   
   #include <boost/interprocess/sync/posix/pthread_helpers.hpp>
   #define BOOST_INTERPROCESS_USE_POSIX
#else
   #include <boost/interprocess/detail/atomic.hpp>
   #include <boost/cstdint.hpp>
   #include <boost/interprocess/detail/os_thread_functions.hpp>
   #define BOOST_INTERPROCESS_USE_GENERIC_EMULATION
#endif

/// @endcond

//!\file
//!Describes process-shared variables interprocess_condition class

namespace boost {

namespace posix_time
{  class ptime;   }

namespace interprocess {

class named_condition;

//!This class is a condition variable that can be placed in shared memory or
//!memory mapped files.
class interprocess_condition
{
   /// @cond
   //Non-copyable
   interprocess_condition(const interprocess_condition &);
   interprocess_condition &operator=(const interprocess_condition &);
   friend class named_condition;
   /// @endcond
   public:
   //!Constructs a interprocess_condition. On error throws interprocess_exception.
   interprocess_condition();

   //!Destroys *this
   //!liberating system resources.
   ~interprocess_condition();

   //!If there is a thread waiting on *this, change that 
   //!thread's state to ready. Otherwise there is no effect.
   void notify_one();

   //!Change the state of all threads waiting on *this to ready.
   //!If there are no waiting threads, notify_all() has no effect.
   void notify_all();

   //!Releases the lock on the interprocess_mutex object associated with lock, blocks 
   //!the current thread of execution until readied by a call to 
   //!this->notify_one() or this->notify_all(), and then reacquires the lock.
   template <typename L>
   void wait(L& lock)
   {
      if (!lock)
         throw lock_exception();
      do_wait(*lock.mutex());
   }

   //!The same as:
   //!while (!pred()) wait(lock)
   template <typename L, typename Pr>
   void wait(L& lock, Pr pred)
   {
      if (!lock)
         throw lock_exception();

      while (!pred())
         do_wait(*lock.mutex());
   }

   //!Releases the lock on the interprocess_mutex object associated with lock, blocks 
   //!the current thread of execution until readied by a call to 
   //!this->notify_one() or this->notify_all(), or until time abs_time is reached, 
   //!and then reacquires the lock.
   //!Returns: false if time abs_time is reached, otherwise true.
   template <typename L>
   bool timed_wait(L& lock, const boost::posix_time::ptime &abs_time)
   {
      if(abs_time == boost::posix_time::pos_infin){
         this->wait(lock);
         return true;
      }
      if (!lock)
            throw lock_exception();
      return do_timed_wait(abs_time, *lock.mutex());
   }

   //!The same as:   while (!pred()) { 
   //!                  if (!timed_wait(lock, abs_time)) return pred(); 
   //!               } return true;
   template <typename L, typename Pr>
   bool timed_wait(L& lock, const boost::posix_time::ptime &abs_time, Pr pred)
   {
      if(abs_time == boost::posix_time::pos_infin){
         this->wait(lock, pred);
         return true;
      }
      if (!lock)
            throw lock_exception();
      while (!pred()){
         if (!do_timed_wait(abs_time, *lock.mutex()))
            return pred();
      }

      return true;
   }

   /// @cond
   private:
   void do_wait(interprocess_mutex  &mut);

   bool do_timed_wait(const boost::posix_time::ptime &abs_time, interprocess_mutex &mut);

   #if defined (BOOST_INTERPROCESS_USE_GENERIC_EMULATION)
      enum { SLEEP = 0, NOTIFY_ONE, NOTIFY_ALL };
      interprocess_mutex m_enter_mut;
      //interprocess_mutex     m_check_mut;
      volatile boost::uint32_t    m_command;
      volatile boost::uint32_t    m_num_waiters;
      bool do_timed_wait(bool tout_enabled, const boost::posix_time::ptime &abs_time, interprocess_mutex &mut);
      void notify(boost::uint32_t command);
   #elif defined(BOOST_INTERPROCESS_USE_POSIX)
      pthread_cond_t   m_condition;
   #endif
   /// @endcond
};

}  //namespace interprocess

}  // namespace boost

#ifdef BOOST_INTERPROCESS_USE_GENERIC_EMULATION
#  undef BOOST_INTERPROCESS_USE_GENERIC_EMULATION
#  include <boost/interprocess/sync/emulation/interprocess_condition.hpp>
#endif

#ifdef BOOST_INTERPROCESS_USE_POSIX
#  undef BOOST_INTERPROCESS_USE_POSIX
#  include <boost/interprocess/sync/posix/interprocess_condition.hpp>
#endif

#include <boost/interprocess/detail/config_end.hpp>

#endif // BOOST_INTERPROCESS_CONDITION_HPP
