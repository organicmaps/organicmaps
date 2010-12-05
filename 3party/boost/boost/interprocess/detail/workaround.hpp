//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_WORKAROUND_HPP
#define BOOST_INTERPROCESS_DETAIL_WORKAROUND_HPP

#include <boost/interprocess/detail/config_begin.hpp>

#if (defined(_WIN32) || defined(__WIN32__) || defined(WIN32))

#define BOOST_INTERPROCESS_WINDOWS

/*
#if !defined(_MSC_EXTENSIONS)
#error "Turn on Microsoft language extensions (_MSC_EXTENSIONS) to be able to call Windows API functions"
#endif
*/

#endif

#if !(defined BOOST_INTERPROCESS_WINDOWS)

   #include <unistd.h>

   #if ((_POSIX_THREAD_PROCESS_SHARED - 0) > 0)
   //Cygwin defines _POSIX_THREAD_PROCESS_SHARED but does not implement it.
   //Mac Os X >= Leopard defines _POSIX_THREAD_PROCESS_SHARED but does not seems to work.
   #  if !defined(__CYGWIN__) && !defined(__APPLE__)
   #  define BOOST_INTERPROCESS_POSIX_PROCESS_SHARED
   #  endif
   #endif
   
   #if ((_POSIX_BARRIERS - 0) > 0)
   # define BOOST_INTERPROCESS_POSIX_BARRIERS
   # endif

   #if ((_POSIX_SEMAPHORES - 0) > 0)
   # define BOOST_INTERPROCESS_POSIX_NAMED_SEMAPHORES
   #  if defined(__CYGWIN__)
      #define BOOST_INTERPROCESS_POSIX_SEMAPHORES_NO_UNLINK
   #  endif
   //#elif defined(__APPLE__)
   //# define BOOST_INTERPROCESS_POSIX_NAMED_SEMAPHORES   
   #endif 

   #if ((defined _V6_ILP32_OFFBIG)  &&(_V6_ILP32_OFFBIG   - 0 > 0)) ||\
       ((defined _V6_LP64_OFF64)    &&(_V6_LP64_OFF64     - 0 > 0)) ||\
       ((defined _V6_LPBIG_OFFBIG)  &&(_V6_LPBIG_OFFBIG   - 0 > 0)) ||\
       ((defined _XBS5_ILP32_OFFBIG)&&(_XBS5_ILP32_OFFBIG - 0 > 0)) ||\
       ((defined _XBS5_LP64_OFF64)  &&(_XBS5_LP64_OFF64   - 0 > 0)) ||\
       ((defined _XBS5_LPBIG_OFFBIG)&&(_XBS5_LPBIG_OFFBIG - 0 > 0)) ||\
       ((defined _FILE_OFFSET_BITS) &&(_FILE_OFFSET_BITS  - 0 >= 64))||\
       ((defined _FILE_OFFSET_BITS) &&(_FILE_OFFSET_BITS  - 0 >= 64))
      #define BOOST_INTERPROCESS_UNIX_64_BIT_OR_BIGGER_OFF_T
   #else
   #endif

   //Check for XSI shared memory objects. They are available in nearly all UNIX platforms
   #if !defined(__QNXNTO__)
   # define BOOST_INTERPROCESS_XSI_SHARED_MEMORY_OBJECTS
   #endif

   #if ((_POSIX_SHARED_MEMORY_OBJECTS - 0) > 0)
   # define BOOST_INTERPROCESS_POSIX_SHARED_MEMORY_OBJECTS
   #else
   //VMS and MACOS don't define it but the have shm_open/close interface
   # if defined(__vms)
   #  if __CRTL_VER >= 70200000
   #  define BOOST_INTERPROCESS_POSIX_SHARED_MEMORY_OBJECTS
   #  endif
   //Mac OS has some non-conformant features like names limited to SHM_NAME_MAX
   # elif defined (__APPLE__)
//   #  define BOOST_INTERPROCESS_POSIX_SHARED_MEMORY_OBJECTS
//   #  define BOOST_INTERPROCESS_POSIX_SHARED_MEMORY_OBJECTS_NO_GROW
   # endif 
   #endif

   //Now check if we have only XSI shared memory
   #if defined(BOOST_INTERPROCESS_XSI_SHARED_MEMORY_OBJECTS) &&\
      !defined(BOOST_INTERPROCESS_POSIX_SHARED_MEMORY_OBJECTS)
   //#  define BOOST_INTERPROCESS_XSI_SHARED_MEMORY_OBJECTS_ONLY
   #endif

   #if ((_POSIX_TIMEOUTS - 0) > 0)
   # define BOOST_INTERPROCESS_POSIX_TIMEOUTS
   #endif 


   #ifdef BOOST_INTERPROCESS_POSIX_SHARED_MEMORY_OBJECTS
      //Some systems have filesystem-based resources, so the
      //portable "/shmname" format does not work due to permission issues
      //For those systems we need to form a path to a temporary directory:
      //          hp-ux               tru64               vms               freebsd
      #if defined(__hpux) || defined(__osf__) || defined(__vms) || (defined(__FreeBSD__) && (__FreeBSD__ < 8)) 
      #define BOOST_INTERPROCESS_FILESYSTEM_BASED_POSIX_SHARED_MEMORY
      #endif
   #endif

   #ifdef BOOST_INTERPROCESS_POSIX_NAMED_SEMAPHORES
      #if defined(__osf__) || defined(__vms)
      #define BOOST_INTERPROCESS_FILESYSTEM_BASED_POSIX_SEMAPHORES
      #endif
   #endif

   #if ((_POSIX_VERSION + 0)>= 200112L || (_XOPEN_VERSION + 0)>= 500)
   #define BOOST_INTERPROCESS_POSIX_RECURSIVE_MUTEXES
   #endif

#endif

#if     defined(BOOST_HAS_RVALUE_REFS) && defined(BOOST_HAS_VARIADIC_TMPL)\
    && !defined(BOOST_INTERPROCESS_DISABLE_VARIADIC_TMPL)
#define BOOST_INTERPROCESS_PERFECT_FORWARDING
#endif

//Now declare some Boost.Interprocess features depending on the implementation

#if defined(BOOST_INTERPROCESS_POSIX_NAMED_SEMAPHORES) && !defined(BOOST_INTERPROCESS_POSIX_SEMAPHORES_NO_UNLINK)

#define BOOST_INTERPROCESS_NAMED_MUTEX_USES_POSIX_SEMAPHORES

#endif

#if defined(BOOST_INTERPROCESS_POSIX_NAMED_SEMAPHORES) && !defined(BOOST_INTERPROCESS_POSIX_SEMAPHORES_NO_UNLINK)

#define BOOST_INTERPROCESS_NAMED_MUTEX_USES_POSIX_SEMAPHORES
#define BOOST_INTERPROCESS_NAMED_SEMAPHORE_USES_POSIX_SEMAPHORES

#endif

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_DETAIL_WORKAROUND_HPP
