#pragma once

#include "../defines.hpp"
#include "opengl.hpp"

namespace graphics
{
  namespace gl
  {
    void convert(EDataType from, GLenum & to);
    void convert(GLenum from, EDataType & to);

    void convert(GLenum from, EDataType & to, size_t & cnt);

    void convert(EShaderType from, GLenum & to);
    void convert(EPrimitives from, GLenum & to);
  }
}
