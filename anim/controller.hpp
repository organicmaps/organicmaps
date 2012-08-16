#pragma once

#include "../std/shared_ptr.hpp"

#include "../base/thread.hpp"
#include "../base/threaded_list.hpp"

namespace anim
{
  class Task;

  // Animation controller class.
  // - Creates and manages the separate thread.
  // - Using ThreadedList to manage the list of active commands.
  // - CPU efficient, which means that when there are no commands
  //   the thread is sleeping and doesn't consume CPU
  class Controller : public threads::IRoutine
  {
  private:

    // Container for tasks
    typedef list<shared_ptr<Task> > TTasks;

    ThreadedList<shared_ptr<Task> > m_tasks;
    // Task for the current step.
    TTasks m_tasksList;

    ThreadedList<shared_ptr<Task> > m_newTasks;
    // Added, but not started tasks.
    // They'll be started in the next animation step.
    TTasks m_newTasksList;

    // Animation thread.
    threads::Thread m_thread;
    // Animation step in miliseconds.
    unsigned m_animStep;
    // MainLoop method
    void Do();

    void CopyTasks(list<shared_ptr<Task> > & from, list<shared_ptr<Task> > & to);

  public:
    // Constructor
    Controller();
    // Destructor
    ~Controller();
    // Adding animation task to the controller
    void AddTask(shared_ptr<Task> const & task);
  };
}
