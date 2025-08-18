#include "drape_frontend/render_state_extension.hpp"

#include <array>

namespace
{
using namespace df;

std::array<RenderStateExtension, static_cast<size_t>(DepthLayer::LayersCount)> kStateExtensions = {
    RenderStateExtension(DepthLayer::GeometryLayer),          RenderStateExtension(DepthLayer::Geometry3dLayer),
    RenderStateExtension(DepthLayer::UserLineLayer),          RenderStateExtension(DepthLayer::OverlayLayer),
    RenderStateExtension(DepthLayer::TransitSchemeLayer),     RenderStateExtension(DepthLayer::UserMarkLayer),
    RenderStateExtension(DepthLayer::RoutingBottomMarkLayer), RenderStateExtension(DepthLayer::RoutingMarkLayer),
    RenderStateExtension(DepthLayer::SearchMarkLayer),        RenderStateExtension(DepthLayer::GuiLayer)};

struct RenderStateExtensionFactory
{
  static ref_ptr<RenderStateExtension> Get(DepthLayer depthLayer)
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
