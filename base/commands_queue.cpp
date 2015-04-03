#include "../base/SRC_FIRST.hpp"

#include "../base/logging.hpp"
#include "../base/assert.hpp"

#include "../std/bind.hpp"

#include "commands_queue.hpp"

namespace core
{
  CommandsQueue::Environment::Environment(size_t threadNum)
    : m_threadNum(threadNum)
  {}

  size_t CommandsQueue::Environment::threadNum() const
  {
    return m_threadNum;
  }

  CommandsQueue::BaseCommand::BaseCommand(bool isWaitable)
    : m_waitCount(0), m_isCompleted(false)
  {
    if (isWaitable)
      m_cond.reset(new threads::Condition());
  }

  void CommandsQueue::BaseCommand::join()
  {
    if (m_cond)
    {
      threads::ConditionGuard g(*m_cond.get());
      m_waitCount++;
      if (!m_isCompleted)
        g.Wait();
    }
    else
      LOG(LERROR, ("command isn't waitable"));
  }

  void CommandsQueue::BaseCommand::finish() const
  {
    if (m_cond)
    {
      threads::ConditionGuard g(*m_cond.get());
      m_isCompleted = true;
      CHECK(m_waitCount < 2, ("only one thread could wait for the queued command"));
      if (m_waitCount)
        g.Signal(true);
    }
  }

  CommandsQueue::Chain::Chain()
  {}

  void CommandsQueue::Chain::operator()(CommandsQueue::Environment const & env)
  {
    for (list<function_t>::const_iterator it = m_fns.begin(); it != m_fns.end(); ++it)
      (*it)(env);
  }

  CommandsQueue::Command::Command(bool isWaitable)
    : BaseCommand(isWaitable)
  {}

  void CommandsQueue::Command::perform(Environment const & env) const
  {
    m_fn(env);
    finish();
  }

  CommandsQueue::Routine::Routine(CommandsQueue * parent, size_t idx)
    : m_parent(parent), m_env(idx)
  {}

  void CommandsQueue::Routine::Do()
  {
    // performing initialization tasks
    for(list<shared_ptr<Command> >::const_iterator it = m_parent->m_initCommands.begin();
        it != m_parent->m_initCommands.end();
        ++it)
      (*it)->perform(m_env);

    // main loop
    while (!IsCancelled())
    {
      shared_ptr<CommandsQueue::Command> cmd = m_parent->m_commands.Front(true);

      if (m_parent->m_commands.IsCancelled())
        break;

      m_env.Reset();

      cmd->perform(m_env);

      m_parent->FinishCommand();
    }

    // performing finalization tasks
    for(list<shared_ptr<Command> >::const_iterator it = m_parent->m_finCommands.begin();
        it != m_parent->m_finCommands.end();
        ++it)
      (*it)->perform(m_env);
  }

  void CommandsQueue::Routine::Cancel()
  {
    m_env.Cancel();

    // performing cancellation tasks
    for(list<shared_ptr<Command> >::const_iterator it = m_parent->m_cancelCommands.begin();
        it != m_parent->m_cancelCommands.end();
        ++it)
      (*it)->perform(m_env);

    IRoutine::Cancel();
  }

  void CommandsQueue::Routine::CancelCommand()
  {
    m_env.Cancel();
  }

  void CommandsQueue::Executor::Cancel()
  {
    if (m_thread.GetRoutine())
      m_thread.Cancel();
  }

  void CommandsQueue::Executor::CancelCommand()
  {
    Routine * routine = m_thread.GetRoutineAs<Routine>();
    CHECK(routine, ());
    routine->CancelCommand();
  }

  CommandsQueue::CommandsQueue(size_t executorsCount)
      : m_executors(executorsCount), m_activeCommands(0)
  {
  }

  CommandsQueue::~CommandsQueue()
  {
    /// @todo memory leak in m_executors? call Cancel()?
    //CHECK ( m_executors == 0, () );
  }

  void CommandsQueue::Cancel()
  {
    m_commands.Cancel();

    for (auto & executor : m_executors)
      executor.Cancel();
    m_executors.clear();
  }

  void CommandsQueue::CancelCommands()
  {
    for (auto & executor : m_executors)
      executor.CancelCommand();
  }

  void CommandsQueue::Start()
  {
    for (size_t i = 0; i < m_executors.size(); ++i)
      m_executors[i].m_thread.Create(make_unique<Routine>(this, i));
  }

  void CommandsQueue::AddCommand(shared_ptr<Command> const & cmd)
  {
    threads::ConditionGuard g(m_cond);
    m_commands.PushBack(cmd);
    ++m_activeCommands;
  }

  void CommandsQueue::AddInitCommand(shared_ptr<Command> const & cmd)
  {
    m_initCommands.push_back(cmd);
  }

  void CommandsQueue::AddFinCommand(shared_ptr<Command> const & cmd)
  {
    m_finCommands.push_back(cmd);
  }

  void CommandsQueue::AddCancelCommand(shared_ptr<Command> const & cmd)
  {
    m_cancelCommands.push_back(cmd);
  }

  void CommandsQueue::FinishCommand()
  {
    threads::ConditionGuard g(m_cond);

    --m_activeCommands;

    if (m_activeCommands == 0)
      g.Signal(true);
  }

  void CommandsQueue::Join()
  {
    threads::ConditionGuard g(m_cond);
    while (m_activeCommands != 0)
      g.Wait();
  }

  void CommandsQueue::ClearImpl(list<shared_ptr<CommandsQueue::Command> > & l)
  {
    threads::ConditionGuard g(m_cond);
    size_t s = l.size();
    l.clear();
    m_activeCommands -= s;
    if (m_activeCommands == 0)
      g.Signal(true);
  }

  void CommandsQueue::Clear()
  {
    /// let us assume that decreasing m_activeCommands is an "operation A"
    /// and clearing the list of commands is an "operation B"
    /// we should perform them atomically (both or none at the same time)
    /// to prevent the situation when Executor could start processing some command
    /// between "operation A" and "operation B" which could lead to underflow of m_activeCommands

    m_commands.ProcessList([this](list<shared_ptr<CommandsQueue::Command> > & l)
                           {
                             ClearImpl(l);
                           });
  }

  size_t CommandsQueue::ExecutorsCount() const
  {
    return m_executors.size();
  }
}
