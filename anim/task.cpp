#include "task.hpp"

namespace anim
{
  Task::Task()
    : m_State(EStarted)
  {}

  Task::~Task()
  {}

  Task::EState Task::State() const
  {
    return m_State;
  }

  void Task::SetState(EState State)
  {
    m_State = State;
  }

  void Task::PerformCallback(EState state)
  {
    TCallback const & cb = m_callbacks[state];
    if (cb)
      cb();
  }

  void Task::OnStart(double ts)
  {
    PerformCallback(EStarted);
    SetState(EInProgress);
  }

  void Task::OnStep(double ts)
  {
    PerformCallback(EInProgress);
  }

  void Task::OnCancel(double ts)
  {
    PerformCallback(ECancelled);
  }

  void Task::OnEnd(double ts)
  {
    PerformCallback(EEnded);
  }

  void Task::Cancel()
  {
    SetState(ECancelled);
  }

  void Task::End()
  {
    SetState(EEnded);
  }

  bool Task::IsEnded() const
  {
    return State() == EEnded;
  }

  bool Task::IsCancelled() const
  {
    return State() == ECancelled;
  }

  bool Task::IsRunning() const
  {
    return State() == EInProgress;
  }

  void Task::SetCallback(EState state, TCallback const & cb)
  {
    m_callbacks[state] = cb;
  }
}
