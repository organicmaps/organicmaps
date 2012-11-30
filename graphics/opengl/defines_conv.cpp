#include "defines_conv.hpp"

namespace graphics
{
  namespace gl
  {
    void convert(EDataType from, GLenum & to)
    {
      switch (from)
      {
      case EFloat:
        to = GL_FLOAT;
        break;
      case EUnsigned:
        to = GL_UNSIGNED_INT;
        break;
      case EInteger:
        to = GL_INT;
        break;
      default:
        LOG(LERROR, ("Unknown EDataType specified: ", from));
        to = 0;
      }
    }

    void convert(EShaderType from, GLenum & to)
    {
      switch (from)
      {
      case EVertexShader:
        to = GL_VERTEX_SHADER;
        break;
      case EFragmentShader:
        to = GL_FRAGMENT_SHADER;
        break;
      default:
        LOG(LERROR, ("Unknown EShaderType specified: ", from));
        to = 0;
      }
    }

    void convert(EPrimitives from, GLenum & to)
    {
      switch (from)
      {
      case ETriangles:
        to = GL_TRIANGLES;
        break;
      case ETrianglesFan:
        to = GL_TRIANGLE_FAN;
        break;
      case ETrianglesStrip:
        to = GL_TRIANGLE_STRIP;
        break;
      default:
        LOG(LERROR, ("Unknown EPrimitives specified: ", from));
        to = 0;
      }
    }
  }
}
