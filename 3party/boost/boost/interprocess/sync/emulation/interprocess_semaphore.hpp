//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include<boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/posix_time_types_wrk.hpp>

namespace boost {
namespace interprocess {


inline interprocess_semaphore::~interprocess_semaphore()
{}

inline interprocess_semaphore::interprocess_semaphore(unsigned int initialCount)
{  detail::atomic_write32(&this->m_count, boost::uint32_t(initialCount));  }

inline void interprocess_semaphore::post()
{
   detail::atomic_inc32(&m_count);
}

inline void interprocess_semaphore::wait()
{
   while(!detail::atomic_add_unless32(&m_count, boost::uint32_t(-1), boost::uint32_t(0))){
      while(detail::atomic_read32(&m_count) == 0){
         detail::thread_yield();
      }
   }
}

inline bool interprocess_semaphore::try_wait()
{
   return detail::atomic_add_unless32(&m_count, boost::uint32_t(-1), boost::uint32_t(0));
}

inline bool interprocess_semaphore::timed_wait(const boost::posix_time::ptime &abs_time)
{
   if(abs_time == boost::posix_time::pos_infin){
      this->wait();
      return true;
   }
   //Obtain current count and target time
   boost::posix_time::ptime now(microsec_clock::universal_time());
   if(now >= abs_time)
      return false;

   do{
      if(this->try_wait()){
         break;
      }
      now = microsec_clock::universal_time();

      if(now >= abs_time){
         return this->try_wait();
      }
      // relinquish current time slice
      detail::thread_yield();
   }while (true);
   return true;
}
/*
inline int interprocess_semaphore::get_count() const
{
   return (int)detail::atomic_read32(&m_count);
}*/

}  //namespace interprocess {
}  //namespace boost {
