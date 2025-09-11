#include "speed_limit_renderer.hpp"

#include <functional>
#include <utility>

using namespace std::placeholders;

namespace gui::speed_limit::renderer
{
SpeedLimitRenderer::SpeedLimitRenderer(Position const & position)
  : Shape(position)
  , m_helper(std::make_shared<RenderingHelper>(DrapeGui::GetSpeedLimit()))
  , m_background(m_helper)
  , m_text(m_helper)
{
}

drape_ptr<ShapeRenderer> SpeedLimitRenderer::Draw(ref_ptr<dp::GraphicsContext> context,
                                                  ref_ptr<dp::TextureManager> tex) const
{
  ShapeControl control;
  m_background.Draw(context, control, tex);
  m_text.Draw(context, control, tex);

  drape_ptr<ShapeRenderer> renderer = make_unique_dp<ShapeRenderer>();
  renderer->AddShapeControl(std::move(control));
  return renderer;
}
}  // namespace gui::speed_limit::renderer
