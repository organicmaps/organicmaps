#pragma once

#include "geometry/latlon.hpp"

#include "indexer/feature_altitude.hpp"

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
  feature::TAltitude GetHeight(ms::LatLon const & coord);

  static std::string GetBase(ms::LatLon coord);

private:
  inline feature::TAltitude const * Data() const
  {
    return reinterpret_cast<feature::TAltitude const *>(m_data.data());
  };

  inline size_t Size() const { return m_data.size() / sizeof(feature::TAltitude); }
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

private:
  std::string m_dir;
  std::unordered_map<std::string, SrtmTile> m_tiles;

  DISALLOW_COPY(SrtmTileManager);
};
}  // namespace generator
