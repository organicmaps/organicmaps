#include "anim/task.hpp"

#include "base/assert.hpp"

#include "std/algorithm.hpp"


namespace anim
{
  Task::Task()
    : m_State(EReady)
  {}

  Task::~Task()
  {}

  Task::EState Task::State() const
  {
    return m_State;
  }

  void Task::Lock()
  {
    m_mutex.Lock();
  }

  void Task::Unlock()
  {
    m_mutex.Unlock();
  }

  void Task::SetState(EState State)
  {
    m_State = State;
  }

  void Task::PerformCallbacks(EState state)
  {
    list<TCallback> const & cb = m_Callbacks[state];
    for_each(cb.begin(), cb.end(), [] (TCallback const & el) { el(); });
  }

  void Task::OnStart(double ts)
  {
    PerformCallbacks(EReady);
  }

  void Task::OnStep(double ts)
  {
    PerformCallbacks(EInProgress);
  }

  void Task::OnCancel(double ts)
  {
    PerformCallbacks(ECancelled);
  }

  void Task::OnEnd(double ts)
  {
    PerformCallbacks(EEnded);
  }

  void Task::Start()
  {
    ASSERT(IsReady(), ());
    SetState(EInProgress);
  }

  void Task::Cancel()
  {
    ASSERT(IsRunning() || IsReady(), ());
    SetState(ECancelled);
  }

  void Task::End()
  {
    ASSERT(IsRunning() || IsReady(), ());
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

  bool Task::IsReady() const
  {
    return State() == EReady;
  }

  bool Task::IsVisual() const
  {
    return false;
  }

  void Task::AddCallback(EState state, TCallback const & cb)
  {
    m_Callbacks[state].push_back(cb);
  }

  void Task::SetController(Controller * controller)
  {
    m_controller = controller;
  }

  Controller * Task::GetController() const
  {
    return m_controller;
  }
}
