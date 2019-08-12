#pragma once

#include "generator/regions/collector_region_info.hpp"
#include "generator/regions/level_region.hpp"
#include "generator/regions/node.hpp"
#include "generator/regions/region.hpp"

namespace generator
{
namespace regions
{
class CountrySpecifier
{
public:
  virtual ~CountrySpecifier() = default;
  virtual void AdjustRegionsLevel(Node::PtrList & outers);
  PlaceLevel GetLevel(Region const & region) const;

  // Return -1 - |l| is under place of |r|, 1 - |r| is under place of |l|.
  virtual int RelateByWeight(LevelRegion const & l, LevelRegion const & r) const;
  static PlaceLevel GetLevel(PlaceType placeType);

private:
  virtual PlaceLevel GetSpecificCountryLevel(Region const & region) const;
};

}  // namespace regions
}  // namespace generator
