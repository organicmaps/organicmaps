#pragma once

#include "local_ads/icons_info.hpp"

#include <cstdint>
#include <string>

namespace local_ads
{
struct Campaign
{
  // Constructor for data Version::v1
  Campaign(uint32_t featureId, uint16_t iconId, uint8_t daysBeforeExpired)
    : m_featureId(featureId), m_iconId(iconId), m_daysBeforeExpired(daysBeforeExpired)
  {
  }

  // Constructor for data Version::v2
  Campaign(uint32_t featureId, uint16_t iconId, uint8_t daysBeforeExpired, uint8_t zoomLevel,
           uint8_t priority)
    : m_featureId(featureId)
    , m_iconId(iconId)
    , m_daysBeforeExpired(daysBeforeExpired)
    , m_minZoomLevel(zoomLevel)
    , m_priority(priority)
  {
  }

  std::string GetIconName() const { return IconsInfo::Instance().GetIcon(m_iconId); }
  uint32_t m_featureId = 0;
  uint16_t m_iconId = 0;
  uint8_t m_daysBeforeExpired = 0;
  uint8_t m_minZoomLevel = 16;  // supported values range: 10-17
  uint8_t m_priority = 0;       // supported values range: 0-7
};

inline bool operator==(Campaign const & a, Campaign const & b)
{
  return a.m_featureId == b.m_featureId && a.m_iconId == b.m_iconId &&
         a.m_daysBeforeExpired == b.m_daysBeforeExpired && a.m_minZoomLevel == b.m_minZoomLevel &&
         a.m_priority == b.m_priority;
}
}  // namespace local_ads
