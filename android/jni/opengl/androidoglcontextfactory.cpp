#include "androidoglcontextfactory.h"
#include "android_gl_utils.h"

#include "../../../base/assert.hpp"

#include <algorithm>

static EGLint * getConfigAttributesList()
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

AndroidOGLContextFactory::AndroidOGLContextFactory(JNIEnv * env, jobject jsurface)
  : m_drawContext(NULL)
  , m_uploadContext(NULL)
  , m_windowSurface(EGL_NO_SURFACE)
  , m_pixelbufferSurface(EGL_NO_SURFACE)
  , m_config(NULL)
  , m_nativeWindow(NULL)
  , m_display(EGL_NO_DISPLAY)
{
  CHECK(jsurface, ());

  m_nativeWindow = ANativeWindow_fromSurface(env, jsurface);
  ASSERT(m_nativeWindow, ());

  m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  ASSERT(m_display != EGL_NO_DISPLAY, ());

  EGLint version[2] = {0};
  VERIFY(eglInitialize(m_display, &version[0], &version[1]), ());

  createWindowSurface();
  createPixelbufferSurface();
}

AndroidOGLContextFactory::~AndroidOGLContextFactory()
{
  delete m_drawContext;
  delete m_uploadContext;

  eglDestroySurface(m_display, m_windowSurface);
  eglDestroySurface(m_display, m_pixelbufferSurface);
  eglTerminate(m_display);

  ANativeWindow_release(m_nativeWindow);
}

OGLContext * AndroidOGLContextFactory::getDrawContext()
{
  CHECK(m_windowSurface != EGL_NO_SURFACE, ());
  if (m_drawContext == NULL)
    m_drawContext = new AndroidOGLContext(m_display, m_windowSurface, m_config, m_uploadContext);
  return m_drawContext;
}

OGLContext * AndroidOGLContextFactory::getResourcesUploadContext()
{
  CHECK(m_pixelbufferSurface != EGL_NO_SURFACE, ());
  if (m_uploadContext == NULL)
    m_uploadContext = new AndroidOGLContext(m_display, m_pixelbufferSurface, m_config, m_drawContext);
  return m_uploadContext;
}

void AndroidOGLContextFactory::createWindowSurface()
{
  EGLConfig configs[40];
  int num_configs = 0;
  VERIFY(eglChooseConfig(m_display, getConfigAttributesList(), configs, 40, &num_configs) == EGL_TRUE, ());
  ASSERT(num_configs > 0, ("Didn't find any configs."));

  std::sort(&configs[0], &configs[num_configs], ConfigComparator(m_display));
  for (int i = 0; i < num_configs; ++i)
  {
    EGLConfig currentConfig = configs[i];

    m_windowSurface = eglCreateWindowSurface(m_display, currentConfig, m_nativeWindow, EGL_BACK_BUFFER);
    if (m_windowSurface == EGL_NO_SURFACE)
      continue;
    else
      m_config = currentConfig;

    EGLint configId = 0;
    eglGetConfigAttrib(m_display, m_config, EGL_CONFIG_ID, &configId);
    LOG(LINFO, ("Choosen config id:", configId));

    break;
  }

  CHECK(m_windowSurface != EGL_NO_SURFACE, ());
}

void AndroidOGLContextFactory::createPixelbufferSurface()
{
  CHECK(m_config != NULL, ());

  const GLuint size = 1; // yes, 1 is the correct size, we dont really draw on it
  static EGLint surfaceConfig[] = {
      EGL_WIDTH, size, EGL_HEIGHT, size, EGL_NONE
  };
  m_pixelbufferSurface = eglCreatePbufferSurface(m_display, m_config, surfaceConfig);
  ASSERT(m_pixelbufferSurface != EGL_NO_SURFACE, ());
}


