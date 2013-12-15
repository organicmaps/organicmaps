#include "gpu_program.hpp"
#include "glfunctions.hpp"

#include "../base/assert.hpp"

GpuProgram::GpuProgram(RefPointer<Shader> vertexShader, RefPointer<Shader> fragmentShader)
{
  m_programID = GLFunctions::glCreateProgram();
  GLFunctions::glAttachShader(m_programID, vertexShader->GetID());
  GLFunctions::glAttachShader(m_programID, fragmentShader->GetID());

  string errorLog;
  VERIFY(GLFunctions::glLinkProgram(m_programID, errorLog), ());

  GLFunctions::glDetachShader(m_programID, vertexShader->GetID());
  GLFunctions::glDetachShader(m_programID, fragmentShader->GetID());
}

GpuProgram::~GpuProgram()
{
  Unbind();
  GLFunctions::glDeleteProgram(m_programID);
}

void GpuProgram::Bind()
{
  GLFunctions::glUseProgram(m_programID);
}

void GpuProgram::Unbind()
{
  GLFunctions::glUseProgram(0);
}

int8_t GpuProgram::GetAttributeLocation(const string & attributeName) const
{
  return GLFunctions::glGetAttribLocation(m_programID, attributeName);
}

int8_t GpuProgram::GetUniformLocation(const string & uniformName) const
{
  return GLFunctions::glGetUniformLocation(m_programID, uniformName);
}

void GpuProgram::ActivateSampler(uint8_t textureBlock, const string & samplerName)
{
  ASSERT(GLFunctions::glGetCurrentProgram() == m_programID, ());
  int8_t location = GLFunctions::glGetUniformLocation(m_programID, samplerName);
  GLFunctions::glUniformValuei(location, textureBlock);
}
