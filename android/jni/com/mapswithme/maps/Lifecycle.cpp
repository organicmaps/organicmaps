#include <jni.h>
#include <android/log.h>
#include "../../../nv_event/nv_event.hpp"
#include "base/logging.hpp"
#include "graphics/opengl/opengl.hpp"
#include "Framework.hpp"
#include "../platform/Platform.hpp"

#define MODULE "MapsWithMe"
#define NVDEBUG(args...) __android_log_print(ANDROID_LOG_DEBUG, MODULE, ## args)

static int32_t s_winWidth = 1;
static int32_t s_winHeight = 1;
static int32_t s_densityDpi = 1;

static bool s_glesLoaded = false;

bool SetupGLESResources()
{
  if (s_glesLoaded)
    return true;

  if (!g_framework->InitRenderPolicy(s_densityDpi, s_winWidth, s_winHeight))
  {
    NVEventReportUnsupported();
    return false;
  }
  else
    NVEventOnRenderingInitialized();

  s_glesLoaded = true;

  return true;
}

bool ShutdownGLESResources()
{
  if (!s_glesLoaded)
    return true;

  // We cannot use GLES calls to release the resources if the context is
  // not bound.  In that case, we simply shut down EGL, which has code to
  // explicitly delete the context
  if (!NVEventStatusEGLIsBound())
  {
    NVDEBUG("ShutdownGLESResources: GLES not bound, shutting down EGL to release");

    graphics::gl::g_hasContext = false;

    g_framework->DeleteRenderPolicy();

    graphics::gl::g_hasContext = true;

    NVDEBUG("Cleaning up EGL");

    if (NVEventCleanupEGL())
    {
      s_glesLoaded = false;
      return true;
    }
    else
    {
      return false;
    }
  }

  NVDEBUG("ShutdownGLESResources event: GLES bound, manually deleting GLES resources");

  g_framework->DeleteRenderPolicy();

  s_glesLoaded = false;

  return true;
}

bool renderFrame(bool allocateIfNeeded)
{
  if (!NVEventReadyToRenderEGL(allocateIfNeeded))
    return false;

  // We've gotten this far, so EGL is ready for us.  Have we loaded our assets?
  // Note that we cannot use APP_STATUS_GLES_LOADED to imply that EGL is
  // ready to render.  We can have a valid context with all GLES resources loaded
  // into it but no surface and thus the context not bound.  These are semi-
  // independent states.
  if (!s_glesLoaded)
  {
    if (!allocateIfNeeded)
      return false;

    if (!SetupGLESResources())
      return false;
  }

  g_framework->DrawFrame();

  return true;
}

namespace
{

const int32_t TIMEOUT_FOREGROUND_IN_FOCUS = 1; // msec
const int32_t TIMEOUT_FOREGROUND_NOT_IN_FOCUS = 100; // msec

int32_t GetEventQueueTimeout(bool isAppInBackground)
{
  if (isAppInBackground)
    return NV_EVENT_WAIT_FOREVER;
  else
    return (NVEventStatusIsFocused() ? TIMEOUT_FOREGROUND_IN_FOCUS : TIMEOUT_FOREGROUND_NOT_IN_FOCUS);
}

} // namespace

// Add any initialization that requires the app Java classes
// to be accessible (such as nv_shader_init, NvAPKInit, etc,
// as listed in the docs)
int32_t NVEventAppInit(int32_t argc, char** argv)
{
  return 0;
}

