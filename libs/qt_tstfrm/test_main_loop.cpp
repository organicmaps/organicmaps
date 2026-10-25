#include "qt_tstfrm/test_main_loop.hpp"

#include "base/assert.hpp"

#include <QtCore/QTimer>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLContext>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

#include <utility>

namespace
{
class MyWidget : public QWidget
{
public:
  explicit MyWidget(testing::RenderFunction && fn) : m_fn(std::move(fn)) {}

protected:
  void paintEvent(QPaintEvent *) override { m_fn(this); }

private:
  testing::RenderFunction m_fn;
};
}  // namespace

void RunTestLoop(std::string testName, testing::RenderFunction && fn, bool autoExit)
{
  char * argv[] = {testName.data(), nullptr};
  int argc = 1;
  QApplication app(argc, argv);

  if (autoExit)
    QTimer::singleShot(3000, &app, SLOT(quit()));

  MyWidget widget(std::move(fn));
  widget.setWindowTitle(testName.c_str());
  widget.show();

  app.exec();
}

void RunTestInOpenGLOffscreenEnvironment(std::string testName, testing::TestFunction const & fn)
{
  char * argv[] = {testName.data(), nullptr};
  int argc = 1;
  QApplication app(argc, argv);

  QSurfaceFormat fmt;
  fmt.setProfile(QSurfaceFormat::CoreProfile);
  fmt.setVersion(3, 2);

  QOpenGLContext context;
  context.setFormat(fmt);
  CHECK(context.create(), ("OpenGL context is not available", testName));

  QOffscreenSurface surface;
  surface.setFormat(context.format());
  surface.create();
  CHECK(surface.isValid(), ("OpenGL offscreen surface is not available", testName));
  CHECK(context.makeCurrent(&surface), ("Cannot make OpenGL context current", testName));

  fn();

  context.doneCurrent();
}
