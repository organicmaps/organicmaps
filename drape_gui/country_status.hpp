#pragma once

#include "shape.hpp"

namespace gui
{
class CountryStatus : public Shape
{
public:
  CountryStatus(Position const & position)
    : Shape(position)
  {}

  drape_ptr<ShapeRenderer> Draw(ref_ptr<dp::TextureManager> tex) const override;
};

}  // namespace gui
