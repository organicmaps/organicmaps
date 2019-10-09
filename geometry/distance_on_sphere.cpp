#include "geometry/distance_on_sphere.hpp"

#include "geometry/point3d.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"

#include <algorithm>
#include <cmath>

using namespace std;

namespace
{
double const kEarthRadiusMetersSquared = ms::kEarthRadiusMeters * ms::kEarthRadiusMeters;
}  // namespace

namespace ms
{
double DistanceOnSphere(double lat1Deg, double lon1Deg, double lat2Deg, double lon2Deg)
{
  double const lat1 = base::DegToRad(lat1Deg);
  double const lat2 = base::DegToRad(lat2Deg);
  double const dlat = sin((lat2 - lat1) * 0.5);
  double const dlon = sin((base::DegToRad(lon2Deg) - base::DegToRad(lon1Deg)) * 0.5);
  double const y = dlat * dlat + dlon * dlon * cos(lat1) * cos(lat2);
  return 2.0 * atan2(sqrt(y), sqrt(max(0.0, 1.0 - y)));
}

double DistanceOnEarth(double lat1Deg, double lon1Deg, double lat2Deg, double lon2Deg)
{
  return kEarthRadiusMeters * DistanceOnSphere(lat1Deg, lon1Deg, lat2Deg, lon2Deg);
}

double DistanceOnEarth(LatLon const & ll1, LatLon const & ll2)
{
  return DistanceOnEarth(ll1.m_lat, ll1.m_lon, ll2.m_lat, ll2.m_lon);
}

m3::PointD GetPointOnSphere(ms::LatLon const & ll, double sphereRadius)
{
  ASSERT(LatLon::kMinLat <= ll.m_lat && ll.m_lat <= LatLon::kMaxLat, (ll));
  ASSERT(LatLon::kMinLon <= ll.m_lon && ll.m_lon <= LatLon::kMaxLon, (ll));

  double const latRad = base::DegToRad(ll.m_lat);
  double const lonRad = base::DegToRad(ll.m_lon);

  double const x = sphereRadius * cos(latRad) * cos(lonRad);
  double const y = sphereRadius * cos(latRad) * sin(lonRad);
  double const z = sphereRadius * sin(latRad);

  return {x, y, z};
}

// Look to https://en.wikipedia.org/wiki/Solid_angle for more details.
// Shortly:
// It's possible to calculate area of triangle on sphere with it's solid angle.
// Ω = A / R^2
// Where Ω is solid angle of triangle, R - sphere radius and A - area of triangle on sphere.
// So A = Ω * R^2
double AreaOnEarth(LatLon const & ll1, LatLon const & ll2, LatLon const & ll3)
{
  m3::PointD const a = GetPointOnSphere(ll1, 1.0 /* sphereRadius */);
  m3::PointD const b = GetPointOnSphere(ll2, 1.0 /* sphereRadius */);
  m3::PointD const c = GetPointOnSphere(ll3, 1.0 /* sphereRadius */);

  double const triple = m3::DotProduct(a, m3::CrossProduct(b, c));

  ASSERT(base::AlmostEqualAbs(a.Length(), 1.0, 1e-5), ());
  ASSERT(base::AlmostEqualAbs(b.Length(), 1.0, 1e-5), ());
  ASSERT(base::AlmostEqualAbs(c.Length(), 1.0, 1e-5), ());

  double constexpr lengthMultiplication = 1.0;  // a.Length() * b.Length() * c.Length()
  double const abc = m3::DotProduct(a, b);  // * c.Length() == 1
  double const acb = m3::DotProduct(a, c);  // * b.Length() == 1
  double const bca = m3::DotProduct(b, c);  // * a.Length() == 1

  double const tanFromHalfSolidAngle = triple / (lengthMultiplication + abc + acb + bca);
  double const halfSolidAngle = atan(tanFromHalfSolidAngle);
  double const solidAngle = halfSolidAngle * 2.0;
  double const area = solidAngle * kEarthRadiusMetersSquared;
  return abs(area);
}

// Look to https://en.wikipedia.org/wiki/Solid_angle for details.
// Shortly:
// Ω = A / R^2
// A is the spherical surface area which confined by solid angle.
// R - sphere radius.
// For circle: Ω = 2π(1 - cos(θ / 2)), where θ - is the cone apex angle.
double CircleAreaOnEarth(double radius)
{
  radius = base::Clamp(radius, 0.0, math::pi * ms::kEarthRadiusMeters);
  double const theta = 2.0 * radius / ms::kEarthRadiusMeters;
  static double const kConst = 2.0 * math::pi * kEarthRadiusMetersSquared;
  double const sinValue = sin(theta / 4.0);
  // 1 - cos(θ / 2) = 2sin^2(θ / 4)
  return kConst * 2.0 * sinValue * sinValue;
}
}  // namespace ms
