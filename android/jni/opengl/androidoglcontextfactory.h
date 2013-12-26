#pragma once

#include "androidoglcontext.hpp"
#include "../../../drape/oglcontextfactory.hpp"


class AndroidOGLContextFactory : public OGLContextFactory
{
public:
  AndroidOGLContextFactory(JNIEnv * env, jobject jsurface);
  ~AndroidOGLContextFactory();

  virtual OGLContext * getDrawContext();
  virtual OGLContext * getResourcesUploadContext();

private:
  AndroidOGLContext * m_drawContext;
  AndroidOGLContext * m_uploadContext;

  ANativeWindow * m_nativeWindow;
  EGLDisplay m_display;
};
