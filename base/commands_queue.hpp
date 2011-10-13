#pragma once

#include "../std/function.hpp"
#include "../std/vector.hpp"

#include "thread.hpp"
#include "threaded_list.hpp"

namespace core
{
  /// class, that executes task, specified as a functors on the specified number of threads
  /// - all tasks are stored in the single ThreadedList
  class CommandsQueue
  {
  private:

    class Routine;

  public:

    struct Command;

    /// execution environment for single command
    /// - passed into the task functor
    /// - task functor should check the IsCancelled()
    ///   on the reasonable small interval and cancel
    ///   it's work upon receiving "true".
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

      int GetThreadNum() const; //< number of thread executing the commands
      bool IsCancelled() const; //< command should ping this flag to see,
                                //  whether it should cancel execution
    };

    /// single commmand
    typedef function<void(Environment const &)> function_t;

    /// chain of commands
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

    /// single command.
    /// - could be chained together, using Chain class
    struct Command
    {
      uint64_t m_id;
      function_t m_fn;

      Command() : m_id(static_cast<uint64_t>(-1))
      {}


      template <typename tt>
      Command(uint64_t id, tt t)
        : m_id(id), m_fn(t)
      {}
    };

  private:

    /// single execution routine
    class Routine : public threads::IRoutine
    {
    private:

      CommandsQueue * m_parent;
      int m_idx;
      Environment m_env;

    public:

      Routine(CommandsQueue * parent, int idx);

      void Do();
      void Cancel();
      void CancelCommand();
    };

    /// class, which excapsulates thread and routine into single class.
    struct Executor
    {
      threads::Thread m_thread;
      Routine * m_routine;
      Executor();
      void Cancel();
      void CancelCommand();
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

    /// Number of executors in this queue
    int ExecutorsCount() const;

    /// Adding different types of commands
    /// @{
    void AddCommand(Command const & cmd);
    void AddInitCommand(Command const & cmd);
    void AddFinCommand(Command const & cmd);
    void AddCancelCommand(Command const & cmd);
    /// @}

    void Start();
    void Cancel();
    void CancelCommands();
    void Join();
    void Clear();

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
