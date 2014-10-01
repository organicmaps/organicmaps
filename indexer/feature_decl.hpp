#pragma once

#include "../std/stdint.hpp"
#include "../std/string.hpp"


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
  size_t m_mwm;
  uint32_t m_offset;

  FeatureID() : m_mwm(-1) {}
  FeatureID(size_t mwm, uint32_t offset) : m_mwm(mwm), m_offset(offset) {}

  bool IsValid() const { return m_mwm != -1; }

  inline bool operator < (FeatureID const & r) const
  {
    if (m_mwm == r.m_mwm)
      return m_offset < r.m_offset;
    else
      return m_mwm < r.m_mwm;
  }

  inline bool operator == (FeatureID const & r) const
  {
    return (m_mwm == r.m_mwm && m_offset == r.m_offset);
  }

  inline bool operator != (FeatureID const & r) const
  {
    return !(*this == r);
  }

  friend string DebugPrint(FeatureID const & id);
};
