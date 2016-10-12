#include "coding/traffic.hpp"

#include "base/math.hpp"

namespace coding
{
// static
uint32_t const TrafficGPSEncoder::kLatestVersion = 0;
uint32_t const TrafficGPSEncoder::kCoordBits = 30;
double const TrafficGPSEncoder::kMinDeltaLat = ms::LatLon::kMinLat - ms::LatLon::kMaxLat;
double const TrafficGPSEncoder::kMaxDeltaLat = ms::LatLon::kMaxLat - ms::LatLon::kMinLat;
double const TrafficGPSEncoder::kMinDeltaLon = ms::LatLon::kMinLon - ms::LatLon::kMaxLon;
double const TrafficGPSEncoder::kMaxDeltaLon = ms::LatLon::kMaxLon - ms::LatLon::kMinLon;

// static
uint32_t TrafficGPSEncoder::DoubleToUint32(double x, double min, double max)
{
  x = my::clamp(x, min, max);
  return static_cast<uint32_t>(0.5 + (x - min) / (max - min) * ((1 << kCoordBits) - 1));
}

// static
double TrafficGPSEncoder::Uint32ToDouble(uint32_t x, double min, double max)
{
  return min + static_cast<double>(x) * (max - min) / ((1 << kCoordBits) - 1);
}
}  // namespace coding
