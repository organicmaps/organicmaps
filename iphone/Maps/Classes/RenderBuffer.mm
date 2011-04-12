/*
 *  RenderBuffer.mm
 *  Maps
 *
 *  Created by Siarhei Rachytski on 8/15/10.
 *  Copyright 2010 OMIM. All rights reserved.
 *
 */

#include "RenderBuffer.hpp"
#include "../../../yg/internal/opengl.hpp"
#include "../../../yg/utils.hpp"

namespace iphone
{
	RenderBuffer::RenderBuffer(shared_ptr<RenderContext> renderContext, CAEAGLLayer * layer) : m_renderContext(renderContext)
	{
		OGLCHECK(glGenRenderbuffersOES(1, &m_id));
		makeCurrent();

		[m_renderContext->getEAGLContext() renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:layer];

		OGLCHECK(glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &m_width));
		OGLCHECK(glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &m_height));
	}

	void RenderBuffer::makeCurrent()
	{
		OGLCHECK(glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_id));
	}

	RenderBuffer::~RenderBuffer()
	{
		OGLCHECK(glDeleteRenderbuffersOES(1, &m_id));
	}

	unsigned int RenderBuffer::id()
	{
		return m_id;
	}

	void RenderBuffer::present()
	{
		makeCurrent();
		int tryCount = 0;
    while (!([m_renderContext->getEAGLContext() presentRenderbuffer:GL_RENDERBUFFER_OES])
					&& (tryCount++ < 100));

		if (tryCount != 0)
			NSLog(@"renderBuffer was presented from %d try", tryCount);
	}

	unsigned RenderBuffer::width() const
	{
		return m_width;
	}

	unsigned RenderBuffer::height() const
	{
		return m_height;
	}

	void RenderBuffer::attachToFrameBuffer()
	{
  	OGLCHECK(glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES,
																				  GL_COLOR_ATTACHMENT0_OES,
  																				GL_RENDERBUFFER_OES,
	  																			m_id));
		yg::gl::utils::setupCoordinates(width(), height(), true);
	}
}