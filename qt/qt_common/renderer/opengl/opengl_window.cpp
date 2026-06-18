#include "opengl_window.hpp"

#include <QtGui/QOpenGLFunctions>
#include <QtGui/QSurfaceFormat>
#include <QtWidgets/QApplication>

#include "base/assert.hpp"
#include "qt/qt_common/renderer/opengl/opengl_context_factory.hpp"

namespace
{
QSurfaceFormat GetOpenGLSurfaceFormat(QString const & platformName)
{
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
#if defined(OMIM_OS_LINUX)
  fmt.setRenderableType(QSurfaceFormat::OpenGLES);
  fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
#endif

  // Set the proper OGL version now (needed for "cocoa" or "xcb"), but have troubles with "wayland" devices.
  // It will be resolved later when OGL context is available.
  if (platformName != QString("wayland"))
  {
#if defined(OMIM_OS_LINUX)
    LOG(LINFO, ("Set default OpenGL version to ES 3.0"));
    fmt.setVersion(3, 0);
#else
    LOG(LINFO, ("Set default OGL version to 3.2"));
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setVersion(3, 2);
#endif
  }

#if defined(ENABLE_OPENGL_DIAGNOSTICS)
  fmt.setOption(QSurfaceFormat::DebugContext);
#endif
  return fmt;
}
}  // namespace

namespace qt::common::renderer::opengl
{
OpenGLWindow::OpenGLWindow(Framework & framework, QWindow * parent)
  : RendererWindow(framework, OpenGLSurface, GetOpenGLSurfaceFormat(QApplication::platformName()), parent)
  , m_glContext(nullptr)
  , m_contextFactory(nullptr)
  , m_updateTimer(std::make_unique<QTimer>(this))
{
  InitTimer();
}

OpenGLWindow::~OpenGLWindow()
{
  m_framework.EnterBackground();
  m_framework.SetRenderingDisabled(true);

  if (m_contextFactory != nullptr)
  {
    m_contextFactory->PrepareToShutdown();
    m_framework.DestroyDrapeEngine();
    m_contextFactory.reset();
  }

  if (m_glContext)
    m_glContext->doneCurrent();
}

void OpenGLWindow::Render()
{
  if (!isExposed())
    return;

  EnsureInitialized();
  VERIFY(m_glContext->makeCurrent(this), ());

  if (m_program == nullptr)
    BuildShaders();

  if (!m_contextFactory->AcquireFrame())
  {
    m_glContext->doneCurrent();
    return;
  }

  m_vao->bind();
  m_program->bind();

  QOpenGLFunctions * funcs = m_glContext->functions();
  funcs->glActiveTexture(GL_TEXTURE0);
  GLuint const image = m_contextFactory->GetTextureHandle();
  funcs->glBindTexture(GL_TEXTURE_2D, image);

  int const samplerLocation = m_program->uniformLocation("u_sampler");
  m_program->setUniformValue(samplerLocation, 0);

  QRectF const & texRect = m_contextFactory->GetTexRect();
  QVector2D const samplerSize(texRect.width(), texRect.height());

  int const samplerSizeLocation = m_program->uniformLocation("u_samplerSize");
  m_program->setUniformValue(samplerSizeLocation, samplerSize);

  funcs->glClearColor(0.0, 0.0, 0.0, 1.0);
  funcs->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  funcs->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  m_program->release();
  m_vao->release();

  m_glContext->swapBuffers(this);
  m_glContext->doneCurrent();
}

void OpenGLWindow::InitTimer()
{
  // Update frequency is 60 fps.
  connect(m_updateTimer.get(), &QTimer::timeout, this, [this]
  {
    if (isExposed())
      requestUpdate();
  });
  m_updateTimer->setSingleShot(false);
  m_updateTimer->start(1000 / 60);
}

void OpenGLWindow::EnsureInitialized()
{
  if (m_contextFactory != nullptr)
    return;

  ASSERT(m_glContext == nullptr, ());
  m_ratio = static_cast<float>(devicePixelRatio());

  m_glContext = std::make_unique<QOpenGLContext>(this);
  m_glContext->setFormat(format());
  VERIFY(m_glContext->create(), ());
  VERIFY(m_glContext->makeCurrent(this), ());

  m_contextFactory.reset(new QtOGLContextFactory(m_glContext.get()));

  CreateDrapeEngine(dp::ApiVersion::OpenGLES3, make_ref(m_contextFactory));
  m_framework.EnterForeground();

  m_glContext->doneCurrent();
  m_contextFactory->WaitForInitialization(nullptr);

  VERIFY(m_glContext->makeCurrent(this), ());
  OnResize(width(), height());
}

void OpenGLWindow::BuildShaders()
{
  std::string_view vertexSrc;
  std::string_view fragmentSrc;
#if defined(OMIM_OS_LINUX)
  vertexSrc = ":common/renderer/opengl/shaders/gles_300.vsh.glsl";
  fragmentSrc = ":common/renderer/opengl/shaders/gles_300.fsh.glsl";
#else
  vertexSrc = ":common/renderer/opengl/shaders/gl_150.vsh.glsl";
  fragmentSrc = ":common/renderer/opengl/shaders/gl_150.fsh.glsl";
#endif

  m_program = std::make_unique<QOpenGLShaderProgram>(this);
  m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, vertexSrc.data());
  m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, fragmentSrc.data());
  m_program->link();

  m_vao = std::make_unique<QOpenGLVertexArrayObject>(this);
  m_vao->create();
  m_vao->bind();

  m_vbo = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
  m_vbo->setUsagePattern(QOpenGLBuffer::StaticDraw);
  m_vbo->create();
  m_vbo->bind();

  QVector4D constexpr vertices[4] = {QVector4D(-1.0, 1.0, 0.0, 1.0), QVector4D(1.0, 1.0, 1.0, 1.0),
                                     QVector4D(-1.0, -1.0, 0.0, 0.0), QVector4D(1.0, -1.0, 1.0, 0.0)};
  m_vbo->allocate(vertices, sizeof(vertices));
  QOpenGLFunctions * f = QOpenGLContext::currentContext()->functions();
  // 0-index of the buffer is linked to "a_position" attribute in vertex shader.
  // Introduced in https://github.com/organicmaps/organicmaps/pull/9814
  f->glEnableVertexAttribArray(0);
  f->glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(QVector4D), nullptr);

  m_program->release();
  m_vao->release();
}
}  // namespace qt::common::renderer::opengl
