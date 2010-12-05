#pragma once

#include "../base/thread.hpp"
#include "../std/function.hpp"
#include "../std/list.hpp"
#include "../std/shared_ptr.hpp"
#include "../base/condition.hpp"

namespace threads
{
  class CommandsQueueRoutine : public threads::IRoutine
  {
  private:

    typedef function<void()> command_t;
    list<command_t> m_commands;

    threads::Condition m_condition;

  public:
    template <typename command_tt>
    void addCommand(command_tt cmd)
    {
      threads::ConditionGuard guard(m_condition);
      bool needToSignal = m_commands.empty();
      m_commands.push_back(cmd);
      if (needToSignal)
        guard.Signal();
    };

    void Do();
    void Cancel();
  };

  class CommandsQueue
  {
  private:

    CommandsQueueRoutine * m_routine;
    threads::Thread m_thread;

  public:

    CommandsQueue();
    ~CommandsQueue();

    template<typename command_tt>
    void addCommand(command_tt cmd)
    {
      m_routine->addCommand(cmd);
    };
  };
}
