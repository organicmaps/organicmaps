#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <optional>

namespace location
{
class GpsInfo;
}

namespace routing
{
class FollowedPolyline;
struct RoutingSettings;

// Matches ordered source progress and confirms downstream rejoins from raw fixes.
// Track geometry starts with an approach segment, including an empty approach.
class TrackMatcher
{
public:
  bool Update(FollowedPolyline & poly, location::GpsInfo const & info, RoutingSettings const & settings,
              double entryDistanceM);

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

  bool TryRejoin(FollowedPolyline & poly, location::GpsInfo const & info, m2::RectD const & rect,
                 double entryDistanceM);

  bool m_joined = false;
  std::optional<Fix> m_lastFix;
  std::optional<Rejoin> m_rejoin;
};
}  // namespace routing
