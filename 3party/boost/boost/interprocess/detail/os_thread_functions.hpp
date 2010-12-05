//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_OS_THREAD_FUNCTIONS_HPP
#define BOOST_INTERPROCESS_DETAIL_OS_THREAD_FUNCTIONS_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#if (defined BOOST_INTERPROCESS_WINDOWS)
#  include <boost/interprocess/detail/win32_api.hpp>
#else
#  ifdef BOOST_HAS_UNISTD_H
#     include <pthread.h>
#     include <unistd.h>
#     include <sched.h>
#  else
#     error Unknown platform
#  endif
#endif

namespace boost {
namespace interprocess {
namespace detail{

#if (defined BOOST_INTERPROCESS_WINDOWS)

typedef unsigned long OS_process_id_t;
typedef unsigned long OS_thread_id_t;
typedef OS_thread_id_t OS_systemwide_thread_id_t;

//process
inline OS_process_id_t get_current_process_id()
{  return winapi::get_current_process_id();  }

inline OS_process_id_t get_invalid_process_id()
{  return OS_process_id_t(0);  }

//thread
inline OS_thread_id_t get_current_thread_id()
{  return winapi::get_current_thread_id();  }

inline OS_thread_id_t get_invalid_thread_id()
{  return OS_thread_id_t(0xffffffff);  }

inline bool equal_thread_id(OS_thread_id_t id1, OS_thread_id_t id2)
{  return id1 == id2;  }

inline void thread_yield()
{  winapi::sched_yield();  }

//systemwide thread
inline OS_systemwide_thread_id_t get_current_systemwide_thread_id()
{
   return get_current_thread_id();
}

inline void systemwide_thread_id_copy
   (const volatile OS_systemwide_thread_id_t &from, volatile OS_systemwide_thread_id_t &to)
{
   to = from;
}

inline bool equal_systemwide_thread_id(const OS_systemwide_thread_id_t &id1, const OS_systemwide_thread_id_t &id2)
{
   return equal_thread_id(id1, id2);
}

inline OS_systemwide_thread_id_t get_invalid_systemwide_thread_id()
{
   return get_invalid_thread_id();
}

#else    //#if (defined BOOST_INTERPROCESS_WINDOWS)

typedef pthread_t OS_thread_id_t;
typedef pid_t     OS_process_id_t;

struct OS_systemwide_thread_id_t
{  
   OS_systemwide_thread_id_t()
      :  pid(), tid()
   {}

   OS_systemwide_thread_id_t(pid_t p, pthread_t t)
      :  pid(p), tid(t)
   {}

   OS_systemwide_thread_id_t(const OS_systemwide_thread_id_t &x)
      :  pid(x.pid), tid(x.tid)
   {}

   OS_systemwide_thread_id_t(const volatile OS_systemwide_thread_id_t &x)
      :  pid(x.pid), tid(x.tid)
   {}

   OS_systemwide_thread_id_t & operator=(const OS_systemwide_thread_id_t &x)
   {  pid = x.pid;   tid = x.tid;   return *this;   }

   OS_systemwide_thread_id_t & operator=(const volatile OS_systemwide_thread_id_t &x)
   {  pid = x.pid;   tid = x.tid;   return *this;  }

   void operator=(const OS_systemwide_thread_id_t &x) volatile
   {  pid = x.pid;   tid = x.tid;   }

   pid_t       pid;
   pthread_t   tid;
};

inline void systemwide_thread_id_copy
   (const volatile OS_systemwide_thread_id_t &from, volatile OS_systemwide_thread_id_t &to)
{
   to.pid = from.pid;
   to.tid = from.tid;
}

//process
inline OS_process_id_t get_current_process_id()
{  return ::getpid();  }

inline OS_process_id_t get_invalid_process_id()
{  return pid_t(0);  }

//thread
inline OS_thread_id_t get_current_thread_id()
{  return ::pthread_self();  }

inline OS_thread_id_t get_invalid_thread_id()
{ 
   static pthread_t invalid_id;
   return invalid_id;
}

inline bool equal_thread_id(OS_thread_id_t id1, OS_thread_id_t id2)
{  return 0 != pthread_equal(id1, id2);  }

inline void thread_yield()
{  ::sched_yield();  }

//systemwide thread
inline OS_systemwide_thread_id_t get_current_systemwide_thread_id()
{
   return OS_systemwide_thread_id_t(::getpid(), ::pthread_self());
}

inline bool equal_systemwide_thread_id(const OS_systemwide_thread_id_t &id1, const OS_systemwide_thread_id_t &id2)
{
   return (0 != pthread_equal(id1.tid, id2.tid)) && (id1.pid == id2.pid);
}

inline OS_systemwide_thread_id_t get_invalid_systemwide_thread_id()
{
   return OS_systemwide_thread_id_t(get_invalid_process_id(), get_invalid_thread_id());
}

#endif   //#if (defined BOOST_INTERPROCESS_WINDOWS)

}  //namespace detail{
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_OS_THREAD_FUNCTIONS_HPP
