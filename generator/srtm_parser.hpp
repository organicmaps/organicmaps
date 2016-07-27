#pragma once

#include "geometry/latlon.hpp"

#include "indexer/feature_altitude.hpp"

#include "base/macros.hpp"

#include "std/cstdint.hpp"
#include "std/string.hpp"
#include "std/unordered_map.hpp"

namespace generator
{
class SrtmTile
{
public:
  SrtmTile();
  SrtmTile(SrtmTile && rhs);

  void Init(string const & dir, ms::LatLon const & coord);

  inline bool IsValid() const { return m_valid; }
  // Returns height in meters at |coord| or kInvalidAltitude.
  feature::TAltitude GetHeight(ms::LatLon const & coord);

  static string GetBase(ms::LatLon coord);

private:
  inline feature::TAltitude const * Data() const
  {
    return reinterpret_cast<feature::TAltitude const *>(m_data.data());
  };

  inline size_t Size() const { return m_data.size() / sizeof(feature::TAltitude); }
  void Invalidate();

  string m_data;
  bool m_valid;

  DISALLOW_COPY(SrtmTile);
};

class SrtmTileManager
{
public:
  SrtmTileManager(string const & dir);

  feature::TAltitude GetHeight(ms::LatLon const & coord);

private:
  string m_dir;
  unordered_map<string, SrtmTile> m_tiles;

  DISALLOW_COPY(SrtmTileManager);
};
}  // namespace generator
