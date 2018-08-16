#include "drape/oglcontext.hpp"
#include "drape/glfunctions.hpp"

namespace dp
{
namespace
{
glConst DecodeTestFunction(TestFunction depthFunction)
{
  switch (depthFunction)
  {
  case TestFunction::Never: return gl_const::GLNever;
  case TestFunction::Less: return gl_const::GLLess;
  case TestFunction::Equal: return gl_const::GLEqual;
  case TestFunction::LessOrEqual: return gl_const::GLLessOrEqual;
  case TestFunction::Greater: return gl_const::GLGreat;
  case TestFunction::NotEqual: return gl_const::GLNotEqual;
  case TestFunction::GreaterOrEqual: return gl_const::GLGreatOrEqual;
  case TestFunction::Always: return gl_const::GLAlways;
  }
  ASSERT(false, ());
}

glConst DecodeStencilFace(StencilFace stencilFace)
{
  switch (stencilFace)
  {
  case StencilFace::Front: return gl_const::GLFront;
  case StencilFace::Back: return gl_const::GLBack;
  case StencilFace::FrontAndBack: return gl_const::GLFrontAndBack;
  }
  ASSERT(false, ());
}

glConst DecodeStencilAction(StencilAction stencilAction)
{
  switch (stencilAction)
  {
  case StencilAction::Keep: return gl_const::GLKeep;
  case StencilAction::Zero: return gl_const::GLZero;
  case StencilAction::Replace: return gl_const::GLReplace;
  case StencilAction::Increment: return gl_const::GLIncr;
  case StencilAction::IncrementWrap: return gl_const::GLIncrWrap;
  case StencilAction::Decrement: return gl_const::GLDecr;
  case StencilAction::DecrementWrap: return gl_const::GLDecrWrap;
  case StencilAction::Invert: return gl_const::GLInvert;
  }
  ASSERT(false, ());
}
}  // namespace

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

void OGLContext::SetDepthTestEnabled(bool enabled)
{
  if (enabled)
    GLFunctions::glEnable(gl_const::GLDepthTest);
  else
    GLFunctions::glDisable(gl_const::GLDepthTest);
}

void OGLContext::SetDepthTestFunction(TestFunction depthFunction)
{
  GLFunctions::glDepthFunc(DecodeTestFunction(depthFunction));
};

void OGLContext::SetStencilTestEnabled(bool enabled)
{
  if (enabled)
    GLFunctions::glEnable(gl_const::GLStencilTest);
  else
    GLFunctions::glDisable(gl_const::GLStencilTest);
}

void OGLContext::SetStencilFunction(StencilFace face, TestFunction stencilFunction)
{
  GLFunctions::glStencilFuncSeparate(DecodeStencilFace(face), DecodeTestFunction(stencilFunction), 1, 1);
}

void OGLContext::SetStencilActions(StencilFace face, StencilAction stencilFailAction, StencilAction depthFailAction,
                                   StencilAction passAction)
{
  GLFunctions::glStencilOpSeparate(DecodeStencilFace(face),
                                   DecodeStencilAction(stencilFailAction),
                                   DecodeStencilAction(depthFailAction),
                                   DecodeStencilAction(passAction));
}
}  // namespace dp
