#pragma once

#include "shape.hpp"

namespace gui
{

class CopyrightLabel : public Shape
{
  using TBase = Shape;
public:
  CopyrightLabel(gui::Position const & position);
  drape_ptr<ShapeRenderer> Draw(m2::PointF & size, ref_ptr<dp::TextureManager> tex) const;
};

}
