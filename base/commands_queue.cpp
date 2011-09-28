#include "../base/SRC_FIRST.hpp"

#include "../base/logging.hpp"

#include "commands_queue.hpp"

namespace core
{
  CommandsQueue::Environment::Environment(int threadNum)
    : m_threadNum(threadNum), m_isCancelled(false)
  {}

  void CommandsQueue::Environment::Cancel()
  {
    m_isCancelled = true;
  }

  bool CommandsQueue::Environment::IsCancelled() const
  {
    return m_isCancelled;
  }

  int CommandsQueue::Environment::GetThreadNum() const
  {
    return m_threadNum;
  }

  CommandsQueue::Chain::Chain()
  {}

  void CommandsQueue::Chain::operator()(CommandsQueue::Environment const & env)
  {
    for (list<function_t>::const_iterator it = m_fns.begin(); it != m_fns.end(); ++it)
    {
      (*it)(env);
      if (env.IsCancelled())
        break;
    }
  }

  CommandsQueue::Routine::Routine() : m_env(-1)
  {}

  CommandsQueue::Routine::Routine(CommandsQueue * parent, int idx)
    : m_parent(parent), m_idx(idx), m_env(idx)
  {}

  void CommandsQueue::Routine::Do()
  {
    // performing initialization tasks
    for(list<Command>::const_iterator it = m_parent->m_initCommands.begin();
        it != m_parent->m_initCommands.end();
        ++it)
      it->m_fn(m_env);

    // main loop
    while (!IsCancelled())
    {
      CommandsQueue::Command cmd = m_parent->m_commands.Front(true);

      if (m_parent->m_commands.IsCancelled())
        break;

      m_env.m_isCancelled = false;

      cmd.m_fn(m_env);

      m_parent->FinishCommand();
    }

    // performing finalization tasks
    for(list<Command>::const_iterator it = m_parent->m_finCommands.begin();
        it != m_parent->m_finCommands.end();
        ++it)
      it->m_fn(m_env);
  }

  void CommandsQueue::Routine::Cancel()
  {
    m_env.Cancel();

    // performing cancellation tasks
    for(list<Command>::const_iterator it = m_parent->m_cancelCommands.begin();
        it != m_parent->m_cancelCommands.end();
        ++it)
      it->m_fn(m_env);

    IRoutine::Cancel();
  }

  void CommandsQueue::Routine::CancelCommand()
  {
    m_env.Cancel();
  }

  CommandsQueue::Executor::Executor() : m_routine(0)
  {}

  void CommandsQueue::Executor::Cancel()
  {
    if (m_routine != 0)
      m_thread.Cancel();
  }

  void CommandsQueue::Executor::CancelCommand()
  {
    m_routine->CancelCommand();
  }

  CommandsQueue::CommandsQueue(size_t executorsCount)
    : m_executorsCount(executorsCount), m_cmdId(0), m_activeCommands(0)
  {
    m_executors = new Executor[executorsCount];
    m_executorsCount = executorsCount;
  }

  CommandsQueue::~CommandsQueue()
  {}

  void CommandsQueue::Cancel()
  {
    m_commands.Cancel();

    for (size_t i = 0; i < m_executorsCount; ++i)
      m_executors[i].Cancel();

    delete [] m_executors;
    m_executors = 0;
  }

  void CommandsQueue::CancelCommands()
  {
    for (size_t i = 0; i < m_executorsCount; ++i)
      m_executors[i].CancelCommand();
  }

  void CommandsQueue::Start()
  {
    for (size_t i = 0; i < m_executorsCount; ++i)
    {
      m_executors[i].m_routine = new CommandsQueue::Routine(this, i);
      m_executors[i].m_thread.Create(m_executors[i].m_routine);
    }
  }

  void CommandsQueue::AddCommand(Command const & cmd)
  {
    threads::ConditionGuard g(m_cond);
    m_commands.PushBack(cmd);
    ++m_activeCommands;
  }

  void CommandsQueue::AddInitCommand(Command const & cmd)
  {
    m_initCommands.push_back(cmd);
  }

  void CommandsQueue::AddFinCommand(Command const & cmd)
  {
    m_finCommands.push_back(cmd);
  }

  void CommandsQueue::AddCancelCommand(Command const & cmd)
  {
    m_cancelCommands.push_back(cmd);
  }

  void CommandsQueue::FinishCommand()
  {
    threads::ConditionGuard g(m_cond);

    --m_activeCommands;
    if (m_activeCommands == 0)
      m_cond.Signal();
  }

  void CommandsQueue::Join()
  {
    threads::ConditionGuard g(m_cond);
    if (m_activeCommands != 0)
      m_cond.Wait();
  }

  void CommandsQueue::Clear()
  {
    m_commands.Clear();
  }

  int CommandsQueue::ExecutorsCount() const
  {
    return m_executorsCount;
  }
}
