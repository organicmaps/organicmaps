#pragma once

#include "drape/graphics_context.hpp"

namespace dp
{
class OGLContext: public GraphicsContext
{
public:
  void Init(ApiVersion apiVersion) override;
  void SetClearColor(float r, float g, float b, float a) override;
  void Clear(uint32_t clearBits) override;
  void Flush() override;
};
}  // namespace dp
