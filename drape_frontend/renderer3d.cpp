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
  , m_offsetZ(0.5f)
  , m_VAO(0)
  , m_bufferId(0)
{
  UpdateProjectionMatrix();
  UpdateRotationMatrix();
  UpdateTranslationMatrix();
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

  UpdateProjectionMatrix();
}

void Renderer3d::SetPlaneAngleX(float angleX)
{
  m_angleX = angleX;

  UpdateRotationMatrix();
}

void Renderer3d::SetPlaneOffsetZ(float offsetZ)
{
  m_offsetZ = offsetZ;

  UpdateTranslationMatrix();
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

  const float vertices[] =
  {
    -1.0f,  1.0f, 0.0f, 1.0f,
     1.0f,  1.0f, 1.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f,
     1.0f, -1.0f, 1.0f, 0.0f
  };
  GLFunctions::glBufferData(gl_const::GLArrayBuffer, sizeof(vertices), vertices, gl_const::GLStaticDraw);
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
  float aspect = (float)m_width / m_height;
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
  float dy = 0.0f;
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
