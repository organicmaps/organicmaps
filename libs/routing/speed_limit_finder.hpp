#pragma once

#include "routing/maxspeeds.hpp"

#include "routing_common/maxspeed_conversion.hpp"

#include "indexer/data_source.hpp"
#include "indexer/mwm_set.hpp"

#include "platform/local_country_file.hpp"

#include "geometry/latlon.hpp"
#include "geometry/point2d.hpp"

#include <ctime>
#include <map>
#include <memory>
#include <mutex>
#include <optional>

namespace routing
{
double constexpr kMaxFreeRideRoadDistanceMeters = 35.0;
double constexpr kFreeRideHeadingPenaltyMeters = 25.0;
double constexpr kFreeRideWrongWayPenaltyMeters = 40.0;

struct RoadMatchCandidate
{
  double m_distanceMeters = 0.0;
  double m_score = 0.0;
  bool m_isForward = true;
};

/// Projects |pt| (Mercator) onto segment [p0, p1] and scores the match using real-world distance
/// plus optional heading alignment and one-way direction consistency.
/// Returns std::nullopt when the segment is degenerate or farther than |maxDistanceMeters|.
std::optional<RoadMatchCandidate> MatchPointToSegment(m2::PointD const & pt, m2::PointD const & p0,
                                                      m2::PointD const & p1,
                                                      std::optional<m2::PointD> const & bearingDir, bool isOneWay,
                                                      double maxDistanceMeters = kMaxFreeRideRoadDistanceMeters);

/// Converts |maxspeed| to meters per second following the FollowingInfo::m_speedLimitMps contract:
/// > 0.0 for numeric limits, 0.0 for unlimited ("none"), < 0.0 when unavailable.
double SpeedLimitToMps(Maxspeed const & maxspeed, bool isForward, time_t now);

/// Thread-safe offline speed-limit lookup for free-ride mode using MWM maxspeeds sections.
class SpeedLimitFinder
{
public:
  explicit SpeedLimitFinder(DataSource const & dataSource);

  /// Returns the speed limit in m/s for the nearest drivable road within 35 m,
  /// 0.0 for unlimited ("none"), or -1.0 when unknown or off-road.
  double GetSpeedLimitMps(ms::LatLon const & pos, std::optional<double> bearingDeg = std::nullopt,
                          time_t now = 0) const;

  void OnMwmDeregistered(platform::LocalCountryFile const & localFile);
  void ClearCache();

private:
  std::shared_ptr<Maxspeeds> GetOrLoadMaxspeeds(MwmSet::MwmId const & mwmId) const;

  DataSource const & m_dataSource;

  mutable std::mutex m_cacheMutex;
  mutable std::map<MwmSet::MwmId, std::shared_ptr<Maxspeeds>> m_cache;
};
}  // namespace routing
