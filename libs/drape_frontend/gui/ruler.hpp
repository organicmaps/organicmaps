#pragma once

#include "drape_frontend/gui/shape.hpp"

namespace gui
{
class Ruler : public Shape
{
public:
  explicit Ruler(Position const & position)
    : Shape(position)
  {}
  drape_ptr<ShapeRenderer> Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> tex) const;

private:
  void DrawRuler(ref_ptr<dp::GraphicsContext> context, ShapeControl & control,
                 ref_ptr<dp::TextureManager> tex, bool isAppearing) const;
  void DrawText(ref_ptr<dp::GraphicsContext> context, ShapeControl & control,
                ref_ptr<dp::TextureManager> tex, bool isAppearing) const;
};
}  // namespace gui
