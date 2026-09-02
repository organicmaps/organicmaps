#pragma once

#include "geometry/rect2d.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace terrain
{
// Terrain blocks share border lines by design, so only their interiors overlap.
inline bool IsInteriorOverlap(m2::RectD const & lhs, m2::RectD const & rhs)
{
  return lhs.minX() < rhs.maxX() && rhs.minX() < lhs.maxX() && lhs.minY() < rhs.maxY() && rhs.minY() < lhs.maxY();
}

// One block of the dynamic (non-regular) TWM blocks grid: an integer-degrees rect named
// by its bottom-left corner (the .twm file name key).
struct GridBlock
{
  int m_left = 0;
  int m_bottom = 0;
  int m_width = 0;
  int m_height = 0;

  // The block .twm file name, e.g. "N35E070.twm" (see GetBlockFileName).
  std::string GetFileName() const;
  m2::RectD GetRectMercator() const;
};

// One registered .twm file: the block name, the version folder that holds it,
// the header coverage and its actual disk size
// (terrain/<version>/<name>.twm, the flat legacy files as the version 0). The on-disk
// truth the provider scan reports to the storage, see TerrainProvider::Rescan and
// Storage::OnTerrainScanned.
struct TwmFile
{
  std::string m_name;
  int64_t m_version = 0;
  m2::RectD m_rect;
  uint64_t m_size = 0;
};

struct VersionDir
{
  std::string m_path;
  int64_t m_version = 0;
};

// The numeric-named terrain version folders newest first, plus the flat legacy root as
// version 0 last. This is both the registration order for "the newest data wins" and
// the folder set of the artifact sweeps.
std::vector<VersionDir> ListVersionDirs(std::string const & terrainDir);

// Parses the block name (the SW corner, e.g. "N40E045") into bottom/left degrees.
bool ParseBlockName(std::string_view name, int & bottom, int & left);

// Validates the block bounds: inside the mercator-safe world band, positive extents.
bool IsValidBlock(GridBlock const & block);

// Cuts the block by the lattice-aligned lines into the generation units, each at most
// lattice x lattice degrees: the 1-degree-granular mountain sub-blocks lie inside one
// lattice cell and stay whole, the big merged blocks split into the lattice squares.
// fn(left, bottom, width, height) per unit.
template <typename Fn>
void ForEachGridUnit(GridBlock const & block, int lattice, Fn && fn)
{
  auto const floorTo = [lattice](int v) { return math::FloorDiv(v, lattice) * lattice; };
  for (int lat = floorTo(block.m_bottom); lat < block.m_bottom + block.m_height; lat += lattice)
  {
    for (int lon = floorTo(block.m_left); lon < block.m_left + block.m_width; lon += lattice)
    {
      int const left = std::max(lon, block.m_left);
      int const bottom = std::max(lat, block.m_bottom);
      int const right = std::min(lon + lattice, block.m_left + block.m_width);
      int const top = std::min(lat + lattice, block.m_bottom + block.m_height);
      fn(left, bottom, right - left, top - bottom);
    }
  }
}
}  // namespace terrain
