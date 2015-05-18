#pragma once

#include "shape.hpp"

namespace gui
{

class Ruler : public Shape
{
public:
  Ruler(gui::Position const & position) : Shape(position) {}
  drape_ptr<ShapeRenderer> Draw(ref_ptr<dp::TextureManager> tex) const;

private:
  void DrawRuler(ShapeControl & control, ref_ptr<dp::TextureManager> tex) const;
  void DrawText(ShapeControl & control, ref_ptr<dp::TextureManager> tex) const;
};

}
