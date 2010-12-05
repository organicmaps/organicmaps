//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
//
// barrier is a modified version of Boost Threads barrier
//
//////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2002-2003
// David Moore, William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.

#ifndef BOOST_INTERPROCESS_BARRIER_HPP
#define BOOST_INTERPROCESS_BARRIER_HPP

/// @cond

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#if defined BOOST_INTERPROCESS_POSIX_PROCESS_SHARED && defined BOOST_INTERPROCESS_POSIX_BARRIERS
#  include <pthread.h>
#  include <errno.h>   
#  include <boost/interprocess/sync/posix/pthread_helpers.hpp>
#  define BOOST_INTERPROCESS_USE_POSIX
#else
#  include <boost/interprocess/sync/interprocess_mutex.hpp>
#  include <boost/interprocess/sync/scoped_lock.hpp>
#  include <boost/interprocess/sync/interprocess_condition.hpp>
#  include <stdexcept>
#  define BOOST_INTERPROCESS_USE_GENERIC_EMULATION
#endif

#  include <boost/interprocess/exceptions.hpp>

/// @endcond

namespace boost {
namespace interprocess {

//!An object of class barrier is a synchronization primitive that 
//!can be placed in shared memory used to cause a set of threads from 
//!different processes to wait until they each perform a certain 
//!function or each reach a particular point in their execution.
class barrier
{
   public:
   //!Constructs a barrier object that will cause count threads 
   //!to block on a call to wait().
   barrier(unsigned int count);

   //!Destroys *this. If threads are still executing their wait() 
   //!operations, the behavior for these threads is undefined.
   ~barrier();

   //!Effects: Wait until N threads call wait(), where N equals the count 
   //!provided to the constructor for the barrier object.
   //!Note that if the barrier is destroyed before wait() can return, 
   //!the behavior is undefined.
   //!Returns: Exactly one of the N threads will receive a return value 
   //!of true, the others will receive a value of false. Precisely which 
   //!thread receives the return value of true will be implementation-defined. 
   //!Applications can use this value to designate one thread as a leader that 
   //!will take a certain action, and the other threads emerging from the barrier 
   //!can wait for that action to take place.
   bool wait();

   /// @cond
   private:
   #if defined(BOOST_INTERPROCESS_USE_GENERIC_EMULATION)
      interprocess_mutex m_mutex;
      interprocess_condition m_cond;
      unsigned int m_threshold;
      unsigned int m_count;
      unsigned int m_generation;
   #else //#if defined BOOST_INTERPROCESS_USE_POSIX
      pthread_barrier_t    m_barrier;
   #endif//#if defined BOOST_INTERPROCESS_USE_POSIX
   /// @endcond
};

}  // namespace interprocess
}  // namespace boost


#ifdef BOOST_INTERPROCESS_USE_GENERIC_EMULATION
#  undef BOOST_INTERPROCESS_USE_GENERIC_EMULATION
#  include <boost/interprocess/sync/emulation/interprocess_barrier.hpp>
#endif

#ifdef BOOST_INTERPROCESS_USE_POSIX
#  undef BOOST_INTERPROCESS_USE_POSIX
#  include <boost/interprocess/sync/posix/interprocess_barrier.hpp>
#endif

#include <boost/interprocess/detail/config_end.hpp>

#endif