int32_t NVEventAppMain(int32_t argc, char** argv)
{
  s_glesLoaded = false;

  bool isAppInBackground = true;

  NVDEBUG("Application entering main loop");

  while (NVEventStatusIsRunning())
  {
    const NVEvent* ev = NULL;
    while (NVEventStatusIsRunning() &&
          (ev = NVEventGetNextEvent(GetEventQueueTimeout(isAppInBackground))))
    {
      switch (ev->m_type)
      {
        case NV_EVENT_KEY:
          NVDEBUG( "Key event: 0x%02x %s",
                 ev->m_data.m_key.m_code,
                 (ev->m_data.m_key.m_action == NV_KEYACTION_DOWN) ? "down" : "up");

          if ((ev->m_data.m_key.m_code == NV_KEYCODE_BACK))
          {
            renderFrame(false);
            NVEventDoneWithEvent(true);
          }
          else
          {
            NVEventDoneWithEvent(false);
          }
          ev = NULL;
          break;

        case NV_EVENT_CHAR:
          NVDEBUG("Char event: 0x%02x", ev->m_data.m_char.m_unichar);
          ev = NULL;
          NVEventDoneWithEvent(false);
          break;
        case NV_EVENT_LONG_CLICK:
          break;
        case NV_EVENT_TOUCH:
          {
              switch (ev->m_data.m_touch.m_action)
              {
              case NV_TOUCHACTION_DOWN:
                g_framework->Move(0, ev->m_data.m_touch.m_x, ev->m_data.m_touch.m_y);
                break;
              case NV_TOUCHACTION_MOVE:
                g_framework->Move(1, ev->m_data.m_touch.m_x, ev->m_data.m_touch.m_y);
                break;
              case NV_TOUCHACTION_UP:
                g_framework->Move(2, ev->m_data.m_touch.m_x, ev->m_data.m_touch.m_y);
                break;
              }
          }
          break;

        case NV_EVENT_MULTITOUCH:
          {
            int maskOnly = (ev->m_data.m_multi.m_action & NV_MULTITOUCH_POINTER_MASK) >> (NV_MULTITOUCH_POINTER_SHIFT);
            int action = ev->m_data.m_multi.m_action & NV_MULTITOUCH_ACTION_MASK;

            g_framework->Touch(action, maskOnly, ev->m_data.m_multi.m_x1, ev->m_data.m_multi.m_y1,
                                                 ev->m_data.m_multi.m_x2, ev->m_data.m_multi.m_y2);
          }
          break;

        case NV_EVENT_SURFACE_CREATED:
        case NV_EVENT_SURFACE_SIZE:
          s_winWidth = ev->m_data.m_size.m_w;
          s_winHeight = ev->m_data.m_size.m_h;
          s_densityDpi = ev->m_data.m_size.m_density;

          g_framework->Resize(s_winWidth, s_winHeight);
          g_framework->NativeFramework()->Invalidate(true);

          NVDEBUG( "Surface create/resize event: %d x %d", s_winWidth, s_winHeight);
          break;

        case NV_EVENT_SURFACE_DESTROYED:
          NVDEBUG("Surface destroyed event");
          NVEventDestroySurfaceEGL();
          break;

        case NV_EVENT_FOCUS_LOST:
          NVDEBUG("Focus lost event");

          renderFrame(false);
          break;

        case NV_EVENT_PAUSE:
          NVDEBUG("Pause event");

          renderFrame(false);
          break;

        case NV_EVENT_STOP:
          NVDEBUG("Stop event");

          // As per Google's recommendation, we release GLES resources here
          ShutdownGLESResources();
          isAppInBackground = true;
          break;

        case NV_EVENT_QUIT:
          NVDEBUG("Quit event");

          break;

        case NV_EVENT_START:
        case NV_EVENT_RESTART:
          NVDEBUG("Start/restart event: %s", NVEventGetEventStr(ev->m_type));
          isAppInBackground = false;
          break;

        case NV_EVENT_ACCEL:
        case NV_EVENT_RESUME:
        case NV_EVENT_FOCUS_GAINED:
          NVDEBUG("%s event: no specific app action", NVEventGetEventStr(ev->m_type));
          break;

        case NV_EVENT_MWM:
        {
          typedef function<void ()> FnT;
          FnT * pFn = reinterpret_cast<FnT *>(ev->m_data.m_mwm.m_pFn);
          (*pFn)();
          delete pFn;
          break;
        }

        default:
          NVDEBUG("UNKNOWN event");
          break;
      };

      // if we do not NULL out the event, then we return that
      // we handled it by default
      if (ev)
        NVEventDoneWithEvent(true);
    }

    // Do not bother to initialize _any_ of EGL, much less go ahead
    // and render to the screen unless we have all we need to go
    // ahead and do our thing.  In many cases,
    // devices will bring us part-way up and then take us down.
    // So, before we bother to init EGL (much less the rendering
    // surface, check that:
    // - we are focused
    // - we have a rendering surface
    // - the surface size is not 0x0
    // - we are resumed, not paused
    if (NVEventStatusIsInteractable())
    {
      // This will try to set up EGL if it isn't set up
      // When we first set up EGL completely, we also load our GLES resources
      // If these are already set up or we succeed at setting them all up now, then
      // we go ahead and render.
      renderFrame(true);
    }
  }

  NVDEBUG("cleanup!!!");

  NVEventCleanupEGL();

  return 0;
}
