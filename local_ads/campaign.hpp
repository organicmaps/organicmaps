#pragma once

#include "local_ads/icons_info.hpp"

#include <cstdint>
#include <string>

namespace local_ads
{
struct Campaign
{
  Campaign(uint32_t featureId,
           uint16_t iconId,
           uint8_t daysBeforeExpired,
           bool priorityBit)
    : m_featureId(featureId)
    , m_iconId(iconId)
    , m_daysBeforeExpired(daysBeforeExpired)
    , m_priorityBit(priorityBit)
  {
  }

  std::string GetIconName() const { return IconsInfo::Instance().GetIcon(m_iconId); }
  uint32_t m_featureId;
  uint16_t m_iconId;
  uint8_t m_daysBeforeExpired;
  bool m_priorityBit;
};

inline bool operator==(Campaign const & a, Campaign const & b)
{
   return
       a.m_featureId == b.m_featureId &&
       a.m_iconId == b.m_iconId &&
       a.m_daysBeforeExpired == b.m_daysBeforeExpired &&
       a.m_priorityBit == b.m_priorityBit;
}
}  // namespace local_ads
