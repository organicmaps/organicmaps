#pragma once

#include "stdint.h"

namespace df
{

class Framebuffer
{
public:
  Framebuffer();
  ~Framebuffer();

  void SetSize(uint32_t width, uint32_t height);

  void Enable();
  void Disable();

  uint32_t GetTextureId() const;

private:
  void Destroy();

  uint32_t m_width;
  uint32_t m_height;

  uint32_t m_colorTextureId;
  uint32_t m_depthTextureId;
  uint32_t m_fbo;
};

}
