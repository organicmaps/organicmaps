#pragma once

#include "shape.hpp"

namespace gui
{

class CopyrightLabel : public Shape
{
  using TBase = Shape;
public:
  CopyrightLabel(gui::Position const & position);
  drape_ptr<ShapeRenderer> Draw(ref_ptr<dp::TextureManager> tex) const override;
};

}
