#include "qt_tstfrm/test_main_loop.hpp"

#include <QtCore/QTimer>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLContext>

#include "base/scope_guard.hpp"

#include <cstring>
#include <memory>

namespace
{
class MyWidget : public QWidget
{
public:
  explicit MyWidget(RenderFunction && fn)
    : m_fn(std::move(fn))
  {}

protected:
  void paintEvent(QPaintEvent * e) override
  {
    m_fn(this);
  }

private:
  RenderFunction m_fn;
};
}  // namespace

void RunTestLoop(char const * testName, RenderFunction && fn, bool autoExit)
{
  auto buf = new char[strlen(testName) + 1];
  MY_SCOPE_GUARD(argvFreeFun, [&buf](){ delete [] buf; });
  strcpy(buf, testName);

  int argc = 1;
  QApplication app(argc, &buf);
  if (autoExit)
    QTimer::singleShot(3000, &app, SLOT(quit()));

  auto widget = new MyWidget(std::move(fn));
  widget->setWindowTitle(testName);
  widget->show();

  app.exec();
  delete widget;
}

void RunTestInOpenGLOffscreenEnvironment(char const * testName, bool apiOpenGLES3,
                                         TestFunction const & fn)
{
  auto buf = new char[strlen(testName) + 1];
  MY_SCOPE_GUARD(argvFreeFun, [&buf](){ delete [] buf; });
  strcpy(buf, testName);

  int argc = 1;
  QApplication app(argc, &buf);

  QSurfaceFormat fmt;
  fmt.setAlphaBufferSize(8);
  fmt.setBlueBufferSize(8);
  fmt.setGreenBufferSize(8);
  fmt.setRedBufferSize(8);
  fmt.setStencilBufferSize(0);
  fmt.setSamples(0);
  fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
  fmt.setSwapInterval(1);
  fmt.setDepthBufferSize(16);
  if (apiOpenGLES3)
  {
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setVersion(3, 2);
  }
  else
  {
    fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    fmt.setVersion(2, 1);
  }

  auto surface = std::make_unique<QOffscreenSurface>();
  surface->setFormat(fmt);
  surface->create();

  auto context = std::make_unique<QOpenGLContext>();
  context->setFormat(fmt);
  context->create();
  context->makeCurrent(surface.get());

  if (fn)
    fn(apiOpenGLES3);

  context->doneCurrent();
  surface->destroy();

  QTimer::singleShot(0, &app, SLOT(quit()));
  app.exec();
}
