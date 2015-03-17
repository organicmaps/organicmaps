#pragma once

#include "shape.hpp"

namespace gui
{

class Ruler : public Shape
{
public:
  virtual dp::TransferPointer<ShapeRenderer> Draw(dp::RefPointer<dp::TextureManager> tex) const override;

private:
  void DrawRuler(ShapeRenderer * renderer, dp::RefPointer<dp::TextureManager> tex) const;
  void DrawText(ShapeRenderer * renderer, dp::RefPointer<dp::TextureManager> tex) const;
};

}
