// Modified by Yury Melnichek (melnichek@gmail.com).

/*
 * Copyright (C) 2004-2008 Wu Yongwei <adah at users dot sourceforge dot net>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any
 * damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute
 * it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must
 *    not claim that you wrote the original software.  If you use this
 *    software in a product, an acknowledgement in the product
 *    documentation would be appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must
 *    not be misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source
 *    distribution.
 *
 * This file is part of Stones of Nvwa:
 *      http://sourceforge.net/projects/nvwa
 *
 */

/**
 * @file    fast_mutex.h
 *
 * A fast mutex implementation for POSIX and Win32.
 *
 * @version 1.18, 2005/05/06
 * @author  Wu Yongwei
 *
 */

#ifndef _FAST_MUTEX_H
#define _FAST_MUTEX_H

# if !defined(_NOTHREADS)
#   if !defined(_WIN32THREADS) && \
            (defined(_WIN32) && defined(_MT))
//      Automatically use _WIN32THREADS when specifying -MT/-MD in MSVC,
//      or -mthreads in MinGW GCC.
#       define _WIN32THREADS
#   elif !defined(_PTHREADS) && \
            defined(_REENTRANT)
//      Automatically use _PTHREADS when specifying -pthread in GCC.
//      N.B. I do not detect on _PTHREAD_H since libstdc++-v3 under
//      Linux will silently include <pthread.h> anyway.
#       define _PTHREADS
#   endif
# endif

# if !defined(_PTHREADS) && !defined(_WIN32THREADS) && !defined(_NOTHREADS)
#   define _NOTHREADS
# endif

# if defined(_NOTHREADS)
#   if defined(_PTHREADS) || defined(_WIN32THREADS)
#       undef _NOTHREADS
#       error "Cannot define multi-threaded mode with -D_NOTHREADS"
#       if defined(__MINGW32__) && defined(_WIN32THREADS) && !defined(_MT)
#           error "Be sure to specify -mthreads with -D_WIN32THREADS"
#       endif
#   endif
# endif

# ifndef _FAST_MUTEX_CHECK_INITIALIZATION
/**
 * Macro to control whether to check for initialization status for each
 * lock/unlock operation.  Defining it to a non-zero value will enable
 * the check, so that the construction/destruction of a static object
 * using a static fast_mutex not yet constructed or already destroyed
 * will work (with lock/unlock operations ignored).  Defining it to zero
 * will disable to check.
 */
#   define _FAST_MUTEX_CHECK_INITIALIZATION 1
# endif

# if defined(_PTHREADS) && defined(_WIN32THREADS)
//  Some C++ libraries have _PTHREADS defined even on Win32 platforms.
//  Thus this hack.
#   undef _PTHREADS
# endif

# ifdef _DEBUG
#   include <stdio.h>
#   include <stdlib.h>
/** Macro for fast_mutex assertions.  Real version (for debug mode). */
#   define _FAST_MUTEX_ASSERT(_Expr, _Msg) \
        if (!(_Expr)) { \
            fprintf(stderr, "fast_mutex::%s\n", _Msg); \
            abort(); \
        }
# else
/** Macro for fast_mutex assertions.  Fake version (for release mode). */
#   define _FAST_MUTEX_ASSERT(_Expr, _Msg) \
        ((void)0)
# endif

# ifdef _PTHREADS
#   include <pthread.h>
/**
 * Macro alias to `volatile' semantics.  Here it is truly volatile since
 * it is in a multi-threaded (POSIX threads) environment.
 */
#   define __VOLATILE volatile
    /**
     * Class for non-reentrant fast mutexes.  This is the implementation
     * for POSIX threads.
     */
    class fast_mutex
    {
        pthread_mutex_t _M_mtx_impl;
#       if _FAST_MUTEX_CHECK_INITIALIZATION
        bool _M_initialized;
#       endif
#       ifdef _DEBUG
        bool _M_locked;
#       endif
    public:
        fast_mutex()
#       ifdef _DEBUG
            : _M_locked(false)
#       endif
        {
            ::pthread_mutex_init(&_M_mtx_impl, NULL);
#       if _FAST_MUTEX_CHECK_INITIALIZATION
            _M_initialized = true;
#       endif
        }
        ~fast_mutex()
        {
            _FAST_MUTEX_ASSERT(!_M_locked, "~fast_mutex(): still locked");
#       if _FAST_MUTEX_CHECK_INITIALIZATION
            _M_initialized = false;
#       endif
            ::pthread_mutex_destroy(&_M_mtx_impl);
        }
        void lock()
        {
#       if _FAST_MUTEX_CHECK_INITIALIZATION
            if (!_M_initialized)
                return;
#       endif
            ::pthread_mutex_lock(&_M_mtx_impl);
#       ifdef _DEBUG
            // The following assertion should _always_ be true for a
            // real `fast' pthread_mutex.  However, this assertion can
            // help sometimes, when people forget to use `-lpthread' and
            // glibc provides an empty implementation.  Having this
            // assertion is also more consistent.
            _FAST_MUTEX_ASSERT(!_M_locked, "lock(): already locked");
            _M_locked = true;
#       endif
        }
        void unlock()
        {
#       if _FAST_MUTEX_CHECK_INITIALIZATION
            if (!_M_initialized)
                return;
#       endif
#       ifdef _DEBUG
            _FAST_MUTEX_ASSERT(_M_locked, "unlock(): not locked");
            _M_locked = false;
#       endif
            ::pthread_mutex_unlock(&_M_mtx_impl);
        }
    private:
        fast_mutex(const fast_mutex&);
        fast_mutex& operator=(const fast_mutex&);
    };
