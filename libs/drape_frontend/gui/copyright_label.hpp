#pragma once

#include "drape_frontend/gui/shape.hpp"

namespace gui
{
class CopyrightLabel : public Shape
{
  using TBase = Shape;

public:
  explicit CopyrightLabel(Position const & position);
  drape_ptr<ShapeRenderer> Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> tex) const;
};
}  // namespace gui
