#include "storage/storage_tests/task_runner.hpp"

#include "base/assert.hpp"

namespace storage
{
TaskRunner::~TaskRunner()
{
  CHECK(m_checker.CalledOnOriginalThread(), ());
}

void TaskRunner::Run()
{
  CHECK(m_checker.CalledOnOriginalThread(), ());
  while (!m_tasks.empty())
  {
    TTask task = m_tasks.front();
    m_tasks.pop();
    task();
  }
}

void TaskRunner::PostTask(TTask const & task)
{
  CHECK(m_checker.CalledOnOriginalThread(), ());
  m_tasks.push(task);
}
}  // namespace storage
