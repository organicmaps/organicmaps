#pragma once

#include "stdint.h"

namespace dp
{
class OGLContext;
}

namespace df
{

class Framebuffer
{
public:
  Framebuffer() = default;
  ~Framebuffer();

  void SetDefaultContext(dp::OGLContext * context);
  void SetSize(uint32_t width, uint32_t height);

  void Enable();
  void Disable();

  uint32_t GetTextureId() const;
  bool IsSupported() const { return m_isSupported; }

private:
  void Destroy();

  uint32_t m_width = 0;
  uint32_t m_height = 0;

  uint32_t m_colorTextureId = 0;
  uint32_t m_depthTextureId = 0;
  uint32_t m_framebufferId = 0;

  dp::OGLContext * m_defaultContext = 0;

  bool m_isSupported = true;
};

}  // namespace df
