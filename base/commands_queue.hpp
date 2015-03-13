#pragma once

#include "../std/function.hpp"
#include "../std/shared_ptr.hpp"
#include "../std/vector.hpp"

#include "cancellable.hpp"
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
    class Environment : public my::Cancellable
    {
    private:
      size_t m_threadNum;

    protected:
      explicit Environment(size_t threadNum);

      friend class Routine;

    public:
      size_t threadNum() const; //< number of the thread, which is executing the commands
    };

    /// single commmand
    typedef function<void(Environment const &)> function_t;

    /// basic command
    /// - could wait for the completion of its execution
    struct BaseCommand
    {
      shared_ptr<threads::Condition> m_cond;
      mutable int m_waitCount;
      mutable bool m_isCompleted;

      /// should we create the threads::Condition ?
      /// this flag is used to save resources
      BaseCommand(bool isWaitable);

      /// call this function when the execution
      /// of the command is finished to release the waiters.
      void finish() const;

      /// @warning only single thread could "join" command
      void join();
    };

    /// chain of commands
    struct Chain
    {
    private:
      list<function_t> m_fns;

    public:
      Chain();

      Chain(function_t const & fn)
      {
        m_fns.push_back(fn);
      }

      Chain & addCommand(function_t const & fn)
      {
        m_fns.push_back(fn);
        return *this;
      }

      void operator()(Environment const & env);
    };

    /// single command.
    /// - commands could be chained together, using Chain class
    struct Command : BaseCommand
    {
    private:
      function_t m_fn;

    public:
      Command(bool isWaitable = false);

      template <typename tt>
      Command(tt t, bool isWaitable = false)
        : BaseCommand(isWaitable), m_fn(t)
      {}

      void perform(Environment const & env) const;
    };

  private:
    /// single execution routine
    class Routine : public threads::IRoutine
    {
    private:
      CommandsQueue * m_parent;
      Environment m_env;

    public:
      Routine(CommandsQueue * parent, size_t idx);

      /// threads::IRoutine overrides:
      void Do() override;
      void Cancel() override;

      void CancelCommand();
    };

    /// class, which excapsulates thread and routine into single class.
    struct Executor
    {
      threads::Thread m_thread;

      void Cancel();
      void CancelCommand();
    };

    vector<Executor> m_executors;
    ThreadedList<shared_ptr<Command> > m_commands;

    list<shared_ptr<Command> > m_initCommands;
    list<shared_ptr<Command> > m_finCommands;
    list<shared_ptr<Command> > m_cancelCommands;

    friend class Routine;

    threads::Condition m_cond;
    size_t m_activeCommands;
    void FinishCommand();

    CommandsQueue(CommandsQueue const &);
    CommandsQueue const & operator=(CommandsQueue const &);

    void ClearImpl(list<shared_ptr<Command> > & l);

  public:
    CommandsQueue(size_t executorsCount);
    ~CommandsQueue();

    /// Number of executors in this queue
    size_t ExecutorsCount() const;

    /// Adding different types of commands
    /// @{
    void AddCommand(shared_ptr<Command> const & cmd);
    void AddInitCommand(shared_ptr<Command> const & cmd);
    void AddFinCommand(shared_ptr<Command> const & cmd);
    void AddCancelCommand(shared_ptr<Command> const & cmd);
    /// @}

    void Start();
    void Cancel();
    void CancelCommands();
    void Join();
    void Clear();

    template<typename command_tt>
    shared_ptr<Command> AddCommand(command_tt cmd, bool isWaitable = false)
    {
      shared_ptr<Command> pcmd(new Command(cmd, isWaitable));
      AddCommand(pcmd);
      return pcmd;
    }

    template <typename command_tt>
    shared_ptr<Command> AddInitCommand(command_tt cmd, bool isWaitable = false)
    {
      shared_ptr<Command> const pcmd(new Command(cmd, isWaitable));
      AddInitCommand(pcmd);
      return pcmd;
    }

    template <typename command_tt>
    shared_ptr<Command> const AddFinCommand(command_tt cmd, bool isWaitable = false)
    {
      shared_ptr<Command> pcmd(new Command(cmd, isWaitable));
      AddFinCommand(pcmd);
      return pcmd;
    }

    template <typename command_tt>
    shared_ptr<Command> const AddCancelCommand(command_tt cmd, bool isWaitable = false)
    {
      shared_ptr<Command> pcmd(new Command(cmd, isWaitable));
      AddCancelCommand(pcmd);
      return pcmd;
    }
  };
}
