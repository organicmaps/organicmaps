/*
 *
 *  RenderBuffer.hpp
 *  Maps
 *
 *  Created by Siarhei Rachytski on 8/15/10.
 *  Copyright 2010 OMIM. All rights reserved.
 *
 */

#import "RenderContext.hpp"
#import "../../../std/shared_ptr.hpp"
#import <QuartzCore/CAEAGLLayer.h>
#import "../../../yg/render_target.hpp"

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

		unsigned int id();

		void present();

		unsigned width() const;
		unsigned height() const;

		void attachToFrameBuffer();
	};
}
