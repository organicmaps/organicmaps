#include "RenderContext.hpp"

namespace iphone
{
	RenderContext::RenderContext()
	{
    m_context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
	}

	RenderContext::RenderContext(RenderContext * renderContext)
	{
		m_context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2 sharegroup:renderContext->m_context.sharegroup];
	}

	RenderContext::~RenderContext()
	{

	}

	void RenderContext::makeCurrent()
	{
    [EAGLContext setCurrentContext:m_context];
	}
 
  graphics::RenderContext * RenderContext::createShared()
	{
    graphics::RenderContext * res = new RenderContext(this);
    res->setResourceManager(resourceManager());
    return res;
	}

	EAGLContext * RenderContext::getEAGLContext()
	{
		return m_context;
	}
}



