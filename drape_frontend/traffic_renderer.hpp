#pragma once

#include "drape_frontend/frame_values.hpp"
#include "drape_frontend/tile_utils.hpp"
#include "drape_frontend/traffic_generator.hpp"

#include "shaders/program_manager.hpp"

#include "drape/pointers.hpp"

#include "geometry/screenbase.hpp"
#include "geometry/spline.hpp"

#include <vector>

namespace df
{
class TrafficRenderer final
{
public:
  TrafficRenderer() = default;

  void AddRenderData(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                     TrafficRenderData && renderData);

  void RenderTraffic(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ScreenBase const & screen,
                     int zoomLevel, float opacity, FrameValues const & frameValues);

  bool HasRenderData() const { return !m_renderData.empty(); }

  void ClearContextDependentResources();
  void Clear(MwmSet::MwmId const & mwmId);

  void OnUpdateViewport(CoverageResult const & coverage, int currentZoomLevel,
                        buffer_vector<TileKey, 8> const & tilesToDelete);
  void OnGeometryReady(int currentZoomLevel);

  static float GetTwoWayOffset(RoadClass const & roadClass, int zoomLevel);
  static bool CanBeRenderedAsLine(RoadClass const & roadClass, int zoomLevel, int & width);

private:
  static float GetPixelWidth(RoadClass const & roadClass, int zoomLevel);
  static float GetPixelWidthInternal(RoadClass const & roadClass, int zoomLevel);

  std::vector<TrafficRenderData> m_renderData;
};
}  // namespace df
