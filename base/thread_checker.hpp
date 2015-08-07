#pragma once

#include "base/assert.hpp"
#include "base/macros.hpp"

#include "std/thread.hpp"

/// This class remembers id of a thread on which it was created, and
/// then can be used to verify that CalledOnOriginalThread() is called
/// from a thread on which it was created.
class ThreadChecker
{
public:
  ThreadChecker();

  /// \return True if this method is called from a thread on which
  ///         current ThreadChecker's instance was created, false otherwise.
  bool CalledOnOriginalThread() const;

private:
  thread::id const m_id;

  DISALLOW_COPY_AND_MOVE(ThreadChecker);
};

// ThreadChecker checker is not valid on Android.
// UI thread (NV thread) can change it's handle value during app lifecycle.
#if defined(DEBUG) && !defined(OMIM_OS_ANDROID)
  #define DECLARE_THREAD_CHECKER(threadCheckerName) ThreadChecker threadCheckerName
  #define ASSERT_THREAD_CHECKER(threadCheckerName, msg) ASSERT(threadCheckerName.CalledOnOriginalThread(), msg)
#else
  #define DECLARE_THREAD_CHECKER(threadCheckerName)
  #define ASSERT_THREAD_CHECKER(threadCheckerName, msg)
#endif
