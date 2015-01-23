#pragma once

#include <EGL/egl.h>
#include <GLES2/gl2.h>

namespace android
{

class ConfigComparator
{
public:
  ConfigComparator(EGLDisplay display)
    : m_display(display)
  {}

  int operator()(EGLConfig const & l, EGLConfig const & r) const
  {
    return configWeight(l) - configWeight(r);
  }

  int configWeight(EGLConfig const & config) const
  {
    int val = -1;
    eglGetConfigAttrib(m_display, config, EGL_CONFIG_CAVEAT, &val);

    switch (val)
    {
      case EGL_NONE:
        return 0;
      case EGL_SLOW_CONFIG:
        return 1;
      case EGL_NON_CONFORMANT_CONFIG:
        return 2;
      default:
        return 0;
    }
  }

private:
  EGLDisplay m_display;
};

} // namespace android
