#pragma once

#include "shape.hpp"

namespace gui
{

class Compass : public Shape
{
public:
  dp::TransferPointer<ShapeRenderer> Draw(dp::RefPointer<dp::TextureManager> tex) const override;
};

}
