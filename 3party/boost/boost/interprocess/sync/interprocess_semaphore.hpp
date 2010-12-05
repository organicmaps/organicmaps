//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SEMAPHORE_HPP
#define BOOST_INTERPROCESS_SEMAPHORE_HPP

/// @cond

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/posix_time_types_wrk.hpp>

#if !defined(BOOST_INTERPROCESS_FORCE_GENERIC_EMULATION) && \
   (defined(BOOST_INTERPROCESS_POSIX_PROCESS_SHARED) && defined(BOOST_INTERPROCESS_POSIX_NAMED_SEMAPHORES))
   #include <fcntl.h>      //O_CREAT, O_*... 
   #include <unistd.h>     //close
   #include <string>       //std::string
   #include <semaphore.h>  //sem_* family, SEM_VALUE_MAX
   #include <sys/stat.h>   //mode_t, S_IRWXG, S_IRWXO, S_IRWXU,
   #include <boost/interprocess/sync/posix/semaphore_wrapper.hpp>
   #define BOOST_INTERPROCESS_USE_POSIX
#else
   #include <boost/interprocess/detail/atomic.hpp>
   #include <boost/cstdint.hpp>
   #include <boost/interprocess/detail/os_thread_functions.hpp>
   #define BOOST_INTERPROCESS_USE_GENERIC_EMULATION
#endif

/// @endcond

//!\file
//!Describes a interprocess_semaphore class for inter-process synchronization

namespace boost {

namespace interprocess {

//!Wraps a interprocess_semaphore that can be placed in shared memory and can be 
//!shared between processes. Allows timed lock tries
class interprocess_semaphore
{
   /// @cond
   //Non-copyable
   interprocess_semaphore(const interprocess_semaphore &);
   interprocess_semaphore &operator=(const interprocess_semaphore &);
   /// @endcond
   public:
   //!Creates a interprocess_semaphore with the given initial count. 
   //!interprocess_exception if there is an error.*/
   interprocess_semaphore(unsigned int initialCount);

   //!Destroys the interprocess_semaphore.
   //!Does not throw
   ~interprocess_semaphore();

   //!Increments the interprocess_semaphore count. If there are processes/threads blocked waiting
   //!for the interprocess_semaphore, then one of these processes will return successfully from
   //!its wait function. If there is an error an interprocess_exception exception is thrown.
   void post();

   //!Decrements the interprocess_semaphore. If the interprocess_semaphore value is not greater than zero,
   //!then the calling process/thread blocks until it can decrement the counter. 
   //!If there is an error an interprocess_exception exception is thrown.
   void wait();

   //!Decrements the interprocess_semaphore if the interprocess_semaphore's value is greater than zero
   //!and returns true. If the value is not greater than zero returns false.
   //!If there is an error an interprocess_exception exception is thrown.
   bool try_wait();

   //!Decrements the interprocess_semaphore if the interprocess_semaphore's value is greater
   //!than zero and returns true. Otherwise, waits for the interprocess_semaphore
   //!to the posted or the timeout expires. If the timeout expires, the
   //!function returns false. If the interprocess_semaphore is posted the function
   //!returns true. If there is an error throws sem_exception
   bool timed_wait(const boost::posix_time::ptime &abs_time);

   //!Returns the interprocess_semaphore count
//   int get_count() const;
   /// @cond
   private:
   #if defined(BOOST_INTERPROCESS_USE_GENERIC_EMULATION)
   volatile boost::uint32_t m_count;
   #else 
   detail::semaphore_wrapper m_sem;
   #endif   //#if defined(BOOST_INTERPROCESS_USE_GENERIC_EMULATION)
   /// @endcond
};

}  //namespace interprocess {

}  //namespace boost {

#ifdef BOOST_INTERPROCESS_USE_GENERIC_EMULATION
#  undef BOOST_INTERPROCESS_USE_GENERIC_EMULATION
#  include <boost/interprocess/sync/emulation/interprocess_semaphore.hpp>
#endif

#ifdef BOOST_INTERPROCESS_USE_POSIX
#  undef BOOST_INTERPROCESS_USE_POSIX
#  include <boost/interprocess/sync/posix/interprocess_semaphore.hpp>
#endif

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_SEMAPHORE_HPP
