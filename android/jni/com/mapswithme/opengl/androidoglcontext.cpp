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
  LOG(LINFO, ("UVR : Context created = ", m_nativeContext));

  CHECK(m_nativeContext != EGL_NO_CONTEXT, ());
  LOG(LINFO, ("UVR : Present"));
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
  LOG(LINFO, ("UVR : Make current for context"));
  if (eglMakeCurrent(m_display, m_surface, m_surface, m_nativeContext) != EGL_TRUE)
    LOG(LINFO, ("Failed to set current context:", eglGetError()));
}

void AndroidOGLContext::present()
{
  LOG(LINFO, ("UVR : Present"));
  if(eglSwapBuffers(m_display, m_surface) != EGL_TRUE)
    LOG(LINFO, ("Failed to swap buffers:", eglGetError()));
}

} // namespace android