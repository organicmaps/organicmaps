#include "thread.hpp"
#include "assert.hpp"

#include "../base/logging.hpp"

#include "../std/chrono.hpp"
#include "../std/exception.hpp"

#if defined(OMIM_OS_ANDROID)
void AndroidThreadAttachToJVM();
void AndroidThreadDetachFromJVM();
#endif  // defined(OMIM_OS_ANDROID)

namespace threads
{
namespace
{
/// Prepares worker thread and runs routine.
void RunRoutine(IRoutine * routine)
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
Thread::Thread() : m_routine(0) {}

Thread::~Thread() { Join(); }

bool Thread::Create(IRoutine * pRoutine)
{
  ASSERT(!m_routine, ("Current implementation doesn't allow to reuse thread"));
  thread routineThread;
  try
  {
    routineThread = thread(&RunRoutine, pRoutine);
  }
  catch (exception & e)
  {
    LOG(LERROR, ("Thread creation failed with error:", e.what()));
    return false;
  }
  m_thread = move(routineThread);
  m_routine = pRoutine;
  return true;
}

void Thread::Cancel()
{
  if (!m_routine)
    return;
  m_routine->Cancel();
  Join();
}

void Thread::Join()
{
  if (m_thread.joinable())
    m_thread.join();
}

SimpleThreadPool::SimpleThreadPool(size_t reserve) { m_pool.reserve(reserve); }

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

IRoutine * SimpleThreadPool::GetRoutine(size_t i) const { return m_pool[i].second; }

void Sleep(size_t ms) { this_thread::sleep_for(milliseconds(ms)); }

ThreadID GetCurrentThreadID() { return this_thread::get_id(); }
}
