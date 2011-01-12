//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_NAMED_CONDITION_HPP
#define BOOST_INTERPROCESS_NAMED_CONDITION_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/static_assert.hpp>
#include <boost/interprocess/detail/type_traits.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/detail/managed_open_or_create_impl.hpp>
#include <boost/interprocess/detail/posix_time_types_wrk.hpp>
#include <boost/interprocess/sync/emulation/named_creation_functor.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/permissions.hpp>
#if defined BOOST_INTERPROCESS_NAMED_MUTEX_USES_POSIX_SEMAPHORES
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#endif


//!\file
//!Describes process-shared variables interprocess_condition class

namespace boost {
namespace interprocess {

/// @cond
namespace detail{ class interprocess_tester; }
/// @endcond

//! A global condition variable that can be created by name.
//! This condition variable is designed to work with named_mutex and
//! can't be placed in shared memory or memory mapped files.
class named_condition
{
   /// @cond
   //Non-copyable
   named_condition();
   named_condition(const named_condition &);
   named_condition &operator=(const named_condition &);
   /// @endcond
   public:
   //!Creates a global condition with a name.
   //!If the condition can't be created throws interprocess_exception
   named_condition(create_only_t create_only, const char *name, const permissions &perm = permissions());

   //!Opens or creates a global condition with a name. 
   //!If the condition is created, this call is equivalent to
   //!named_condition(create_only_t, ... )
   //!If the condition is already created, this call is equivalent
   //!named_condition(open_only_t, ... )
   //!Does not throw
   named_condition(open_or_create_t open_or_create, const char *name, const permissions &perm = permissions());

   //!Opens a global condition with a name if that condition is previously
   //!created. If it is not previously created this function throws
   //!interprocess_exception.
   named_condition(open_only_t open_only, const char *name);

   //!Destroys *this and indicates that the calling process is finished using
   //!the resource. The destructor function will deallocate
   //!any system resources allocated by the system for use by this process for
   //!this resource. The resource can still be opened again calling
   //!the open constructor overload. To erase the resource from the system
   //!use remove().
   ~named_condition();

   //!If there is a thread waiting on *this, change that 
   //!thread's state to ready. Otherwise there is no effect.*/
   void notify_one();

   //!Change the state of all threads waiting on *this to ready.
   //!If there are no waiting threads, notify_all() has no effect.
   void notify_all();

   //!Releases the lock on the named_mutex object associated with lock, blocks 
   //!the current thread of execution until readied by a call to 
   //!this->notify_one() or this->notify_all(), and then reacquires the lock.
   template <typename L>
   void wait(L& lock);

   //!The same as:
   //!while (!pred()) wait(lock)
   template <typename L, typename Pr>
   void wait(L& lock, Pr pred);

   //!Releases the lock on the named_mutex object associated with lock, blocks 
   //!the current thread of execution until readied by a call to 
   //!this->notify_one() or this->notify_all(), or until time abs_time is reached, 
   //!and then reacquires the lock.
   //!Returns: false if time abs_time is reached, otherwise true.
   template <typename L>
   bool timed_wait(L& lock, const boost::posix_time::ptime &abs_time);

   //!The same as:   while (!pred()) { 
   //!                  if (!timed_wait(lock, abs_time)) return pred(); 
   //!               } return true;
   template <typename L, typename Pr>
   bool timed_wait(L& lock, const boost::posix_time::ptime &abs_time, Pr pred);

   //!Erases a named condition from the system.
   //!Returns false on error. Never throws.
   static bool remove(const char *name);

   /// @cond
   private:

   struct condition_holder
   {
      interprocess_condition cond_;
      //If named_mutex is implemented using semaphores
      //we need to store an additional mutex
      #if defined (BOOST_INTERPROCESS_NAMED_MUTEX_USES_POSIX_SEMAPHORES)
      interprocess_mutex mutex_;
      #endif
   };

   interprocess_condition *condition() const
   {  return &static_cast<condition_holder*>(m_shmem.get_user_address())->cond_; }

   template <class Lock>
   class lock_inverter
   {
      Lock &l_;
      public:
      lock_inverter(Lock &l)
         :  l_(l)
      {}
      void lock()    {   l_.unlock();   }
      void unlock()  {   l_.lock();     }
   };

   #if defined (BOOST_INTERPROCESS_NAMED_MUTEX_USES_POSIX_SEMAPHORES)
   interprocess_mutex *mutex() const
   {  return &static_cast<condition_holder*>(m_shmem.get_user_address())->mutex_; }

   template <class Lock>
   void do_wait(Lock& lock)
   {
      //named_condition only works with named_mutex
      BOOST_STATIC_ASSERT((detail::is_convertible<typename Lock::mutex_type&, named_mutex&>::value == true));
      
      //lock internal before unlocking external to avoid race with a notifier
      scoped_lock<interprocess_mutex>     internal_lock(*this->mutex());
      lock_inverter<Lock> inverted_lock(lock);
      scoped_lock<lock_inverter<Lock> >   external_unlock(inverted_lock);

      //unlock internal first to avoid deadlock with near simultaneous waits
      scoped_lock<interprocess_mutex>     internal_unlock;
      internal_lock.swap(internal_unlock);
      this->condition()->wait(internal_unlock);
   }

