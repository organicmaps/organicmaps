#include "controller.hpp"
#include "task.hpp"

#include "../base/timer.hpp"
#include "../std/bind.hpp"

namespace anim
{
  Controller::Controller()
  {
    m_animStep = 10;
    m_thread.Create(this);
  }

  Controller::~Controller()
  {
    m_tasks.Cancel();
    m_newTasks.Cancel();
    m_thread.Cancel();
  }

  void Controller::AddTask(shared_ptr<Task> const & task)
  {
    m_newTasks.PushBack(task);
  }

  void Controller::CopyTasks(TTasks & from, TTasks & to)
  {
    to.clear();
    swap(from, to);
  }

  void Controller::Do()
  {
    while (true)
    {
      // making synchronized copy of tasks to process
      // them without intervention from other threads.
      m_newTasks.ProcessList(bind(&Controller::CopyTasks, this, _1, ref(m_newTasksList)));
      m_tasks.ProcessList(bind(&Controller::CopyTasks, this, _1, ref(m_tasksList)));

      // checking for thread cancellation
      if (m_newTasks.IsCancelled()
       || m_tasks.IsCancelled()
       || IsCancelled())
        break;

      // current animation step timestamp
      double timeStamp = my::Timer::LocalTime();

      // starting new tasks and adding them to the pool.
      // they we'll be processed in the next animation step
      for (TTasks::const_iterator it = m_newTasksList.begin();
           it != m_newTasksList.end();
           ++it)
      {
        shared_ptr<Task> task = *it;
        task->OnStart(timeStamp);
        m_tasks.PushBack(task);
      }

      m_newTasksList.clear();

      // processing current tasks
      for (TTasks::const_iterator it = m_tasksList.begin();
           it != m_tasksList.end();
           ++it)
      {
        shared_ptr<Task> task = *it;
        task->OnStep(timeStamp);
        if (!task->IsFinished())
          m_tasks.PushBack(task);
        else
          task->OnEnd(timeStamp);
      }

      m_tasksList.clear();

      // sleeping till the next animation step.
      threads::Sleep(m_animStep);
    }
  }
}
