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

enum class TestFunction : uint8_t
{
  Never,
  Less,
  Equal,
  LessOrEqual,
  Greater,
  NotEqual,
  GreaterOrEqual,
  Always
};

enum class StencilFace : uint8_t
{
  Front,
  Back,
  FrontAndBack
};

enum class StencilAction : uint8_t
{
  Keep,
  Zero,
  Replace,
  Increment,
  IncrementWrap,
  Decrement,
  DecrementWrap,
  Invert
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
  virtual void SetDepthTestEnabled(bool enabled) = 0;
  virtual void SetDepthTestFunction(TestFunction depthFunction) = 0;
  virtual void SetStencilTestEnabled(bool enabled) = 0;
  virtual void SetStencilFunction(StencilFace face, TestFunction stencilFunction) = 0;
  virtual void SetStencilActions(StencilFace face, StencilAction stencilFailAction, StencilAction depthFailAction,
                                 StencilAction passAction) = 0;


};
}  // namespace dp
