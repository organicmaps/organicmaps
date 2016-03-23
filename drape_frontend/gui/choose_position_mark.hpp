#pragma once

#include "shape.hpp"

namespace gui
{

class ChoosePositionMark : public Shape
{
public:
  ChoosePositionMark(gui::Position const & position)
    : Shape(position)
  {}

  drape_ptr<ShapeRenderer> Draw(ref_ptr<dp::TextureManager> tex) const;
};

} // namespace gui
