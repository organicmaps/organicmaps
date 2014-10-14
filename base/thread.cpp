#include "thread.hpp"
#include "assert.hpp"

#if !defined(OMIM_OS_WINDOWS_NATIVE)
  #include <pthread.h>
  #if defined (OMIM_OS_ANDROID)
    /// External implementations are in android/jni code
    void AndroidThreadAttachToJVM();
    void AndroidThreadDetachFromJVM();
  #endif
#endif


namespace threads
{
#if defined(OMIM_OS_WINDOWS_NATIVE)
  /// Windows native implementation
  class ThreadImpl
  {
    HANDLE m_handle;

  public:
    ThreadImpl() : m_handle(0) {}

    ~ThreadImpl()
    {
      if (m_handle)
        ::CloseHandle(m_handle);
    }

    static DWORD WINAPI WindowsWrapperThreadProc(void * p)
    {
      IRoutine * pRoutine = reinterpret_cast<IRoutine *>(p);
      pRoutine->Do();
      return 0;
    }

    int Create(IRoutine * pRoutine)
    {
      int error = 0;
      m_handle = ::CreateThread(NULL, 0, &WindowsWrapperThreadProc, reinterpret_cast<void *>(pRoutine), 0, NULL);
      if (0 == m_handle)
        error = ::GetLastError();
      return error;
    }

    int Join()
    {
      int error = 0;
      if (WAIT_OBJECT_0 != ::WaitForSingleObject(m_handle, INFINITE))
        error = ::GetLastError();
      return error;
    }
  };
  // end of Windows Native implementation

#else
  /// POSIX pthreads implementation
  class ThreadImpl
  {
    pthread_t m_handle;

  public:
    ThreadImpl() {}

    static void * PthreadsWrapperThreadProc(void * p)
    {
#ifdef OMIM_OS_ANDROID
      // Attach thread to JVM, implemented in android/jni code
      AndroidThreadAttachToJVM();
#endif

      IRoutine * pRoutine = reinterpret_cast<IRoutine *>(p);
      pRoutine->Do();

#ifdef OMIM_OS_ANDROID
      // Detach thread from JVM, implemented in android/jni code
      AndroidThreadDetachFromJVM();
#endif

      ::pthread_exit(NULL);
      return NULL;
    }

    int Create(IRoutine * pRoutine)
    {
      return ::pthread_create(&m_handle, NULL, &PthreadsWrapperThreadProc, reinterpret_cast<void *>(pRoutine));
    }

    int Join()
    {
      return ::pthread_join(m_handle, NULL);
    }
  };
  //////////////////////// end of POSIX pthreads implementation
#endif

  /////////////////////////////////////////////////////////////////////
  // Thread wrapper implementation
  Thread::Thread() : m_impl(new ThreadImpl()), m_routine(0)
  {
  }

  Thread::~Thread()
  {
    delete m_impl;
  }

  bool Thread::Create(IRoutine * pRoutine)
  {
    ASSERT_EQUAL(m_routine, 0, ("Current implementation doesn't allow to reuse thread"));
    int error = m_impl->Create(pRoutine);
    if (0 != error)
    {
      ASSERT ( !"Thread creation failed with error", (error) );
      return false;
    }
    m_routine = pRoutine;
    return true;
  }

  void Thread::Cancel()
  {
    if (m_routine)
    {
      m_routine->Cancel();
      Join();
    }
  }

  void Thread::Join()
  {
    if (m_routine)
    {
      int const error = m_impl->Join();
      if (0 != error)
      {
        ASSERT ( !"Thread join failed. See error value.", (error) );
      }
    }
  }


  SimpleThreadPool::SimpleThreadPool(size_t reserve)
  {
    m_pool.reserve(reserve);
  }

  SimpleThreadPool::~SimpleThreadPool()
  {
    for (size_t i = 0; i < m_pool.size(); ++i)
    {
      delete m_pool[i].first;
      delete m_pool[i].second;
    }
  }

  void SimpleThreadPool::Add(IRoutine * pRoutine)
  {
    ValueT v;
    v.first = new Thread();
    v.second = pRoutine;

    m_pool.push_back(v);

    v.first->Create(pRoutine);
  }

  void SimpleThreadPool::Join()
  {
    for (size_t i = 0; i < m_pool.size(); ++i)
      m_pool[i].first->Join();
  }

  IRoutine * SimpleThreadPool::GetRoutine(size_t i) const
  {
    return m_pool[i].second;
  }


  void Sleep(size_t ms)
  {
#ifdef OMIM_OS_WINDOWS
    ::Sleep(ms);
#else
    timespec t;
    t.tv_nsec =(ms * 1000000) % 1000000000;
    t.tv_sec = (ms * 1000000) / 1000000000;
    nanosleep(&t, 0);
#endif
  }

  ThreadID GetCurrentThreadID()
  {
#ifdef OMIM_OS_WINDOWS
    return ::GetCurrentThreadId();
#else
    return reinterpret_cast<void *>(pthread_self());
#endif
  }
}
