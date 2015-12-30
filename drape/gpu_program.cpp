#include "drape/gpu_program.hpp"
#include "drape/glfunctions.hpp"
#include "drape/support_manager.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#ifdef DEBUG
  #include "std/map.hpp"
#endif

namespace dp
{

GpuProgram::GpuProgram(ref_ptr<Shader> vertexShader, ref_ptr<Shader> fragmentShader)
  : m_vertexShader(vertexShader)
  , m_fragmentShader(fragmentShader)
{
  m_programID = GLFunctions::glCreateProgram();
  GLFunctions::glAttachShader(m_programID, m_vertexShader->GetID());
  GLFunctions::glAttachShader(m_programID, m_fragmentShader->GetID());

  string errorLog;
  if (!GLFunctions::glLinkProgram(m_programID, errorLog))
    LOG(LERROR, ("Program ", m_programID, " link error = ", errorLog));

  // originaly i detached shaders there, but then i try it on Tegra3 device.
  // on Tegra3, glGetActiveUniform will not work if you detach shaders after linking
  LoadUniformLocations();

  // On Tegra2 we cannot detach shaders at all.
  // https://devtalk.nvidia.com/default/topic/528941/alpha-blending-not-working-on-t20-and-t30-under-ice-cream-sandwich/
  if (!SupportManager::Instance().IsTegraDevice())
  {
    GLFunctions::glDetachShader(m_programID, m_vertexShader->GetID());
    GLFunctions::glDetachShader(m_programID, m_fragmentShader->GetID());
  }
}

GpuProgram::~GpuProgram()
{
  Unbind();

  if (SupportManager::Instance().IsTegraDevice())
  {
    GLFunctions::glDetachShader(m_programID, m_vertexShader->GetID());
    GLFunctions::glDetachShader(m_programID, m_fragmentShader->GetID());
  }

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

int8_t GpuProgram::GetAttributeLocation(string const & attributeName) const
{
  return GLFunctions::glGetAttribLocation(m_programID, attributeName);
}

int8_t GpuProgram::GetUniformLocation(string const & uniformName) const
{
  auto const it = m_uniforms.find(uniformName);
  if (it == m_uniforms.end())
    return -1;

  return it->second;
}

void GpuProgram::LoadUniformLocations()
{
  int32_t uniformsCount = GLFunctions::glGetProgramiv(m_programID, gl_const::GLActiveUniforms);
  for (int32_t i = 0; i < uniformsCount; ++i)
  {
    int32_t size = 0;
    glConst type = gl_const::GLFloatVec4;
    string name;
    GLFunctions::glGetActiveUniform(m_programID, i, &size, &type, name);
    m_uniforms[name] = GLFunctions::glGetUniformLocation(m_programID, name);
  }
}

} // namespace dp
