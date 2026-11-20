#include "qt/qt_common/frame_blit.hpp"

#include "base/assert.hpp"

#include <QOpenGLContext>
#include <QOpenGLFunctions>

namespace qt::common
{
FrameBlit::FrameBlit()
{
  bool const isGles = QOpenGLContext::currentContext()->isOpenGLES();
  auto const * vertexShader = isGles ? ":common/shaders/gles_300.vsh.glsl" : ":common/shaders/gl_150.vsh.glsl";
  auto const * fragmentShader = isGles ? ":common/shaders/gles_300.fsh.glsl" : ":common/shaders/gl_150.fsh.glsl";

  CHECK(m_program.addShaderFromSourceFile(QOpenGLShader::Vertex, vertexShader),
        (vertexShader, m_program.log().toStdString()));
  CHECK(m_program.addShaderFromSourceFile(QOpenGLShader::Fragment, fragmentShader),
        (fragmentShader, m_program.log().toStdString()));
  CHECK(m_program.link(), (vertexShader, fragmentShader, m_program.log().toStdString()));

  // Desktop core OpenGL requires a VAO even when the shader generates the vertices.
  CHECK(m_vao.create(), ("Cannot create the map frame VAO"));
}

void FrameBlit::Draw(GLuint texture)
{
  auto * functions = QOpenGLContext::currentContext()->functions();
  if (texture == 0)
  {
    // QOpenGLWidget can discard the previous color buffer.
    functions->glClearColor(0.0, 0.0, 0.0, 1.0);
    functions->glClear(GL_COLOR_BUFFER_BIT);
    return;
  }

  m_vao.bind();
  m_program.bind();
  functions->glActiveTexture(GL_TEXTURE0);  // The linked sampler defaults to unit 0.
  functions->glBindTexture(GL_TEXTURE_2D, texture);
  functions->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  m_program.release();
  m_vao.release();
}
}  // namespace qt::common
