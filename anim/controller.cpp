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

  void Controller::AddTaskImpl(list<shared_ptr<Task> > & l, shared_ptr<Task> const & task)
  {
    l.push_back(task);
    task->SetController(this);
    if (task->IsVisual())
      m_IdleFrames = m_IdleThreshold;
  }

  void Controller::AddTask(shared_ptr<Task> const & task)
  {
    m_tasks.ProcessList(bind(&Controller::AddTaskImpl, this, _1, task));
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

  void Controller::HasVisualTasksImpl(list<shared_ptr<Task> > &l, bool *res) const
  {
    *res = false;
    for (list<shared_ptr<Task> >::const_iterator it = l.begin();
         it != l.end();
         ++it)
    {
      if ((*it)->IsVisual())
      {
        *res = true;
        break;
      }
    }
  }

  bool Controller::HasVisualTasks()
  {
    bool res;
    m_tasks.ProcessList(bind(&Controller::HasVisualTasksImpl, this, _1, &res));
    return res;
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

    bool hasVisualTasks = false;
    for (list<shared_ptr<Task> >::const_iterator it = m_tasksList.begin();
         it != m_tasksList.end();
         ++it)
      if ((*it)->IsVisual())
      {
        hasVisualTasks = true;
        break;
      }

    for (TTasks::const_iterator it = m_tasksList.begin(); it != m_tasksList.end(); ++it)
    {
      shared_ptr<Task> const & task = *it;

      task->Lock();

      if (task->IsVisual())
        m_IdleFrames = m_IdleThreshold;

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

    if (!hasVisualTasks && m_IdleFrames > 0)
      m_IdleFrames -= 1;

    m_tasks.ProcessList(bind(&Controller::MergeTasks, ref(l), _1));
  }

  bool Controller::IsVisuallyPreWarmed() const
  {
    return m_IdleFrames > 0;
  }

  double Controller::GetCurrentTime() const
  {
    return my::Timer::LocalTime();
  }
}
