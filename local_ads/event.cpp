#include "local_ads/event.hpp"

#include "base/math.hpp"

#include <sstream>

namespace local_ads
{
Event::Event(EventType type, int64_t mwmVersion, std::string const & countryId, uint32_t featureId,
             uint8_t zoomLevel, Timestamp const & timestamp, double latitude, double longitude,
             uint16_t accuracyInMeters)
  : m_type(type)
  , m_mwmVersion(mwmVersion)
  , m_countryId(countryId)
  , m_featureId(featureId)
  , m_zoomLevel(zoomLevel)
  , m_timestamp(timestamp)
  , m_latitude(latitude)
  , m_longitude(longitude)
  , m_accuracyInMeters(accuracyInMeters)
{
}

bool Event::operator<(Event const & event) const
{
  if (m_mwmVersion != event.m_mwmVersion)
    return m_mwmVersion < event.m_mwmVersion;

  if (m_countryId != event.m_countryId)
    return m_countryId < event.m_countryId;

  return m_timestamp < event.m_timestamp;
}

bool Event::operator==(Event const & event) const
{
  double const kEps = 1e-5;
  using namespace std::chrono;
  return m_type == event.m_type && m_mwmVersion == event.m_mwmVersion &&
         m_countryId == event.m_countryId && m_featureId == event.m_featureId &&
         m_zoomLevel == event.m_zoomLevel &&
         base::AlmostEqualAbs(m_latitude, event.m_latitude, kEps) &&
         base::AlmostEqualAbs(m_longitude, event.m_longitude, kEps) &&
         m_accuracyInMeters == event.m_accuracyInMeters &&
         duration_cast<seconds>(m_timestamp - event.m_timestamp).count() == 0;
}

std::string DebugPrint(Event const & event)
{
  using namespace std::chrono;
  std::ostringstream s;
  s << "[Type:" << static_cast<uint32_t>(event.m_type) << "; Country: " << event.m_countryId
    << "; Version: " << event.m_mwmVersion << "; FID: " << event.m_featureId
    << "; Zoom: " << static_cast<uint32_t>(event.m_zoomLevel)
    << "; Ts: " << duration_cast<std::chrono::seconds>(event.m_timestamp.time_since_epoch()).count()
    << "; LatLon: " << event.m_latitude << ", " << event.m_longitude
    << "; Accuracy: " << event.m_accuracyInMeters << "]";
  return s.str();
}
}  // namespace local_ads
