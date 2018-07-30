#include "drape/oglcontext.hpp"
#include "drape/glfunctions.hpp"

namespace dp
{
void OGLContext::SetApiVersion(ApiVersion apiVersion)
{
  GLFunctions::Init(apiVersion);
}

void OGLContext::Init()
{
  GLFunctions::glPixelStore(gl_const::GLUnpackAlignment, 1);

  GLFunctions::glClearDepthValue(1.0);
  GLFunctions::glDepthFunc(gl_const::GLLessOrEqual);
  GLFunctions::glDepthMask(true);

  GLFunctions::glFrontFace(gl_const::GLClockwise);
  GLFunctions::glCullFace(gl_const::GLBack);
  GLFunctions::glEnable(gl_const::GLCullFace);
  GLFunctions::glEnable(gl_const::GLScissorTest);
}

void OGLContext::SetClearColor(float r, float g, float b, float a)
{
  GLFunctions::glClearColor(r, g, b, a);
}

void OGLContext::Clear(ContextConst clearBits)
{
  glConst glBits = 0;
  if (clearBits & ClearBits::ColorBit)
    glBits |= gl_const::GLColorBit;
  if (clearBits & ClearBits::DepthBit)
    glBits |= gl_const::GLDepthBit;
  if (clearBits & ClearBits::StencilBit)
    glBits |= gl_const::GLStencilBit;

  GLFunctions::glClear(glBits);
}
}  // namespace dp
