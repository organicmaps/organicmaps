#pragma once

#include "../std/shared_ptr.hpp"

#include "../base/thread.hpp"
#include "../base/threaded_list.hpp"

namespace anim
{
  class Task;

  // Animation controller class.
  class Controller
  {
  private:

    // Container for tasks
    typedef list<shared_ptr<Task> > TTasks;

    ThreadedList<shared_ptr<Task> > m_tasks;
    // Task for the current step.
    TTasks m_tasksList;

    int m_LockCount;

    static void CopyAndClearTasks(list<shared_ptr<Task> > & from, list<shared_ptr<Task> > & to);

  public:
    // Constructor
    Controller();
    // Destructor
    ~Controller();
    // Adding animation task to the controller
    void AddTask(shared_ptr<Task> const & task);
    // Do we have animation tasks, which are currently running?
    bool HasTasks();
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
  };
}
