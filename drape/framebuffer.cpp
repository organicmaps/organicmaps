#include "drape/framebuffer.hpp"
#include "drape/glfunctions.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

namespace dp
{
Framebuffer::DepthStencil::DepthStencil(bool stencilEnabled)
  : m_stencilEnabled(stencilEnabled)
{
  if (m_stencilEnabled)
  {
    m_layout = gl_const::GLDepthStencil;
    m_pixelType = gl_const::GLUnsignedInt24_8Type;
  }
  else
  {
    m_layout = gl_const::GLDepthComponent;
    m_pixelType = gl_const::GLUnsignedIntType;
  }
}

Framebuffer::DepthStencil::~DepthStencil()
{
  Destroy();
}

void Framebuffer::DepthStencil::SetSize(uint32_t width, uint32_t height)
{
  Destroy();

  m_textureId = GLFunctions::glGenTexture();
  GLFunctions::glBindTexture(m_textureId);
  GLFunctions::glTexImage2D(width, height, m_layout, m_pixelType, nullptr);
  if (GLFunctions::CurrentApiVersion == dp::ApiVersion::OpenGLES3)
  {
    GLFunctions::glTexParameter(gl_const::GLMagFilter, gl_const::GLNearest);
    GLFunctions::glTexParameter(gl_const::GLMinFilter, gl_const::GLNearest);
    GLFunctions::glTexParameter(gl_const::GLWrapT, gl_const::GLClampToEdge);
    GLFunctions::glTexParameter(gl_const::GLWrapS, gl_const::GLClampToEdge);
  }
}

void Framebuffer::DepthStencil::Destroy()
{
  if (m_textureId != 0)
  {
    GLFunctions::glDeleteTexture(m_textureId);
    m_textureId = 0;
  }
}

uint32_t Framebuffer::DepthStencil::GetDepthAttachmentId() const
{
  return m_textureId;
}

uint32_t Framebuffer::DepthStencil::GetStencilAttachmentId() const
{
  return m_stencilEnabled ? m_textureId : 0;
}

Framebuffer::Framebuffer()
  : m_colorFormat(gl_const::GLRGBA)
{
  ApplyOwnDepthStencil();
}

Framebuffer::Framebuffer(uint32_t colorFormat)
  : m_colorFormat(colorFormat)
{
  ApplyOwnDepthStencil();
}

Framebuffer::Framebuffer(uint32_t colorFormat, bool stencilEnabled)
  : m_depthStencil(make_unique_dp<dp::Framebuffer::DepthStencil>(stencilEnabled))
  , m_colorFormat(colorFormat)
{
  ApplyOwnDepthStencil();
}

Framebuffer::~Framebuffer() { Destroy(); }

void Framebuffer::Destroy()
{
  if (m_colorTextureId != 0)
  {
    GLFunctions::glDeleteTexture(m_colorTextureId);
    m_colorTextureId = 0;
  }

  if (m_depthStencil != nullptr)
    m_depthStencil->Destroy();

  if (m_framebufferId != 0)
  {
    GLFunctions::glDeleteFramebuffer(&m_framebufferId);
    m_framebufferId = 0;
  }
}

void Framebuffer::SetFramebufferFallback(FramebufferFallback && fallback)
{
  m_framebufferFallback = std::move(fallback);
}

void Framebuffer::SetSize(uint32_t width, uint32_t height)
{
  if (!m_isSupported)
    return;

  if (m_width == width && m_height == height)
    return;

  m_width = width;
  m_height = height;

  Destroy();

  m_colorTextureId = GLFunctions::glGenTexture();
  GLFunctions::glBindTexture(m_colorTextureId);
  GLFunctions::glTexImage2D(m_width, m_height, m_colorFormat, gl_const::GLUnsignedByteType,
                            nullptr);
  GLFunctions::glTexParameter(gl_const::GLMagFilter, gl_const::GLLinear);
  GLFunctions::glTexParameter(gl_const::GLMinFilter, gl_const::GLLinear);
  GLFunctions::glTexParameter(gl_const::GLWrapT, gl_const::GLClampToEdge);
  GLFunctions::glTexParameter(gl_const::GLWrapS, gl_const::GLClampToEdge);

  glConst depthAttachmentId = 0;
  glConst stencilAttachmentId = 0;
  if (m_depthStencilRef != nullptr)
  {
    if (m_depthStencilRef == m_depthStencil.get())
      m_depthStencilRef->SetSize(m_width, m_height);
    depthAttachmentId = m_depthStencilRef->GetDepthAttachmentId();
    stencilAttachmentId = m_depthStencilRef->GetStencilAttachmentId();
  }

  GLFunctions::glBindTexture(0);

  GLFunctions::glGenFramebuffer(&m_framebufferId);
  GLFunctions::glBindFramebuffer(m_framebufferId);

  GLFunctions::glFramebufferTexture2D(gl_const::GLColorAttachment, m_colorTextureId);
  if (depthAttachmentId != stencilAttachmentId)
  {
    GLFunctions::glFramebufferTexture2D(gl_const::GLDepthAttachment, depthAttachmentId);
    GLFunctions::glFramebufferTexture2D(gl_const::GLStencilAttachment, stencilAttachmentId);
  }
  else
  {
    GLFunctions::glFramebufferTexture2D(gl_const::GLDepthStencilAttachment, depthAttachmentId);
  }

  uint32_t const status = GLFunctions::glCheckFramebufferStatus();
  if (status != gl_const::GLFramebufferComplete)
  {
    m_isSupported = false;
    Destroy();
    LOG(LWARNING, ("Framebuffer is unsupported. Framebuffer status =", status));
  }

  if (m_framebufferFallback != nullptr)
    m_framebufferFallback();
}

void Framebuffer::SetDepthStencilRef(ref_ptr<DepthStencil> depthStencilRef)
{
  m_depthStencilRef = depthStencilRef;
}

void Framebuffer::ApplyOwnDepthStencil()
{
  m_depthStencilRef = make_ref(m_depthStencil);
}

void Framebuffer::Enable()
{
  ASSERT(m_isSupported, ());
  GLFunctions::glBindFramebuffer(m_framebufferId);
}

void Framebuffer::Disable()
{
  ASSERT(m_isSupported, ());
  if (m_framebufferFallback != nullptr)
    m_framebufferFallback();
}

uint32_t Framebuffer::GetTextureId() const { return m_colorTextureId; }

ref_ptr<Framebuffer::DepthStencil> Framebuffer::GetDepthStencilRef() const
{
  return m_depthStencilRef;
}
}  // namespace dp
