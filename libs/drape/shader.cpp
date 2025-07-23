#include "drape/shader.hpp"
#include "drape/gl_functions.hpp"

#include "base/assert.hpp"

namespace dp
{
namespace
{
glConst ConvertType(Shader::Type t)
{
  if (t == Shader::Type::VertexShader)
    return gl_const::GLVertexShader;

  return gl_const::GLFragmentShader;
}
}  // namespace

Shader::Shader(std::string const & shaderName, std::string const & shaderSource, std::string const & defines, Type type)
  : m_shaderName(shaderName)
  , m_glID(0)
{
  m_glID = GLFunctions::glCreateShader(ConvertType(type));
  GLFunctions::glShaderSource(m_glID, shaderSource, defines);
  std::string errorLog;
  bool const result = GLFunctions::glCompileShader(m_glID, errorLog);
  CHECK(result, (m_shaderName, "> Shader compile error : ", errorLog));
}

Shader::~Shader()
{
  GLFunctions::glDeleteShader(m_glID);
}

uint32_t Shader::GetID() const
{
  return m_glID;
}
}  // namespace dp
