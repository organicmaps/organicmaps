#pragma once

#include "shape.hpp"

namespace gui
{

class CopyrightLabel : public Shape
{
  using TBase = Shape;
public:
  CopyrightLabel(gui::Position const & position);
  dp::TransferPointer<ShapeRenderer> Draw(dp::RefPointer<dp::TextureManager> tex) const override;
};

}
