#pragma once

#include "geometry/rect2d.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace terrain
{
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

// Loads and validates the grid index. Throws RootException on the unreadable or
// malformed input (a missing/corrupt grid is a build system error, not user data).
std::vector<GridBlock> LoadTwmGrid(std::string const & filePath);

// The same over an in-memory buffer: the client reads the bundled grid through the
// platform reader (on Android the file lives inside the APK, not on the filesystem).
// sourceName is used in the error messages only.
std::vector<GridBlock> ParseTwmGrid(std::string const & content, std::string const & sourceName);

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
