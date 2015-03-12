#pragma once

#include "../base/cancellable.hpp"
#include "../base/macros.hpp"

#include "../std/target_os.hpp"

#include "../std/noncopyable.hpp"
#include "../std/stdint.hpp"
#include "../std/thread.hpp"
#include "../std/unique_ptr.hpp"
#include "../std/utility.hpp"
#include "../std/vector.hpp"

#ifdef OMIM_OS_WINDOWS
#include "../std/windows.hpp"  // for DWORD
#endif

namespace threads
{
class IRoutine : public my::Cancellable
{
public:
  /// Performing the main task
  virtual void Do() = 0;
};

/// wrapper for Create and Terminate threads API
class Thread
{
  thread m_thread;
  IRoutine * m_routine;

public:
  Thread();

  ~Thread();

  /// Run thread immediately.
  /// @param pRoutine is owned by Thread class
  bool Create(IRoutine * pRoutine);

  /// Calling the IRoutine::Cancel method, and Join'ing with the task execution.
  void Cancel();

  /// Wait for thread ending.
  void Join();

private:
  DISALLOW_COPY_AND_MOVE(Thread);
};

/// Simple threads container. Takes ownership for every added IRoutine.
class SimpleThreadPool : public noncopyable
{
  typedef pair<Thread *, IRoutine *> ValueT;
  vector<ValueT> m_pool;

public:
  SimpleThreadPool(size_t reserve = 0);
  ~SimpleThreadPool();

  void Add(IRoutine * pRoutine);
  void Join();

  IRoutine * GetRoutine(size_t i) const;
};

/// Suspends the execution of the current thread until the time-out interval elapses.
/// @param[in] ms time-out interval in milliseconds
void Sleep(size_t ms);

typedef thread::id ThreadID;

ThreadID GetCurrentThreadID();

}  // namespace threads
