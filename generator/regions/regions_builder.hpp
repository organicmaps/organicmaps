#pragma once

#include "generator/regions/node.hpp"
#include "generator/regions/region.hpp"
#include "generator/regions/to_string_policy.hpp"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace generator
{
namespace regions
{
// This class is needed to build a hierarchy of regions. We can have several nodes for a region
// with the same name, represented by a multi-polygon (several polygons).
class RegionsBuilder
{
public:
  using Regions = std::vector<Region>;
  using StringsList = std::vector<std::string>;
  using CountryFn = std::function<void(std::string const &, Node::PtrList const &)>;

  explicit RegionsBuilder(Regions && regions, size_t threadsCount = 1);

  Regions const & GetCountriesOuters() const;
  StringsList GetCountryNames() const;
  void ForEachCountry(CountryFn fn);

  static PlaceLevel GetLevel(Region const & region);

private:
  static constexpr double kAreaRelativeErrorPercent = 0.1;

  Regions FormRegionsInAreaOrder(Regions && regions);
  Regions ExtractCountriesOuters(Regions & regions);
  Node::PtrList BuildCountryRegionTrees(Regions const & outers);
  static Node::Ptr BuildCountryRegionTree(Region const & outer, Regions const & allRegions);
  static Node::PtrList MakeSelectedRegionsByCountry(Region const & outer,
                                                    Regions const & allRegions);
  // Return: 0 - no relation, 1 - |l| contains |r|, -1 - |r| contains |l|.
  static int Compare(LevelRegion const & l, LevelRegion const & r);
  static bool IsAreaLess(Region const & l, Region const & r);
  static int RelateByWeight(LevelRegion const & l, LevelRegion const & r);

  Regions m_countriesOuters;
  Regions m_regionsInAreaOrder;
  size_t m_threadsCount;
};
}  // namespace regions
}  // namespace generator
