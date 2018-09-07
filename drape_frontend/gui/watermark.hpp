#pragma once

#include "drape_frontend/gui/shape.hpp"

namespace gui
{
class Watermark : public Shape
{
public:
  explicit Watermark(gui::Position const & position) : Shape(position) {}

  drape_ptr<ShapeRenderer> Draw(ref_ptr<dp::GraphicsContext> context, m2::PointF & size,
                                ref_ptr<dp::TextureManager> tex) const;
};
}  // namespace gui
