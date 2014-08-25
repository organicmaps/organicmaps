#include "shader.hpp"

#include "../base/assert.hpp"
#include "../base/string_utils.hpp"


namespace dp
{

#if defined(OMIM_OS_DESKTOP) && !defined(COMPILER_TESTS)
  #define LOW_P string("")
  #define MEDIUM_P string("")
  #define HIGH_P string("")
#else
  #define LOW_P string("lowp")
  #define MEDIUM_P string("mediump")
  #define HIGH_P string("highp")
#endif

using strings::to_string;

namespace
{
glConst convert(Shader::Type t)
{
  if (t == Shader::VertexShader)
    return gl_const::GLVertexShader;

  return gl_const::GLFragmentShader;
}

void ResolveGetTexel(string & result, string const & sampler, int count)
{
  string const index = "Index";
  string const answer = "answer";
  string const texindex = "texIndex";
  string const texcoord = "texCoord";

  for (int i = 0; i < count; ++i)
  {
    result += "const int " + index + to_string(i) + " = " + to_string(i) + ";\n";
  }
  result += "\n";

  result += "uniform sampler2D u_textures[" + to_string(count) + "];\n";

  //Head
  result += MEDIUM_P + " vec4 getTexel(int " + texindex + ", " + LOW_P + " vec2 " + texcoord + ") \n";
  result += "{\n";
  result += "  " + MEDIUM_P + " vec4 " + answer + "; \n";
  //Body
  result += "  if (" + texindex + " == " + index + "0) \n";
  result += "    " + answer + " = texture2D(" + sampler + "[" + index + "0], " + texcoord + "); \n";
  for (int i = 1; i < count; ++i)
  {
    string num = to_string(i);
    result += "  else if (" + texindex + " == " + index + num + ") \n";
    result += "    " + answer + " = texture2D(" + sampler + "[" + index + num + "], " + texcoord + "); \n";
  }
  //Tail
  result += "  return " + answer + ";\n";
  result += "}\n";
}
}

void sh::Inject(string & src)
{
  string const replacement("~getTexel~");
  int const pos = src.find(replacement);
  if (pos == string::npos)
    return;
  string injector = "";
  int const count = min(8, GLFunctions::glGetInteger(gl_const::GLMaxFragmentTextures));
  ResolveGetTexel(injector, "u_textures", count);
  src.replace(pos, replacement.length(), injector);
}

Shader::Shader(string const & shaderSource, Type type)
  : m_source(shaderSource)
  , m_type(type)
  , m_glID(0)
{
  m_glID = GLFunctions::glCreateShader(convert(m_type));
  sh::Inject(m_source);
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

} // namespace dp
