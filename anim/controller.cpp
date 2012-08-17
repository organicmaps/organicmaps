#include "controller.hpp"
#include "task.hpp"

#include "../base/timer.hpp"
#include "../std/bind.hpp"

namespace anim
{
  Controller::Controller()
  {
    m_LockCount = 0;
  }

  Controller::~Controller()
  {
  }

  void Controller::AddTask(shared_ptr<Task> const & task)
  {
    m_tasks.PushBack(task);
  }

  void Controller::CopyTasks(TTasks & from, TTasks & to)
  {
    to.clear();
    swap(from, to);
  }

  bool Controller::HasTasks()
  {
    return !m_tasks.Empty();
  }

  void Controller::Lock()
  {
    ++m_LockCount;
  }

  void Controller::Unlock()
  {
    --m_LockCount;
  }

  int Controller::LockCount()
  {
    if (m_LockCount < 0)
      LOG(LWARNING, ("Lock/Unlock is unbalanced! LockCount < 0!"));

    return m_LockCount;
  }

  void Controller::PerformStep()
  {
    m_tasks.ProcessList(bind(&Controller::CopyTasks, this, _1, ref(m_tasksList)));

    double ts = my::Timer::LocalTime();

    TTasks l;

    for (TTasks::const_iterator it = m_tasksList.begin(); it != m_tasksList.end(); ++it)
    {
      shared_ptr<Task> const & task = *it;
      if (task->State() == Task::EWaitStart)
        task->OnStart(ts);
      if (task->State() == Task::EInProgress)
        task->OnStep(ts);
      if (task->State() == Task::EWaitEnd)
        task->OnEnd(ts);
      else
        l.push_back(task);
    }

    m_tasks.ProcessList(bind(&Controller::CopyTasks, this, ref(l), _1));
  }
}
