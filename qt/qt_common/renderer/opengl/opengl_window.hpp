#pragma once

#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QTimer>

#include "qt/qt_common/renderer/base/renderer_window.hpp"
#include "qt/qt_common/renderer/opengl/opengl_context_factory.hpp"

namespace qt::common::renderer::opengl
{
class OpenGLWindow : public base::RendererWindow
{
  Q_OBJECT

public:
  explicit OpenGLWindow(Framework & framework, QWindow * parent = nullptr);
  ~OpenGLWindow() override;

protected:
  void Render() override;

private:
  void InitTimer();
  void EnsureInitialized();
  void BuildShaders();

  std::unique_ptr<QOpenGLContext> m_glContext;
  drape_ptr<QtOGLContextFactory> m_contextFactory;
  std::unique_ptr<QOpenGLShaderProgram> m_program;
  std::unique_ptr<QOpenGLVertexArrayObject> m_vao;
  std::unique_ptr<QOpenGLBuffer> m_vbo;

  std::unique_ptr<QTimer> m_updateTimer;
};
}  // namespace qt::common::renderer::opengl
