//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_EMULATION_MUTEX_HPP
#define BOOST_INTERPROCESS_DETAIL_EMULATION_MUTEX_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/posix_time_types_wrk.hpp>
#include <boost/assert.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/cstdint.hpp>
#include <boost/interprocess/detail/os_thread_functions.hpp>

namespace boost {
namespace interprocess {
namespace detail {

class emulation_mutex
{
   emulation_mutex(const emulation_mutex &);
   emulation_mutex &operator=(const emulation_mutex &);
   public:

   emulation_mutex();
   ~emulation_mutex();

   void lock();
   bool try_lock();
   bool timed_lock(const boost::posix_time::ptime &abs_time);
   void unlock();
   void take_ownership(){};
   private:
   volatile boost::uint32_t m_s;
};

inline emulation_mutex::emulation_mutex() 
   : m_s(0) 
{
   //Note that this class is initialized to zero.
   //So zeroed memory can be interpreted as an
   //initialized mutex
}

inline emulation_mutex::~emulation_mutex() 
{
   //Trivial destructor
}

inline void emulation_mutex::lock(void)
{
   do{
      boost::uint32_t prev_s = detail::atomic_cas32(const_cast<boost::uint32_t*>(&m_s), 1, 0);

      if (m_s == 1 && prev_s == 0){
            break;
      }
      // relinquish current timeslice
      detail::thread_yield();
   }while (true);
}

inline bool emulation_mutex::try_lock(void)
{
   boost::uint32_t prev_s = detail::atomic_cas32(const_cast<boost::uint32_t*>(&m_s), 1, 0);   
   return m_s == 1 && prev_s == 0;
}

inline bool emulation_mutex::timed_lock(const boost::posix_time::ptime &abs_time)
{
   if(abs_time == boost::posix_time::pos_infin){
      this->lock();
      return true;
   }
   //Obtain current count and target time
   boost::posix_time::ptime now = microsec_clock::universal_time();

   if(now >= abs_time) return false;

   do{
      if(this->try_lock()){
         break;
      }
      now = microsec_clock::universal_time();

      if(now >= abs_time){
         return false;
      }
      // relinquish current time slice
     detail::thread_yield();
   }while (true);

   return true;
}

inline void emulation_mutex::unlock(void)
{  detail::atomic_cas32(const_cast<boost::uint32_t*>(&m_s), 0, 1);   }

}  //namespace detail {
}  //namespace interprocess {
}  //namespace boost {

#endif   //BOOST_INTERPROCESS_DETAIL_EMULATION_MUTEX_HPP
