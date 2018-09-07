#pragma once

#include "drape_frontend/gui/shape.hpp"

namespace gui
{
class CopyrightLabel : public Shape
{
  using TBase = Shape;

public:
  explicit CopyrightLabel(gui::Position const & position);
  drape_ptr<ShapeRenderer> Draw(ref_ptr<dp::GraphicsContext> context, m2::PointF & size,
                                ref_ptr<dp::TextureManager> tex) const;
};
}  // namespace gui
