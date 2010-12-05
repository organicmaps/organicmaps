/*
 * render_context.cpp
 *
 *  Created on: 26 Aug 2010
 *      Author: Alex
 */

#include "../../../base/SRC_FIRST.hpp"
#include "render_context.hpp"
#include "maps_form.h"
#include <FUiControls.h>
#include <FGraphicsOpenGL.h>
#include "../../../base/logging.hpp"
#include "../../../base/assert.hpp"

using namespace Osp::Graphics::Opengl;
using namespace Osp::Ui::Controls;

namespace bada
{
  RenderContext::RenderContext(shared_ptr<RenderContext> context)
    : m_eglDisplay(context->m_eglDisplay),
      m_eglConfig(context->m_eglConfig),
      m_eglSurface(context->m_eglSurface)
  {
    EGLint numConfigs = 1;
    EGLint eglConfigList[] =
    {
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_ALPHA_SIZE, 8,
      EGL_DEPTH_SIZE, 16,
      EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
      EGL_NONE
    };

    EGLint eglContextList[] =
    { EGL_CONTEXT_CLIENT_VERSION, 1, EGL_NONE };

    eglChooseConfig(m_eglDisplay, eglConfigList, &m_eglConfig, 1, &numConfigs); EGLCHECK;

    ASSERT(numConfigs != 0, ("eglChooseConfig() has been failed. because of matching config doesn't exist"));

    m_eglContext = eglCreateContext(
            context->m_eglDisplay,
            context->m_eglConfig,
            context->m_eglContext,
            eglContextList); EGLCHECK;

    ASSERT(m_eglContext != EGL_NO_CONTEXT, ("Context Creation Failed"));

	EGLint attribs[] = {
		EGL_WIDTH,   1024,
		EGL_HEIGHT,  1024,
		EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
		EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA,
//		EGL_BIND_TO_TEXTURE_RGBA, EGL_TRUE,
		EGL_NONE
	};

    m_eglSurface = eglCreatePbufferSurface(
			m_eglDisplay,
			m_eglConfig,
			attribs); EGLCHECK;

    ASSERT(m_eglSurface != EGL_NO_SURFACE, ("PBuffer Creation Failed"));
  }

  RenderContext::RenderContext(MapsForm * form):
      m_eglDisplay(EGL_DEFAULT_DISPLAY),
	  m_eglSurface(EGL_NO_SURFACE),
      m_eglContext(EGL_NO_CONTEXT),
      m_eglConfig(null)
  {
      EGLint numConfigs = 1;

      EGLint eglConfigList[] =
      {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
        EGL_NONE
      };

      EGLint eglContextList[] =
      { EGL_CONTEXT_CLIENT_VERSION, 1, EGL_NONE };

      eglBindAPI(EGL_OPENGL_ES_API);

      m_eglDisplay = eglGetDisplay((EGLNativeDisplayType) EGL_DEFAULT_DISPLAY); EGLCHECK;

      ASSERT(m_eglDisplay != EGL_NO_DISPLAY, ("eglGetDisplay() failed"));

      EGLint res = eglInitialize(m_eglDisplay, null, null); EGLCHECK;
      ASSERT(res != EGL_FALSE, ("eglInitialize() failed"));

      res = eglChooseConfig(m_eglDisplay, eglConfigList, &m_eglConfig, 1, &numConfigs); EGLCHECK;
      ASSERT(res != EGL_FALSE, ("eglChooseConfig() failed"));

      ASSERT(numConfigs != 0, ("eglChooseConfig() has been failed. because of matching config doesn't exist"));

      m_eglSurface = eglCreateWindowSurface(m_eglDisplay, m_eglConfig,
          (EGLNativeWindowType)form, null); EGLCHECK;

      ASSERT(m_eglSurface != EGL_NO_SURFACE, ("eglCreateWindowSurface failed"));

      m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, EGL_NO_CONTEXT, eglContextList); EGLCHECK;
      ASSERT(m_eglContext != EGL_NO_CONTEXT, ("eglCreateContext failed"));
  }

  RenderContext::~RenderContext()
  {
    if (EGL_NO_DISPLAY != m_eglDisplay)
    {
      eglMakeCurrent(m_eglDisplay, null, null, null);
      if (m_eglContext != EGL_NO_CONTEXT)
      {
        eglDestroyContext(m_eglDisplay, m_eglContext);
        m_eglContext = EGL_NO_CONTEXT;
      }

      if (m_eglSurface != EGL_NO_SURFACE)
      {
        eglDestroySurface(m_eglDisplay, m_eglSurface);
        m_eglSurface = EGL_NO_SURFACE;
      }

      eglTerminate(m_eglDisplay);
      m_eglDisplay = EGL_NO_DISPLAY;
    }

    m_eglConfig = null;
  }

  void RenderContext::makeCurrent()
  {
	  bool res = eglMakeCurrent(m_eglDisplay,
                                m_eglSurface,
                                m_eglSurface,
                                m_eglContext); EGLCHECK;
     ASSERT(res, ("eglMakeCurrent failed with error=", eglGetError()))
  }

  void RenderContext::releaseTexImage()
  {
      OGLCHECK(glEnable(GL_TEXTURE_2D));
      glBindTexture(GL_TEXTURE_2D, 0);
	  eglReleaseTexImage(m_eglDisplay, m_eglSurface, EGL_BACK_BUFFER); EGLCHECK;
  }

  void RenderContext::bindTexImage(shared_ptr<yg::gl::RGBA8Texture> texture)
  {
	  OGLCHECK(glEnable(GL_TEXTURE_2D));
	  texture->makeCurrent();
	  eglBindTexImage(m_eglDisplay, m_eglSurface, EGL_BACK_BUFFER); EGLCHECK;
  }

  void RenderContext::bindTexImage(int id)
  {
	  glEnable(GL_TEXTURE_2D);
	  glBindTexture(GL_TEXTURE_2D, id);
	  eglBindTexImage(m_eglDisplay, m_eglSurface, EGL_BACK_BUFFER); EGLCHECK;
  }

  void RenderContext::swapBuffers()
  {
	  eglSwapBuffers(m_eglDisplay, m_eglSurface); EGLCHECK;
  }
}
