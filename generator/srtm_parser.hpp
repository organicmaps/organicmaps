#pragma once

#include "geometry/latlon.hpp"

#include "base/macros.hpp"

#include "std/cstdint.hpp"
#include "std/string.hpp"
#include "std/unordered_map.hpp"

namespace generator
{
class SrtmTile
{
public:
  using THeight = int16_t;

  static THeight constexpr kInvalidHeight = -32768;

  SrtmTile();
  SrtmTile(SrtmTile && rhs);

  void Init(string const & dir, ms::LatLon const & coord);

  inline bool IsValid() const { return m_valid; }

  // Returns height in meters at |coord|, or kInvalidHeight if is not initialized.
  THeight GetHeight(ms::LatLon const & coord);

  static string GetBase(ms::LatLon coord);

private:
  inline THeight const * Data() const { return reinterpret_cast<THeight const *>(m_data.data()); };

  inline size_t Size() const { return m_data.size() / sizeof(THeight); }

  void Invalidate();

  string m_data;
  bool m_valid;

  DISALLOW_COPY(SrtmTile);
};

class SrtmTileManager
{
public:
  SrtmTileManager(string const & dir);

  SrtmTile::THeight GetHeight(ms::LatLon const & coord);

private:
  string m_dir;
  unordered_map<string, SrtmTile> m_tiles;

  DISALLOW_COPY(SrtmTileManager);
};
}  // namespace generator
