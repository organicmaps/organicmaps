#include "androidoglcontext.hpp"
#include "../../../base/assert.hpp"
#include "../../../base/logging.hpp"

namespace android
{

static EGLint * getContextAttributesList()
{
  static EGLint contextAttrList[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };
  return contextAttrList;
}

AndroidOGLContext::AndroidOGLContext(EGLDisplay display, EGLSurface surface, EGLConfig config, AndroidOGLContext * contextToShareWith)
  : m_nativeContext(EGL_NO_CONTEXT)
  , m_surface(surface)
  , m_display(display)
{
  ASSERT(m_surface != EGL_NO_SURFACE, ());
  ASSERT(m_display != EGL_NO_DISPLAY, ());

  EGLContext sharedContext = (contextToShareWith == NULL) ? EGL_NO_CONTEXT : contextToShareWith->m_nativeContext;
  m_nativeContext = eglCreateContext(m_display, config, sharedContext, getContextAttributesList());
  CHECK(m_nativeContext != EGL_NO_CONTEXT, ());
}

AndroidOGLContext::~AndroidOGLContext()
{
  // Native context must exist
  eglDestroyContext(m_display, m_nativeContext);
}

void AndroidOGLContext::setDefaultFramebuffer()
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void AndroidOGLContext::makeCurrent()
{
  eglMakeCurrent(m_display, m_surface, m_surface, m_nativeContext);
}

void AndroidOGLContext::present()
{
  eglSwapBuffers(m_display, m_surface);
}

} // namespace android