   template <class Lock>
   bool do_timed_wait(Lock& lock, const boost::posix_time::ptime &abs_time)
   {
      //named_condition only works with named_mutex
      BOOST_STATIC_ASSERT((detail::is_convertible<typename Lock::mutex_type&, named_mutex&>::value == true));
      //lock internal before unlocking external to avoid race with a notifier  
      scoped_lock<interprocess_mutex>     internal_lock(*this->mutex(), abs_time);  
      if(!internal_lock) return false;
      lock_inverter<Lock> inverted_lock(lock);  
      scoped_lock<lock_inverter<Lock> >   external_unlock(inverted_lock);  

      //unlock internal first to avoid deadlock with near simultaneous waits  
      scoped_lock<interprocess_mutex>     internal_unlock;  
      internal_lock.swap(internal_unlock);  
      return this->condition()->timed_wait(internal_unlock, abs_time);  
   }
   #endif

   friend class detail::interprocess_tester;
   void dont_close_on_destruction();

   detail::managed_open_or_create_impl<shared_memory_object> m_shmem;

   template <class T, class Arg> friend class boost::interprocess::detail::named_creation_functor;
   typedef detail::named_creation_functor<condition_holder> construct_func_t;
   /// @endcond
};

/// @cond

inline named_condition::~named_condition()
{}

inline named_condition::named_condition(create_only_t, const char *name, const permissions &perm)
   :  m_shmem  (create_only
               ,name
               ,sizeof(condition_holder) +
                  detail::managed_open_or_create_impl<shared_memory_object>::
                     ManagedOpenOrCreateUserOffset
               ,read_write
               ,0
               ,construct_func_t(detail::DoCreate)
               ,perm)
{}

inline named_condition::named_condition(open_or_create_t, const char *name, const permissions &perm)
   :  m_shmem  (open_or_create
               ,name
               ,sizeof(condition_holder) +
                  detail::managed_open_or_create_impl<shared_memory_object>::
                     ManagedOpenOrCreateUserOffset
               ,read_write
               ,0
               ,construct_func_t(detail::DoOpenOrCreate)
               ,perm)
{}

inline named_condition::named_condition(open_only_t, const char *name)
   :  m_shmem  (open_only
               ,name
               ,read_write
               ,0
               ,construct_func_t(detail::DoOpen))
{}

inline void named_condition::dont_close_on_destruction()
{  detail::interprocess_tester::dont_close_on_destruction(m_shmem);  }

#if defined(BOOST_INTERPROCESS_NAMED_MUTEX_USES_POSIX_SEMAPHORES)

inline void named_condition::notify_one()
{
   scoped_lock<interprocess_mutex> internal_lock(*this->mutex());
   this->condition()->notify_one();
}

inline void named_condition::notify_all()
{
   scoped_lock<interprocess_mutex> internal_lock(*this->mutex());
   this->condition()->notify_all();
}

template <typename L>
inline void named_condition::wait(L& lock)
{
   if (!lock)
      throw lock_exception();
   this->do_wait(lock);
}

template <typename L, typename Pr>
inline void named_condition::wait(L& lock, Pr pred)
{
   if (!lock)
      throw lock_exception();
   while (!pred())
      this->do_wait(lock);
}

template <typename L>
inline bool named_condition::timed_wait
   (L& lock, const boost::posix_time::ptime &abs_time)
{
   if(abs_time == boost::posix_time::pos_infin){
      this->wait(lock);
      return true;
   }
   if (!lock)
      throw lock_exception();
   return this->do_timed_wait(lock, abs_time);
}

template <typename L, typename Pr>
inline bool named_condition::timed_wait
   (L& lock, const boost::posix_time::ptime &abs_time, Pr pred)
{
   if(abs_time == boost::posix_time::pos_infin){
      this->wait(lock, pred);
      return true;
   }
   if (!lock)
      throw lock_exception();

   while (!pred()){
      if(!this->do_timed_wait(lock, abs_time)){
         return pred();
      }
   }
   return true;
}

#else

inline void named_condition::notify_one()
{  this->condition()->notify_one();  }

inline void named_condition::notify_all()
{  this->condition()->notify_all();  }

template <typename L>
inline void named_condition::wait(L& lock)
{
   if (!lock)
      throw lock_exception();
   this->condition()->do_wait(*lock.mutex()->mutex());
}

template <typename L, typename Pr>
inline void named_condition::wait(L& lock, Pr pred)
{
   if (!lock)
      throw lock_exception();

   while (!pred())
      this->condition()->do_wait(*lock.mutex()->mutex());
}

template <typename L>
inline bool named_condition::timed_wait
   (L& lock, const boost::posix_time::ptime &abs_time)
{
   if(abs_time == boost::posix_time::pos_infin){
      this->wait(lock);
      return true;
   }
   if (!lock)
      throw lock_exception();
   return this->condition()->do_timed_wait(abs_time, *lock.mutex()->mutex());
}

template <typename L, typename Pr>
inline bool named_condition::timed_wait
   (L& lock, const boost::posix_time::ptime &abs_time, Pr pred)
{
   if(abs_time == boost::posix_time::pos_infin){
      this->wait(lock, pred);
      return true;
   }
   if (!lock)
      throw lock_exception();

   while (!pred()){
      if (!this->condition()->do_timed_wait(abs_time, *lock.mutex()->mutex()))
         return pred();
   }

   return true;
}

#endif

inline bool named_condition::remove(const char *name)
{  return shared_memory_object::remove(name); }

/// @endcond

}  //namespace interprocess
}  //namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif // BOOST_INTERPROCESS_NAMED_CONDITION_HPP
