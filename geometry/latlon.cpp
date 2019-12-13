#include "latlon.hpp"

#include <sstream>
#include <tuple>

namespace ms
{
// static
double const LatLon::kMinLat = -90.0;
double const LatLon::kMaxLat = 90.0;
double const LatLon::kMinLon = -180.0;
double const LatLon::kMaxLon = 180.0;

// Note. LatLon(-180.0, -180.0) are invalid coordinates that are used in statistics.
// So if you want to change the value you should change the code of processing the statistics.
LatLon const LatLon::kInvalidValue = LatLon(-180.0, -180.0);

bool LatLon::operator==(ms::LatLon const & rhs) const { return m_lat == rhs.m_lat && m_lon == rhs.m_lon; }

bool LatLon::operator<(ms::LatLon const & rhs) const
{
  return std::tie(m_lat, m_lon) < std::tie(rhs.m_lat, rhs.m_lon);
}

bool LatLon::EqualDxDy(LatLon const & p, double eps) const
{
  return (base::AlmostEqualAbs(m_lat, p.m_lat, eps) && base::AlmostEqualAbs(m_lon, p.m_lon, eps));
}

std::string DebugPrint(LatLon const & t)
{
  std::ostringstream out;
  out.precision(20);
  out << "ms::LatLon(" << t.m_lat << ", " << t.m_lon << ")";
  return out.str();
}
}  // namespace ms

namespace base
{
bool AlmostEqualAbs(ms::LatLon const & ll1, ms::LatLon const & ll2, double const & eps)
{
  return base::AlmostEqualAbs(ll1.m_lat, ll2.m_lat, eps) &&
         base::AlmostEqualAbs(ll1.m_lon, ll2.m_lon, eps);
}
}  // namespace base
