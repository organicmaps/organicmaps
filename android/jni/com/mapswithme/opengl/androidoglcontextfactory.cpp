#include "androidoglcontextfactory.hpp"
#include "android_gl_utils.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "std/algorithm.hpp"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>

namespace android
{

static EGLint * getMultisampleAttributesList()
{
  static EGLint attr_list[] =
  {
      EGL_RED_SIZE, 5,
      EGL_GREEN_SIZE, 6,
      EGL_BLUE_SIZE, 5,
      EGL_STENCIL_SIZE, 0,
      EGL_DEPTH_SIZE, 16,
      EGL_SAMPLE_BUFFERS, 1,
      EGL_SAMPLES, 2,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
      EGL_SURFACE_TYPE, EGL_PBUFFER_BIT | EGL_WINDOW_BIT,
      EGL_NONE
  };
  return attr_list;
}

/// nVidia Tegra2 doen't support EGL_SAMPLE_BUFFERS, but provide they own extension NV_coverage_sample
/// Also https://gfxbench.com/result.jsp says, that this extension supported on many Samsung, Asus, LG, Sony and other devices
static EGLint * getNVCoverageAttributesList()
{
  static EGLint attr_list[] =
  {
      EGL_RED_SIZE, 5,
      EGL_GREEN_SIZE, 6,
      EGL_BLUE_SIZE, 5,
      EGL_STENCIL_SIZE, 0,
      EGL_DEPTH_SIZE, 16,
      EGL_COVERAGE_BUFFERS_NV, 1,
      EGL_COVERAGE_SAMPLES_NV, 2,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
      EGL_SURFACE_TYPE, EGL_PBUFFER_BIT | EGL_WINDOW_BIT,
      EGL_NONE
  };
  return attr_list;
}

static EGLint * getConfigAttributesList()
{
  static EGLint attr_list[] =
  {
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
  , m_useCSAA(false)
{
  if (!jsurface)
    return;

  m_nativeWindow = ANativeWindow_fromSurface(env, jsurface);
  if (!m_nativeWindow)
  {
    LOG(LINFO, ("Can't get native window from Java surface"));
    return;
  }

  m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (m_display == EGL_NO_DISPLAY)
  {
    CHECK_EGL_CALL();
    return;
  }

  EGLint version[2] = {0};
  if (!eglInitialize(m_display, &version[0], &version[1]))
  {
    CHECK_EGL_CALL();
    return;
  }

  if (!(createWindowSurface() && createPixelbufferSurface()))
  {
    CHECK_EGL(eglTerminate(m_display));
    return;
  }

  if (!QuerySurfaceSize())
    return;

  m_valid = true;
}

AndroidOGLContextFactory::~AndroidOGLContextFactory()
{
  if (IsValid())
  {
    delete m_drawContext;
    delete m_uploadContext;

    eglDestroySurface(m_display, m_windowSurface);
    CHECK_EGL_CALL();
    eglDestroySurface(m_display, m_pixelbufferSurface);
    CHECK_EGL_CALL();
    eglTerminate(m_display);
    CHECK_EGL_CALL();

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
  QuerySurfaceSize();
}

bool AndroidOGLContextFactory::QuerySurfaceSize()
{
  EGLint queryResult;
  if (eglQuerySurface(m_display, m_windowSurface, EGL_WIDTH, &queryResult) == EGL_FALSE)
  {
    CHECK_EGL_CALL();
    return false;
  }

  m_surfaceWidth = static_cast<int>(queryResult);
  if (eglQuerySurface(m_display, m_windowSurface, EGL_HEIGHT, &queryResult) == EGL_FALSE)
  {
    CHECK_EGL_CALL();
    return false;
  }

  m_surfaceHeight = static_cast<int>(queryResult);
  return true;
}

dp::OGLContext * AndroidOGLContextFactory::getDrawContext()
{
  ASSERT(IsValid(), ());
  ASSERT(m_windowSurface != EGL_NO_SURFACE, ());
  if (m_drawContext == nullptr)
    m_drawContext = new AndroidOGLContext(m_display, m_windowSurface, m_config, m_uploadContext, m_useCSAA);
  return m_drawContext;
}

dp::OGLContext * AndroidOGLContextFactory::getResourcesUploadContext()
{
  ASSERT(IsValid(), ());
  ASSERT(m_pixelbufferSurface != EGL_NO_SURFACE, ());
  if (m_uploadContext == nullptr)
    m_uploadContext = new AndroidOGLContext(m_display, m_pixelbufferSurface, m_config, m_drawContext, false /* don't use CSAA for upload context*/);
  return m_uploadContext;
}

bool AndroidOGLContextFactory::isDrawContextCreated() const
{
  return m_drawContext != nullptr;
}

bool AndroidOGLContextFactory::isUploadContextCreated() const
{
  return m_uploadContext != nullptr;
}

namespace
{

int GetConfigCount(EGLDisplay display, EGLint * attribs)
{
  int count = 0;
  VERIFY(eglChooseConfig(display, attribs, nullptr, 0, &count) == EGL_TRUE, ());
  LOG(LINFO, ("Matched Config count = ", count));
  return count;
}

EGLint GetConfigValue(EGLDisplay display, EGLConfig config, EGLint attr)
{
  EGLint v = 0;
  VERIFY(eglGetConfigAttrib(display, config, attr, &v) == EGL_TRUE, ());
  return v;
}

/// It's a usefull code in debug
//void PrintConfig(EGLDisplay display, EGLConfig config)
//{
//  LOG(LINFO, ("=================="));
//  LOG(LINFO, ("Alpha = ", GetConfigValue(display, config, EGL_ALPHA_SIZE)));
//  LOG(LINFO, ("Red = ", GetConfigValue(display, config, EGL_RED_SIZE)));
//  LOG(LINFO, ("Blue = ", GetConfigValue(display, config, EGL_BLUE_SIZE)));
//  LOG(LINFO, ("Green = ", GetConfigValue(display, config, EGL_GREEN_SIZE)));
//  LOG(LINFO, ("Depth = ", GetConfigValue(display, config, EGL_DEPTH_SIZE)));
//  LOG(LINFO, ("Sample buffer = ", GetConfigValue(display, config, EGL_SAMPLE_BUFFERS)));
//  LOG(LINFO, ("Samples = ", GetConfigValue(display, config, EGL_SAMPLES)));
//  LOG(LINFO, ("NV Coverage buffers = ", GetConfigValue(display, config, 0x30E0)));
//  LOG(LINFO, ("NV Sample = ", GetConfigValue(display, config, 0x30E1)));
//
//  LOG(LINFO, ("Caveat = ", GetConfigValue(display, config, EGL_CONFIG_CAVEAT)));
//  LOG(LINFO, ("Conformant = ", GetConfigValue(display, config, EGL_CONFORMANT)));
//  LOG(LINFO, ("Transparent = ", GetConfigValue(display, config, EGL_TRANSPARENT_TYPE)));
//}

} // namespace

EGLint * AndroidOGLContextFactory::GetSupportedAttributes()
{
  EGLint * attribs = getMultisampleAttributesList();
  if (GetConfigCount(m_display, attribs) > 0)
    return attribs;

  attribs = getNVCoverageAttributesList();
  if (GetConfigCount(m_display, attribs) > 0)
  {
    m_useCSAA = true;
    return attribs;
  }

  return getConfigAttributesList();
}

bool AndroidOGLContextFactory::createWindowSurface()
{
  EGLConfig configs[40];
  int count = 0;
  EGLint * attribs = GetSupportedAttributes();
  VERIFY(eglChooseConfig(m_display, attribs, configs, 40, &count) == EGL_TRUE, ());
  ASSERT(count > 0, ("No one OGL config found"));

  sort(&configs[0], &configs[count], ConfigComparator(m_display));
  for (int i = 0; i < count; ++i)
  {
    EGLConfig currentConfig = configs[i];
    if (GetConfigValue(m_display, currentConfig, EGL_RED_SIZE) != 5)
      continue;

    EGLint format;
    eglGetConfigAttrib(m_display, currentConfig, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(m_nativeWindow, 0, 0, format);

    EGLint surfaceAttributes[] = { EGL_RENDER_BUFFER, EGL_BACK_BUFFER, EGL_NONE };
    m_windowSurface = eglCreateWindowSurface(m_display, currentConfig, m_nativeWindow, surfaceAttributes);
    if (m_windowSurface == EGL_NO_SURFACE)
      continue;
    else
      m_config = currentConfig;

    break;
  }

  if (m_windowSurface == EGL_NO_SURFACE)
  {
    CHECK_EGL_CALL();
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
    CHECK_EGL_CALL();
    return false;
  }

  return true;
}

} // namespace android
