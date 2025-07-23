#include "drape/framebuffer.hpp"
#include "drape/gl_functions.hpp"
#include "drape/texture.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

namespace dp
{
Framebuffer::DepthStencil::DepthStencil(bool depthEnabled, bool stencilEnabled)
  : m_depthEnabled(depthEnabled)
  , m_stencilEnabled(stencilEnabled)
{}

Framebuffer::DepthStencil::~DepthStencil()
{
  Destroy();
}

void Framebuffer::DepthStencil::SetSize(ref_ptr<dp::GraphicsContext> context, uint32_t width, uint32_t height)
{
  if (m_texture && m_texture->GetWidth() == width && m_texture->GetHeight() == height)
    return;

  Destroy();

  Texture::Params params;
  params.m_width = width;
  params.m_height = height;
  if (m_depthEnabled && m_stencilEnabled)
    params.m_format = TextureFormat::DepthStencil;
  else if (m_depthEnabled)
    params.m_format = TextureFormat::Depth;
  else
    CHECK(false, ("Unsupported depth-stencil combination."));
  params.m_allocator = GetDefaultAllocator(context);
  params.m_isRenderTarget = true;

  m_texture = make_unique_dp<FramebufferTexture>();
  m_texture->Create(context, params);
}

void Framebuffer::DepthStencil::Destroy()
{
  m_texture.reset();
}

uint32_t Framebuffer::DepthStencil::GetDepthAttachmentId() const
{
  ASSERT(m_texture != nullptr, ());
  return m_texture->GetID();
}

uint32_t Framebuffer::DepthStencil::GetStencilAttachmentId() const
{
  ASSERT(m_stencilEnabled ? m_texture != nullptr : true, ());
  return m_stencilEnabled ? m_texture->GetID() : 0;
}

ref_ptr<FramebufferTexture> Framebuffer::DepthStencil::GetTexture() const
{
  return make_ref(m_texture);
}

Framebuffer::Framebuffer() : m_colorFormat(TextureFormat::RGBA8)
{
  ApplyOwnDepthStencil();
}

Framebuffer::Framebuffer(TextureFormat colorFormat) : m_colorFormat(colorFormat)
{
  ApplyOwnDepthStencil();
}

Framebuffer::Framebuffer(TextureFormat colorFormat, bool depthEnabled, bool stencilEnabled)
  : m_depthStencil(make_unique_dp<dp::Framebuffer::DepthStencil>(depthEnabled, stencilEnabled))
  , m_colorFormat(colorFormat)
{
  ApplyOwnDepthStencil();
}

Framebuffer::~Framebuffer()
{
  Destroy();
}

void Framebuffer::Destroy()
{
  m_colorTexture.reset();

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

void Framebuffer::SetSize(ref_ptr<dp::GraphicsContext> context, uint32_t width, uint32_t height)
{
  if (!m_isSupported)
    return;

  if (m_width == width && m_height == height)
    return;

  m_width = width;
  m_height = height;

  Destroy();

  Texture::Params params;
  params.m_width = width;
  params.m_height = height;
  params.m_format = m_colorFormat;
  params.m_allocator = GetDefaultAllocator(context);
  params.m_isRenderTarget = true;

  m_colorTexture = make_unique_dp<FramebufferTexture>();
  m_colorTexture->Create(context, params);

  if (m_depthStencilRef != nullptr && m_depthStencilRef == m_depthStencil.get())
    m_depthStencilRef->SetSize(context, m_width, m_height);

  auto const apiVersion = context->GetApiVersion();
  if (apiVersion == dp::ApiVersion::OpenGLES3)
  {
    glConst depthAttachmentId = 0;
    glConst stencilAttachmentId = 0;
    if (m_depthStencilRef != nullptr)
    {
      depthAttachmentId = m_depthStencilRef->GetDepthAttachmentId();
      stencilAttachmentId = m_depthStencilRef->GetStencilAttachmentId();
    }

    GLFunctions::glGenFramebuffer(&m_framebufferId);
    GLFunctions::glBindFramebuffer(m_framebufferId);

    GLFunctions::glFramebufferTexture2D(gl_const::GLColorAttachment, m_colorTexture->GetID());
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
}

void Framebuffer::SetDepthStencilRef(ref_ptr<DepthStencil> depthStencilRef)
{
  m_depthStencilRef = std::move(depthStencilRef);
}

void Framebuffer::ApplyOwnDepthStencil()
{
  m_depthStencilRef = make_ref(m_depthStencil);
}

void Framebuffer::Bind()
{
  ASSERT(m_isSupported, ());
  ASSERT_NOT_EQUAL(m_framebufferId, 0, ());
  GLFunctions::glBindFramebuffer(m_framebufferId);
}

void Framebuffer::ApplyFallback()
{
  ASSERT(m_isSupported, ());
  if (m_framebufferFallback != nullptr)
    m_framebufferFallback();
}

ref_ptr<Texture> Framebuffer::GetTexture() const
{
  return make_ref(m_colorTexture);
}

ref_ptr<Framebuffer::DepthStencil> Framebuffer::GetDepthStencilRef() const
{
  return m_depthStencilRef;
}
}  // namespace dp
