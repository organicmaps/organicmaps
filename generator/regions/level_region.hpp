#pragma once

#include "generator/regions/collector_region_info.hpp"
#include "generator/regions/region.hpp"

namespace generator
{
namespace regions
{
class LevelRegion : public Region
{
public:
  LevelRegion(PlaceLevel level, Region const & region)
      : Region(region), m_level{level} { }

  PlaceLevel GetLevel() const noexcept { return m_level; }
  void SetLevel(PlaceLevel level) { m_level = level; }

  // Absolute rank values do not mean anything (except 1-4 values). But if the rank of the first
  // object is more than the rank of the second object, then the first object is considered
  // more nested.
  // Specific values: 1 - country, 2 - region, 3 - subregion, 4 - locality.
  uint8_t GetRank() const { return static_cast<uint8_t>(m_level); }

private:
  PlaceLevel m_level;
};
}  // namespace regions
}  // namespace generator
