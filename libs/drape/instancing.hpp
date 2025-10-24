#pragma once

#include "drape/graphics_context.hpp"
#include "drape/pointers.hpp"

#include <cstdint>
#include <memory>

namespace dp
{
class InstancingImpl
{
public:
  InstancingImpl() = default;
  virtual ~InstancingImpl() = default;

  virtual void DrawInstancedTriangleStrip(ref_ptr<dp::GraphicsContext> context, uint32_t instanceCount,
                                          uint32_t verticesCount) = 0;
};

class Instancing
{
public:
  void DrawInstancedTriangleStrip(ref_ptr<dp::GraphicsContext> context, uint32_t instanceCount, uint32_t verticesCount);

private:
  std::unique_ptr<InstancingImpl> m_impl;
};
}  // namespace dp
