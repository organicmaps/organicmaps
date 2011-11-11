#pragma once

#include "RenderContext.hpp"
#include "../../../std/shared_ptr.hpp"
#include "../../../yg/render_target.hpp"

@class CAEAGLLayer;

namespace	iphone
{
	class RenderBuffer : public yg::gl::RenderTarget
	{
	private:

		unsigned int m_id;
		shared_ptr<RenderContext> m_renderContext;
		int m_width;
		int m_height;

	public:

		RenderBuffer(shared_ptr<RenderContext> renderContext, CAEAGLLayer * layer);
		~RenderBuffer();

		void makeCurrent();

		unsigned int id() const;

		void present();

		unsigned width() const;
		unsigned height() const;

		void attachToFrameBuffer();
	};
}
