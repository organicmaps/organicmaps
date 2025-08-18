#include "drape/gl_gpu_program.hpp"
#include "drape/gl_functions.hpp"
#include "drape/render_state.hpp"
#include "drape/support_manager.hpp"

#include "base/logging.hpp"

#include <set>

namespace dp
{
GLGpuProgram::GLGpuProgram(std::string const & programName, ref_ptr<Shader> vertexShader,
                           ref_ptr<Shader> fragmentShader)
  : GpuProgram(programName)
  , m_vertexShader(vertexShader)
  , m_fragmentShader(fragmentShader)
{
  m_programID = GLFunctions::glCreateProgram();
  GLFunctions::glAttachShader(m_programID, m_vertexShader->GetID());
  GLFunctions::glAttachShader(m_programID, m_fragmentShader->GetID());

  std::string errorLog;
  if (!GLFunctions::glLinkProgram(m_programID, errorLog))
    LOG(LERROR, ("Program ", programName, " link error = ", errorLog));

  LoadUniformLocations();

  GLFunctions::glDetachShader(m_programID, m_vertexShader->GetID());
  GLFunctions::glDetachShader(m_programID, m_fragmentShader->GetID());
}

GLGpuProgram::~GLGpuProgram()
{
  GLFunctions::glDeleteProgram(m_programID);
}

void GLGpuProgram::Bind()
{
  // Deactivate all unused textures.
  uint8_t const usedSlots = TextureState::GetLastUsedSlots();
  for (uint8_t i = m_textureSlotsCount; i < usedSlots; i++)
  {
    GLFunctions::glActiveTexture(gl_const::GLTexture0 + i);
    GLFunctions::glBindTexture(0);
  }

  GLFunctions::glUseProgram(m_programID);
}

void GLGpuProgram::Unbind()
{
  GLFunctions::glUseProgram(0);
}

int8_t GLGpuProgram::GetAttributeLocation(std::string const & attributeName) const
{
  return GLFunctions::glGetAttribLocation(m_programID, attributeName);
}

int8_t GLGpuProgram::GetUniformLocation(std::string const & uniformName) const
{
  auto const it = m_uniforms.find(uniformName);
  if (it == m_uniforms.end())
    return -1;

  return it->second.m_location;
}

glConst GLGpuProgram::GetUniformType(std::string const & uniformName) const
{
  auto const it = m_uniforms.find(uniformName);
  if (it == m_uniforms.end())
    return -1;

  return it->second.m_type;
}

GLGpuProgram::UniformsInfo const & GLGpuProgram::GetUniformsInfo() const
{
  return m_uniforms;
}

void GLGpuProgram::LoadUniformLocations()
{
  static std::set<glConst> const kSupportedTypes = {gl_const::GLFloatType, gl_const::GLFloatVec2, gl_const::GLFloatVec3,
                                                    gl_const::GLFloatVec4, gl_const::GLIntType,   gl_const::GLIntVec2,
                                                    gl_const::GLIntVec3,   gl_const::GLIntVec4,   gl_const::GLFloatMat4,
                                                    gl_const::GLSampler2D};

  auto const uniformsCount = GLFunctions::glGetProgramiv(m_programID, gl_const::GLActiveUniforms);
  for (int i = 0; i < uniformsCount; ++i)
  {
    int32_t size = 0;
    UniformInfo info;
    std::string name;
    GLFunctions::glGetActiveUniform(m_programID, static_cast<uint32_t>(i), &size, &info.m_type, name);
    CHECK(kSupportedTypes.find(info.m_type) != kSupportedTypes.cend(),
          ("Used uniform has unsupported type. Program =", m_programName, "; Type =", info.m_type, "; Name =", name));

    info.m_location = GLFunctions::glGetUniformLocation(m_programID, name);
    m_uniforms[name] = std::move(info);
  }
  m_numericUniformsCount = CalculateNumericUniformsCount();
  m_textureSlotsCount = static_cast<uint8_t>(m_uniforms.size() - m_numericUniformsCount);
}

uint32_t GLGpuProgram::CalculateNumericUniformsCount() const
{
  uint32_t counter = 0;
  for (auto const & u : m_uniforms)
    if (u.second.m_type != gl_const::GLSampler2D)
      counter++;
  return counter;
}
}  // namespace dp
