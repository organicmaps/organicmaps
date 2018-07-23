#include "latlon.hpp"

#include <sstream>

using namespace std;

namespace ms
{
// static
double const LatLon::kMinLat = -90;
double const LatLon::kMaxLat = 90;
double const LatLon::kMinLon = -180;
double const LatLon::kMaxLon = 180;

string DebugPrint(LatLon const & t)
{
  ostringstream out;
  out.precision(20);
  out << "ms::LatLon(" << t.lat << ", " << t.lon << ")";
  return out.str();
}

bool LatLon::operator==(ms::LatLon const & p) const { return lat == p.lat && lon == p.lon; }

bool LatLon::EqualDxDy(LatLon const & p, double eps) const
{
  return (my::AlmostEqualAbs(lat, p.lat, eps) && my::AlmostEqualAbs(lon, p.lon, eps));
}
}  // namespace ms