# endif // _PTHREADS

# ifdef _WIN32THREADS
#   include "../../std/windows.hpp"
/**
 * Macro alias to `volatile' semantics.  Here it is truly volatile since
 * it is in a multi-threaded (Win32 threads) environment.
 */
#   define __VOLATILE volatile
    /**
     * Class for non-reentrant fast mutexes.  This is the implementation
     * for Win32 threads.
     */
    class fast_mutex
    {
        CRITICAL_SECTION _M_mtx_impl;
#       if _FAST_MUTEX_CHECK_INITIALIZATION
        bool _M_initialized;
#       endif
#       ifdef _DEBUG
        bool _M_locked;
#       endif
    public:
        fast_mutex()
#       ifdef _DEBUG
            : _M_locked(false)
#       endif
        {
            ::InitializeCriticalSection(&_M_mtx_impl);
#       if _FAST_MUTEX_CHECK_INITIALIZATION
            _M_initialized = true;
#       endif
        }
        ~fast_mutex()
        {
            _FAST_MUTEX_ASSERT(!_M_locked, "~fast_mutex(): still locked");
#       if _FAST_MUTEX_CHECK_INITIALIZATION
            _M_initialized = false;
#       endif
            ::DeleteCriticalSection(&_M_mtx_impl);
        }
        void lock()
        {
#       if _FAST_MUTEX_CHECK_INITIALIZATION
            if (!_M_initialized)
                return;
#       endif
            ::EnterCriticalSection(&_M_mtx_impl);
#       ifdef _DEBUG
            _FAST_MUTEX_ASSERT(!_M_locked, "lock(): already locked");
            _M_locked = true;
#       endif
        }
        void unlock()
        {
#       if _FAST_MUTEX_CHECK_INITIALIZATION
            if (!_M_initialized)
                return;
#       endif
#       ifdef _DEBUG
            _FAST_MUTEX_ASSERT(_M_locked, "unlock(): not locked");
            _M_locked = false;
#       endif
            ::LeaveCriticalSection(&_M_mtx_impl);
        }
    private:
        fast_mutex(const fast_mutex&);
        fast_mutex& operator=(const fast_mutex&);
    };
# endif // _WIN32THREADS

# ifdef _NOTHREADS
/**
 * Macro alias to `volatile' semantics.  Here it is not truly volatile
 * since it is in a single-threaded environment.
 */
#   define __VOLATILE
    /**
     * Class for non-reentrant fast mutexes.  This is the null
     * implementation for single-threaded environments.
     */
    class fast_mutex
    {
#       ifdef _DEBUG
        bool _M_locked;
#       endif
    public:
        fast_mutex()
#       ifdef _DEBUG
            : _M_locked(false)
#       endif
        {
        }
        ~fast_mutex()
        {
            _FAST_MUTEX_ASSERT(!_M_locked, "~fast_mutex(): still locked");
        }
        void lock()
        {
#       ifdef _DEBUG
            _FAST_MUTEX_ASSERT(!_M_locked, "lock(): already locked");
            _M_locked = true;
#       endif
        }
        void unlock()
        {
#       ifdef _DEBUG
            _FAST_MUTEX_ASSERT(_M_locked, "unlock(): not locked");
            _M_locked = false;
#       endif
        }
    private:
        fast_mutex(const fast_mutex&);
        fast_mutex& operator=(const fast_mutex&);
    };
# endif // _NOTHREADS

/** An acquistion-on-initialization lock class based on fast_mutex. */
class fast_mutex_autolock
{
    fast_mutex& _M_mtx;
public:
    explicit fast_mutex_autolock(fast_mutex& __mtx) : _M_mtx(__mtx)
    {
        _M_mtx.lock();
    }
    ~fast_mutex_autolock()
    {
        _M_mtx.unlock();
    }
private:
    fast_mutex_autolock(const fast_mutex_autolock&);
    fast_mutex_autolock& operator=(const fast_mutex_autolock&);
};

#endif // _FAST_MUTEX_H
