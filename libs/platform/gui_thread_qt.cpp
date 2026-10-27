#include "platform/gui_thread.hpp"

#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QMetaObject>

namespace platform
{
base::TaskLoop::PushResult GuiThread::Push(Task && task)
{
  // Following hack is used to post on main message loop |fn| when
  // |source| is destroyed (at the exit of the code block).
  QObject source;
  QObject::connect(&source, &QObject::destroyed, QCoreApplication::instance(), std::move(task));

  return {true, base::TaskLoop::kNoId};
}

base::TaskLoop::PushResult GuiThread::Push(Task const & task)
{
  // Following hack is used to post on main message loop |fn| when
  // |source| is destroyed (at the exit of the code block).
  QObject source;
  QObject::connect(&source, &QObject::destroyed, QCoreApplication::instance(), task);

  return {true, base::TaskLoop::kNoId};
}

base::TaskLoop::PushResult GuiThread::PushDeferred(Task && task)
{
  return {QMetaObject::invokeMethod(QCoreApplication::instance(), std::move(task), Qt::QueuedConnection), kNoId};
}
}  // namespace platform
