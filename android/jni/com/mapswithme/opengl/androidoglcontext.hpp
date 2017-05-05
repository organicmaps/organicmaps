#pragma once

#include "drape/glIncludes.hpp"
#include "drape/oglcontext.hpp"

namespace android
{

class AndroidOGLContext : public dp::OGLContext
{
public:
  AndroidOGLContext(bool supportedES3, EGLDisplay display, EGLSurface surface,
                    EGLConfig config, AndroidOGLContext * contextToShareWith);
  ~AndroidOGLContext();

  void makeCurrent() override;
  void doneCurrent() override;
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

}  // namespace android
