#pragma once

#include "../indexer/feature_decl.hpp"
#include "../indexer/mercator.hpp"

#include "../std/vector.hpp"
#include "../std/noncopyable.hpp"

namespace df
{
  struct FeatureInfo
  {
    FeatureInfo(const FeatureID & id)
      : m_id(id), m_isOwner(false) {}

    bool operator < (FeatureInfo const & other) const
    {
      if (!(m_id == other.m_id))
        return m_id < other.m_id;

      return m_isOwner < other.m_isOwner;
    }

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

      return m_x < other.m_x;
    }

    bool operator == (const TileKey & other) const
    {
      return m_x == other.m_x &&
             m_y == other.m_y &&
             m_zoomLevel == other.m_zoomLevel;
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

    m2::RectD GetGlobalRect() const
    {
      double const worldSizeDevisor = 1 << m_key.m_zoomLevel;
      double const rectSizeX = (MercatorBounds::maxX - MercatorBounds::minX) / worldSizeDevisor;
      double const rectSizeY = (MercatorBounds::maxY - MercatorBounds::minY) / worldSizeDevisor;

      m2::RectD tileRect(m_key.m_x * rectSizeX,
                         m_key.m_y * rectSizeY,
                         (m_key.m_x + 1) * rectSizeX,
                         (m_key.m_y + 1) * rectSizeY);

      return tileRect;
    }

    bool operator < (const TileInfo & other) const
    {
      return m_key < other.m_key;
    }

    TileKey m_key;

    vector<FeatureInfo> m_featureInfo;
  };
}
