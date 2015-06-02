#include "storage/storage_tests/message_loop.hpp"

#include "base/assert.hpp"

namespace storage
{
MessageLoop::~MessageLoop()
{
  CHECK(m_checker.CalledOnOriginalThread(), ());
}

void MessageLoop::Run()
{
  CHECK(m_checker.CalledOnOriginalThread(), ());
  while (!m_tasks.empty())
  {
    TTask task = m_tasks.front();
    m_tasks.pop();
    task();
  }
}

void MessageLoop::PostTask(TTask const & task)
{
  CHECK(m_checker.CalledOnOriginalThread(), ());
  m_tasks.push(task);
}
}  // namespace storage
