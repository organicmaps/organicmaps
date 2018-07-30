#pragma once

#include "drape/graphic_context.hpp"

namespace dp
{
class OGLContext: public GraphicContext
{
public:
  void SetApiVersion(ApiVersion apiVersion) override;
  void Init() override;
  void SetClearColor(float r, float g, float b, float a) override;
  void Clear(ContextConst clearBits) override;
};
}  // namespace dp
