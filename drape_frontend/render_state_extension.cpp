#include "drape_frontend/render_state_extension.hpp"

#include <array>

namespace df
{
std::array<drape_ptr<RenderStateExtension>, static_cast<size_t>(DepthLayer::LayersCount)> kStateExtensions = {
    make_unique_dp<RenderStateExtension>(DepthLayer::GeometryLayer),
    make_unique_dp<RenderStateExtension>(DepthLayer::Geometry3dLayer),
    make_unique_dp<RenderStateExtension>(DepthLayer::UserLineLayer),
    make_unique_dp<RenderStateExtension>(DepthLayer::OverlayLayer),
    make_unique_dp<RenderStateExtension>(DepthLayer::LocalAdsMarkLayer),
    make_unique_dp<RenderStateExtension>(DepthLayer::TransitSchemeLayer),
    make_unique_dp<RenderStateExtension>(DepthLayer::UserMarkLayer),
    make_unique_dp<RenderStateExtension>(DepthLayer::NavigationLayer),
    make_unique_dp<RenderStateExtension>(DepthLayer::RoutingBottomMarkLayer),
    make_unique_dp<RenderStateExtension>(DepthLayer::RoutingMarkLayer),
    make_unique_dp<RenderStateExtension>(DepthLayer::GuidesBottomMarkLayer),
    make_unique_dp<RenderStateExtension>(DepthLayer::GuidesMarkLayer),
    make_unique_dp<RenderStateExtension>(DepthLayer::SearchMarkLayer),
    make_unique_dp<RenderStateExtension>(DepthLayer::GuiLayer)
};

RenderStateExtension::RenderStateExtension(DepthLayer depthLayer)
  : m_depthLayer(depthLayer)
{}

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

// static
ref_ptr<RenderStateExtension> RenderStateExtensionFactory::Get(DepthLayer depthLayer)
{
  ASSERT_LESS(static_cast<size_t>(depthLayer), kStateExtensions.size(), ());
  return make_ref(kStateExtensions[static_cast<size_t>(depthLayer)]);
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
