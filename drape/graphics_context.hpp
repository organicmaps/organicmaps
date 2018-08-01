#pragma once

#include "drape/drape_global.hpp"

namespace dp
{
enum ClearBits: uint32_t
{
  ColorBit = 1,
  DepthBit = 1 << 1,
  StencilBit = 1 << 2
};

class GraphicsContext
{
public:
  virtual ~GraphicsContext() = default;
  virtual void Present() = 0;
  virtual void MakeCurrent() = 0;
  virtual void DoneCurrent() {}
  virtual void SetDefaultFramebuffer() = 0;
  // w, h - pixel size of render target (logical size * visual scale).
  virtual void Resize(int /*w*/, int /*h*/) {}
  virtual void SetRenderingEnabled(bool /*enabled*/) {}
  virtual void SetPresentAvailable(bool /*available*/) {}
  virtual bool Validate() { return true; }

  virtual void Init(ApiVersion apiVersion) = 0;
  virtual void SetClearColor(dp::Color const & color) = 0;
  virtual void Clear(uint32_t clearBits) = 0;
  virtual void Flush() = 0;
};
}  // namespace dp
