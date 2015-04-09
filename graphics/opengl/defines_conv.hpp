#pragma once

#include "graphics/defines.hpp"
#include "graphics/opengl/opengl.hpp"

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
