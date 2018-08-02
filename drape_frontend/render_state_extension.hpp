#pragma once

#include "shaders/programs.hpp"

#include "drape/pointers.hpp"
#include "drape/render_state.hpp"

#include <cstdint>

namespace df
{
enum class DepthLayer : uint8_t
{
  // Do not change the order.
  GeometryLayer = 0,
  Geometry3dLayer,
  UserLineLayer,
  OverlayLayer,
  LocalAdsMarkLayer,
  TransitSchemeLayer,
  UserMarkLayer,
  NavigationLayer,
  TransitMarkLayer,
  RoutingMarkLayer,
  SearchMarkLayer,
  GuiLayer,
  LayersCount
};

class RenderStateExtension : public dp::BaseRenderStateExtension
{
public:
  explicit RenderStateExtension(DepthLayer depthLayer);

  bool Less(ref_ptr<dp::BaseRenderStateExtension> other) const override;
  bool Equal(ref_ptr<dp::BaseRenderStateExtension> other) const override;

  DepthLayer GetDepthLayer() const { return m_depthLayer; }

private:
  DepthLayer const m_depthLayer;
};

class RenderStateExtensionFactory
{
public:
  static ref_ptr<RenderStateExtension> Get(DepthLayer depthLayer);
};

extern DepthLayer GetDepthLayer(dp::RenderState const & state);
extern dp::RenderState CreateRenderState(gpu::Program program, DepthLayer depthLayer);
}  // namespace df
