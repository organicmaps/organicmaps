#include "drape_frontend/render_state_extension.hpp"

#include <array>

namespace
{
std::array<df::RenderStateExtension, static_cast<size_t>(df::DepthLayer::LayersCount)> kStateExtensions = {
    df::RenderStateExtension(df::DepthLayer::GeometryLayer),
    df::RenderStateExtension(df::DepthLayer::Geometry3dLayer),
    df::RenderStateExtension(df::DepthLayer::UserLineLayer),
    df::RenderStateExtension(df::DepthLayer::MwmBorderLayer),
    df::RenderStateExtension(df::DepthLayer::OverlayLayer),
    df::RenderStateExtension(df::DepthLayer::TransitSchemeLayer),
    df::RenderStateExtension(df::DepthLayer::UserMarkLayer),
    df::RenderStateExtension(df::DepthLayer::RoutingBottomMarkLayer),
    df::RenderStateExtension(df::DepthLayer::RoutingMarkLayer),
    df::RenderStateExtension(df::DepthLayer::SearchMarkLayer),
    df::RenderStateExtension(df::DepthLayer::GuiLayer)};

struct RenderStateExtensionFactory
{
  static ref_ptr<df::RenderStateExtension> Get(df::DepthLayer depthLayer)
  {
    ASSERT_LESS(static_cast<size_t>(depthLayer), kStateExtensions.size(), ());
    return make_ref(&kStateExtensions[static_cast<size_t>(depthLayer)]);
  }
};
}  // namespace

namespace df
{
RenderStateExtension::RenderStateExtension(DepthLayer depthLayer) : m_depthLayer(depthLayer) {}

bool RenderStateExtension::Less(ref_ptr<dp::BaseRenderStateExtension> other) const
{
  ASSERT(dynamic_cast<RenderStateExtension const *>(other.get()) != nullptr, ());
  auto const renderState = static_cast<RenderStateExtension const *>(other.get());
  return m_depthLayer < renderState->m_depthLayer;
}

bool RenderStateExtension::Equal(ref_ptr<dp::BaseRenderStateExtension> other) const
{
  ASSERT(dynamic_cast<RenderStateExtension const *>(other.get()) != nullptr, ());
  auto const renderState = static_cast<RenderStateExtension const *>(other.get());
  return m_depthLayer == renderState->m_depthLayer;
}

DepthLayer GetDepthLayer(dp::RenderState const & state)
{
  return state.GetRenderStateExtension<RenderStateExtension>()->GetDepthLayer();
}

dp::RenderState CreateRenderState(gpu::Program program, DepthLayer depthLayer)
{
  return dp::RenderState(program, RenderStateExtensionFactory::Get(depthLayer));
}
}  // namespace df
