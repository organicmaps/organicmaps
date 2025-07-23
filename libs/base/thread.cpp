#include "base/thread.hpp"

#include "base/logging.hpp"

#include "std/target_os.hpp"

#include <chrono>
#include <exception>

#if defined(OMIM_OS_ANDROID)
void AndroidThreadAttachToJVM();
void AndroidThreadDetachFromJVM();
#endif  // defined(OMIM_OS_ANDROID)

namespace threads
{
namespace
{
/// Prepares worker thread and runs routine.
void RunRoutine(std::shared_ptr<IRoutine> routine)
{
#if defined(OMIM_OS_ANDROID)
  AndroidThreadAttachToJVM();
#endif  // defined(OMIM_OS_ANDROID)

  routine->Do();

#if defined(OMIM_OS_ANDROID)
  AndroidThreadDetachFromJVM();
#endif  // defined(OMIM_OS_ANDROID)
}
}  // namespace

/////////////////////////////////////////////////////////////////////
// Thread wrapper implementation

Thread::Thread() {}

Thread::~Thread()
{
  // @todo (ygorshenin@): in general, it's not a good practice to
  // implicitly detach thread since detached threads work in
  // background, consume system resources and make it hard to reason
  // about program. Thus, all places where Thread is instantiated
  // should be fixed to explicitly detach thread.
  if (m_thread.joinable())
    m_thread.detach();
}

bool Thread::Create(std::unique_ptr<IRoutine> && routine)
{
  ASSERT(!m_routine, ("Current implementation doesn't allow to reuse thread"));
  std::thread routineThread;
  try
  {
    m_routine.reset(routine.release());
    routineThread = std::thread(&RunRoutine, m_routine);
  }
  catch (std::exception & e)
  {
    LOG(LERROR, ("Thread creation failed with error:", e.what()));
    m_routine.reset();
    return false;
  }
  m_thread = std::move(routineThread);
  return true;
}

void Thread::Cancel()
{
  if (!m_routine)
    return;
  m_routine->Cancel();
  Join();
  m_routine.reset();
}

void Thread::Join()
{
  if (m_thread.joinable())
    m_thread.join();
}

IRoutine * Thread::GetRoutine()
{
  return m_routine.get();
}

void Sleep(size_t ms)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

ThreadID GetCurrentThreadID()
{
  return std::this_thread::get_id();
}

/////////////////////////////////////////////////////////////////////
// SimpleThread implementation

void SimpleThread::ThreadFunc(std::function<void()> && fn)
{
#if defined(OMIM_OS_ANDROID)
  AndroidThreadAttachToJVM();
#endif  // defined(OMIM_OS_ANDROID)

  fn();

  // https://github.com/organicmaps/organicmaps/issues/9397
  // https://github.com/organicmaps/organicmaps/issues/6139
  // Manually clear function object to free possible captured data/resources/lambdas _inside_ Attach/Detach context.
  fn = {};

#if defined(OMIM_OS_ANDROID)
  AndroidThreadDetachFromJVM();
#endif  // defined(OMIM_OS_ANDROID)
}
}  // namespace threads
