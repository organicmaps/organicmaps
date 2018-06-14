#pragma once

#include "drape/glIncludes.hpp"
#include "drape/oglcontext.hpp"

#include <atomic>

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
  void setPresentAvailable(bool available) override;
  bool validate() override;

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

  std::atomic<bool> m_presentAvailable;
};
}  // namespace android
