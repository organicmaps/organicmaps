#pragma once

#include "../../../drape/oglcontext.hpp"

#include <GLES2/gl2.h>
#include <EGL/egl.h>

namespace android
{

class AndroidOGLContext : public dp::OGLContext
{
public:
  AndroidOGLContext(EGLDisplay display, EGLSurface surface, EGLConfig config, AndroidOGLContext * contextToShareWith);
  ~AndroidOGLContext();

  virtual void makeCurrent();
  virtual void present();
  virtual void setDefaultFramebuffer();

private:
  // {@ Owned by Context
  EGLContext m_nativeContext;
  // @}

  // {@ Owned by Factory
  EGLSurface m_surface;
  EGLDisplay m_display;
  // @}
};

} // namespace android
