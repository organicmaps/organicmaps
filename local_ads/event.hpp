#pragma once

#include <chrono>
#include <string>

namespace local_ads
{
using Clock = std::chrono::system_clock;
using Timestamp = Clock::time_point;

enum class EventType
{
  ShowPoint = 0,
  OpenInfo,
  ClickedPhone,
  ClickedWebsite
};

struct Event
{
  EventType m_type;
  int64_t m_mwmVersion;
  std::string m_countryId;
  uint32_t m_featureId;
  uint8_t m_zoomLevel;
  Timestamp m_timestamp;
  double m_latitude;
  double m_longitude;
  uint16_t m_accuracyInMeters;

  Event(EventType type, int64_t mwmVersion, std::string const & countryId, uint32_t featureId,
        uint8_t zoomLevel, Timestamp const & timestamp, double latitude, double longitude,
        uint16_t accuracyInMeters);

  bool operator<(Event const & event) const;
  bool operator==(Event const & event) const;
};

std::string DebugPrint(Event const & event);
}  // namespace local_ads
