#include "glIncludes.hpp"

#include <cassert>

void CheckGLError()
{
  GLenum result = glGetError();
  if (result != GL_NO_ERROR)
    assert(false);
}
