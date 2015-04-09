#pragma once

#include "base/exception.hpp"

#include "graphics/defines.hpp"

#include "graphics/opengl/opengl.hpp"

namespace graphics
{
  namespace gl
  {
    class Shader
    {
    private:
      GLuint m_handle;
    public:

      DECLARE_EXCEPTION(Exception, RootException);
      DECLARE_EXCEPTION(CompileException, Exception);

      /// Constructor.
      Shader(char const * src, EShaderType type);
      /// Destructor.
      ~Shader();
      /// Handle to the program.
      GLuint id() const;
    };
  }
}
