#pragma once

#include "../std/shared_ptr.hpp"

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
    shared_ptr<ThreadImpl> m_impl;
    shared_ptr<IRoutine> m_routine;

  public:
    Thread();

    /// Run thread immediately
    /// @param pRoutine is owned by Thread class
    bool Create(IRoutine * pRoutine);
    /// Calling the IRoutine::Cancel method, and Join'ing with the task execution.
    void Cancel();
    /// wait for thread ending
    void Join();
  };
} // namespace threads
