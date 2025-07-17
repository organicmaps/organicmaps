#include "coding/traffic.hpp"

#include "base/math.hpp"

namespace coding
{
// static
uint32_t const TrafficGPSEncoder::kLatestVersion = 1;
uint32_t const TrafficGPSEncoder::kCoordBits = 30;
double const TrafficGPSEncoder::kMinDeltaLat = ms::LatLon::kMinLat - ms::LatLon::kMaxLat;
double const TrafficGPSEncoder::kMaxDeltaLat = ms::LatLon::kMaxLat - ms::LatLon::kMinLat;
double const TrafficGPSEncoder::kMinDeltaLon = ms::LatLon::kMinLon - ms::LatLon::kMaxLon;
double const TrafficGPSEncoder::kMaxDeltaLon = ms::LatLon::kMaxLon - ms::LatLon::kMinLon;
}  // namespace coding
