#pragma once

#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

namespace qt::common
{
// Construct and draw with the same OpenGL context current.
class FrameBlit
{
public:
  FrameBlit();
  void Draw(GLuint texture);

private:
  QOpenGLShaderProgram m_program;
  QOpenGLVertexArrayObject m_vao;
};
}  // namespace qt::common
