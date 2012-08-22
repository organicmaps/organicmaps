//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2011. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_WINDOWS_CONDITION_HPP
#define BOOST_INTERPROCESS_DETAIL_WINDOWS_CONDITION_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/posix_time_types_wrk.hpp>

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/sync/windows/semaphore.hpp>
#include <boost/interprocess/sync/windows/mutex.hpp>
#include <boost/interprocess/sync/detail/condition_algorithm_8a.hpp>


namespace boost {
namespace interprocess {
namespace ipcdetail {

class windows_condition
{
   windows_condition(const windows_condition &);
   windows_condition &operator=(const windows_condition &);

   public:
   windows_condition();
   ~windows_condition();

   void notify_one();
   void notify_all();

   template <typename L>
   bool timed_wait(L& lock, const boost::posix_time::ptime &abs_time)
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
   bool timed_wait(L& lock, const boost::posix_time::ptime &abs_time, Pr pred)
   {
      if(abs_time == boost::posix_time::pos_infin){
         this->wait(lock, pred);
         return true;
      }
      if (!lock)
         throw lock_exception();
      while (!pred()){
         if (!this->do_timed_wait(abs_time, *lock.mutex()))
            return pred();
      }
      return true;
   }

   template <typename L>
   void wait(L& lock)
   {
      if (!lock)
         throw lock_exception();
      do_wait(*lock.mutex());
   }

   template <typename L, typename Pr>
   void wait(L& lock, Pr pred)
   {
      if (!lock)
         throw lock_exception();

      while (!pred())
         do_wait(*lock.mutex());
   }

   template<class InterprocessMutex>
   void do_wait(InterprocessMutex &mut);

   template<class InterprocessMutex>
   bool do_timed_wait(const boost::posix_time::ptime &abs_time, InterprocessMutex &mut);

   private:

   struct condition_data
   {
      typedef boost::int32_t     integer_type;
      typedef windows_semaphore  semaphore_type;
      typedef windows_mutex      mutex_type;

      condition_data()
         : m_nwaiters_blocked(0)
         , m_nwaiters_gone(0)
         , m_nwaiters_to_unblock(0)
         , m_sem_block_queue(0)
         , m_sem_block_lock(1)
         , m_mtx_unblock_lock()
      {}

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

      boost::int32_t    m_nwaiters_blocked;
      boost::int32_t    m_nwaiters_gone;
      boost::int32_t    m_nwaiters_to_unblock;
      windows_semaphore m_sem_block_queue;
      windows_semaphore m_sem_block_lock;
      windows_mutex     m_mtx_unblock_lock;
   } m_condition_data;

   typedef condition_algorithm_8a<condition_data> algorithm_type;
};

inline windows_condition::windows_condition()
   : m_condition_data()
{}

inline windows_condition::~windows_condition()
{}

inline void windows_condition::notify_one()
{  algorithm_type::signal(m_condition_data, false);  }

inline void windows_condition::notify_all()
{  algorithm_type::signal(m_condition_data, true);  }

template<class InterprocessMutex>
inline void windows_condition::do_wait(InterprocessMutex &mut)
{  algorithm_type::wait(m_condition_data, false, boost::posix_time::ptime(), mut);  }

template<class InterprocessMutex>
inline bool windows_condition::do_timed_wait
   (const boost::posix_time::ptime &abs_time, InterprocessMutex &mut)
{  return algorithm_type::wait(m_condition_data, true, abs_time, mut);  }

}  //namespace ipcdetail
}  //namespace interprocess
}  //namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_WINDOWS_CONDITION_HPP
