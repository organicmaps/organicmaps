#pragma once

#include "../../../drape/oglcontext.hpp"

#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <android/native_window.h>


class AndroidOGLContext : public OGLContext
{
public:
  AndroidOGLContext(AndroidOGLContext * contextToShareWith);
  AndroidOGLContext(EGLDisplay * display, ANativeWindow * nativeWindow);
  ~AndroidOGLContext();

  virtual void makeCurrent();
  virtual void present();
  virtual void setDefaultFramebuffer();

private:
  static EGLint * getConfigAttributesList();
  static EGLint * getContextAttributesList();
  void createNativeContextAndSurface();
  void createPixelbufferSurface();

  // {@ Owned by Context
  EGLContext m_nativeContext;
  EGLSurface m_surface;
  // @}

  // {@ Owned by Factory
  EGLDisplay m_display;
  ANativeWindow * m_nativeWindow;
  // @}

  EGLConfig m_config;

  bool m_needPixelBufferSurface;
};

