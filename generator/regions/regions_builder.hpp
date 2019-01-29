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
  using IdStringList = std::vector<std::pair<base::GeoObjectId, std::string>>;
  using CountryTrees = std::multimap<std::string, Node::Ptr>;
  using NormalizedCountryFn = std::function<void(std::string const &, Node::Ptr const &)>;

  explicit RegionsBuilder(Regions && regions,
                          std::unique_ptr<ToStringPolicyInterface> toStringPolicy,
                          int cpuCount = -1);
  explicit RegionsBuilder(Regions && regions, int cpuCount = -1);

  Regions const & GetCountries() const;
  StringsList GetCountryNames() const;
  void ForEachNormalizedCountry(NormalizedCountryFn fn);
  IdStringList ToIdStringList(Node::Ptr const & tree) const;
private:
  static Node::PtrList MakeSelectedRegionsByCountry(Region const & country,
                                                    Regions const & allRegions);
  static Node::Ptr BuildCountryRegionTree(Region const & country, Regions const & allRegions);
  std::vector<Node::Ptr> BuildCountryRegionTrees(RegionsBuilder::Regions const & countries);

  std::unique_ptr<ToStringPolicyInterface> m_toStringPolicy;
  Regions m_countries;
  Regions m_regions;
  int m_cpuCount;
};
}  // namespace regions
}  // namespace generator
