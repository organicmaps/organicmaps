#include "controller.hpp"
#include "task.hpp"

#include "../base/assert.hpp"
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

  void Controller::CopyAndClearTasks(TTasks & from, TTasks & to)
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
    ASSERT(m_LockCount >=0, ("Lock/Unlock is unbalanced! LockCount < 0!"));
    return m_LockCount;
  }

  void Controller::PerformStep()
  {
    m_tasks.ProcessList(bind(&Controller::CopyAndClearTasks, _1, ref(m_tasksList)));

    double ts = my::Timer::LocalTime();

    TTasks l;

    for (TTasks::const_iterator it = m_tasksList.begin(); it != m_tasksList.end(); ++it)
    {
      shared_ptr<Task> const & task = *it;
      if (task->State() == Task::EStarted)
        task->OnStart(ts);
      if (task->State() == Task::EInProgress)
        task->OnStep(ts);

      if (task->State() == Task::EInProgress)
        l.push_back(task);
      else
      {
        if (task->State() == Task::ECancelled)
          task->OnCancel(ts);
        if (task->State() == Task::EEnded)
          task->OnEnd(ts);
      }
    }

    m_tasks.ProcessList(bind(&Controller::CopyAndClearTasks, ref(l), _1));
  }
}
