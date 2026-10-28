#include "platform/gui_thread.hpp"

#include "base/assert.hpp"

#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QMetaObject>

namespace platform
{
base::TaskLoop::PushResult GuiThread::Push(Task && task)
{
  auto const application = QCoreApplication::instance();
  ASSERT(application, ("GUI tasks require an application instance"));
  return {QMetaObject::invokeMethod(application, std::move(task), Qt::QueuedConnection), kNoId};
}

base::TaskLoop::PushResult GuiThread::Push(Task const & task)
{
  return Push(Task(task));
}
}  // namespace platform
