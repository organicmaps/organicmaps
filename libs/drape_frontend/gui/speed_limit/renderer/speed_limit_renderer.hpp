#pragma once

#include "drape_frontend/gui/shape.hpp"
#include "drape_frontend/gui/speed_limit/renderer/background_renderer.hpp"
#include "drape_frontend/gui/speed_limit/renderer/rendering_helper.hpp"
#include "drape_frontend/gui/speed_limit/renderer/text_renderer.hpp"

namespace gui::speed_limit::renderer
{
class SpeedLimitRenderer : public Shape
{
public:
  explicit SpeedLimitRenderer(Position const & position);

  drape_ptr<ShapeRenderer> Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> tex) const;

private:
  RenderingHelperPtr m_helper;
  BackgroundRenderer m_background;
  TextRenderer m_text;
};
}  // namespace gui::speed_limit::renderer
