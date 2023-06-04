#include "platform/gui_thread.hpp"

#include <utility>

#include <QtCore/QCoreApplication>

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
}  // namespace platform
