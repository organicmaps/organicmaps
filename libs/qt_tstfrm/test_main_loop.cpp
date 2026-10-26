#include "qt_tstfrm/test_main_loop.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "std/target_os.hpp"

#include <QtCore/QTimer>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
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
  fmt.setRenderableType(QSurfaceFormat::OpenGL);
  fmt.setProfile(QSurfaceFormat::CoreProfile);
#if defined(OMIM_OS_MAC) || defined(OMIM_OS_WINDOWS)
  fmt.setVersion(4, 1);
#else
  fmt.setVersion(3, 2);
#endif

  QOpenGLContext context;
  context.setFormat(fmt);
  CHECK(context.create(), ("OpenGL context is not available", testName, "requested", fmt.version()));

  QOffscreenSurface surface;
  surface.setFormat(context.format());
  surface.create();
  CHECK(surface.isValid(), ("OpenGL offscreen surface is not available", testName));
  CHECK(context.makeCurrent(&surface), ("Cannot make OpenGL context current", testName));

  auto const actual = context.format();
  auto * functions = context.functions();
  LOG(LINFO, ("OpenGL test context", testName, "requested", fmt.version(), "actual", actual.version(), "profile",
              actual.profile(), "GL", reinterpret_cast<char const *>(functions->glGetString(GL_VERSION)), "GLSL",
              reinterpret_cast<char const *>(functions->glGetString(GL_SHADING_LANGUAGE_VERSION)), "renderer",
              reinterpret_cast<char const *>(functions->glGetString(GL_RENDERER))));
  CHECK_GREATER_OR_EQUAL(actual.version(), fmt.version(), ("Insufficient OpenGL context version", testName));
  CHECK_EQUAL(actual.profile(), fmt.profile(), ("OpenGL core profile is required", testName));

  fn();

  context.doneCurrent();
}
