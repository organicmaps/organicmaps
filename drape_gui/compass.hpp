#pragma once

#include "shape.hpp"

namespace gui
{

class Compass : public Shape
{
public:
  virtual void Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureManager> tex) const;
  virtual uint16_t GetVertexCount() const;
  virtual uint16_t GetIndexCount() const;
};

}
