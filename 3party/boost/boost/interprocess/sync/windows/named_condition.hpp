 //////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2011-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_WINDOWS_NAMED_CONDITION_HPP
#define BOOST_INTERPROCESS_WINDOWS_NAMED_CONDITION_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/permissions.hpp>
#include <boost/interprocess/detail/interprocess_tester.hpp>
#include <boost/interprocess/detail/posix_time_types_wrk.hpp>
#include <boost/interprocess/sync/windows/named_sync.hpp>
#include <boost/interprocess/sync/windows/winapi_semaphore_wrapper.hpp>
#include <boost/interprocess/sync/detail/condition_algorithm_8a.hpp>

namespace boost {
namespace interprocess {
namespace ipcdetail {

class windows_named_condition
{
   /// @cond

   //Non-copyable
   windows_named_condition();
   windows_named_condition(const windows_named_condition &);
   windows_named_condition &operator=(const windows_named_condition &);
   /// @endcond

   public:
   windows_named_condition(create_only_t, const char *name, const permissions &perm = permissions());

   windows_named_condition(open_or_create_t, const char *name, const permissions &perm = permissions());

   windows_named_condition(open_only_t, const char *name);

   ~windows_named_condition();

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

   static bool remove(const char *name);

   /// @cond
   private:
   friend class interprocess_tester;
   void dont_close_on_destruction();

   template <class InterprocessMutex>
   void do_wait(InterprocessMutex& lock);

   template <class InterprocessMutex>
   bool do_timed_wait(const boost::posix_time::ptime &abs_time, InterprocessMutex& lock);

   struct condition_data
   {
      typedef boost::int32_t           integer_type;
      typedef winapi_semaphore_wrapper semaphore_type;
      typedef winapi_mutex_wrapper     mutex_type;

      integer_type    &get_nwaiters_blocked()
      {  return m_nwaiters_blocked;  }

      integer_type    &get_nwaiters_gone()
      {  return m_nwaiters_gone;  }

      integer_type    &get_nwaiters_to_unblock()
      {  return m_nwaiters_to_unblock;  }

      semaphore_type  &get_sem_block_queue()
      {  return m_sem_block_queue;  }

      semaphore_type  &get_sem_block_lock()
      {  return m_sem_block_lock;  }

      mutex_type      &get_mtx_unblock_lock()
      {  return m_mtx_unblock_lock;  }

      integer_type               m_nwaiters_blocked;
      integer_type               m_nwaiters_gone;
      integer_type               m_nwaiters_to_unblock;
      winapi_semaphore_wrapper   m_sem_block_queue;
      winapi_semaphore_wrapper   m_sem_block_lock;
      winapi_mutex_wrapper       m_mtx_unblock_lock;
   } m_condition_data;

   typedef condition_algorithm_8a<condition_data> algorithm_type;

   class named_cond_callbacks : public windows_named_sync_interface
   {
      typedef __int64 sem_count_t;
      mutable sem_count_t sem_counts [2];

      public:
      named_cond_callbacks(condition_data &cond_data)
         : m_condition_data(cond_data)
      {}

      virtual std::size_t get_data_size() const
      {  return sizeof(sem_counts);   }

      virtual const void *buffer_with_final_data_to_file()
      {
         sem_counts[0] = m_condition_data.m_sem_block_queue.value();
         sem_counts[1] = m_condition_data.m_sem_block_lock.value();
         return &sem_counts;
      }

      virtual const void *buffer_with_init_data_to_file()
      {
         sem_counts[0] = 0;
         sem_counts[1] = 1;
         return &sem_counts;
      }

      virtual void *buffer_to_store_init_data_from_file()
      {  return &sem_counts; }

