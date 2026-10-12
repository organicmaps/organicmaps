#include "map/nogps/geo.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <cmath>

namespace nogps
{
ms::LatLon Move(ms::LatLon const & from, double bearingDeg, double distanceM)
{
  double const bearing = math::DegToRad(bearingDeg);
  double const lat = from.m_lat + math::RadToDeg(distanceM * std::cos(bearing) / kEarthRadiusM);
  double const lon =
      from.m_lon + math::RadToDeg(distanceM * std::sin(bearing) / (kEarthRadiusM * std::cos(math::DegToRad(lat))));
  return {lat, lon};
}

double Distance(ms::LatLon const & a, ms::LatLon const & b)
{
  double const dLat = math::DegToRad(b.m_lat - a.m_lat);
  double const dLon = math::DegToRad(b.m_lon - a.m_lon);
  double const sinLat = std::sin(dLat / 2);
  double const sinLon = std::sin(dLon / 2);
  double const h =
      sinLat * sinLat + std::cos(math::DegToRad(a.m_lat)) * std::cos(math::DegToRad(b.m_lat)) * sinLon * sinLon;
  return 2 * kEarthRadiusM * std::asin(std::min(1.0, std::sqrt(h)));
}

double Bearing(ms::LatLon const & from, ms::LatLon const & to)
{
  double const lat1 = math::DegToRad(from.m_lat);
  double const lat2 = math::DegToRad(to.m_lat);
  double const dLon = math::DegToRad(to.m_lon - from.m_lon);
  double const y = std::sin(dLon) * std::cos(lat2);
  double const x = std::cos(lat1) * std::sin(lat2) - std::sin(lat1) * std::cos(lat2) * std::cos(dLon);
  return Normalize(math::RadToDeg(std::atan2(y, x)));
}

double DistanceToPiece(ms::LatLon const & point, ms::LatLon const & from, ms::LatLon const & to)
{
  double constexpr kMetersPerDegLat = 111'320;
  double const metersPerDegLon = kMetersPerDegLat * std::cos(math::DegToRad(from.m_lat));
  double const px = (point.m_lon - from.m_lon) * metersPerDegLon;
  double const py = (point.m_lat - from.m_lat) * kMetersPerDegLat;
  double const dx = (to.m_lon - from.m_lon) * metersPerDegLon;
  double const dy = (to.m_lat - from.m_lat) * kMetersPerDegLat;
  double const lengthSq = dx * dx + dy * dy;
  double const t = lengthSq == 0 ? 0 : std::clamp((px * dx + py * dy) / lengthSq, 0.0, 1.0);
  return std::hypot(px - t * dx, py - t * dy);
}

double Normalize(double deg)
{
  double const result = std::fmod(deg, 360.0);
  return result < 0 ? result + 360 : result;
}

double AngleDiff(double to, double from)
{
  return Normalize(to - from + 180) - 180;
}

double Orient(double roadBearingDeg, std::optional<double> headingDeg)
{
  if (!headingDeg || std::fabs(AngleDiff(roadBearingDeg, *headingDeg)) <= 90)
    return Normalize(roadBearingDeg);
  return Normalize(roadBearingDeg + 180);
}
}  // namespace nogps
