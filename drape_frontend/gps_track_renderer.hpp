#pragma once

#include "drape_frontend/circles_pack_shape.hpp"
#include "drape_frontend/frame_values.hpp"
#include "drape_frontend/gps_track_point.hpp"

#include "shaders/program_manager.hpp"

#include "drape/graphics_context.hpp"
#include "drape/pointers.hpp"

#include "geometry/screenbase.hpp"
#include "geometry/spline.hpp"

#include <functional>
#include <map>
#include <vector>

namespace df
{
class GpsTrackRenderer final
{
public:
  using TRenderDataRequestFn = std::function<void(uint32_t)>;
  explicit GpsTrackRenderer(TRenderDataRequestFn const & dataRequestFn);

  void AddRenderData(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                     drape_ptr<CirclesPackRenderData> && renderData);

  void UpdatePoints(std::vector<GpsTrackPoint> const & toAdd, std::vector<uint32_t> const & toRemove);

  void RenderTrack(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ScreenBase const & screen,
                   int zoomLevel, FrameValues const & frameValues);

  void Update();
  void Clear();
  void ClearRenderData();

private:
  size_t GetAvailablePointsCount() const;
  dp::Color CalculatePointColor(size_t pointIndex, m2::PointD const & curPoint, double lengthFromStart,
                                double fullLength) const;
  dp::Color GetColorBySpeed(double speed) const;

  TRenderDataRequestFn m_dataRequestFn;
  std::vector<drape_ptr<CirclesPackRenderData>> m_renderData;
  std::vector<GpsTrackPoint> m_points;
  m2::Spline m_pointsSpline;
  bool m_needUpdate;
  bool m_waitForRenderData;
  std::vector<std::pair<CirclesPackHandle *, size_t>> m_handlesCache;
  float m_radius;
  m2::PointD m_pivot;
};
}  // namespace df
