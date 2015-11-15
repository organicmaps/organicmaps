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

  int32_t GetMaxSize();

  void Enable();
  void Disable();

  uint32_t GetTextureId() const;

private:
  void Destroy();

  uint32_t m_width = 0;
  uint32_t m_height = 0;

  uint32_t m_colorTextureId = 0;
  uint32_t m_depthTextureId = 0;
  uint32_t m_framebufferId = 0;

  dp::OGLContext * m_defaultContext = 0;

  int32_t m_maxTextureSize = 0;
};

}  // namespace df
