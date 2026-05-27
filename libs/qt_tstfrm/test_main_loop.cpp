#include "qt_tstfrm/test_main_loop.hpp"

#include "base/logging.hpp"

#include <QtCore/QTimer>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLContext>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

#include <memory>

namespace
{
class MyWidget : public QWidget
{
public:
  explicit MyWidget(testing::RenderFunction && fn) : m_fn(std::move(fn)) {}

protected:
  void paintEvent(QPaintEvent * e) override { m_fn(this); }

private:
  testing::RenderFunction m_fn;
};
}  // namespace

void RunTestLoop(std::string testName, testing::RenderFunction && fn, bool autoExit)
{
  char * raw = testName.data();

  int argc = 1;
  QApplication app(argc, &raw);

  if (autoExit)
    QTimer::singleShot(3000, &app, SLOT(quit()));

  auto widget = std::make_unique<MyWidget>(std::move(fn));
  widget->setWindowTitle(testName.c_str());
  widget->show();

  app.exec();
}

void RunTestInOpenGLOffscreenEnvironment(std::string testName, testing::TestFunction const & fn)
{
  char * raw = testName.data();

  int argc = 1;
  QApplication app(argc, &raw);

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
  fmt.setProfile(QSurfaceFormat::CoreProfile);
  fmt.setVersion(3, 2);

  auto surface = std::make_unique<QOffscreenSurface>();
  surface->setFormat(fmt);
  surface->create();

  auto context = std::make_unique<QOpenGLContext>();
  context->setFormat(fmt);
  context->create();

  if (context->isValid())
  {
    context->makeCurrent(surface.get());

    if (fn)
      fn();

    context->doneCurrent();
  }
  else
    LOG(LWARNING, ("OpenGL context is not available, skipping OGL render"));

  surface->destroy();

  QTimer::singleShot(0, &app, SLOT(quit()));
  app.exec();
}
