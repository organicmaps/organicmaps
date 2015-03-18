#pragma once

#include "shape.hpp"

namespace gui
{

class Ruler : public Shape
{
public:
  dp::TransferPointer<ShapeRenderer> Draw(dp::RefPointer<dp::TextureManager> tex) const override;

private:
  void DrawRuler(dp::RefPointer<ShapeRenderer> renderer, dp::RefPointer<dp::TextureManager> tex) const;
  void DrawText(dp::RefPointer<ShapeRenderer> renderer, dp::RefPointer<dp::TextureManager> tex) const;
};

}
