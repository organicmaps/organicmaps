#include "test_main_loop.hpp"

#include <QtCore/QTimer>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

#include "../base/scope_guard.hpp"

#include "../std/cstring.hpp"

TestMainLoop::TestMainLoop(TestMainLoop::TRednerFn const & fn)
  : m_renderFn(fn)
{
}

void TestMainLoop::exec(char const * testName)
{
  char * buf = (char *)malloc(strlen(testName) + 1);
  MY_SCOPE_GUARD(argvFreeFun, [&buf](){ free(buf); });
  strcpy(buf, testName);

  int argc = 1;
  QApplication app(argc, &buf);
  QTimer::singleShot(3000, &app, SLOT(quit()));

  QWidget w;
  w.setWindowTitle(testName);
  w.show();
  w.installEventFilter(this);

  app.exec();
}

bool TestMainLoop::eventFilter(QObject * obj, QEvent * event)
{
  if (event->type() == QEvent::Paint)
  {
    m_renderFn(qobject_cast<QWidget *>(obj));
    return true;
  }

  return false;
}
