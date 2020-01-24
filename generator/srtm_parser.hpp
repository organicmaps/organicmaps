#pragma once

#include "geometry/latlon.hpp"

#include "indexer/feature_altitude.hpp"

#include "geometry/point_with_altitude.hpp"

#include "base/macros.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace generator
{
class SrtmTile
{
public:
  SrtmTile();
  SrtmTile(SrtmTile && rhs);

  void Init(std::string const & dir, ms::LatLon const & coord);

  inline bool IsValid() const { return m_valid; }
  // Returns height in meters at |coord| or kInvalidAltitude.
  geometry::Altitude GetHeight(ms::LatLon const & coord);

  static std::string GetBase(ms::LatLon coord);
  static std::string GetPath(std::string const & dir, std::string const & base);

private:
  inline geometry::Altitude const * Data() const
  {
    return reinterpret_cast<geometry::Altitude const *>(m_data.data());
  };

  inline size_t Size() const { return m_data.size() / sizeof(geometry::Altitude); }
  void Invalidate();

  std::string m_data;
  bool m_valid;

  DISALLOW_COPY(SrtmTile);
};

class SrtmTileManager
{
public:
  SrtmTileManager(std::string const & dir);

  feature::TAltitude GetHeight(ms::LatLon const & coord);
  bool HasValidTile(ms::LatLon const & coord) const;

private:
  std::string m_dir;

  using LatLonKey = std::pair<int, int>;
  struct Hash
  {
    size_t operator()(LatLonKey const & key) const
    {
      return (static_cast<size_t>(key.first) << 32u) | static_cast<size_t>(key.second);
    }
  };

  std::unordered_map<LatLonKey, SrtmTile, Hash> m_tiles;

  DISALLOW_COPY(SrtmTileManager);
};
}  // namespace generator
