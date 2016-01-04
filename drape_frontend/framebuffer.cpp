#include "framebuffer.hpp"

#include "drape/glfunctions.hpp"
#include "drape/oglcontext.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "math.h"

namespace df
{

Framebuffer::~Framebuffer()
{
  Destroy();
}

void Framebuffer::Destroy()
{
  if (m_colorTextureId != 0)
  {
    GLFunctions::glDeleteTexture(m_colorTextureId);
    m_colorTextureId = 0;
  }
  if (m_depthTextureId != 0)
  {
    GLFunctions::glDeleteTexture(m_depthTextureId);
    m_depthTextureId = 0;
  }
  if (m_framebufferId != 0)
  {
    GLFunctions::glDeleteFramebuffer(&m_framebufferId);
    m_framebufferId = 0;
  }
}

void Framebuffer::SetDefaultContext(dp::OGLContext * context)
{
  m_defaultContext = context;
}

int32_t Framebuffer::GetMaxSize()
{
  if (m_maxTextureSize == 0)
    m_maxTextureSize = GLFunctions::glGetInteger(gl_const::GLMaxTextureSize);
  return m_maxTextureSize;
}

void Framebuffer::SetSize(uint32_t width, uint32_t height)
{
  ASSERT(m_defaultContext, ());

  if (m_width == width && m_height == height)
    return;

  m_height = height;
  m_width = width;

  Destroy();

  m_colorTextureId = GLFunctions::glGenTexture();
  GLFunctions::glBindTexture(m_colorTextureId);
  GLFunctions::glTexImage2D(m_width, m_height, gl_const::GLRGBA, gl_const::GLUnsignedByteType, NULL);
  GLFunctions::glTexParameter(gl_const::GLMagFilter, gl_const::GLLinear);
  GLFunctions::glTexParameter(gl_const::GLMinFilter, gl_const::GLLinear);
  GLFunctions::glTexParameter(gl_const::GLWrapT, gl_const::GLClampToEdge);
  GLFunctions::glTexParameter(gl_const::GLWrapS, gl_const::GLClampToEdge);

  m_depthTextureId = GLFunctions::glGenTexture();
  GLFunctions::glBindTexture(m_depthTextureId);
  GLFunctions::glTexImage2D(m_width, m_height, gl_const::GLDepthComponent, gl_const::GLUnsignedIntType, NULL);

  GLFunctions::glBindTexture(0);

  GLFunctions::glGenFramebuffer(&m_framebufferId);
  GLFunctions::glBindFramebuffer(m_framebufferId);

  GLFunctions::glFramebufferTexture2D(gl_const::GLColorAttachment, m_colorTextureId);
  GLFunctions::glFramebufferTexture2D(gl_const::GLDepthAttachment, m_depthTextureId);
  GLFunctions::glFramebufferTexture2D(gl_const::GLStencilAttachment, 0);

  uint32_t const status = GLFunctions::glCheckFramebufferStatus();
  if (status != gl_const::GLFramebufferComplete)
    LOG(LWARNING, ("Incomplete framebuffer:", strings::to_string(status)));

  m_defaultContext->setDefaultFramebuffer();
}

void Framebuffer::Enable()
{
  GLFunctions::glBindFramebuffer(m_framebufferId);
}

void Framebuffer::Disable()
{
  ASSERT(m_defaultContext, ());
  m_defaultContext->setDefaultFramebuffer();
}

uint32_t Framebuffer::GetTextureId() const
{
  return m_colorTextureId;
}
}  // namespace df
