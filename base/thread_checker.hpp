#pragma once

#include "base/assert.hpp"
#include "base/macros.hpp"

#include <thread>

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
  std::thread::id const m_id;

  DISALLOW_COPY_AND_MOVE(ThreadChecker);
};

#define DECLARE_THREAD_CHECKER(threadCheckerName)    ThreadChecker threadCheckerName
#define CHECK_THREAD_CHECKER(threadCheckerName, msg) CHECK(threadCheckerName.CalledOnOriginalThread(), msg)
#define DECLARE_AND_CHECK_THREAD_CHECKER(msg)             \
  {                                                       \
    static const ThreadChecker threadChecker;             \
    CHECK(threadChecker.CalledOnOriginalThread(), (msg)); \
  }
