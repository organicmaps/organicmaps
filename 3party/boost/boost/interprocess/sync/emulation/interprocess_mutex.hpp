//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/posix_time_types_wrk.hpp>

namespace boost {

namespace interprocess {

inline interprocess_mutex::interprocess_mutex() 
   : m_s(0) 
{
   //Note that this class is initialized to zero.
   //So zeroed memory can be interpreted as an
   //initialized mutex
}

inline interprocess_mutex::~interprocess_mutex() 
{
   //Trivial destructor
}

inline void interprocess_mutex::lock(void)
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

inline bool interprocess_mutex::try_lock(void)
{
   boost::uint32_t prev_s = detail::atomic_cas32(const_cast<boost::uint32_t*>(&m_s), 1, 0);   
   return m_s == 1 && prev_s == 0;
}

inline bool interprocess_mutex::timed_lock(const boost::posix_time::ptime &abs_time)
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

inline void interprocess_mutex::unlock(void)
{  detail::atomic_cas32(const_cast<boost::uint32_t*>(&m_s), 0, 1);   }

}  //namespace interprocess {

}  //namespace boost {
