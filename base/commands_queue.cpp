#include "../base/SRC_FIRST.hpp"

#include "../base/logging.hpp"

#include "commands_queue.hpp"

namespace threads
{
  void CommandsQueueRoutine::Do()
  {
    while (!IsCancelled())
    {
      command_t cmd;
      {
        threads::ConditionGuard guard(m_condition);

        while (m_commands.empty())
        {
          guard.Wait();
          /// could be awaken on empty queue
          /// with the purpose to finish thread execution.
          if (IsCancelled())
           return;
        }

        cmd = m_commands.front();
        m_commands.pop_front();
      }

      /// command is executed without holding the m_condition lock
      /// to allow other threads to add another command while the
      /// current one is executing
      LOG(LINFO, ("Processing Command"));
      cmd();
    }
  }

  void CommandsQueueRoutine::Cancel()
  {
    IRoutine::Cancel();
    if (m_commands.empty())
      m_condition.Signal();
  }

  CommandsQueue::CommandsQueue()
  : m_routine(new CommandsQueueRoutine())
  {
    m_thread.Create(m_routine);
  }

  CommandsQueue::~CommandsQueue()
  {
    m_thread.Cancel();
  }
}
