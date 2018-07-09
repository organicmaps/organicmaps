#include "platform/gui_thread.hpp"

#include <utility>

#include <QtCore/QCoreApplication>

namespace platform
{
bool GuiThread::Push(Task && task)
{
  // Following hack is used to post on main message loop |fn| when
  // |source| is destroyed (at the exit of the code block).
  QObject source;
  return QObject::connect(&source, &QObject::destroyed, QCoreApplication::instance(), std::move(task));
}

bool GuiThread::Push(Task const & task)
{
  // Following hack is used to post on main message loop |fn| when
  // |source| is destroyed (at the exit of the code block).
  QObject source;
  return QObject::connect(&source, &QObject::destroyed, QCoreApplication::instance(), task);
}
}  // namespace platform
