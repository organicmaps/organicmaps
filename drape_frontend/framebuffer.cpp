#include "framebuffer.hpp"

#include "drape/glfunctions.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "math.h"

#include "assert.h"

namespace df
{

Framebuffer::Framebuffer()
  : m_colorTextureId(0)
  , m_depthTextureId(0)
  , m_fbo(0)
{

}

Framebuffer::~Framebuffer()
{
  Destroy();
}

void Framebuffer::Destroy()
{
  if (m_colorTextureId)
  {
    GLFunctions::glDeleteTexture(m_depthTextureId);
    m_colorTextureId = 0;
  }
  if (m_depthTextureId)
  {
    GLFunctions::glDeleteTexture(m_depthTextureId);
    m_depthTextureId = 0;
  }
  if (m_fbo)
  {
    GLFunctions::glDeleteFramebuffer(&m_fbo);
    m_fbo = 0;
  }
}

void Framebuffer::SetSize(uint32_t width, uint32_t height)
{
  assert(width > 0 && height > 0);
  if (m_width == width && m_height == height)
    return;

  m_width = width;
  m_height = height;

  Destroy();

  m_colorTextureId = GLFunctions::glGenTexture();
  GLFunctions::glBindTexture(m_colorTextureId);
  GLFunctions::glTexImage2D(m_width, m_height, gl_const::GLRGBA, gl_const::GLUnsignedByteType, NULL);
  GLFunctions::glTexParameter(gl_const::GLMagFilter, gl_const::GLLinear);
  GLFunctions::glTexParameter(gl_const::GLMinFilter, gl_const::GLLinear);

  m_depthTextureId = GLFunctions::glGenTexture();
  GLFunctions::glBindTexture(m_depthTextureId);
  GLFunctions::glTexImage2D(m_width, m_height, gl_const::GLDepthComponent, gl_const::GLUnsignedIntType, NULL);

  GLFunctions::glBindTexture(0);

  GLFunctions::glGenFramebuffer(&m_fbo);
  GLFunctions::glBindFramebuffer(m_fbo);

  GLFunctions::glFramebufferTexture2D(gl_const::GLColorAttachment, m_colorTextureId);
  GLFunctions::glFramebufferTexture2D(gl_const::GLDepthAttachment, m_depthTextureId);
  GLFunctions::glFramebufferTexture2D(gl_const::GlStencilAttachment, 0);

  uint32_t status = GLFunctions::glCheckFramebufferStatus();
  if (status != gl_const::GlFramebufferComplete)
    LOG(LWARNING, ("INCOMPLETE FRAMEBUFFER: ", strings::to_string(status)));

  //GLFunctions::glFlush();
  GLFunctions::glBindFramebuffer(0);
}

void Framebuffer::Enable()
{
  GLFunctions::glBindFramebuffer(m_fbo);
}

void Framebuffer::Disable()
{
  GLFunctions::glBindFramebuffer(0);
}

uint32_t Framebuffer::GetTextureId() const
{
  return m_colorTextureId;
}

}
