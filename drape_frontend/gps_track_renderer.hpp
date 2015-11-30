#pragma once

#include "drape_frontend/gps_track_point.hpp"
#include "drape_frontend/gps_track_shape.hpp"

#include "drape/gpu_program_manager.hpp"
#include "drape/pointers.hpp"
#include "drape/uniform_values_storage.hpp"

#include "map/gps_track_container.hpp"

#include "geometry/screenbase.hpp"
#include "geometry/spline.hpp"

#include "std/map.hpp"
#include "std/vector.hpp"

namespace df
{

class GpsTrackRenderer final
{
public:
  using TRenderDataRequestFn = function<void(size_t)>;
  explicit GpsTrackRenderer(TRenderDataRequestFn const & dataRequestFn);

  void AddRenderData(ref_ptr<dp::GpuProgramManager> mng,
                     drape_ptr<GpsTrackRenderData> && renderData);

  void UpdatePoints(vector<GpsTrackPoint> const & toAdd,
                    vector<uint32_t> const & toRemove);

  void RenderTrack(ScreenBase const & screen, int zoomLevel,
                   ref_ptr<dp::GpuProgramManager> mng,
                   dp::UniformValuesStorage const & commonUniforms);

  void Update();
  void Clear();

private:
  float CalculateRadius(ScreenBase const & screen) const;
  double CalculateTrackLength() const;
  void UpdateSpeedsAndColors();
  size_t GetAvailablePointsCount() const;
  double PlacePoints(size_t & cacheIndex,
                     m2::PointD const & start, m2::PointD const & end,
                     double startSpeed, double endSpeed,
                     float radius, double diameterMercator,
                     double offset, double trackLengthMercator,
                     double lengthFromStart, bool & gap);

  TRenderDataRequestFn m_dataRequestFn;
  vector<drape_ptr<GpsTrackRenderData>> m_renderData;
  vector<GpsTrackPoint> m_points;
  bool m_needUpdate;
  bool m_waitForRenderData;
  vector<pair<GpsTrackHandle*, size_t>> m_handlesCache;

  double m_startSpeed;
  double m_endSpeed;
  dp::Color m_startColor;
  float m_radius;
};

} // namespace df
