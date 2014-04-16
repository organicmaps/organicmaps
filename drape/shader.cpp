#include "shader.hpp"
#include "glfunctions.hpp"

#include "../base/assert.hpp"

namespace
{

glConst convert(Shader::Type t)
{
  if (t == Shader::VertexShader)
    return gl_const::GLVertexShader;

  return gl_const::GLFragmentShader;
}

} // namespace

Shader::Shader(string const & shaderSource, Type type)
  : m_source(shaderSource)
  , m_type(type)
  , m_glID(0)
{
  m_glID = GLFunctions::glCreateShader(convert(m_type));
  GLFunctions::glShaderSource(m_glID, m_source);
  string errorLog;
  bool result = GLFunctions::glCompileShader(m_glID, errorLog);
  CHECK(result, ("Shader compile error : ", errorLog));
}

Shader::~Shader()
{
  GLFunctions::glDeleteShader(m_glID);
}

int Shader::GetID() const
{
  return m_glID;
}
