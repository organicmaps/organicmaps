/* Copyright 2006-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#ifndef BOOST_FLYWEIGHT_DETAIL_RECURSIVE_LW_MUTEX_HPP
#define BOOST_FLYWEIGHT_DETAIL_RECURSIVE_LW_MUTEX_HPP

#if defined(_MSC_VER)&&(_MSC_VER>=1200)
#pragma once
#endif

/* Recursive lightweight mutex. Relies entirely on
 * boost::detail::lightweight_mutex, except in Pthreads, where we
 * explicitly use the PTHREAD_MUTEX_RECURSIVE attribute
 * (lightweight_mutex uses the default mutex type instead).
 */

#include <boost/config.hpp>

#if !defined(BOOST_HAS_PTHREADS)
#include <boost/detail/lightweight_mutex.hpp>
namespace boost{

namespace flyweights{

namespace detail{

typedef boost::detail::lightweight_mutex recursive_lightweight_mutex;

} /* namespace flyweights::detail */

} /* namespace flyweights */

} /* namespace boost */
#else
/* code shamelessly ripped from <boost/detail/lwm_pthreads.hpp> */

#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace boost{

namespace flyweights{

namespace detail{

struct recursive_lightweight_mutex:noncopyable
{
  recursive_lightweight_mutex()
  {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&m_,&attr);
    pthread_mutexattr_destroy(&attr);
  }

  ~recursive_lightweight_mutex(){pthread_mutex_destroy(&m_);}

  struct scoped_lock;
  friend struct scoped_lock;
  struct scoped_lock:noncopyable
  {
  public:
    scoped_lock(recursive_lightweight_mutex& m):m_(m.m_)
    {
        pthread_mutex_lock(&m_);
    }

    ~scoped_lock(){pthread_mutex_unlock(&m_);}

  private:
    pthread_mutex_t& m_;
  };

private:
  pthread_mutex_t m_;
};

} /* namespace flyweights::detail */

} /* namespace flyweights */

} /* namespace boost */
#endif

#endif
