#pragma once

#include "drape/drape_global.hpp"

namespace dp
{
using ContextConst = uint32_t;

enum ClearBits: ContextConst
{
  ColorBit = 1,
  DepthBit = 1 << 1,
  StencilBit = 1 << 2
};

class GraphicContext
{
public:
  virtual ~GraphicContext() {}
  virtual void Present() = 0;
  virtual void MakeCurrent() = 0;
  virtual void DoneCurrent() {}
  virtual void SetDefaultFramebuffer() = 0;
  // w, h - pixel size of render target (logical size * visual scale).
  virtual void Resize(int /*w*/, int /*h*/) {}
  virtual void SetRenderingEnabled(bool /*enabled*/) {}
  virtual void SetPresentAvailable(bool /*available*/) {}
  virtual bool Validate() { return true; }

  virtual void SetApiVersion(ApiVersion apiVersion) = 0;
  virtual void Init() = 0;
  virtual void SetClearColor(float r, float g, float b, float a) = 0;
  virtual void Clear(ContextConst clearBits) = 0;
};
}  // namespace dp
