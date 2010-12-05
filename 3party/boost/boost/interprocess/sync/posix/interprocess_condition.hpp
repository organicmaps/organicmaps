//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/sync/posix/ptime_to_timespec.hpp>
#include <boost/interprocess/detail/posix_time_types_wrk.hpp>

namespace boost {

namespace interprocess {

inline interprocess_condition::interprocess_condition()
{
   int res;
   pthread_condattr_t cond_attr;
   res = pthread_condattr_init(&cond_attr);
   if(res != 0){
      throw interprocess_exception();
   }
   res = pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
   if(res != 0){
      pthread_condattr_destroy(&cond_attr);
      throw interprocess_exception(res);
   }
   res = pthread_cond_init(&m_condition, &cond_attr);
   pthread_condattr_destroy(&cond_attr);
   if(res != 0){
      throw interprocess_exception(res);
   }
}

inline interprocess_condition::~interprocess_condition()
{
    int res = 0;
    res = pthread_cond_destroy(&m_condition);
    assert(res == 0);
}

inline void interprocess_condition::notify_one()
{
    int res = 0;
    res = pthread_cond_signal(&m_condition);
    assert(res == 0);
}

inline void interprocess_condition::notify_all()
{
    int res = 0;
    res = pthread_cond_broadcast(&m_condition);
    assert(res == 0);
}

inline void interprocess_condition::do_wait(interprocess_mutex &mut)
{
   pthread_mutex_t* pmutex = &mut.m_mut;
   int res = 0;
   res = pthread_cond_wait(&m_condition, pmutex);
   assert(res == 0);
}

inline bool interprocess_condition::do_timed_wait
   (const boost::posix_time::ptime &abs_time, interprocess_mutex &mut)
{
   timespec ts = detail::ptime_to_timespec(abs_time);
   pthread_mutex_t* pmutex = &mut.m_mut;
   int res = 0;
   res = pthread_cond_timedwait(&m_condition, pmutex, &ts);
   assert(res == 0 || res == ETIMEDOUT);

   return res != ETIMEDOUT;
}

}  //namespace interprocess

}  // namespace boost
