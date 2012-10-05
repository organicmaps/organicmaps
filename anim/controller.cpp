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
    m_IdleThreshold = 5;
    m_IdleFrames = 0;
  }

  Controller::~Controller()
  {
  }

  void Controller::AddTask(shared_ptr<Task> const & task)
  {
    task->SetController(this);
    m_tasks.PushBack(task);
    m_IdleFrames = m_IdleThreshold;
  }

  void Controller::CopyAndClearTasks(TTasks & from, TTasks & to)
  {
    to.clear();
    swap(from, to);
  }

  void Controller::MergeTasks(TTasks & from, TTasks & to)
  {
    copy(from.begin(), from.end(), back_inserter(to));
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

    double ts = GetCurrentTime();

    TTasks l;

    bool hasTasks = !m_tasksList.empty();

    for (TTasks::const_iterator it = m_tasksList.begin(); it != m_tasksList.end(); ++it)
    {
      m_IdleFrames = m_IdleThreshold;

      shared_ptr<Task> const & task = *it;

      task->Lock();

      if (task->IsReady())
      {
        task->Start();
        task->OnStart(ts);
      }
      if (task->IsRunning())
        task->OnStep(ts);

      if (task->IsRunning())
        l.push_back(task);
      else
      {
        if (task->IsCancelled())
          task->OnCancel(ts);
        if (task->IsEnded())
          task->OnEnd(ts);
      }

      task->Unlock();
    }

    if (!hasTasks && m_IdleFrames > 0)
      m_IdleFrames -= 1;

    m_tasks.ProcessList(bind(&Controller::MergeTasks, ref(l), _1));
  }

  bool Controller::IsPreWarmed() const
  {
    return m_IdleFrames > 0;
  }

  double Controller::GetCurrentTime() const
  {
    return my::Timer::LocalTime();
  }
}
