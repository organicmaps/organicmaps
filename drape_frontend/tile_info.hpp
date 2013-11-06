#pragma once

#include "../indexer/feature_decl.hpp"

#include "../std/vector.hpp"

namespace df
{
  struct FeatureInfo
  {
    FeatureInfo(const FeatureID & id)
      : m_id(id), m_isOwner(false) {}

    FeatureID m_id;
    bool m_isOwner;
  };

  class TileInfo
  {
  public:
    //TileInfo() : m_x(-1), m_y(-1), m_zoomLevel(-1) {}
    TileInfo(int x, int y, int zoomLevel)
      : m_x(x), m_y(y), m_zoomLevel(zoomLevel) {}

    bool operator < (const TileInfo & other) const
    {
      if (m_zoomLevel != other.m_zoomLevel)
        return m_zoomLevel < other.m_zoomLevel;
      if (m_y != other.m_y)
        return m_y < other.m_y;
      if (m_x != other.m_x)
        return m_x < other.m_x;
      return false;
    }

    int m_x, m_y, m_zoomLevel;
    vector<FeatureInfo> m_featureInfo;
  };
}
