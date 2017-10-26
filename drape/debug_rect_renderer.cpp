#include "drape/debug_rect_renderer.hpp"

#include "drape/glextensions_list.hpp"
#include "drape/glfunctions.hpp"
#include "drape/gpu_program_manager.hpp"

namespace dp
{
namespace
{
m2::PointF PixelPointToScreenSpace(ScreenBase const & screen, m2::PointF const & pt)
{
  float const szX = static_cast<float>(screen.PixelRectIn3d().SizeX());
  float const szY = static_cast<float>(screen.PixelRectIn3d().SizeY());
  return m2::PointF(pt.x / szX - 0.5f, -pt.y / szY + 0.5f) * 2.0f;
}
}  // namespace

DebugRectRenderer & DebugRectRenderer::Instance()
{
  static DebugRectRenderer renderer;
  return renderer;
}

DebugRectRenderer::DebugRectRenderer()
  : m_VAO(0)
  , m_vertexBuffer(0)
  , m_isEnabled(false)
{}

DebugRectRenderer::~DebugRectRenderer()
{
  ASSERT_EQUAL(m_VAO, 0, ());
  ASSERT_EQUAL(m_vertexBuffer, 0, ());
}

void DebugRectRenderer::Init(ref_ptr<dp::GpuProgramManager> mng, int programId)
{
  if (dp::GLExtensionsList::Instance().IsSupported(dp::GLExtensionsList::VertexArrayObject))
  {
    m_VAO = GLFunctions::glGenVertexArray();
    GLFunctions::glBindVertexArray(m_VAO);
  }

  m_vertexBuffer = GLFunctions::glGenBuffer();
  GLFunctions::glBindBuffer(m_vertexBuffer, gl_const::GLArrayBuffer);
  m_program = mng->GetProgram(programId);
  int8_t attributeLocation = m_program->GetAttributeLocation("a_position");
  ASSERT_NOT_EQUAL(attributeLocation, -1, ());
  GLFunctions::glEnableVertexAttribute(attributeLocation);
  GLFunctions::glVertexAttributePointer(attributeLocation, 2, gl_const::GLFloatType, false,
                                        sizeof(float) * 2, 0);

  if (m_VAO != 0)
    GLFunctions::glBindVertexArray(0);

  GLFunctions::glBindBuffer(0, gl_const::GLArrayBuffer);
}

void DebugRectRenderer::Destroy()
{
  if (m_vertexBuffer != 0)
  {
    GLFunctions::glDeleteBuffer(m_vertexBuffer);
    m_vertexBuffer = 0;
  }

  if (m_VAO != 0)
  {
    GLFunctions::glDeleteVertexArray(m_VAO);
    m_VAO = 0;
  }

  m_program = nullptr;
}

bool DebugRectRenderer::IsEnabled() const
{
  return m_isEnabled;
}

void DebugRectRenderer::SetEnabled(bool enabled)
{
  m_isEnabled = enabled;
}

void DebugRectRenderer::DrawRect(ScreenBase const & screen, m2::RectF const & rect,
                                 dp::Color const & color) const
{
  if (!m_isEnabled)
    return;

  ASSERT(m_program != nullptr, ());
  m_program->Bind();

  GLFunctions::glBindBuffer(m_vertexBuffer, gl_const::GLArrayBuffer);

  if (m_VAO != 0)
    GLFunctions::glBindVertexArray(m_VAO);

  array<m2::PointF, 5> vertices;
  vertices[0] = PixelPointToScreenSpace(screen, rect.LeftBottom());
  vertices[1] = PixelPointToScreenSpace(screen, rect.LeftTop());
  vertices[2] = PixelPointToScreenSpace(screen, rect.RightTop());
  vertices[3] = PixelPointToScreenSpace(screen, rect.RightBottom());
  vertices[4] = vertices[0];

  GLFunctions::glBufferData(gl_const::GLArrayBuffer,
                            static_cast<uint32_t>(vertices.size() * sizeof(vertices[0])),
                            vertices.data(), gl_const::GLStaticDraw);

  int8_t const location = m_program->GetUniformLocation("u_color");
  if (location >= 0)
    GLFunctions::glUniformValuef(location, color.GetRedF(), color.GetGreenF(),
                                 color.GetBlueF(), color.GetAlphaF());

  GLFunctions::glDrawArrays(gl_const::GLLineStrip, 0, static_cast<uint32_t>(vertices.size()));

  if (m_VAO != 0)
    GLFunctions::glBindVertexArray(0);

  GLFunctions::glBindBuffer(0, gl_const::GLArrayBuffer);

  m_program->Unbind();
}

void DebugRectRenderer::DrawArrow(ScreenBase const & screen,
                                  OverlayTree::DisplacementData const & data) const
{
  if (!m_isEnabled)
    return;

  if (data.m_arrowStart.EqualDxDy(data.m_arrowEnd, 1e-5))
    return;

  ASSERT(m_program != nullptr, ());
  m_program->Bind();

  GLFunctions::glBindBuffer(m_vertexBuffer, gl_const::GLArrayBuffer);

  if (m_VAO != 0)
    GLFunctions::glBindVertexArray(m_VAO);

  array<m2::PointF, 5> vertices;
  m2::PointF const dir = (data.m_arrowEnd - data.m_arrowStart).Normalize();
  m2::PointF const side = m2::PointF(-dir.y, dir.x);
  vertices[0] = PixelPointToScreenSpace(screen, data.m_arrowStart);
  vertices[1] = PixelPointToScreenSpace(screen, data.m_arrowEnd);
  vertices[2] = PixelPointToScreenSpace(screen, data.m_arrowEnd - dir * 20 + side * 10);
  vertices[3] = vertices[1];
  vertices[4] = PixelPointToScreenSpace(screen, data.m_arrowEnd - dir * 20 - side * 10);

  GLFunctions::glBufferData(gl_const::GLArrayBuffer,
                            static_cast<uint32_t>(vertices.size() * sizeof(vertices[0])),
                            vertices.data(), gl_const::GLStaticDraw);

  int8_t const location = m_program->GetUniformLocation("u_color");
  if (location >= 0)
    GLFunctions::glUniformValuef(location, data.m_arrowColor.GetRedF(), data.m_arrowColor.GetGreenF(),
                                 data.m_arrowColor.GetBlueF(), data.m_arrowColor.GetAlphaF());

  GLFunctions::glDrawArrays(gl_const::GLLineStrip, 0, static_cast<uint32_t>(vertices.size()));

  if (m_VAO != 0)
    GLFunctions::glBindVertexArray(0);

  GLFunctions::glBindBuffer(0, gl_const::GLArrayBuffer);

  m_program->Unbind();
}
}  // namespace dp
