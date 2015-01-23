#include "androidoglcontextfactory.hpp"
#include "android_gl_utils.hpp"

#include "../../../../../base/assert.hpp"
#include "../../../../../base/logging.hpp"

#include "../../../../../std/algorithm.hpp"

#include <EGL/egl.h>
#include <android/native_window_jni.h>

namespace android
{

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
  , m_surfaceWidth(0)
  , m_surfaceHeight(0)
  , m_valid(false)
{
  LOG(LINFO, ("UVR : Start create factory"));
  if (!jsurface)
  {
    LOG(LINFO, ("Java surface incorrect"));
    return;
  }

  m_nativeWindow = ANativeWindow_fromSurface(env, jsurface);
  if (!m_nativeWindow)
  {
    LOG(LINFO, ("Can't get native window from Java surface"));
    return;
  }

  m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (m_display == EGL_NO_DISPLAY)
  {
    LOG(LINFO, ("Can't query default egl display. Error code = ", eglGetError()));
    return;
  }

  EGLint version[2] = {0};
  if (!eglInitialize(m_display, &version[0], &version[1]))
  {
    LOG(LINFO, ("EGL initialize failed. Error code = ", eglGetError()));
    return;
  }

  if (!(createWindowSurface() && createPixelbufferSurface()))
  {
    eglTerminate(m_display);
    return;
  }

  if (!QuerySurfaceSize())
    return;

  LOG(LINFO, ("UVR : Correct created factory"));
  m_valid = true;
}

AndroidOGLContextFactory::~AndroidOGLContextFactory()
{
  if (IsValid())
  {
    delete m_drawContext;
    delete m_uploadContext;

    eglDestroySurface(m_display, m_windowSurface);
    eglDestroySurface(m_display, m_pixelbufferSurface);
    eglTerminate(m_display);

    ANativeWindow_release(m_nativeWindow);
  }
}

bool AndroidOGLContextFactory::IsValid() const
{
  return m_valid;
}

int AndroidOGLContextFactory::GetWidth() const
{
  ASSERT(IsValid(), ());
  return m_surfaceWidth;
}

int AndroidOGLContextFactory::GetHeight() const
{
  ASSERT(IsValid(), ());
  return m_surfaceHeight;
}

void AndroidOGLContextFactory::UpdateSurfaceSize()
{
  ASSERT(IsValid(), ());
  VERIFY(QuerySurfaceSize(), ());
}

bool AndroidOGLContextFactory::QuerySurfaceSize()
{
  EGLint queryResult;
  if (eglQuerySurface(m_display, m_windowSurface, EGL_WIDTH, &queryResult) == EGL_FALSE)
  {
    LOG(LINFO, ("Can't query surface width. Error code = ", eglGetError()));
    return false;
  }

  m_surfaceWidth = static_cast<int>(queryResult);
  if (eglQuerySurface(m_display, m_windowSurface, EGL_HEIGHT, &queryResult) == EGL_FALSE)
  {
    LOG(LINFO, ("Can't query surface height. Error code = ", eglGetError()));
    return false;
  }

  m_surfaceHeight = static_cast<int>(queryResult);
  return true;
}

dp::OGLContext * AndroidOGLContextFactory::getDrawContext()
{
  ASSERT(IsValid(), ());
  ASSERT(m_windowSurface != EGL_NO_SURFACE, ());
  if (m_drawContext == NULL)
    m_drawContext = new AndroidOGLContext(m_display, m_windowSurface, m_config, m_uploadContext);
  return m_drawContext;
}

dp::OGLContext * AndroidOGLContextFactory::getResourcesUploadContext()
{
  ASSERT(IsValid(), ());
  ASSERT(m_pixelbufferSurface != EGL_NO_SURFACE, ());
  if (m_uploadContext == NULL)
    m_uploadContext = new AndroidOGLContext(m_display, m_pixelbufferSurface, m_config, m_drawContext);
  return m_uploadContext;
}

bool AndroidOGLContextFactory::createWindowSurface()
{
  EGLConfig configs[40];
  int count = 0;
  VERIFY(eglChooseConfig(m_display, getConfigAttributesList(), configs, 40, &count) == EGL_TRUE, ());
  ASSERT(count > 0, ("Didn't find any configs."));

  sort(&configs[0], &configs[count], ConfigComparator(m_display));
  for (int i = 0; i < count; ++i)
  {
    EGLConfig currentConfig = configs[i];
    EGLint surfaceAttributes[] = { EGL_RENDER_BUFFER, EGL_BACK_BUFFER, EGL_NONE };
    m_windowSurface = eglCreateWindowSurface(m_display, currentConfig, m_nativeWindow, surfaceAttributes);
    if (m_windowSurface == EGL_NO_SURFACE)
      continue;
    else
      m_config = currentConfig;

    EGLint configId = 0;
    eglGetConfigAttrib(m_display, m_config, EGL_CONFIG_ID, &configId);
    LOG(LINFO, ("Choosen config id:", configId));

    break;
  }

  if (m_windowSurface == EGL_NO_SURFACE)
  {
    LOG(LINFO, ("Can't create EGLWindowSurface. Error code = ", eglGetError()));
    return false;
  }

  return true;
}

bool AndroidOGLContextFactory::createPixelbufferSurface()
{
  //ASSERT(m_config != NULL, ());

  const GLuint size = 1; // yes, 1 is the correct size, we dont really draw on it
  static EGLint surfaceConfig[] = {
      EGL_WIDTH, size,
      EGL_HEIGHT, size,
      EGL_NONE
  };

  m_pixelbufferSurface = eglCreatePbufferSurface(m_display, m_config, surfaceConfig);

  if (m_pixelbufferSurface == EGL_NO_SURFACE)
  {
    LOG(LINFO, ("Can't create buffer surface for UploadThread. Error code = ", eglGetError()));
    return false;
  }

  return true;
}

} // namespace android
