#pragma once

#include "base/assert.hpp"
#include "base/cancellable.hpp"
#include "base/macros.hpp"

#include <functional>
#include <memory>
#include <thread>

namespace threads
{
class IRoutine : public base::Cancellable
{
public:
  /// Perform the main task.
  virtual void Do() = 0;
};

/// A wrapper for system threads API.
///
/// Thread class manages lifetime of a running IRoutine and guarantees
/// that it will be possible to access the IRoutine after
/// Thread::Create() call until cancellation or destruction.  In the
/// latter case, system thread will be responsible for deletion of a
/// IRoutine.
class Thread
{
  std::thread m_thread;
  std::shared_ptr<IRoutine> m_routine;

  DISALLOW_COPY(Thread);

public:
  Thread();
  ~Thread();

  /// Run thread immediately.
  ///
  /// @param routine Routine that will be executed on m_thread and
  ///                destroyed by the current Thread instance or by
  ///                the m_thread, if it is detached during the
  ///                execution of routine.
  bool Create(std::unique_ptr<IRoutine> && routine);

  /// Calling the IRoutine::Cancel method, and Join'ing with the task
  /// execution.  After that, routine is deleted.
  void Cancel();

  /// Wait for thread ending.
  void Join();

  /// \return Pointer to the routine.
  IRoutine * GetRoutine();

  /// \return Pointer to the routine converted to T *.  When it's not
  ///         possible to convert routine to the T *, release version
  ///         returns nullptr, debug version fails.
  template <typename T>
  T * GetRoutineAs()
  {
    ASSERT(m_routine.get(), ("Routine is not set"));
    T * ptr = dynamic_cast<T *>(m_routine.get());
    ASSERT(ptr, ("Can't convert IRoutine* to", TO_STRING(T) "*"));
    return ptr;
  }
};

/// Suspends the execution of the current thread until the time-out interval elapses.
/// @param[in] ms time-out interval in milliseconds
void Sleep(size_t ms);

using ThreadID = std::thread::id;

ThreadID GetCurrentThreadID();

/// A wrapper around a std thread which executes callable object in thread which is attached to JVM
/// Class has the same interface as std::thread
class SimpleThread
{
public:
  using id = std::thread::id;
  using native_handle_type = std::thread::native_handle_type;

  SimpleThread() noexcept {}
  SimpleThread(SimpleThread && x) noexcept : m_thread(std::move(x.m_thread)) {}

  template <class Fn, class... Args>
  explicit SimpleThread(Fn && fn, Args &&... args)
    : m_thread(&SimpleThread::ThreadFunc, std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...))
  {}

  SimpleThread & operator=(SimpleThread && x) noexcept
  {
    m_thread = std::move(x.m_thread);
    return *this;
  }

  void detach() { m_thread.detach(); }
  id get_id() const noexcept { return m_thread.get_id(); }
  void join() { m_thread.join(); }
  bool joinable() const noexcept { return m_thread.joinable(); }
  native_handle_type native_handle() { return m_thread.native_handle(); }
  void swap(SimpleThread & x) noexcept { m_thread.swap(x.m_thread); }

private:
  static void ThreadFunc(std::function<void()> && fn);

  DISALLOW_COPY(SimpleThread);

  std::thread m_thread;
};
}  // namespace threads
