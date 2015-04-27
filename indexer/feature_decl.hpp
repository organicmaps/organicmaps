#pragma once

#include "indexer/mwm_set.hpp"

#include "std/stdint.hpp"
#include "std/string.hpp"

namespace feature
{

enum EGeomType
{
  GEOM_UNDEFINED = -1,
  // Note! do not change this values. Should be equal with FeatureGeoType.
  GEOM_POINT = 0,
  GEOM_LINE = 1,
  GEOM_AREA = 2
};

}

struct FeatureID
{
  MwmSet::MwmId m_mwmId;
  uint32_t m_offset;

  FeatureID() = default;
  FeatureID(MwmSet::MwmId const & mwmId, uint32_t offset) : m_mwmId(mwmId), m_offset(offset) {}

  bool IsValid() const { return m_mwmId.IsAlive(); }

  inline bool operator < (FeatureID const & r) const
  {
    if (m_mwmId == r.m_mwmId)
      return m_offset < r.m_offset;
    else
      return m_mwmId < r.m_mwmId;
  }

  inline bool operator == (FeatureID const & r) const
  {
    return m_mwmId == r.m_mwmId && m_offset == r.m_offset;
  }

  inline bool operator != (FeatureID const & r) const
  {
    return !(*this == r);
  }

  friend string DebugPrint(FeatureID const & id);
};
