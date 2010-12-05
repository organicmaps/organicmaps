//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006. Distributed under the Boost
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

inline barrier::barrier(unsigned int count)
{
   if (count == 0)
      throw std::invalid_argument("count cannot be zero.");
   detail::barrierattr_wrapper barrier_attr;
   detail::barrier_initializer barrier
      (m_barrier, barrier_attr, static_cast<int>(count));
   barrier.release();
}

inline barrier::~barrier()
{
   int res = pthread_barrier_destroy(&m_barrier);
   assert(res  == 0);(void)res;
}

inline bool barrier::wait()
{
   int res = pthread_barrier_wait(&m_barrier);

   if (res != PTHREAD_BARRIER_SERIAL_THREAD && res != 0){
      throw interprocess_exception(res);
   }
   return res == PTHREAD_BARRIER_SERIAL_THREAD;
}

}  //namespace interprocess {
}  //namespace boost {
