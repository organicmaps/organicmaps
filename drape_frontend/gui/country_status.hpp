#pragma once

#include "shape.hpp"
#include "country_status_helper.hpp"

namespace gui
{
class CountryStatus : public Shape
{
public:
  CountryStatus(Position const & position)
    : Shape(position)
  {}

  using TButtonHandlers = map<CountryStatusHelper::EButtonType, TTapHandler>;

  drape_ptr<ShapeRenderer> Draw(ref_ptr<dp::TextureManager> tex,
                                TButtonHandlers const & buttonHandlers) const;
};

}  // namespace gui
