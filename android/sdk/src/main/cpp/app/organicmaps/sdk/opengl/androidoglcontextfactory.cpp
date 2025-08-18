#include "androidoglcontextfactory.hpp"
#include "android_gl_utils.hpp"

#include "app/organicmaps/sdk/platform/AndroidPlatform.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/src_point.hpp"

#include <algorithm>

#include <EGL/egl.h>
#include <android/api-level.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#define EGL_OPENGL_ES3_BIT 0x00000040

namespace android
{
namespace
{
static EGLint * getConfigAttributesListRGB8()
{
  static EGLint attr_list[] = {EGL_RED_SIZE,
                               8,
                               EGL_GREEN_SIZE,
                               8,
                               EGL_BLUE_SIZE,
                               8,
                               EGL_ALPHA_SIZE,
                               0,
                               EGL_STENCIL_SIZE,
                               0,
                               EGL_DEPTH_SIZE,
                               16,
                               EGL_RENDERABLE_TYPE,
                               EGL_OPENGL_ES3_BIT,
                               EGL_SURFACE_TYPE,
                               EGL_PBUFFER_BIT | EGL_WINDOW_BIT,
                               EGL_NONE};
  return attr_list;
}

int const kMaxConfigCount = 40;

bool IsSupportedRGB8(EGLDisplay display)
{
  EGLConfig configs[kMaxConfigCount];
  int count = 0;
  return eglChooseConfig(display, getConfigAttributesListRGB8(), configs, kMaxConfigCount, &count) == EGL_TRUE &&
         count != 0;
}

size_t constexpr kGLThreadsCount = 2;
}  // namespace

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
  , m_windowSurfaceValid(false)
{
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

  CHECK(gl3stubInit(), ("Could not initialize OpenGL ES3"));

  SetSurface(env, jsurface);

  if (!CreatePixelbufferSurface())
  {
    CHECK_EGL(eglTerminate(m_display));
    return;
  }
}

AndroidOGLContextFactory::~AndroidOGLContextFactory()
{
  if (m_drawContext != nullptr)
  {
    delete m_drawContext;
    m_drawContext = nullptr;
  }

  if (m_uploadContext != nullptr)
  {
    delete m_uploadContext;
    m_uploadContext = nullptr;
  }

  ResetSurface();

  if (m_pixelbufferSurface != EGL_NO_SURFACE)
  {
    eglDestroySurface(m_display, m_pixelbufferSurface);
    CHECK_EGL_CALL();
    m_pixelbufferSurface = EGL_NO_SURFACE;
  }

  if (m_display != EGL_NO_DISPLAY)
  {
    eglTerminate(m_display);
    CHECK_EGL_CALL();
  }
}

void AndroidOGLContextFactory::SetSurface(JNIEnv * env, jobject jsurface)
{
  if (!jsurface)
    return;

  m_nativeWindow = ANativeWindow_fromSurface(env, jsurface);
  if (!m_nativeWindow)
  {
    LOG(LINFO, ("Can't get native window from Java surface"));
    return;
  }

  if (!CreateWindowSurface())
  {
    CHECK_EGL(eglTerminate(m_display));
    return;
  }

  if (!QuerySurfaceSize())
    return;

  if (m_drawContext != nullptr)
    m_drawContext->SetSurface(m_windowSurface);

  m_windowSurfaceValid = true;
}

void AndroidOGLContextFactory::ResetSurface()
{
  {
    std::unique_lock<std::mutex> lock(m_initializationMutex);
    if (m_initializationCounter > 0 && m_initializationCounter < kGLThreadsCount)
      m_initializationCondition.wait(lock, [this] { return m_isInitialized; });
    m_initializationCounter = 0;
    m_isInitialized = false;
  }

  if (m_drawContext != nullptr)
    m_drawContext->ResetSurface();

  if (IsValid())
  {
    eglDestroySurface(m_display, m_windowSurface);
    CHECK_EGL_CALL();
    m_windowSurface = EGL_NO_SURFACE;

    ANativeWindow_release(m_nativeWindow);
    m_nativeWindow = NULL;

    m_windowSurfaceValid = false;
  }
}

bool AndroidOGLContextFactory::IsValid() const
{
  return m_windowSurfaceValid && m_pixelbufferSurface != EGL_NO_SURFACE;
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

void AndroidOGLContextFactory::UpdateSurfaceSize(int w, int h)
{
  ASSERT(IsValid(), ());
  if ((m_surfaceWidth != w && m_surfaceWidth != h) || (m_surfaceHeight != w && m_surfaceHeight != h))
  {
    LOG(LINFO, ("Surface size changed and must be re-queried."));
    if (!QuerySurfaceSize())
    {
      m_surfaceWidth = w;
      m_surfaceHeight = h;
    }
  }
  else
  {
    m_surfaceWidth = w;
    m_surfaceHeight = h;
  }
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

dp::GraphicsContext * AndroidOGLContextFactory::GetDrawContext()
{
  ASSERT(IsValid(), ());
  ASSERT(m_windowSurface != EGL_NO_SURFACE, ());
  if (m_drawContext == nullptr)
    m_drawContext = new AndroidOGLContext(m_display, m_windowSurface, m_config, m_uploadContext);
  return m_drawContext;
}

dp::GraphicsContext * AndroidOGLContextFactory::GetResourcesUploadContext()
{
  ASSERT(IsValid(), ());
  ASSERT(m_pixelbufferSurface != EGL_NO_SURFACE, ());
  if (m_uploadContext == nullptr)
    m_uploadContext = new AndroidOGLContext(m_display, m_pixelbufferSurface, m_config, m_drawContext);
  return m_uploadContext;
}

bool AndroidOGLContextFactory::IsDrawContextCreated() const
{
  return m_drawContext != nullptr;
}

bool AndroidOGLContextFactory::IsUploadContextCreated() const
{
  return m_uploadContext != nullptr;
}

void AndroidOGLContextFactory::WaitForInitialization(dp::GraphicsContext *)
{
  std::unique_lock<std::mutex> lock(m_initializationMutex);
  if (m_isInitialized)
    return;

  m_initializationCounter++;
  if (m_initializationCounter >= kGLThreadsCount)
  {
    m_isInitialized = true;
    m_initializationCondition.notify_all();
  }
  else
  {
    m_initializationCondition.wait(lock, [this] { return m_isInitialized; });
  }
}

void AndroidOGLContextFactory::SetPresentAvailable(bool available)
{
  if (m_drawContext != nullptr)
    m_drawContext->SetPresentAvailable(available);
}

bool AndroidOGLContextFactory::CreateWindowSurface()
{
  EGLConfig configs[kMaxConfigCount];
  int count = 0;
  if (eglChooseConfig(m_display, getConfigAttributesListRGB8(), configs, kMaxConfigCount, &count) == EGL_TRUE)
  {
    CHECK(IsSupportedRGB8(m_display), ("RGB8 is not suported on this device"));
    LOG(LDEBUG, ("Backbuffer format: RGB8"));
  }
  else
  {
    CHECK(false, ("OpenGL ES3 is not supported"));
  }
  ASSERT(count > 0, ("Didn't find any configs."));

  std::sort(&configs[0], &configs[count], ConfigComparator(m_display));
  for (int i = 0; i < count; ++i)
  {
    EGLConfig currentConfig = configs[i];

    EGLint format;
    eglGetConfigAttrib(m_display, currentConfig, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(m_nativeWindow, 0, 0, format);

    EGLint surfaceAttributes[] = {EGL_RENDER_BUFFER, EGL_BACK_BUFFER, EGL_NONE};
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

bool AndroidOGLContextFactory::CreatePixelbufferSurface()
{
  // ASSERT(m_config != NULL, ());

  GLuint const size = 1;  // yes, 1 is the correct size, we dont really draw on it
  static EGLint surfaceConfig[] = {EGL_WIDTH, size, EGL_HEIGHT, size, EGL_NONE};

  m_pixelbufferSurface = eglCreatePbufferSurface(m_display, m_config, surfaceConfig);

  if (m_pixelbufferSurface == EGL_NO_SURFACE)
  {
    CHECK_EGL_CALL();
    return false;
  }

  return true;
}
}  // namespace android
