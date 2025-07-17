#pragma once

#include "base/thread_checker.hpp"

#include <functional>
#include <queue>

namespace storage
{
// This class can be used in tests to mimic asynchronious calls.  For
// example, when task A invokes task B and passes a callback C as an
// argument to B, it's silly for B to call C directly if B is an
// asynchronious task. So, the solution is to post C on the same
// message loop where B is run.
//
// *NOTE*, this class is not thread-safe.
class TaskRunner
{
public:
  using TTask = std::function<void()>;

  ~TaskRunner();

  void Run();
  void PostTask(TTask const & task);

private:
  std::queue<TTask> m_tasks;

  ThreadChecker m_checker;
};
}  // namespace storage
