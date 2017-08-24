#include "drape_frontend/render_state.hpp"

#include <vector>

namespace df
{
RenderState::RenderState(DepthLayer depthLayer)
  : m_depthLayer(depthLayer)
{}

bool RenderState::Less(ref_ptr<dp::BaseRenderState> other) const
{
  ASSERT(dynamic_cast<RenderState const *>(other.get()) != nullptr, ());
  auto const renderState = static_cast<RenderState const *>(other.get());
  return m_depthLayer < renderState->m_depthLayer;
}

bool RenderState::Equal(ref_ptr<dp::BaseRenderState> other) const
{
  ASSERT(dynamic_cast<RenderState const *>(other.get()) != nullptr, ());
  auto const renderState = static_cast<RenderState const *>(other.get());
  return m_depthLayer == renderState->m_depthLayer;
}

// static
ref_ptr<RenderState> RenderStateFactory::Get(RenderState::DepthLayer depthLayer)
{
  static std::vector<drape_ptr<RenderState>> m_states;
  if (m_states.empty())
  {
    m_states.reserve(RenderState::LayersCount);
    for (size_t i = 0; i < RenderState::LayersCount; ++i)
      m_states.emplace_back(make_unique_dp<RenderState>(static_cast<RenderState::DepthLayer>(i)));
  }
  return make_ref(m_states[static_cast<size_t>(depthLayer)]);
}

RenderState::DepthLayer GetDepthLayer(dp::GLState const & state)
{
  return state.GetRenderState<RenderState>()->GetDepthLayer();
}

dp::GLState CreateGLState(int gpuProgramIndex, RenderState::DepthLayer depthLayer)
{
  return dp::GLState(gpuProgramIndex, RenderStateFactory::Get(depthLayer));
}
}  // namespace df
