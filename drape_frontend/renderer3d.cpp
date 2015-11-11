#include "renderer3d.hpp"

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

Renderer3d::Renderer3d()
{
  m_vertices = { -1.0f,  1.0f, 0.0f, 1.0f,
                  1.0f,  1.0f, 1.0f, 1.0f,
                 -1.0f, -1.0f, 0.0f, 0.0f,
                  1.0f, -1.0f, 1.0f, 0.0f };
}

Renderer3d::~Renderer3d()
{
  if (m_bufferId != 0)
    GLFunctions::glDeleteBuffer(m_bufferId);
  if (m_VAO != 0)
    GLFunctions::glDeleteVertexArray(m_VAO);
}

void Renderer3d::SetSize(uint32_t width, uint32_t height)
{
  m_width = width;
  m_height = height;
}

void Renderer3d::Build(ref_ptr<dp::GpuProgram> prg)
{
  m_bufferId = GLFunctions::glGenBuffer();
  GLFunctions::glBindBuffer(m_bufferId, gl_const::GLArrayBuffer);

  m_VAO = GLFunctions::glGenVertexArray();
  GLFunctions::glBindVertexArray(m_VAO);

  int8_t attributeLocation = prg->GetAttributeLocation("a_pos");
  ASSERT_NOT_EQUAL(attributeLocation, -1, ());
  GLFunctions::glEnableVertexAttribute(attributeLocation);
  GLFunctions::glVertexAttributePointer(attributeLocation, 2, gl_const::GLFloatType, false,
                                        sizeof(float) * 4, 0);

  attributeLocation = prg->GetAttributeLocation("a_tcoord");
  ASSERT_NOT_EQUAL(attributeLocation, -1, ());
  GLFunctions::glEnableVertexAttribute(attributeLocation);
  GLFunctions::glVertexAttributePointer(attributeLocation, 2, gl_const::GLFloatType, false,
                                        sizeof(float) * 4, sizeof(float) * 2);

  GLFunctions::glBufferData(gl_const::GLArrayBuffer, m_vertices.size() * sizeof(m_vertices[0]),
                            m_vertices.data(), gl_const::GLStaticDraw);

  GLFunctions::glBindVertexArray(0);
  GLFunctions::glBindBuffer(0, gl_const::GLArrayBuffer);
}

void Renderer3d::Render(ScreenBase const & screen, uint32_t textureId, ref_ptr<dp::GpuProgramManager> mng)
{
  ref_ptr<dp::GpuProgram> prg = mng->GetProgram(gpu::TEXTURING_3D_PROGRAM);
  prg->Bind();

  if (m_VAO == 0)
    Build(prg);

  math::Matrix<float, 4, 4> const transform(screen.PTo3dMatrix());

  dp::UniformValuesStorage uniforms;
  uniforms.SetIntValue("tex", 0);
  uniforms.SetMatrix4x4Value("m_transform", transform.m_data);

  dp::ApplyUniforms(uniforms, prg);

  GLFunctions::glDisable(gl_const::GLDepthTest);

  GLFunctions::glActiveTexture(gl_const::GLTexture0);
  GLFunctions::glBindTexture(textureId);
  GLFunctions::glBindBuffer(m_bufferId, gl_const::GLArrayBuffer);
  GLFunctions::glBindVertexArray(m_VAO);

  GLFunctions::glViewport(0, 0, m_width, m_height);
  GLFunctions::glClear();

  GLFunctions::glDrawArrays(gl_const::GLTriangleStrip, 0, 4);

  prg->Unbind();
  GLFunctions::glBindTexture(0);
  GLFunctions::glBindVertexArray(0);
  GLFunctions::glBindBuffer(0, gl_const::GLArrayBuffer);
}

}  // namespace df
