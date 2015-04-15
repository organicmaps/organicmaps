#include "drape/gpu_program.hpp"
#include "drape/glfunctions.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#ifdef DEBUG
  #include "std/map.hpp"
#endif

namespace dp
{

#ifdef DEBUG
  class UniformValidator
  {
  private:
    uint32_t m_programID;
    map<string, UniformTypeAndSize> m_uniformsMap;

  public:
    UniformValidator(uint32_t programId)
      : m_programID(programId)
    {
      int32_t numberOfUnis = GLFunctions::glGetProgramiv(m_programID, gl_const::GLActiveUniforms);
      for (size_t unIndex = 0; unIndex < numberOfUnis; ++unIndex)
      {
        string name;
        glConst type;
        UniformSize size;
        GLCHECK(GLFunctions::glGetActiveUniform(m_programID, unIndex, &size, &type, name));
        m_uniformsMap[name] = make_pair(type, size);
      }
    }

    bool HasValidTypeAndSizeForName(string const & name, glConst type, UniformSize size)
    {
      map<string, UniformTypeAndSize>::iterator it = m_uniformsMap.find(name);
      if (it != m_uniformsMap.end())
      {
        UniformTypeAndSize actualParams = (*it).second;
        return type == actualParams.first && size == actualParams.second;
      }
      else
        return false;
    }
  };

  bool GpuProgram::HasUniform(string const & name, glConst type, UniformSize size)
  {
    return m_validator->HasValidTypeAndSizeForName(name, type, size);
  }
#endif // UniformValidator


GpuProgram::GpuProgram(RefPointer<Shader> vertexShader, RefPointer<Shader> fragmentShader)
{
  m_programID = GLFunctions::glCreateProgram();
  GLFunctions::glAttachShader(m_programID, vertexShader->GetID());
  GLFunctions::glAttachShader(m_programID, fragmentShader->GetID());

  string errorLog;
  if (!GLFunctions::glLinkProgram(m_programID, errorLog))
    LOG(LINFO, ("Program ", m_programID, " link error = ", errorLog));

  GLFunctions::glDetachShader(m_programID, vertexShader->GetID());
  GLFunctions::glDetachShader(m_programID, fragmentShader->GetID());


#ifdef DEBUG
  m_validator.reset(new UniformValidator(m_programID));
#endif
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

int8_t GpuProgram::GetAttributeLocation(string const & attributeName) const
{
  return GLFunctions::glGetAttribLocation(m_programID, attributeName);
}

int8_t GpuProgram::GetUniformLocation(string const & uniformName) const
{
  return GLFunctions::glGetUniformLocation(m_programID, uniformName);
}

} // namespace dp
