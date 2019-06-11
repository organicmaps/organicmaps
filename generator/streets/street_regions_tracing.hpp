#pragma once

#include "generator/key_value_storage.hpp"

#include "geometry/meter.hpp"
#include "geometry/point2d.hpp"

#include <functional>
#include <vector>

namespace generator
{
namespace streets
{
using m2::literals::operator"" _m;

class StreetRegionsTracing final
{
public:
  using Meter = m2::Meter;
  using StreetRegionInfoGetter = std::function<boost::optional<KeyValue>(m2::PointD const & pathPoint)>;
  using Path = std::vector<m2::PointD>;

  constexpr static auto const kRegionCheckStepDistance = 100.0_m;
  constexpr static auto const kRegionBoundarySearchStepDistance = 10.0_m;

  struct Segment
  {
    KeyValue m_region;
    Path m_path;
    Meter m_pathLengthMeters;
  };

  using PathSegments = std::vector<Segment>;

  StreetRegionsTracing(Path const & path, StreetRegionInfoGetter const & streetRegionInfoGetter);

  PathSegments && StealPathSegments();

private:
  void Trace();
  bool TraceToNextCheckPointInCurrentRegion();
  void TraceUpToNextRegion();
  void AdvanceTo(m2::PointD const toPoint, Meter distance, Path::const_iterator nextPathPoint);
  void AdvanceToBoundary(boost::optional<KeyValue> const & newRegion, m2::PointD const newRegionPoint,
                         Meter distance, Path::const_iterator nextPathPoint);
  void StartNewSegment();
  void CloseCurrentSegment(m2::PointD const endPoint, Meter distance, Path::const_iterator pathEndPoint);
  void ReleaseCurrentSegment();
  m2::PointD FollowToNextPoint(m2::PointD const & startPoint, Meter stepDistance,
      Path::const_iterator nextPathPoint, Meter & distance, Path::const_iterator & newNextPathPoint) const;
  bool IsSameRegion(boost::optional<KeyValue> const & region) const;

  Path const & m_path;
  StreetRegionInfoGetter const & m_streetRegionInfoGetter;
  m2::PointD m_currentPoint;
  Path::const_iterator m_nextPathPoint;
  boost::optional<KeyValue> m_currentRegion;
  PathSegments m_pathSegments;
};
}  // namespace streets
}  // namesapce generator
