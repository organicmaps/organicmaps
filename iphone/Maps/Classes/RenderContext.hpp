/*
 *  RenderContext.hpp
 *  Maps
 *
 *  Created by Siarhei Rachytski on 8/14/10.
 *  Copyright 2010 OMIM. All rights reserved.
 *
 */

#include <OpenGLES/EAGL.h>
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>

#include "../../../yg/rendercontext.hpp"
#include "../../../std/shared_ptr.hpp"

namespace iphone
{
  class RenderContext : public yg::gl::RenderContext
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
		shared_ptr<yg::gl::RenderContext> createShared();
    /// @TODO
    void endThreadDrawing() {}

  	EAGLContext * getEAGLContext();
  };
}