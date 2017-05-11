#include "drape_frontend/screen_quad_renderer.hpp"
#include "drape_frontend/shader_def.hpp"

#include "drape/data_buffer.hpp"
#include "drape/glconstants.hpp"
#include "drape/glextensions_list.hpp"
#include "drape/glfunctions.hpp"
#include "drape/glstate.hpp"
#include "drape/gpu_program_manager.hpp"
#include "drape/uniform_values_storage.hpp"

#include <vector>

namespace df
{
ScreenQuadRenderer::~ScreenQuadRenderer()
{
  if (m_bufferId != 0)
    GLFunctions::glDeleteBuffer(m_bufferId);
}

void ScreenQuadRenderer::Build(ref_ptr<dp::GpuProgram> prg)
{
  m_attributePosition = prg->GetAttributeLocation("a_pos");
  ASSERT_NOT_EQUAL(m_attributePosition, -1, ());

  m_attributeTexCoord = prg->GetAttributeLocation("a_tcoord");
  ASSERT_NOT_EQUAL(m_attributeTexCoord, -1, ());

  m_textureLocation = prg->GetUniformLocation("u_colorTex");
  ASSERT_NOT_EQUAL(m_textureLocation, -1, ());

  std::vector<float> vertices = {-1.0f, 1.0f,  m_textureRect.minX(), m_textureRect.maxY(),
                                 1.0f,  1.0f,  m_textureRect.maxX(), m_textureRect.maxY(),
                                 -1.0f, -1.0f, m_textureRect.minX(), m_textureRect.minY(),
                                 1.0f,  -1.0f, m_textureRect.maxX(), m_textureRect.minY()};

  m_bufferId = GLFunctions::glGenBuffer();
  GLFunctions::glBindBuffer(m_bufferId, gl_const::GLArrayBuffer);
  GLFunctions::glBufferData(gl_const::GLArrayBuffer,
                            static_cast<uint32_t>(vertices.size()) * sizeof(vertices[0]),
                            vertices.data(), gl_const::GLStaticDraw);
  GLFunctions::glBindBuffer(0, gl_const::GLArrayBuffer);
}

void ScreenQuadRenderer::RenderTexture(uint32_t textureId, ref_ptr<dp::GpuProgramManager> mng,
                                       float opacity)
{
  // Unbind current VAO, because glVertexAttributePointer and glEnableVertexAttribute can affect it.
  if (dp::GLExtensionsList::Instance().IsSupported(dp::GLExtensionsList::VertexArrayObject))
    GLFunctions::glBindVertexArray(0);

  ref_ptr<dp::GpuProgram> prg = mng->GetProgram(gpu::SCREEN_QUAD_PROGRAM);
  prg->Bind();

  if (m_bufferId == 0)
    Build(prg);

  if (m_textureLocation >= 0)
  {
    GLFunctions::glActiveTexture(gl_const::GLTexture0);
    GLFunctions::glBindTexture(textureId);
    GLFunctions::glUniformValuei(m_textureLocation, 0);
    GLFunctions::glTexParameter(gl_const::GLMinFilter, gl_const::GLLinear);
    GLFunctions::glTexParameter(gl_const::GLMagFilter, gl_const::GLLinear);
    GLFunctions::glTexParameter(gl_const::GLWrapS, gl_const::GLClampToEdge);
    GLFunctions::glTexParameter(gl_const::GLWrapT, gl_const::GLClampToEdge);
  }

  GLFunctions::glBindBuffer(m_bufferId, gl_const::GLArrayBuffer);

  GLFunctions::glEnableVertexAttribute(m_attributePosition);
  GLFunctions::glVertexAttributePointer(m_attributePosition, 2, gl_const::GLFloatType, false,
                                        sizeof(float) * 4, 0);
  GLFunctions::glEnableVertexAttribute(m_attributeTexCoord);
  GLFunctions::glVertexAttributePointer(m_attributeTexCoord, 2, gl_const::GLFloatType, false,
                                        sizeof(float) * 4, sizeof(float) * 2);

  dp::UniformValuesStorage uniforms;
  uniforms.SetFloatValue("u_opacity", opacity);
  dp::ApplyUniforms(uniforms, prg);

  GLFunctions::glEnable(gl_const::GLBlending);
  GLFunctions::glDrawArrays(gl_const::GLTriangleStrip, 0, 4);
  GLFunctions::glDisable(gl_const::GLBlending);

  prg->Unbind();
  GLFunctions::glBindTexture(0);
  GLFunctions::glBindBuffer(0, gl_const::GLArrayBuffer);
}

void ScreenQuadRenderer::SetTextureRect(m2::RectF const & rect, ref_ptr<dp::GpuProgramManager> mng)
{
  m_textureRect = rect;

  if (m_bufferId != 0)
    GLFunctions::glDeleteBuffer(m_bufferId);

  ref_ptr<dp::GpuProgram> prg = mng->GetProgram(gpu::SCREEN_QUAD_PROGRAM);
  prg->Bind();
  Build(prg);
  prg->Unbind();
}
}  // namespace df
