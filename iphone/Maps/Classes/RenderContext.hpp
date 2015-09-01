#pragma once

#include <OpenGLES/EAGL.h>
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>

#include "graphics/opengl/gl_render_context.hpp"
#include "std/shared_ptr.hpp"


namespace iphone
{
  class RenderContext : public graphics::gl::RenderContext
  {
	private:
		/// Pointer to render context.
		EAGLContext * m_context;
	public:
		/// Create rendering context.
		RenderContext();
		/// Create shared render context.
		RenderContext(RenderContext * renderContext);
		/// Destroy render context
		~RenderContext();
		/// Make this rendering context current
		void makeCurrent();
		/// create a shared render context
    graphics::RenderContext * createShared();

  	EAGLContext * getEAGLContext();
  };
}