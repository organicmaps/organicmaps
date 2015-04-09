#pragma once

#include "std/shared_ptr.hpp"

#include "base/thread.hpp"
#include "base/threaded_list.hpp"

namespace anim
{
  class Task;

  // Animation controller class.
  class Controller
  {
  private:

    typedef shared_ptr<Task> TTaskPtr;
    // Container for tasks
    typedef list<TTaskPtr> TTasks;

    ThreadedList<TTaskPtr> m_tasks;
    // Task for the current step.
    TTasks m_tasksList;

    int m_LockCount = 0;
    bool m_hasVisualTasks = false;

  public:

    struct Guard
    {
      Controller * m_controller;
      Guard(Controller * controller);
      ~Guard();
    };

    // Adding animation task to the controller
    void AddTask(TTaskPtr const & task);
    // Do we have animation tasks, which are currently running?
    bool HasTasks();
    // Do we have visual animation tasks, which are currently running?
    bool HasVisualTasks();
    // Lock/Unlock controller. Locked controller
    // is considered to be in "transition" mode from one task to another
    // and this situation is taken into account into RenderPolicy when
    // checking for "need redraw" status.
    void Lock();
    void Unlock();
    // Getting current lock count
    int LockCount();
    // Perform single animation step
    void PerformStep();
    // Getting current simulation time
    double GetCurrentTime() const;
  };
}
