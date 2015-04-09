#include "anim/controller.hpp"
#include "anim/task.hpp"

#include "base/assert.hpp"
#include "base/timer.hpp"

#include "std/algorithm.hpp"


namespace anim
{
  Controller::Guard::Guard(Controller * controller)
    : m_controller(controller)
  {
    m_controller->Lock();
  }

  Controller::Guard::~Guard()
  {
    m_controller->Unlock();
  }

  void Controller::AddTask(TTaskPtr const & task)
  {
    m_tasks.ProcessList([&] (TTasks & taskList)
    {
      taskList.push_back(task);
      task->SetController(this);
    });
  }

  bool Controller::HasTasks()
  {
    return !m_tasks.Empty();
  }

  bool Controller::HasVisualTasks()
  {
    return m_hasVisualTasks;
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
    m_tasks.ProcessList([this] (TTasks & from)
    {
      m_tasksList.clear();
      swap(from, m_tasksList);
    });

    double ts = GetCurrentTime();

    TTasks resultList;

    for (TTaskPtr const & task : m_tasksList)
    {
      task->Lock();

      if (task->IsReady())
      {
        task->Start();
        task->OnStart(ts);
      }
      if (task->IsRunning())
        task->OnStep(ts);

      if (task->IsRunning())
        resultList.push_back(task);
      else
      {
        if (task->IsCancelled())
          task->OnCancel(ts);
        if (task->IsEnded())
          task->OnEnd(ts);
      }

      task->Unlock();
    }

    m_hasVisualTasks = false;
    m_tasks.ProcessList([&] (TTasks & to)
    {
      for_each(resultList.begin(), resultList.end(), [&] (TTaskPtr const & task)
      {
        m_hasVisualTasks |= task->IsVisual();
        to.push_back(task);
      });
    });
  }

  double Controller::GetCurrentTime() const
  {
    return my::Timer::LocalTime();
  }
}
