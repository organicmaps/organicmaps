#include "renderer3d.hpp"

#include "drape/data_buffer.hpp"
#include "drape/glconstants.hpp"
#include "drape/glfunctions.hpp"
#include "drape/glstate.hpp"
#include "drape/shader_def.hpp"
#include "drape/uniform_values_storage.hpp"

#include "math.h"

namespace df
{

Renderer3d::Renderer3d()
  : m_width(0)
  , m_height(0)
  , m_VAO(0)
  , m_bufferId(0)
{
  m_vertices[0] = -1.0f;
  m_vertices[1] = 1.0;
  m_vertices[2] = 0.0f;
  m_vertices[3] = 1.0f;

  m_vertices[4] = 1.0f;
  m_vertices[5] = 1.0f;
  m_vertices[6] = 1.0f;
  m_vertices[7] = 1.0f;

  m_vertices[8] = -1.0f;
  m_vertices[9] = -1.0f;
  m_vertices[10] = 0.0f;
  m_vertices[11] = 0.0f;

  m_vertices[12] = 1.0f;
  m_vertices[13] = -1.0f;
  m_vertices[14] = 1.0f;
  m_vertices[15] = 0.0f;
}

Renderer3d::~Renderer3d()
{
  if (m_bufferId)
    GLFunctions::glDeleteBuffer(m_bufferId);
  if (m_VAO)
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
  assert(attributeLocation != -1);
  GLFunctions::glEnableVertexAttribute(attributeLocation);
  GLFunctions::glVertexAttributePointer(attributeLocation, 2, gl_const::GLFloatType, false,
                                        sizeof(float) * 4, 0);

  attributeLocation = prg->GetAttributeLocation("a_tcoord");
  assert(attributeLocation != -1);
  GLFunctions::glEnableVertexAttribute(attributeLocation);
  GLFunctions::glVertexAttributePointer(attributeLocation, 2, gl_const::GLFloatType, false,
                                        sizeof(float) * 4, sizeof(float) * 2);
}

void Renderer3d::Render(ScreenBase const & screen, uint32_t textureId, ref_ptr<dp::GpuProgramManager> mng)
{
  ref_ptr<dp::GpuProgram> prg = mng->GetProgram(gpu::TEXTURING_3D_PROGRAM);
  prg->Bind();
  
  if (!m_VAO)
    Build(prg);

  ScreenBase::Matrix3dT const & PTo3d = screen.PTo3dMatrix();
  float transform[16];
  copy(begin(PTo3d.m_data), end(PTo3d.m_data), transform);

  dp::UniformValuesStorage uniforms;
  uniforms.SetIntValue("tex", 0);
  uniforms.SetMatrix4x4Value("m_transform", transform);

  dp::ApplyUniforms(uniforms, prg);

  GLFunctions::glDisable(gl_const::GLDepthTest);

  GLFunctions::glActiveTexture(gl_const::GLTexture0);
  GLFunctions::glBindTexture(textureId);
  GLFunctions::glBindBuffer(m_bufferId, gl_const::GLArrayBuffer);
  GLFunctions::glBindVertexArray(m_VAO);

  GLFunctions::glBufferData(gl_const::GLArrayBuffer, m_vertices.size() * sizeof(m_vertices[0]),
                            m_vertices.data(), gl_const::GLStaticDraw);

  GLFunctions::glViewport(0, 0, m_width, m_height);
  GLFunctions::glClear();

  GLFunctions::glDrawArrays(gl_const::GLTriangleStrip, 0, 4);

  prg->Unbind();
  GLFunctions::glBindTexture(0);
  GLFunctions::glBindVertexArray(0);
  GLFunctions::glBindBuffer(0, gl_const::GLArrayBuffer);
}

}
