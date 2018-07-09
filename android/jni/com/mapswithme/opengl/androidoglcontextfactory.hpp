#pragma once

#include "com/mapswithme/core/jni_helper.hpp"
#include "androidoglcontext.hpp"
#include "drape/oglcontextfactory.hpp"

#include "base/src_point.hpp"

namespace android
{
class AndroidOGLContextFactory : public dp::OGLContextFactory
{
public:
  AndroidOGLContextFactory(JNIEnv * env, jobject jsurface);
  ~AndroidOGLContextFactory();

  bool IsValid() const;

  dp::OGLContext * getDrawContext() override;
  dp::OGLContext * getResourcesUploadContext() override;
  bool isDrawContextCreated() const override;
  bool isUploadContextCreated() const override;
  void setPresentAvailable(bool available) override;

  void SetSurface(JNIEnv * env, jobject jsurface);
  void ResetSurface();

  int GetWidth() const;
  int GetHeight() const;
  void UpdateSurfaceSize(int w, int h);

  bool IsSupportedOpenGLES3() const { return m_supportedES3; }

private:
  bool QuerySurfaceSize();

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

  bool m_windowSurfaceValid;
  bool m_supportedES3;
};
}  // namespace android
