#pragma once

#include "drape/oglcontext.hpp"

#include <GLES2/gl2.h>
#include <EGL/egl.h>

namespace android
{

class AndroidOGLContext : public dp::OGLContext
{
public:
  AndroidOGLContext(EGLDisplay display, EGLSurface surface, EGLConfig config, AndroidOGLContext * contextToShareWith);
  ~AndroidOGLContext();

  void makeCurrent() override;
  void present() override;
  void setDefaultFramebuffer() override;
  void setRenderingEnabled(bool enabled) override;

  void setSurface(EGLSurface surface);
  void resetSurface();

  void clearCurrent();

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
