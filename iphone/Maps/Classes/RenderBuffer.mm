#import <QuartzCore/CAEAGLLayer.h>

#include "RenderBuffer.hpp"
#include "graphics/opengl/opengl.hpp"
#include "graphics/coordinates.hpp"

namespace iphone
{

RenderBuffer::RenderBuffer(shared_ptr<RenderContext> renderContext, CAEAGLLayer * layer)
  : m_renderContext(renderContext)
{
  OGLCHECK(glGenRenderbuffersOES(1, &m_id));
  makeCurrent();

  BOOL res = [m_renderContext->getEAGLContext() renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:layer];

  if (res == NO)
    LOG(LINFO, ("renderbufferStorage:fromDrawable has failed!"));

  OGLCHECK(glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &m_width));
  OGLCHECK(glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &m_height));
}

RenderBuffer::RenderBuffer(shared_ptr<RenderContext> renderContext, int width, int height)
  : m_renderContext(renderContext)
{
  OGLCHECK(glGenRenderbuffersOES(1, &m_id));
  makeCurrent();

  m_width = width;
  m_height = height;
  OGLCHECK(glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_RGBA8_OES, m_width, m_height));
}

void RenderBuffer::makeCurrent()
{
  OGLCHECK(glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_id));
}

RenderBuffer::~RenderBuffer()
{
  OGLCHECK(glDeleteRenderbuffersOES(1, &m_id));
}

unsigned int RenderBuffer::id() const
{
  return m_id;
}

void RenderBuffer::present()
{
  makeCurrent();

  const int maxTryCount = 100;
  int tryCount = 0;
    
  while (!([m_renderContext->getEAGLContext() presentRenderbuffer:GL_RENDERBUFFER_OES])
         && (tryCount++ < maxTryCount));

  if (tryCount == maxTryCount + 1)
    NSLog(@"failed to present renderbuffer");
  else if (tryCount != 0)
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
}

void RenderBuffer::detachFromFrameBuffer()
{
  OGLCHECK(glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES,
                                        GL_COLOR_ATTACHMENT0_OES,
                                        GL_RENDERBUFFER_OES,
                                        0));
}
  
void RenderBuffer::coordMatrix(math::Matrix<float, 4, 4> & m)
{
  graphics::getOrthoMatrix(m, 0, width(), height(), 0, -graphics::maxDepth, graphics::maxDepth);
}
  
}
