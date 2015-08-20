#include "latlon.hpp"

#include "std/sstream.hpp"

namespace ms
{

string DebugPrint(LatLon const & t)
{
  ostringstream out;
  out.precision(20);
  out << "ms::LatLon(" << t.lat << ", " << t.lon << ")";
  return out.str();
}

}  // namespace ms
