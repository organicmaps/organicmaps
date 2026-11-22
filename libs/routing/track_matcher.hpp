#pragma once

#include "routing/base/followed_polyline.hpp"

#include "geometry/point2d.hpp"

#include <optional>

namespace location
{
class GpsInfo;
}

namespace routing
{
struct RoutingSettings;

// Matches ordered source progress and confirms downstream rejoins from raw fixes.
// Track geometry starts with an approach segment, including an empty approach.
class TrackMatcher
{
public:
  bool Update(FollowedPolyline & poly, location::GpsInfo const & info, RoutingSettings const & settings,
              double entryDistanceM);

  // Render a predicted position within the measured matching window. Rejoins belong to Update().
  FollowedPolyline::Iter GetLocalProjection(FollowedPolyline const & poly, location::GpsInfo const & info,
                                            RoutingSettings const & settings, double entryDistanceM) const;

private:
  struct Fix
  {
    m2::PointD m_point;
    double m_timestamp = 0.0;
  };
  struct Rejoin
  {
    m2::PointD m_origin;
    Fix m_lastFix;
    double m_progressM = 0.0;
    unsigned m_matches = 0;
  };

  bool TryRejoin(FollowedPolyline & poly, location::GpsInfo const & info, double matchingThresholdM,
                 double entryDistanceM);

  bool m_joined = false;
  std::optional<Fix> m_lastFix;
  std::optional<Rejoin> m_rejoin;
};
}  // namespace routing
