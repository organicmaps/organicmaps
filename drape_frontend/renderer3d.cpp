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
  , m_offsetY(0.0f)
  , m_offsetZ(0.0f)
  , m_scaleX(1.0)
  , m_scaleY(1.0)
  , m_scaleMatrix(math::Zero<float, 4>())
  , m_rotationMatrix(math::Zero<float, 4>())
  , m_translationMatrix(math::Zero<float, 4>())
  , m_projectionMatrix(math::Zero<float, 4>())
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
}

math::Matrix<float, 4, 4> const & Renderer3d::GetTransform() const
{
  return m_transformMatrix;
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

void Renderer3d::SetVerticalFOV(float fov)
{
  m_fov = fov;
  CalculateGeometry();
}

void Renderer3d::CalculateGeometry()
{
  float cameraZ = 1 / tan(m_fov / 2.0);

  m_scaleY = cos(-m_angleX) + sin(-m_angleX) * tan(m_fov / 2.0 - m_angleX);
  m_scaleX = 1.0 + 2*sin(-m_angleX) * cos(m_fov / 2.0) / (cameraZ * cos(m_fov / 2.0 - m_angleX));
  m_scaleX = m_scaleY = max(m_scaleX, m_scaleY);

  m_offsetZ = cameraZ + sin(-m_angleX) * m_scaleY;
  m_offsetY = cos(-m_angleX) * m_scaleX - 1.0;

/*
  const float vertices[] =
  {
    -1.0f,  1.0f, 0.0f, 1.0f,
     1.0f,  1.0f, 1.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f,
     1.0f, -1.0f, 1.0f, 0.0f
  };
*/
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

  UpdateScaleMatrix();
  UpdateRotationMatrix();
  UpdateTranslationMatrix();
  UpdateProjectionMatrix();

  m_transformMatrix =  m_scaleMatrix * m_rotationMatrix * m_translationMatrix * m_projectionMatrix;
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
  uniforms.SetMatrix4x4Value("m_transform", m_transformMatrix.m_data);

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

void Renderer3d::UpdateProjectionMatrix()
{
  float ctg_fovy = 1.0 / tanf(m_fov / 2.0f);
  float aspect = 1.0;
  float near = 0.1f;
  float far = 100.0f;

  m_projectionMatrix(0, 0) = ctg_fovy / aspect;
  m_projectionMatrix(1, 1) = ctg_fovy;
  m_projectionMatrix(2, 2) = (far + near) / (far - near);
  m_projectionMatrix(2, 3) = 1.0f;
  m_projectionMatrix(3, 2) = -2 * far * near / (far - near);
}

void Renderer3d::UpdateScaleMatrix()
{
  m_scaleMatrix(0, 0) = m_scaleX;
  m_scaleMatrix(1, 1) = m_scaleY;
  m_scaleMatrix(2, 2) = 1.0f;
  m_scaleMatrix(3, 3) = 1.0f;
}

void Renderer3d::UpdateRotationMatrix()
{
  m_rotationMatrix(0, 0) = 1.0f;
  m_rotationMatrix(1, 1) = cos(m_angleX);
  m_rotationMatrix(1, 2) = -sin(m_angleX);
  m_rotationMatrix(2, 1) = sin(m_angleX);
  m_rotationMatrix(2, 2) = cos(m_angleX);
  m_rotationMatrix(3, 3) = 1.0f;
}

void Renderer3d::UpdateTranslationMatrix()
{
  m_translationMatrix(0, 0) = 1.0f;
  m_translationMatrix(1, 1) = 1.0f;
  m_translationMatrix(2, 2) = 1.0f;
  m_translationMatrix(3, 0) = 0.0f;
  m_translationMatrix(3, 1) = m_offsetY;
  m_translationMatrix(3, 2) = m_offsetZ;
  m_translationMatrix(3, 3) = 1.0f;
}

}
