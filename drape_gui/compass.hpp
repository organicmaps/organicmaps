#pragma once

#include "shape.hpp"

namespace gui
{

class Compass : public Shape
{
public:
  Compass(gui::Position const & position)
    : Shape(position)
  {}

  drape_ptr<ShapeRenderer> Draw(ref_ptr<dp::TextureManager> tex, TTapHandler const & tapHandler) const;
};

}
