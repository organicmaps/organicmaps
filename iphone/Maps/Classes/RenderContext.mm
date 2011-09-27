/*
 *  RenderContext.mm
 *  Maps
 *
 *  Created by Siarhei Rachytski on 8/14/10.
 *  Copyright 2010 OMIM. All rights reserved.
 *
 */

#include "RenderContext.hpp"

namespace iphone
{
	RenderContext::RenderContext()
	{
    m_context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
	}

	RenderContext::RenderContext(RenderContext * renderContext)
	{
		m_context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1 sharegroup:renderContext->m_context.sharegroup];
	}

	RenderContext::~RenderContext()
	{
		[m_context release];
	}

	void RenderContext::makeCurrent()
	{
    [EAGLContext setCurrentContext:m_context];
    yg::gl::RenderContext::initParams();
	}
 
  shared_ptr<yg::gl::RenderContext> RenderContext::createShared()
	{
		return shared_ptr<yg::gl::RenderContext>(new RenderContext(this));
	}

	EAGLContext * RenderContext::getEAGLContext()
	{
		return m_context;
	}
}



