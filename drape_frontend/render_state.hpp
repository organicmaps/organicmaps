#pragma once

#include "drape/glstate.hpp"
#include "drape/pointers.hpp"

namespace df
{
class RenderState : public dp::BaseRenderState
{
public:
  enum DepthLayer
  {
    // Do not change the order.
    GeometryLayer = 0,
    Geometry3dLayer,
    UserLineLayer,
    OverlayLayer,
    LocalAdsMarkLayer,
    UserMarkLayer,
    NavigationLayer,
    TransitMarkLayer,
    RoutingMarkLayer,
    GuiLayer,
    LayersCount
  };

  explicit RenderState(DepthLayer depthLayer);

  bool Less(ref_ptr<dp::BaseRenderState> other) const override;
  bool Equal(ref_ptr<dp::BaseRenderState> other) const override;

  DepthLayer GetDepthLayer() const { return m_depthLayer; }

private:
  DepthLayer const m_depthLayer;
};

class RenderStateFactory
{
public:
  static ref_ptr<RenderState> Get(RenderState::DepthLayer depthLayer);
};

extern RenderState::DepthLayer GetDepthLayer(dp::GLState const & state);
extern dp::GLState CreateGLState(int gpuProgramIndex, RenderState::DepthLayer depthLayer);
}  // namespace df
