#include "platform/gui_thread.hpp"

#include <memory>
#include <utility>

#import <Foundation/Foundation.h>

namespace
{
void PerformImpl(void * task)
{
  using base::TaskLoop;
  std::unique_ptr<TaskLoop::Task> t(reinterpret_cast<TaskLoop::Task *>(task));
  (*t)();
}
}

namespace platform
{
bool GuiThread::Push(Task && task)
{
  dispatch_async_f(dispatch_get_main_queue(), new Task(std::move(task)), &PerformImpl);
  return true;
}

bool GuiThread::Push(Task const & task)
{
  dispatch_async_f(dispatch_get_main_queue(), new Task(task), &PerformImpl);
  return true;
}
}  // namespace platform
