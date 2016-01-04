#include "transparent_layer.hpp"

#include "drape/data_buffer.hpp"
#include "drape/glconstants.hpp"
#include "drape/glfunctions.hpp"
#include "drape/glstate.hpp"
#include "drape/gpu_program_manager.hpp"
#include "drape/shader_def.hpp"
#include "drape/uniform_values_storage.hpp"

#include "geometry/screenbase.hpp"

#include "std/cmath.hpp"

namespace df
{

TransparentLayer::TransparentLayer()
{
  m_vertices = {
    -1.0f,  1.0f, 0.0f, 1.0f,
     1.0f,  1.0f, 1.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f,
     1.0f, -1.0f, 1.0f, 0.0f };
}

TransparentLayer::~TransparentLayer()
{
  if (m_bufferId != 0)
    GLFunctions::glDeleteBuffer(m_bufferId);
}

void TransparentLayer::Build(ref_ptr<dp::GpuProgram> prg)
{
  m_attributePosition = prg->GetAttributeLocation("a_pos");
  ASSERT_NOT_EQUAL(m_attributePosition, -1, ());

  m_attributeTexCoord = prg->GetAttributeLocation("a_tcoord");
  ASSERT_NOT_EQUAL(m_attributeTexCoord, -1, ());

  m_bufferId = GLFunctions::glGenBuffer();
  GLFunctions::glBindBuffer(m_bufferId, gl_const::GLArrayBuffer);
  GLFunctions::glBufferData(gl_const::GLArrayBuffer, m_vertices.size() * sizeof(m_vertices[0]),
                            m_vertices.data(), gl_const::GLStaticDraw);
  GLFunctions::glBindBuffer(0, gl_const::GLArrayBuffer);
}

void TransparentLayer::Render(uint32_t textureId, ref_ptr<dp::GpuProgramManager> mng)
{
  // Unbind current VAO, because glVertexAttributePointer and glEnableVertexAttribute can affect it.
  GLFunctions::glBindVertexArray(0);

  ref_ptr<dp::GpuProgram> prg = mng->GetProgram(gpu::TRANSPARENT_LAYER_PROGRAM);
  prg->Bind();

  if (m_bufferId == 0)
    Build(prg);

  GLFunctions::glActiveTexture(gl_const::GLTexture0);
  GLFunctions::glBindTexture(textureId);
  GLFunctions::glBindBuffer(m_bufferId, gl_const::GLArrayBuffer);

  GLFunctions::glEnableVertexAttribute(m_attributePosition);
  GLFunctions::glVertexAttributePointer(m_attributePosition, 2, gl_const::GLFloatType, false,
                                        sizeof(float) * 4, 0);
  GLFunctions::glEnableVertexAttribute(m_attributeTexCoord);
  GLFunctions::glVertexAttributePointer(m_attributeTexCoord, 2, gl_const::GLFloatType, false,
                                        sizeof(float) * 4, sizeof(float) * 2);

  GLFunctions::glEnable(gl_const::GLBlending);
  GLFunctions::glDrawArrays(gl_const::GLTriangleStrip, 0, 4);
  GLFunctions::glDisable(gl_const::GLBlending);

  prg->Unbind();
  GLFunctions::glBindTexture(0);
  GLFunctions::glBindVertexArray(0);
  GLFunctions::glBindBuffer(0, gl_const::GLArrayBuffer);
}

}  // namespace df
