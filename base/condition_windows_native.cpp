#include "../std/target_os.hpp"

#ifdef OMIM_OS_WINDOWS_NATIVE

#include "condition.hpp"
#include "mutex.hpp"

#include "../std/windows.hpp"

typedef void (WINAPI *InitFn)(PCONDITION_VARIABLE);
typedef void (WINAPI *WakeFn)(PCONDITION_VARIABLE);
typedef BOOL (WINAPI *SleepFn)(PCONDITION_VARIABLE, PCRITICAL_SECTION, DWORD);

namespace threads
{
  namespace impl
  {
    class ConditionImpl
    {
    public:
      virtual ~ConditionImpl() {}
      virtual void Signal() = 0;
      virtual void Wait() = 0;
      virtual void Lock() = 0;
      virtual void Unlock() = 0;
    };

    class ImplWinVista : public ConditionImpl
    {
      InitFn m_pInit;
      WakeFn m_pWake;
      SleepFn m_pSleep;

      CONDITION_VARIABLE m_Condition;
      Mutex m_mutex;

    public:
      ImplWinVista(InitFn pInit, WakeFn pWake, SleepFn pSleep)
        : m_pInit(pInit), m_pWake(pWake), m_pSleep(pSleep)
      {
        m_pInit(&m_Condition);
      }

      void Signal()
      {
        m_pWake(&m_Condition);
      }

      void Wait()
      {
        m_pSleep(&m_Condition, &m_mutex.m_Mutex, INFINITE);
      }

      void Lock()
      {
        m_mutex.Lock();
      }

      void Unlock()
      {
        m_mutex.Unlock();
      }
    };

    ///////////////////////////////////////////////////////////////
    /// Based on Richter's SignalObjectAndWait solution
    class ImplWinXP : public ConditionImpl
    {
      /// Number of waiting threads
      int waiters_count_;
      /// Serialize access to <waiters_count_>
      CRITICAL_SECTION waiters_count_lock_;
      /// Semaphore used to queue up threads waiting for the condition to
      /// become signaled
      HANDLE sema_;
      /// An auto-reset event used by the broadcast/signal thread to wait
      /// for all the waiting thread(s) to wake up and be released from the
      /// semaphore
      HANDLE waiters_done_;
      /// Keeps track of whether we were broadcasting or signaling.  This
      /// allows us to optimize the code if we're just signaling
      size_t was_broadcast_;

      HANDLE m_mutex;

    public:
      ImplWinXP() : waiters_count_(0), was_broadcast_(0)
      {
        ::InitializeCriticalSection(&waiters_count_lock_);
        m_mutex = ::CreateMutexA(NULL, FALSE, NULL);

        sema_ = ::CreateSemaphore(NULL,         // no security
                                  0,            // initially 0
                                  0x7fffffff,   // max count
                                  NULL);        // unnamed
        waiters_done_ = CreateEvent(NULL,       // no security
                                    FALSE,      // auto-reset
                                    FALSE,      // non-signaled initially
                                    NULL);      // unnamed
      }

      ~ImplWinXP()
      {
        ::CloseHandle(m_mutex);
        ::DeleteCriticalSection(&waiters_count_lock_);
      }

      void Signal()
      {
        EnterCriticalSection(&waiters_count_lock_);
        bool const have_waiters = waiters_count_ > 0;
        LeaveCriticalSection(&waiters_count_lock_);

        // If there aren't any waiters, then this is a no-op.
        if (have_waiters)
          ::ReleaseSemaphore(sema_, 1, 0);
      }

      void Wait()
      {
        // Avoid race conditions
        ::EnterCriticalSection(&waiters_count_lock_);
        ++waiters_count_;
        ::LeaveCriticalSection(&waiters_count_lock_);

        // This call atomically releases the mutex and waits on the
        // semaphore until <pthread_cond_signal> or <pthread_cond_broadcast>
        // are called by another thread
        ::SignalObjectAndWait(m_mutex, sema_, INFINITE, FALSE);

        // Reacquire lock to avoid race conditions
        ::EnterCriticalSection(&waiters_count_lock_);

        // We're no longer waiting...
        --waiters_count_;

        // Check to see if we're the last waiter after <pthread_cond_broadcast>.
        bool const last_waiter = was_broadcast_ && waiters_count_ == 0;

        ::LeaveCriticalSection(&waiters_count_lock_);

        // If we're the last waiter thread during this particular broadcast
        // then let all the other threads proceed
        if (last_waiter)
          // This call atomically signals the <waiters_done_> event and waits until
          // it can acquire the <external_mutex>.  This is required to ensure fairness.
          ::SignalObjectAndWait(waiters_done_, m_mutex, INFINITE, FALSE);
        else
          // Always regain the external mutex since that's the guarantee we
          // give to our callers.
          ::WaitForSingleObject(m_mutex, INFINITE);
      }

      void Lock()
      {
        ::WaitForSingleObject(m_mutex, INFINITE);
      }

      void Unlock()
      {
        ::ReleaseMutex(m_mutex);
      }
    };
  }
  ///////////////////////////////////////////////////////////////
  Condition::Condition()
  {
    HMODULE handle = GetModuleHandle(TEXT("kernel32.dll"));
    InitFn pInit = (InitFn)GetProcAddress(handle, "InitializeConditionVariable");
    WakeFn pWake = (WakeFn)GetProcAddress(handle, "WakeConditionVariable");
    SleepFn pSleep = (SleepFn)GetProcAddress(handle, "SleepConditionVariableCS");

    if (pInit && pWake && pSleep)
      m_pImpl = new impl::ImplWinVista(pInit, pWake, pSleep);
    else
      m_pImpl = new impl::ImplWinXP();
  }

  Condition::~Condition()
  {
    delete m_pImpl;
  }

  void Condition::Signal()
  {
    m_pImpl->Signal();
  }

  void Condition::Wait()
  {
    m_pImpl->Wait();
  }

  void Condition::Lock()
  {
    m_pImpl->Lock();
  }

  void Condition::Unlock()
  {
    m_pImpl->Unlock();
  }
}

#endif // OMIM_OS_WINDOWS_NATIVE
