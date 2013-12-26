#include "androidoglcontextfactory.h"

#include "../../../base/assert.hpp"

AndroidOGLContextFactory::AndroidOGLContextFactory(JNIEnv * env, jobject jsurface)
  : m_drawContext(NULL)
  , m_uploadContext(NULL)
  , m_nativeWindow(NULL)
  , m_display(EGL_NO_DISPLAY)
{
  ASSERT(jsurface, ());

  m_nativeWindow = ANativeWindow_fromSurface(env, jsurface);
  ASSERT(m_nativeWindow, ());

  m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  ASSERT(m_display != EGL_NO_DISPLAY, ());

  EGLint version[2] = {0};
  ASSERT(eglInitialize(m_display, &version[0], &version[1]), ());
}

AndroidOGLContextFactory::~AndroidOGLContextFactory()
{
  delete m_drawContext;
  delete m_uploadContext;

  eglTerminate(m_display);
  ANativeWindow_release(m_nativeWindow);
}

OGLContext * AndroidOGLContextFactory::getDrawContext()
{
  if (m_drawContext == NULL)
    m_drawContext = new AndroidOGLContext(m_display, m_nativeWindow);
  return m_drawContext;
}

OGLContext * AndroidOGLContextFactory::getResourcesUploadContext()
{
  if (m_uploadContext == NULL)
    m_uploadContext = new AndroidOGLContext(getDrawContext());
  return m_uploadContext;
}


