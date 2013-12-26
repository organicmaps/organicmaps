#include "androidoglcontext.hpp"
#include "android_gl_utils.h"
#include "../../../base/assert.hpp"
#include "../../../base/logging.hpp"

#include <algorithm>

EGLint * AndroidOGLContext::getConfigAttributesList()
{
  static EGLint attr_list[] = {
    EGL_RED_SIZE, 5,
    EGL_GREEN_SIZE, 6,
    EGL_BLUE_SIZE, 5,
    EGL_STENCIL_SIZE, 0,
    EGL_DEPTH_SIZE, 16,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_SURFACE_TYPE, EGL_PBUFFER_BIT | EGL_WINDOW_BIT,
    EGL_NONE
  };
  return attr_list;
}

EGLint * AndroidOGLContext::getContextAttributesList()
{
  static EGLint contextAttrList[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };
  return contextAttrList;
}

AndroidOGLContext::AndroidOGLContext(EGLDisplay * display, ANativeWindow * nativeWindow)
  : m_nativeContext(EGL_NO_CONTEXT)
  , m_surface(EGL_NO_SURFACE)
  , m_display(display)
  , m_nativeWindow(nativeWindow)
  , m_config(NULL)
  , m_needPixelBufferSurface(false)
{
  ASSERT(m_display != EGL_NO_DISPLAY, ());
  ASSERT(m_nativeWindow != NULL, ());

  createNativeContextAndSurface();
}

void AndroidOGLContext::createNativeContextAndSurface()
{
  EGLConfig configs[40];
  int num_configs = 0;
  ASSERT(eglChooseConfig(m_display, getConfigAttributesList(), configs, 40, &num_configs) == EGL_TRUE, ());
  ASSERT(num_configs > 0, ("Didn't find any configs."));
  std::sort(&configs[0], &configs[num_configs], ConfigComparator(m_display));

  for (int i = 0; i < num_configs; ++i)
  {
    EGLConfig currentConfig = configs[i];

    m_surface = eglCreateWindowSurface(m_display, currentConfig, m_nativeWindow, 0);
    if (m_surface == EGL_NO_SURFACE)
      continue;

    m_nativeContext = eglCreateContext(m_display, currentConfig, EGL_NO_CONTEXT, getConfigAttributesList());
    if (m_nativeContext == EGL_NO_CONTEXT)
    {
      eglDestroySurface(m_display, m_surface);
      continue;
    }

    // Here we have valid config
    m_config = currentConfig;

    EGLint configId = 0;
    eglGetConfigAttrib(m_display, m_config, EGL_CONFIG_ID, &configId);
    LOG(LINFO, ("Choosen config id:", configId));

    break;
  }

  ASSERT(m_surface != EGL_NO_SURFACE, ());
  ASSERT(m_nativeContext != EGL_NO_CONTEXT, ());
}

AndroidOGLContext::AndroidOGLContext(AndroidOGLContext * contextToShareWith)
  : m_nativeContext(EGL_NO_CONTEXT)
  , m_surface(EGL_NO_SURFACE)
  , m_display(EGL_NO_DISPLAY)
  , m_nativeWindow(NULL)
  , m_config(NULL)
  , m_needPixelBufferSurface(true)
{
  ASSERT(contextToShareWith != NULL, ("Must pass valid context to share with"));
  m_config = contextToShareWith->m_config;
  m_display = contextToShareWith->m_display;
  m_nativeContext = eglCreateContext(m_display, m_config, contextToShareWith->m_nativeContext, getContextAttributesList());
  ASSERT(m_nativeContext != EGL_NO_CONTEXT, ());

  createPixelbufferSurface();
}

void AndroidOGLContext::createPixelbufferSurface()
{
  ASSERT(m_needPixelBufferSurface, ());
  static GLuint size = 1; // yes, 1 is the correct size, we dont really draw on it
  static EGLint surfaceConfig[] = {
      EGL_WIDTH, size, EGL_HEIGHT, size, EGL_NONE
  };
  m_surface = eglCreatePbufferSurface(m_display, m_config, surfaceConfig);
  ASSERT(m_surface != EGL_NO_SURFACE, ());
}

AndroidOGLContext::~AndroidOGLContext()
{
  if (m_display != EGL_NO_DISPLAY)
  {
    if (m_nativeContext != EGL_NO_CONTEXT)
    eglDestroyContext(m_display, m_nativeContext);

    if (m_surface != EGL_NO_SURFACE)
    eglDestroySurface(m_display, m_surface);
  }
}

void AndroidOGLContext::setDefaultFramebuffer()
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void AndroidOGLContext::makeCurrent()
{
  ASSERT(eglMakeCurrent(m_display, m_surface, m_surface, m_nativeContext) == EGL_TRUE, ());
}

void AndroidOGLContext::present()
{
  ASSERT(eglSwapBuffers(m_display, m_surface) == EGL_TRUE, ());
}
