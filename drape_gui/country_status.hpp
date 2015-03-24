#pragma once

#include "shape.hpp"

namespace gui
{
class CountryStatus : public Shape
{
public:
  CountryStatus(Position position) : Shape(position) {}
  dp::TransferPointer<ShapeRenderer> Draw(dp::RefPointer<dp::TextureManager> tex) const override;
};

}  // namespace gui
