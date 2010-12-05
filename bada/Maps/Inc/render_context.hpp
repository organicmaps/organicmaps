/*
 * render_context.hpp
 *
 *  Created on: 26 Aug 2010
 *      Author: Alex
 */

#ifndef RENDER_CONTEXT_HPP_
#define RENDER_CONTEXT_HPP_

#include <FGraphicsOpenGL.h>
#include "../../../std/shared_ptr.hpp"
#include "../../../yg/texture.hpp"

class MapsForm;

namespace bada
{
  struct RenderContext
  {
    Osp::Graphics::Opengl::EGLDisplay m_eglDisplay;
    Osp::Graphics::Opengl::EGLConfig  m_eglConfig;
    Osp::Graphics::Opengl::EGLSurface m_eglSurface;
    Osp::Graphics::Opengl::EGLContext m_eglContext;

    RenderContext(shared_ptr<RenderContext> context);
    RenderContext(MapsForm * form);
    ~RenderContext();

    void makeCurrent();
    void releaseTexImage();
	void bindTexImage(shared_ptr<yg::gl::RGBA8Texture> texture);
	void bindTexImage(int id);
	void swapBuffers();
  };
}

#endif /* RENDER_CONTEXT_HPP_ */
