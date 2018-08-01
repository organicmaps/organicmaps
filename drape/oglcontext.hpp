#pragma once

#include "drape/graphics_context.hpp"

namespace dp
{
class OGLContext: public GraphicsContext
{
public:
  void Init(ApiVersion apiVersion) override;
  void SetClearColor(dp::Color const & color) override;
  void Clear(uint32_t clearBits) override;
  void Flush() override;
};
}  // namespace dp