      virtual bool open(create_enum_t, const char *id_name)
      {
         m_condition_data.m_nwaiters_blocked = 0;
         m_condition_data.m_nwaiters_gone = 0;
         m_condition_data.m_nwaiters_to_unblock = 0;

         //Now open semaphores and mutex.
         //Use local variables + swap to guarantee consistent
         //initialization and cleanup in case any opening fails
         permissions perm;
         perm.set_unrestricted();
         std::string aux_str  = "Global\\bipc.cond.";
         aux_str += id_name;
         std::size_t pos = aux_str.size();

         //sem_block_queue
         aux_str += "_bq";
         winapi_semaphore_wrapper sem_block_queue;
         bool created;
         if(!sem_block_queue.open_or_create
            (aux_str.c_str(), sem_counts[0], winapi_semaphore_wrapper::MaxCount, perm, created))
            return false;
         aux_str.erase(pos);

         //sem_block_lock
         aux_str += "_bl";
         winapi_semaphore_wrapper sem_block_lock;
         if(!sem_block_lock.open_or_create
            (aux_str.c_str(), sem_counts[1], winapi_semaphore_wrapper::MaxCount, perm, created))
            return false;
         aux_str.erase(pos);

         //mtx_unblock_lock
         aux_str += "_ul";
         winapi_mutex_wrapper mtx_unblock_lock;
         if(!mtx_unblock_lock.open_or_create(aux_str.c_str(), perm))
            return false;

         //All ok, commit data
         m_condition_data.m_sem_block_queue.swap(sem_block_queue);
         m_condition_data.m_sem_block_lock.swap(sem_block_lock);
         m_condition_data.m_mtx_unblock_lock.swap(mtx_unblock_lock);
         return true;
      }

      virtual void close()
      {
         m_condition_data.m_sem_block_queue.close();
         m_condition_data.m_sem_block_lock.close();
         m_condition_data.m_mtx_unblock_lock.close();
         m_condition_data.m_nwaiters_blocked = 0;
         m_condition_data.m_nwaiters_gone = 0;
         m_condition_data.m_nwaiters_to_unblock = 0;
      }

      virtual ~named_cond_callbacks()
      {}

      private:
      condition_data &m_condition_data;
   };

   windows_named_sync   m_named_sync;
   /// @endcond
};

inline windows_named_condition::~windows_named_condition()
{
   named_cond_callbacks callbacks(m_condition_data);
   m_named_sync.close(callbacks);
}

inline void windows_named_condition::dont_close_on_destruction()
{}

inline windows_named_condition::windows_named_condition
   (create_only_t, const char *name, const permissions &perm)
   : m_condition_data()
{
   named_cond_callbacks callbacks(m_condition_data);
   m_named_sync.open_or_create(DoCreate, name, perm, callbacks);
}

inline windows_named_condition::windows_named_condition
   (open_or_create_t, const char *name, const permissions &perm)
   : m_condition_data()
{
   named_cond_callbacks callbacks(m_condition_data);
   m_named_sync.open_or_create(DoOpenOrCreate, name, perm, callbacks);
}

inline windows_named_condition::windows_named_condition(open_only_t, const char *name)
   : m_condition_data()
{
   named_cond_callbacks callbacks(m_condition_data);
   m_named_sync.open_or_create(DoOpen, name, permissions(), callbacks);
}

inline void windows_named_condition::notify_one()
{  algorithm_type::signal(m_condition_data, false);  }

inline void windows_named_condition::notify_all()
{  algorithm_type::signal(m_condition_data, true);  }

template<class InterprocessMutex>
inline void windows_named_condition::do_wait(InterprocessMutex &mut)
{  algorithm_type::wait(m_condition_data, false, boost::posix_time::ptime(), mut);  }

template<class InterprocessMutex>
inline bool windows_named_condition::do_timed_wait
   (const boost::posix_time::ptime &abs_time, InterprocessMutex &mut)
{  return algorithm_type::wait(m_condition_data, true, abs_time, mut);  }

template <typename L>
inline void windows_named_condition::wait(L& lock)
{
   if (!lock)
      throw lock_exception();
   this->do_wait(*lock.mutex());
}

template <typename L, typename Pr>
inline void windows_named_condition::wait(L& lock, Pr pred)
{
   if (!lock)
      throw lock_exception();
   while (!pred())
      this->do_wait(*lock.mutex());
}

template <typename L>
inline bool windows_named_condition::timed_wait
   (L& lock, const boost::posix_time::ptime &abs_time)
{
   if(abs_time == boost::posix_time::pos_infin){
      this->wait(lock);
      return true;
   }
   if (!lock)
      throw lock_exception();
   return this->do_timed_wait(abs_time, *lock.mutex());
}

template <typename L, typename Pr>
inline bool windows_named_condition::timed_wait
   (L& lock, const boost::posix_time::ptime &abs_time, Pr pred)
{
   if(abs_time == boost::posix_time::pos_infin){
      this->wait(lock, pred);
      return true;
   }
   if (!lock)
      throw lock_exception();

   while (!pred()){
      if(!this->do_timed_wait(abs_time, *lock.mutex())){
         return pred();
      }
   }
   return true;
}

inline bool windows_named_condition::remove(const char *name)
{
   return windows_named_sync::remove(name);
}

}  //namespace ipcdetail {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_WINDOWS_NAMED_CONDITION_HPP
