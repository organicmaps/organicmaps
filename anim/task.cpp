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

  void Task::OnStart(double ts)
  {
    SetState(EInProgress);
  }

  void Task::OnStep(double ts)
  {
  }

  void Task::OnCancel(double ts)
  {
  }

  void Task::OnEnd(double ts)
  {
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
}
