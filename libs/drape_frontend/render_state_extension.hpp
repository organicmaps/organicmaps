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
  TransitSchemeLayer,
  UserMarkLayer,
  RoutingBottomMarkLayer,
  RoutingMarkLayer,
  SearchMarkLayer,
  GuiLayer,
  LayersCount
};

/// @todo Replace with simple struct without polymorphism.
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

extern DepthLayer GetDepthLayer(dp::RenderState const & state);
extern dp::RenderState CreateRenderState(gpu::Program program, DepthLayer depthLayer);

inline std::string DebugPrint(DepthLayer layer)
{
  switch (layer)
  {
  case DepthLayer::GeometryLayer: return "GeometryLayer";
  case DepthLayer::Geometry3dLayer: return "Geometry3dLayer";
  case DepthLayer::UserLineLayer: return "UserLineLayer";
  case DepthLayer::OverlayLayer: return "OverlayLayer";
  case DepthLayer::TransitSchemeLayer: return "TransitSchemeLayer";
  case DepthLayer::UserMarkLayer: return "UserMarkLayer";
  case DepthLayer::RoutingBottomMarkLayer: return "RoutingBottomMarkLayer";
  case DepthLayer::RoutingMarkLayer: return "RoutingMarkLayer";
  case DepthLayer::SearchMarkLayer: return "SearchMarkLayer";
  case DepthLayer::GuiLayer: return "GuiLayer";
  case DepthLayer::LayersCount: CHECK(false, ("Try to output LayersCount"));
  }
  CHECK(false, ("Unknown layer"));
  return {};
}
}  // namespace df
