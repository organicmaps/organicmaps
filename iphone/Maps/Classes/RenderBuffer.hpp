/*
 *
 *  RenderBuffer.hpp
 *  Maps
 *
 *  Created by Siarhei Rachytski on 8/15/10.
 *  Copyright 2010 OMIM. All rights reserved.
 *
 */

#pragma once

#include "RenderContext.hpp"
#include "../../../std/shared_ptr.hpp"
#include "../../../yg/render_target.hpp"

#import <QuartzCore/CAEAGLLayer.h>

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
