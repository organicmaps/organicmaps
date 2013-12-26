#pragma once

#include <EGL/egl.h>
#include <GLES2/gl2.h>

static int gl_config_weight(EGLConfig const & config, EGLDisplay const & display)
{
  int val = -1;
  eglGetConfigAttrib(display, config, EGL_CONFIG_CAVEAT, &val);

  switch (val) {
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

int gl_compare_config(EGLConfig const & l, EGLConfig const & r, EGLDisplay const & display)
{
  return gl_config_weight(l, display) - gl_config_weight(r, display);
}

class ConfigComparator
{
public:
  ConfigComparator(EGLDisplay display)
    : m_display(display)
  {}

  int operator()(EGLConfig const & l, EGLConfig const & r)
  {
    return gl_compare_config(l ,r, m_display);
  }

private:
  EGLDisplay m_display;
};

