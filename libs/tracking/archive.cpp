#include "tracking/archive.hpp"

#include "geometry/latlon.hpp"

namespace tracking
{
namespace helpers
{
double const Restrictions::kMinDeltaLat = ms::LatLon::kMinLat - ms::LatLon::kMaxLat;
double const Restrictions::kMaxDeltaLat = ms::LatLon::kMaxLat - ms::LatLon::kMinLat;
double const Restrictions::kMinDeltaLon = ms::LatLon::kMinLon - ms::LatLon::kMaxLon;
double const Restrictions::kMaxDeltaLon = ms::LatLon::kMaxLon - ms::LatLon::kMinLon;

Limits GetLimits(bool isDelta)
{
  if (isDelta)
  {
    return {Restrictions::kMinDeltaLat, Restrictions::kMaxDeltaLat, Restrictions::kMinDeltaLon,
            Restrictions::kMaxDeltaLon};
  }
  return {ms::LatLon::kMinLat, ms::LatLon::kMaxLat, ms::LatLon::kMinLon, ms::LatLon::kMaxLon};
}
}  // namespace helpers

Packet::Packet() : m_lat(0.0), m_lon(0.0), m_timestamp(0) {}

Packet::Packet(location::GpsInfo const & info)
  : m_lat(info.m_latitude)
  , m_lon(info.m_longitude)
  , m_timestamp(info.m_timestamp)
{}

Packet::Packet(double lat, double lon, uint32_t timestamp) : m_lat(lat), m_lon(lon), m_timestamp(timestamp) {}

PacketCar::PacketCar() : m_speedGroup(traffic::SpeedGroup::Unknown) {}

PacketCar::PacketCar(location::GpsInfo const & info, traffic::SpeedGroup const & speedGroup)
  : Packet(info)
  , m_speedGroup(speedGroup)
{}

PacketCar::PacketCar(double lat, double lon, uint32_t timestamp, traffic::SpeedGroup speed)
  : Packet(lat, lon, timestamp)
  , m_speedGroup(speed)
{}
}  // namespace tracking
