#include "qt_tstfrm/test_main_loop.hpp"

#include <QtCore/QTimer>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

#include "base/scope_guard.hpp"

#include "std/cstring.hpp"

namespace
{

class MyWidget : public QWidget
{
public:
  MyWidget(TRednerFn const & fn)
    : m_fn(fn)
  {
  }

protected:
  void paintEvent(QPaintEvent * e)
  {
    m_fn(this);
  }

private:
  TRednerFn m_fn;
};

} // namespace

void RunTestLoop(char const * testName, TRednerFn const & fn, bool autoExit)
{
  char * buf = (char *)malloc(strlen(testName) + 1);
  MY_SCOPE_GUARD(argvFreeFun, [&buf](){ free(buf); });
  strcpy(buf, testName);

  int argc = 1;
  QApplication app(argc, &buf);
  if (autoExit)
    QTimer::singleShot(3000, &app, SLOT(quit()));

  MyWidget * widget = new MyWidget(fn);
  widget->setWindowTitle(testName);
  widget->show();

  app.exec();
  delete widget;
}
