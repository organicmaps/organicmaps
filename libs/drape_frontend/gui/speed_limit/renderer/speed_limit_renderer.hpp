#pragma once

#include "background_renderer.hpp"
#include "drape_frontend/gui/shape.hpp"

namespace gui::speed_limit::renderer
{
class SpeedLimitRenderer : public Shape
{
public:
  explicit SpeedLimitRenderer(gui::Position const & position);

  drape_ptr<ShapeRenderer> Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> tex) const;

private:
  void DrawText(ref_ptr<dp::GraphicsContext> context, ShapeControl & control, ref_ptr<dp::TextureManager> tex) const;

  BackgroundRenderer m_background;
};
}  // namespace gui::speed_limit::renderer
