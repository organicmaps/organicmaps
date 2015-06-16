#pragma once

#include "../core/jni_helper.hpp"
#include "androidoglcontext.hpp"
#include "../../../drape/oglcontextfactory.hpp"

namespace android
{

class AndroidOGLContextFactory : public dp::OGLContextFactory
{
public:
  AndroidOGLContextFactory(JNIEnv * env, jobject jsurface);
  ~AndroidOGLContextFactory();

  bool IsValid() const;

  virtual dp::OGLContext * getDrawContext();
  virtual dp::OGLContext * getResourcesUploadContext();
  virtual bool isDrawContextCreated() const;
  virtual bool isUploadContextCreated() const;

  int GetWidth() const;
  int GetHeight() const;
  void UpdateSurfaceSize();

private:
  bool QuerySurfaceSize();
  EGLint * GetSupportedAttributes();

private:
  bool createWindowSurface();
  bool createPixelbufferSurface();

  AndroidOGLContext * m_drawContext;
  AndroidOGLContext * m_uploadContext;

  EGLSurface m_windowSurface;
  EGLSurface m_pixelbufferSurface;
  EGLConfig  m_config;

  ANativeWindow * m_nativeWindow;
  EGLDisplay m_display;

  int m_surfaceWidth;
  int m_surfaceHeight;

  bool m_valid;
  bool m_useCSAA;
};

} // namespace android
