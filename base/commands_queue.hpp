#pragma once

#include "../std/function.hpp"
#include "../std/vector.hpp"

#include "thread.hpp"
#include "threaded_list.hpp"

namespace core
{
  class CommandsQueue
  {
  private:

    class Routine;

  public:

    class Command;

    /// execution environment for single command
    class Environment
    {
    private:

      int m_threadNum;
      bool m_isCancelled;

    protected:

      explicit Environment(int threadNum);
      void Cancel();

      friend class Routine;

    public:

      int GetThreadNum() const; //< number of thread, that is executing the command
      bool IsCancelled() const; //< command should ping this flag to see, whether it should cancel execution
    };

    /// single commmand
    typedef function<void(Environment const &)> function_t;

    struct Chain
    {
      list<function_t> m_fns;

      Chain();

      template <typename fun_tt>
      Chain(fun_tt fn)
      {
        m_fns.push_back(fn);
      }

      template <typename fun_tt>
      Chain & addCommand(fun_tt fn)
      {
        m_fns.push_back(fn);
        return *this;
      }

      void operator()(Environment const & env);
    };

    struct Command
    {
      uint64_t m_id;
      function_t m_fn;

      Command() : m_id(-1)
      {}

      template <typename tt>
      Command(uint64_t id, tt t)
        : m_id(id), m_fn(t)
      {}
    };

  private:

    class Routine : public threads::IRoutine
    {
    private:

      CommandsQueue * m_parent;
      int m_idx;
      Environment m_env;

    public:

      Routine();
      Routine(CommandsQueue * parent, int idx);

      void Do();
      void Cancel();
    };

    struct Executor
    {
      threads::Thread m_thread;
      Routine * m_routine;
      Executor();
      void Cancel();
    };

    Executor * m_executors;
    size_t m_executorsCount;
    ThreadedList<Command> m_commands;
    uint64_t m_cmdId;


    list<Command> m_initCommands;
    list<Command> m_finCommands;
    list<Command> m_cancelCommands;

    friend class Routine;

    threads::Condition m_cond;
    size_t m_activeCommands;
    void FinishCommand();

    CommandsQueue(CommandsQueue const &);
    CommandsQueue const & operator=(CommandsQueue const &);

  public:

    CommandsQueue(size_t executorsCount);
    ~CommandsQueue();

    int ExecutorsCount() const;
    void AddCommand(Command const & cmd);
    void AddInitCommand(Command const & cmd);
    void AddFinCommand(Command const & cmd);
    void AddCancelCommand(Command const & cmd);
    void Start();
    void Clear();
    void Cancel();
    void Join();

    template<typename command_tt>
    void AddCommand(command_tt cmd)
    {
      AddCommand(Command(m_cmdId++, cmd));
    }

    template <typename command_tt>
    void AddInitCommand(command_tt cmd)
    {
      AddInitCommand(Command(m_cmdId++, cmd));
    }

    template <typename command_tt>
    void AddFinCommand(command_tt cmd)
    {
      AddFinCommand(Command(m_cmdId++, cmd));
    }

    template <typename command_tt>
    void AddCancelCommand(command_tt cmd)
    {
      AddCancelCommand(Command(m_cmdId++, cmd));
    }

  };
}
