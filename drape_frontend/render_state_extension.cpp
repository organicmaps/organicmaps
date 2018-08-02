#include "drape_frontend/render_state_extension.hpp"

#include <vector>

namespace df
{
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
  static std::vector<drape_ptr<RenderStateExtension>> m_states;
  if (m_states.empty())
  {
    m_states.reserve(static_cast<size_t>(DepthLayer::LayersCount));
    for (size_t i = 0; i < static_cast<size_t>(DepthLayer::LayersCount); ++i)
      m_states.emplace_back(make_unique_dp<RenderStateExtension>(static_cast<DepthLayer>(i)));
  }
  return make_ref(m_states[static_cast<size_t>(depthLayer)]);
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
