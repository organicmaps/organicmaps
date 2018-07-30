#pragma once

#include "drape/pointers.hpp"

#include <cstdint>
#include <functional>

namespace dp
{
using FramebufferFallback = std::function<bool()>;

class Framebuffer
{
public:
  class DepthStencil
  {
  public:
    DepthStencil(bool depthEnabled, bool stencilEnabled);
    ~DepthStencil();
    void SetSize(uint32_t width, uint32_t height);
    void Destroy();
    uint32_t GetDepthAttachmentId() const;
    uint32_t GetStencilAttachmentId() const;
  private:
    bool const m_depthEnabled = false;
    bool const m_stencilEnabled = false;
    uint32_t m_layout = 0;
    uint32_t m_pixelType = 0;
    uint32_t m_textureId = 0;
  };

  Framebuffer();
  explicit Framebuffer(uint32_t colorFormat);
  Framebuffer(uint32_t colorFormat, bool depthEnabled, bool stencilEnabled);
  ~Framebuffer();

  void SetFramebufferFallback(FramebufferFallback && fallback);
  void SetSize(uint32_t width, uint32_t height);
  void SetDepthStencilRef(ref_ptr<DepthStencil> depthStencilRef);
  void ApplyOwnDepthStencil();

  void Enable();
  void Disable();

  uint32_t GetTextureId() const;
  ref_ptr<DepthStencil> GetDepthStencilRef() const;

  bool IsSupported() const { return m_isSupported; }
private:
  void Destroy();

  drape_ptr<DepthStencil> m_depthStencil;
  ref_ptr<DepthStencil> m_depthStencilRef;
  uint32_t m_width = 0;
  uint32_t m_height = 0;
  uint32_t m_colorTextureId = 0;
  uint32_t m_framebufferId = 0;
  uint32_t m_colorFormat;
  FramebufferFallback m_framebufferFallback;
  bool m_isSupported = true;
};
}  // namespace dp
