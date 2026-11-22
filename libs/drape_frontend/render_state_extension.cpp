#include "drape_frontend/render_state_extension.hpp"

#include <array>
#include <tuple>

namespace
{
using namespace df;

// Indexed by DepthLayer; keep the entries in enum order.
std::array<RenderStateExtension, static_cast<size_t>(DepthLayer::LayersCount)> kStateExtensions = {
    RenderStateExtension(DepthLayer::GeometryLayer),    RenderStateExtension(DepthLayer::Geometry3dLayer),
    RenderStateExtension(DepthLayer::UserLineLayer),    RenderStateExtension(DepthLayer::MwmBorderLayer),
    RenderStateExtension(DepthLayer::OverlayLayer),     RenderStateExtension(DepthLayer::TransitSchemeLayer),
    RenderStateExtension(DepthLayer::UserMarkLayer),    RenderStateExtension(DepthLayer::RoutingBottomMarkLayer),
    RenderStateExtension(DepthLayer::RoutingMarkLayer), RenderStateExtension(DepthLayer::SearchMarkLayer),
    RenderStateExtension(DepthLayer::GuiLayer),         RenderStateExtension(DepthLayer::SelectedUserLineLayer),
    RenderStateExtension(DepthLayer::UserAreaLayer)};

RenderStateExtension kAnimatedBookmarkState(DepthLayer::UserMarkLayer, true /* animateBookmark */);

}  // namespace

namespace df
{
RenderStateExtension::RenderStateExtension(DepthLayer depthLayer, bool animateBookmark)
  : m_depthLayer(depthLayer)
  , m_animateBookmark(animateBookmark)
{}

bool RenderStateExtension::Less(ref_ptr<dp::BaseRenderStateExtension> other) const
{
  auto const rhs = ref_ptr<RenderStateExtension>(other);
  return std::tie(m_depthLayer, m_animateBookmark) < std::tie(rhs->m_depthLayer, rhs->m_animateBookmark);
}

bool RenderStateExtension::Equal(ref_ptr<dp::BaseRenderStateExtension> other) const
{
  auto const rhs = ref_ptr<RenderStateExtension>(other);
  return m_depthLayer == rhs->m_depthLayer && m_animateBookmark == rhs->m_animateBookmark;
}

DepthLayer GetDepthLayer(dp::RenderState const & state)
{
  return state.GetRenderStateExtension<RenderStateExtension>()->GetDepthLayer();
}

dp::RenderState CreateRenderState(gpu::Program program, DepthLayer depthLayer, bool animateBookmark)
{
  if (animateBookmark)
  {
    ASSERT_EQUAL(depthLayer, kAnimatedBookmarkState.GetDepthLayer(), ());
    return dp::RenderState(program, make_ref(&kAnimatedBookmarkState));
  }
  auto const index = static_cast<size_t>(depthLayer);
  ASSERT_LESS(index, kStateExtensions.size(), ());
  return dp::RenderState(program, make_ref(&kStateExtensions[index]));
}
}  // namespace df
