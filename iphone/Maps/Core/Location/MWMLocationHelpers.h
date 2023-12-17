#import "MWMMyPositionMode.h"

#include "platform/localization.hpp"
#include "platform/location.hpp"
#include "platform/distance.hpp"

#include "geometry/mercator.hpp"

namespace location_helpers
{

static inline NSString * formattedDistance(double const & meters) {
  if (meters < 0.)
    return nil;

  return @(platform::Distance::CreateFormatted(meters).ToString().c_str());
}

static inline ms::LatLon ToLatLon(m2::PointD const & p) { return mercator::ToLatLon(p); }

static inline m2::PointD ToMercator(CLLocationCoordinate2D const & l)
{
  return mercator::FromLatLon(l.latitude, l.longitude);
}

static inline m2::PointD ToMercator(ms::LatLon const & l) { return mercator::FromLatLon(l); }
static inline MWMMyPositionMode mwmMyPositionMode(location::EMyPositionMode mode)
{
  switch (mode)
  {
  case location::EMyPositionMode::PendingPosition: return MWMMyPositionModePendingPosition;
  case location::EMyPositionMode::NotFollowNoPosition: return MWMMyPositionModeNotFollowNoPosition;
  case location::EMyPositionMode::NotFollow: return MWMMyPositionModeNotFollow;
  case location::EMyPositionMode::Follow: return MWMMyPositionModeFollow;
  case location::EMyPositionMode::FollowAndRotate: return MWMMyPositionModeFollowAndRotate;
  }
}
} // namespace location_helpers
