#pragma once

#include "androidoglcontext.hpp"
#include "drape/oglcontextfactory.hpp"


class AndroidOGLContextFactory : public OGLContextFactory
{
public:
  AndroidOGLContextFactory(JNIEnv * env, jobject jsurface);
  ~AndroidOGLContextFactory();

  virtual OGLContext * getDrawContext();
  virtual OGLContext * getResourcesUploadContext();

private:
  void createWindowSurface();
  void createPixelbufferSurface();


  AndroidOGLContext * m_drawContext;
  AndroidOGLContext * m_uploadContext;

  EGLSurface m_windowSurface;
  EGLSurface m_pixelbufferSurface;
  EGLConfig  m_config;

  ANativeWindow * m_nativeWindow;
  EGLDisplay m_display;
};
