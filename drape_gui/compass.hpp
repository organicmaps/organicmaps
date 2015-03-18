#pragma once

#include "shape.hpp"

namespace gui
{

class Compass : public Shape
{
public:
  Compass(gui::Position const & position) : Shape(position) {}
  dp::TransferPointer<ShapeRenderer> Draw(dp::RefPointer<dp::TextureManager> tex) const override;
};

}
