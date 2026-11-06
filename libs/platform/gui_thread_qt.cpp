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
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
  return {QMetaObject::invokeMethod(application, std::move(task), Qt::QueuedConnection), kNoId};
#else
  QObject sender;
  return {static_cast<bool>(
              QObject::connect(&sender, &QObject::destroyed, application, std::move(task), Qt::QueuedConnection)),
          kNoId};
#endif
}

base::TaskLoop::PushResult GuiThread::Push(Task const & task)
{
  return Push(Task(task));
}
}  // namespace platform
