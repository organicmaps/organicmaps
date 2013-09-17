#include "shader_reference.hpp"
#include "glfunctions.hpp"

#include "../base/assert.hpp"

namespace
{
  glConst convert(ShaderReference::Type t)
  {
    if (t == ShaderReference::VertexShader)
      return GLConst::GLVertexShader;

    return GLConst::GLFragmentShader;
  }
}

ShaderReference::ShaderReference(const string & shaderSource, Type type)
  : m_source(shaderSource)
  , m_type(type)
  , m_glID(0)
  , m_refCount(0)
{
}

int ShaderReference::GetID()
{
  return m_glID;
}

void ShaderReference::Ref()
{
  /// todo atomic compare
  if (m_refCount)
  {
    m_glID = GLFunctions::glCreateShader(convert(m_type));
    GLFunctions::glShaderSource(m_glID, m_source);
    string errorLog;
    if (!GLFunctions::glCompileShader(m_glID, errorLog))
      ASSERT(false, ());
  }

  /// todo atomic inc
  m_refCount++;
}

void ShaderReference::Deref()
{
  /// todo atomic dec ref on shader and delete if ref == 0
  m_refCount--;

  if (m_refCount == 0)
  {
    GLFunctions::glDeleteShader(m_glID);
    m_glID = 0;
  }
}
