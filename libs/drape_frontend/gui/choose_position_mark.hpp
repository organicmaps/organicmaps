#pragma once

#include "drape_frontend/gui/shape.hpp"

namespace gui
{
class ChoosePositionMark : public Shape
{
public:
  explicit ChoosePositionMark(gui::Position const & position) : Shape(position) {}

  drape_ptr<ShapeRenderer> Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> tex) const;
};
}  // namespace gui
