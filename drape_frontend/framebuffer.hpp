#pragma once

#include "drape/oglcontext.hpp"

#include "stdint.h"

namespace df
{

class Framebuffer
{
public:
  Framebuffer();
  ~Framebuffer();

  void SetDefaultContext(dp::OGLContext * context);
  void SetSize(uint32_t width, uint32_t height);

  int32_t GetMaxSize() const;

  void Enable();
  void Disable();

  uint32_t GetTextureId() const;

private:
  void Destroy();

  uint32_t m_width;
  uint32_t m_height;

  uint32_t m_colorTextureId;
  uint32_t m_depthTextureId;
  uint32_t m_framebufferId;

  const int32_t m_maxTextureSize;

  dp::OGLContext * m_defaultContext;
};

}
