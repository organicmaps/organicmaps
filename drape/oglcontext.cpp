#include "drape/oglcontext.hpp"
#include "drape/glfunctions.hpp"

namespace dp
{
void OGLContext::Init(ApiVersion apiVersion)
{
  GLFunctions::Init(apiVersion);

  GLFunctions::glPixelStore(gl_const::GLUnpackAlignment, 1);

  GLFunctions::glClearDepthValue(1.0);
  GLFunctions::glDepthFunc(gl_const::GLLessOrEqual);
  GLFunctions::glDepthMask(true);

  GLFunctions::glFrontFace(gl_const::GLClockwise);
  GLFunctions::glCullFace(gl_const::GLBack);
  GLFunctions::glEnable(gl_const::GLCullFace);
  GLFunctions::glEnable(gl_const::GLScissorTest);
}

void OGLContext::SetClearColor(dp::Color const & color)
{
  GLFunctions::glClearColor(color.GetRedF(), color.GetGreenF(), color.GetBlueF(), color.GetAlphaF());
}

void OGLContext::Clear(uint32_t clearBits)
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

void OGLContext::Flush()
{
  GLFunctions::glFlush();
}
}  // namespace dp
