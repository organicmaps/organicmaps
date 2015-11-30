#pragma once

#include "base/assert.hpp"

#include "std/target_os.hpp"

#if defined(OMIM_OS_WINDOWS_NATIVE)
  #include "std/windows.hpp"
#else
  #include <pthread.h>
#endif

namespace threads
{
  class Condition;
  namespace impl
  {
    class ConditionImpl;
    class ImplWinVista;
  }

  /// Mutex primitive, used only for synchronizing this process threads
  /// based on Critical Section under Win32 and pthreads under Linux
  /// @author Siarhei Rachytski
  /// @deprecated As the MacOS implementation doesn't support recursive mutexes we should emulate them by ourselves.
  /// The code is taken from @a http://www.omnigroup.com/mailman/archive/macosx-dev/2002-March/036465.html
  class Mutex
  {
  private:

#if defined(OMIM_OS_WINDOWS_NATIVE)
    CRITICAL_SECTION m_Mutex;
#else
    pthread_mutex_t m_Mutex;
#endif

    Mutex & operator=(Mutex const &);
    Mutex(Mutex const &);

    friend class threads::impl::ConditionImpl;
    friend class threads::impl::ImplWinVista;
    friend class threads::Condition;

  public:

    Mutex()
    {
#if defined(OMIM_OS_WINDOWS_NATIVE)
      ::InitializeCriticalSection(&m_Mutex);
#else
      ::pthread_mutex_init(&m_Mutex, 0);
#endif
    }

    ~Mutex()
    {
#if defined(OMIM_OS_WINDOWS_NATIVE)
      ::DeleteCriticalSection(&m_Mutex);
#else
      ::pthread_mutex_destroy(&m_Mutex);
#endif
    }
    
    void Lock()
    {
#if defined(OMIM_OS_WINDOWS_NATIVE)
      ::EnterCriticalSection(&m_Mutex);
#else
      VERIFY(0 == ::pthread_mutex_lock(&m_Mutex), ());
#endif
    }

    bool TryLock()
    {
#if defined(OMIM_OS_WINDOWS_NATIVE)
      return (TRUE == ::TryEnterCriticalSection(&m_Mutex));
#else
      return (0 == ::pthread_mutex_trylock(&m_Mutex));
#endif
    }

    void Unlock()
    {
#if defined(OMIM_OS_WINDOWS_NATIVE)
      ::LeaveCriticalSection(&m_Mutex);
#else
      VERIFY(0 == ::pthread_mutex_unlock(&m_Mutex), ());
#endif
    }

  };

  /// ScopeGuard wrapper around mutex
  class MutexGuard
  {
  public:
  	MutexGuard(Mutex & mutex): m_Mutex(mutex) { m_Mutex.Lock(); }
  	~MutexGuard() { m_Mutex.Unlock(); }
  private:
    Mutex & m_Mutex;
  };
  
} // namespace threads
