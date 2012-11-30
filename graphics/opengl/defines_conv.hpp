#pragma once

#include "../defines.hpp"
#include "opengl.hpp"

namespace graphics
{
  namespace gl
  {
    void convert(EDataType from, GLenum & to);
    void convert(EShaderType from, GLenum & to);
    void convert(EPrimitives from, GLenum & to);
  }
}
