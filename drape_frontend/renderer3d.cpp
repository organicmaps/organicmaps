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
  , m_fov(M_PI / 3.0f)
  , m_angleX(-M_PI_4)
  , m_offsetZ(0.0f)
  , m_offsetY(0.0f)
  , m_offsetX(0.0f)
  , m_scaleX(1.0)
  , m_scaleY(1.0)
  , m_VAO(0)
  , m_bufferId(0)
{
  SetPlaneAngleX(m_angleX);
  SetVerticalFOV(m_fov);
  CalculateGeometry();
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

  UpdateProjectionMatrix();
}

void Renderer3d::SetVerticalFOV(float fov)
{
  m_fov = fov;

  CalculateGeometry();
  UpdateProjectionMatrix();
}

void Renderer3d::CalculateGeometry()
{
  m_offsetZ = 1 / tan(m_fov / 2.0) + sin(-m_angleX);
  m_offsetY = -1 + cos(-m_angleX);

  m_scaleY = cos(-m_angleX) + sin(-m_angleX) * tan(m_fov / 2.0 - m_angleX);
  m_scaleX = 1.0 + 2*sin(-m_angleX) * cos(m_fov / 2.0) / (m_offsetZ * cos(m_fov / 2.0 - m_angleX));
  m_scaleX = m_scaleY;
/*
  const float vertices[] =
  {
    -1.0f,  1.0f, 0.0f, 1.0f,
     1.0f,  1.0f, 1.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f,
     1.0f, -1.0f, 1.0f, 0.0f
  };
*/
  m_vertices[0] = -1.0f * m_scaleX;
  m_vertices[1] = 2.0f * m_scaleY - 1.0f;
  m_vertices[2] = 0.0f;
  m_vertices[3] = 1.0f;

  m_vertices[4] = 1.0f * m_scaleX;
  m_vertices[5] = 2.0f * m_scaleY - 1.0f;
  m_vertices[6] = 1.0f;
  m_vertices[7] = 1.0f;

  m_vertices[8] = -1.0f * m_scaleX;
  m_vertices[9] = -1.0f;
  m_vertices[10] = 0.0f;
  m_vertices[11] = 0.0f;

  m_vertices[12] = 1.0f * m_scaleX;
  m_vertices[13] = -1.0f;
  m_vertices[14] = 1.0f;
  m_vertices[15] = 0.0f;

  UpdateRotationMatrix();
  UpdateTranslationMatrix();
}

float Renderer3d::GetScaleX() const
{
  return m_scaleX;
}

float Renderer3d::GetScaleY() const
{
  return m_scaleY;
}

void Renderer3d::SetPlaneAngleX(float angleX)
{
  m_angleX = angleX;
  CalculateGeometry();
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

void Renderer3d::Render(uint32_t textureId, ref_ptr<dp::GpuProgramManager> mng)
{
  ref_ptr<dp::GpuProgram> prg = mng->GetProgram(gpu::TEXTURING_3D_PROGRAM);
  prg->Bind();
  
  if (!m_VAO)
    Build(prg);

  dp::UniformValuesStorage uniforms;
  uniforms.SetIntValue("tex", 0);
  uniforms.SetMatrix4x4Value("rotate", m_rotationMatrix.data());
  uniforms.SetMatrix4x4Value("translate", m_translationMatrix.data());
  uniforms.SetMatrix4x4Value("projection", m_projectionMatrix.data());

  dp::ApplyUniforms(uniforms, prg);

  GLFunctions::glDisable(gl_const::GLDepthTest);

  GLFunctions::glActiveTexture(gl_const::GLTexture0);
  GLFunctions::glBindTexture(textureId);
  GLFunctions::glBindBuffer(m_bufferId, gl_const::GLArrayBuffer);
  GLFunctions::glBindVertexArray(m_VAO);



  GLFunctions::glBufferData(gl_const::GLArrayBuffer, sizeof(m_vertices), m_vertices, gl_const::GLStaticDraw);

  GLFunctions::glViewport(0, 0, m_width, m_height);
  GLFunctions::glClear();
  GLFunctions::glDrawArrays(gl_const::GLTriangleStrip, 0, 4);

  prg->Unbind();
  GLFunctions::glBindTexture(0);
  GLFunctions::glBindVertexArray(0);
  GLFunctions::glBindBuffer(0, gl_const::GLArrayBuffer);
}

void Renderer3d::UpdateProjectionMatrix()
{
  float ctg_fovy = 1.0/tanf(m_fov/2.0f);
  float aspect = 1.0;
  float near = 0.1f;
  float far = 100.0f;

  m_projectionMatrix.fill(0.0f);

  m_projectionMatrix[0] = ctg_fovy / aspect;
  m_projectionMatrix[5] = ctg_fovy;
  m_projectionMatrix[10] = (far + near) / (far - near);
  m_projectionMatrix[11] = 1.0f;
  m_projectionMatrix[14] = -2 * far * near / (far - near);
}

void Renderer3d::UpdateRotationMatrix()
{
  m_rotationMatrix.fill(0.0f);

  m_rotationMatrix[0] = 1.0f;
  m_rotationMatrix[5] = cos(m_angleX);
  m_rotationMatrix[6] = -sin(m_angleX);
  m_rotationMatrix[9] = sin(m_angleX);
  m_rotationMatrix[10] = cos(m_angleX);
  m_rotationMatrix[15] = 1.0f;
}

void Renderer3d::UpdateTranslationMatrix()
{
  m_translationMatrix.fill(0.0f);

  float dx = 0.0f;
  float dy = m_offsetY;
  float dz = m_offsetZ;

  m_translationMatrix[0] = 1.0f;
  m_translationMatrix[5] = 1.0f;
  m_translationMatrix[10] = 1.0f;
  m_translationMatrix[12] = dx;
  m_translationMatrix[13] = dy;
  m_translationMatrix[14] = dz;
  m_translationMatrix[15] = 1.0f;
}

}
