#include "RenderContext.hpp"
#include "../../../base/assert.hpp"
#include <FGraphicsOpengl2.h>

using namespace Tizen::Graphics::Opengl;

namespace tizen
{

RenderContext::RenderContext()
:m_display(EGL_NO_DISPLAY),
 m_surface(EGL_NO_SURFACE),
 m_context(EGL_NO_CONTEXT)
{


}

bool RenderContext::Init(Tizen::Ui::Controls::Form * form)
{
  eglBindAPI(EGL_OPENGL_ES_API);
  m_display = eglGetDisplay(EGLNativeDisplayType(EGL_DEFAULT_DISPLAY));
  if (m_display == EGL_NO_DISPLAY)
    return false;
  if (!eglInitialize(m_display, 0, 0))
  {
    m_display = EGL_NO_DISPLAY;
    return false;
  }
  EGLint eglConfigList[] =
  {
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE,  8,
      EGL_ALPHA_SIZE, 8,
      EGL_DEPTH_SIZE, 8,
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
      EGL_NONE
  };

  EGLint num_configs = 1;
  EGLConfig config;
  if (!eglChooseConfig(m_display, eglConfigList, &config, 1, &num_configs))
  {
    eglTerminate(m_display);
    m_display = EGL_NO_DISPLAY;
    return false;
  }

  ASSERT(num_configs > 0, ());

  m_surface = eglCreateWindowSurface(m_display, config, (EGLNativeWindowType)(form), 0);

  if (m_surface == EGL_NO_SURFACE)
  {
    eglTerminate(m_display);
    m_display = EGL_NO_DISPLAY;
    return false;
  }

  EGLint eglContextList[] =
  {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
  };

  m_context = eglCreateContext(m_display, config, EGL_NO_CONTEXT, eglContextList);

  if (m_context == EGL_NO_CONTEXT)
  {
    eglDestroySurface(m_display, m_surface);
    m_surface = EGL_NO_SURFACE;
    eglTerminate(m_display);
    m_display = EGL_NO_DISPLAY;

    return false;
  }

  return true;
}

RenderContext::~RenderContext()
{
  if (m_context != EGL_NO_CONTEXT)
  {
    eglDestroyContext(m_display, m_context);
    if (m_surface != EGL_NO_SURFACE)
    {
      eglDestroySurface(m_display, m_surface);
      if (m_display != EGL_NO_DISPLAY)
      {
        eglTerminate(m_display);
      }
    }
  }
}

void RenderContext::SwapBuffers()
{
  eglSwapBuffers(m_display, m_surface);
}


void RenderContext::makeCurrent()
{
  EGLBoolean b_res = eglMakeCurrent(m_display, m_surface, m_surface, m_context);

  if (b_res == EGL_FALSE)
  {
    /// todo throw eglGetError();
  }
  eglSwapInterval(m_display, 1);
}

RenderContext * RenderContext::createShared()
{
  ASSERT(false, ());
  return 0;
}

}
