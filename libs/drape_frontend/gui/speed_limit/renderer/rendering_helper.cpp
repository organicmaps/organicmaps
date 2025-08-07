#include "rendering_helper.hpp"

namespace gui::speed_limit::renderer
{
namespace
{
glsl::vec2 GetColorTexCoords(ref_ptr<dp::TextureManager> const & tex, dp::Color const color)
{
  dp::TextureManager::ColorRegion reg;
  tex->GetColorRegion(color, reg);

  return glsl::ToVec2(reg.GetTexRect().Center());
}
}  // namespace

RenderingHelper::RenderingHelper(SpeedLimit const & speedLimit) : m_speedLimit(speedLimit) {}

m2::PointF RenderingHelper::GetPosition() const
{
  return m_speedLimit.GetPosition();
}

bool RenderingHelper::IsVisible() const
{
  return m_speedLimit.IsEnabled();
}

bool RenderingHelper::IsBackgroundUpdateNeeded() const
{
  // return m_needsBackgroundUpdate;
  return true;
}

bool RenderingHelper::IsTextUpdateNeeded() const
{
  return m_needsTextUpdate;
}

void RenderingHelper::UpdateBackgroundParams(gpu::GuiProgramParams & params, ref_ptr<dp::TextureManager> tex) const
{
  auto const config = m_speedLimit.GetConfig();
  params.m_length = m_speedLimit.GetSize();
  params.m_radius = config.cornerRadius;

  params.m_colorTexCoords = GetColorTexCoords(tex, config.backgroundColor);
  params.m_outlineColorTexCoords = GetColorTexCoords(tex, config.backgroundOutlineColor);
  params.m_edgeColorTexCoords = GetColorTexCoords(tex, config.backgroundEdgeColor);

  params.m_outlineWidthRatio = config.outlineWidthRatio;
  params.m_edgeWidthRatio = config.edgeWidthRatio;

  m_needsBackgroundUpdate = false;
}
}  // namespace gui::speed_limit::renderer
