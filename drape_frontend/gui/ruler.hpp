#pragma once

#include "drape_frontend/gui/shape.hpp"

namespace gui
{
class Ruler : public Shape
{
public:
  explicit Ruler(gui::Position const & position)
    : Shape(position)
  {}
  drape_ptr<ShapeRenderer> Draw(m2::PointF & size, ref_ptr<dp::TextureManager> tex) const;

private:
  void DrawRuler(m2::PointF & size, ShapeControl & control, ref_ptr<dp::TextureManager> tex,
                 bool isAppearing) const;
  void DrawText(m2::PointF & size, ShapeControl & control, ref_ptr<dp::TextureManager> tex,
                bool isAppearing) const;
};
}  // namespace gui
