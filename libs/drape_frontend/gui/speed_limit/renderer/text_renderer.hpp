#pragma once

#include "drape_frontend/gui/shape.hpp"
#include "drape_frontend/gui/speed_limit/renderer/rendering_helper.hpp"

namespace gui::speed_limit::renderer
{
class TextRenderer
{
public:
  explicit TextRenderer(RenderingHelperPtr helper);

  void Draw(ref_ptr<dp::GraphicsContext> context, ShapeControl & control, ref_ptr<dp::TextureManager> tex) const;

private:
  RenderingHelperPtr m_helper;
};
}  // namespace gui::speed_limit::renderer
