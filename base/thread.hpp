#pragma once

#include "../std/stdint.hpp"

namespace threads
{
  class IRoutine
  {
  private:
    bool m_isCancelled;
  protected:
    bool IsCancelled() { return m_isCancelled; }
  public:
    IRoutine() : m_isCancelled(false) {}
    virtual ~IRoutine() {}
    /// Performing the main task
    virtual void Do() = 0;
    /// Implement this function to respond to the cancellation event.
    /// Cancellation means that IRoutine should exit as fast as possible.
    virtual void Cancel() { m_isCancelled = true; }
  };

  class ThreadImpl;
  /// wrapper for Create and Terminate threads API
  class Thread
  {
    ThreadImpl * m_impl;
    IRoutine * m_routine;

    Thread(Thread const &);
    Thread & operator=(Thread const &);

  public:
    Thread();
    ~Thread();

    /// Run thread immediately
    /// @param pRoutine is owned by Thread class
    bool Create(IRoutine * pRoutine);
    /// Calling the IRoutine::Cancel method, and Join'ing with the task execution.
    void Cancel();
    /// wait for thread ending
    void Join();
  };

  /// Suspends the execution of the current thread until the time-out interval elapses.
  /// @param[in] ms time-out interval in milliseconds
  void Sleep(size_t ms);

  int GetCurrentThreadID();

} // namespace threads
