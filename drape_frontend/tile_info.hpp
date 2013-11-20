#pragma once

#include "../indexer/feature_decl.hpp"

#include "../std/vector.hpp"
#include "../std/noncopyable.hpp"

namespace df
{
  struct FeatureInfo
  {
    FeatureInfo(const FeatureID & id)
      : m_id(id), m_isOwner(false) {}

    FeatureID m_id;
    bool m_isOwner;
  };

  struct TileKey
  {
  public:
    TileKey() : m_x(-1), m_y(-1), m_zoomLevel(-1) {}
    TileKey(int x, int y, int zoomLevel)
      : m_x(x), m_y(y), m_zoomLevel(zoomLevel) {}

    bool operator < (const TileKey & other) const
    {
      if (m_zoomLevel != other.m_zoomLevel)
        return m_zoomLevel < other.m_zoomLevel;
      if (m_y != other.m_y)
        return m_y < other.m_y;
      if (m_x != other.m_x)
        return m_x < other.m_x;
      return false;
    }

    int m_x;
    int m_y;
    int m_zoomLevel;
  };

  class TileInfo : private noncopyable
  {
  public:
    TileInfo(const TileKey & key)
      : m_key(key) {}
    TileInfo(int x, int y, int zoomLevel)
      : m_key(x, y, zoomLevel) {}

    bool operator < (const TileInfo & other) const
    {
      return m_key < other.m_key;
    }

    TileKey m_key;
    vector<FeatureInfo> m_featureInfo;
  };
}
