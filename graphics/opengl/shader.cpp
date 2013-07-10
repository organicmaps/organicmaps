#include "shader.hpp"

#include "defines_conv.hpp"
#include "opengl.hpp"

// For strlen
#include "../../std/memcpy.hpp"

namespace graphics
{
  namespace gl
  {
    Shader::Shader(const char *src, EShaderType type)
    {
      GLenum glType;
      convert(type, glType);

      m_handle = glCreateShaderFn(glType);
      OGLCHECKAFTER;

      if (!m_handle)
        throw Exception("CreateShader error", "could not create Shader!");

      int len = strlen(src);

      OGLCHECK(glShaderSourceFn(m_handle, 1, &src, &len));

      OGLCHECK(glCompileShaderFn(m_handle));

      GLint compileRes = GL_FALSE;
      OGLCHECK(glGetShaderivFn(m_handle, GL_COMPILE_STATUS, &compileRes));

      if (compileRes == GL_FALSE)
      {
        GLchar msg[256];
        OGLCHECK(glGetShaderInfoLogFn(m_handle, sizeof(msg), 0, msg));
        throw CompileException("Couldn't compile shader: ", msg);
      }
    }

    Shader::~Shader()
    {
      OGLCHECK(glDeleteShaderFn(m_handle));
    }

    GLuint Shader::id() const
    {
      return m_handle;
    }
  }
}
