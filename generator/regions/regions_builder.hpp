#pragma once

#include "generator/regions/node.hpp"
#include "generator/regions/region.hpp"
#include "generator/regions/to_string_policy.hpp"

#include <memory>

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

  explicit RegionsBuilder(Regions && regions,
                          std::unique_ptr<ToStringPolicyInterface> toStringPolicy,
                          int cpuCount = -1);
  explicit RegionsBuilder(Regions && regions, int cpuCount = -1);

  Regions const & GetCountries() const;
  StringsList GetCountryNames() const;
  CountryTrees const & GetCountryTrees() const;
  IdStringList ToIdStringList(Node::Ptr tree) const;
  Node::Ptr GetNormalizedCountryTree(std::string const & name);

private:
  static Node::PtrList MakeSelectedRegionsByCountry(Region const & country,
                                                    Regions const & allRegions);
  static Node::Ptr BuildCountryRegionTree(Region const & country, Regions const & allRegions);
  void MakeCountryTrees(Regions const & regions);

  std::unique_ptr<ToStringPolicyInterface> m_toStringPolicy;
  CountryTrees m_countryTrees;
  Regions m_countries;
  int m_cpuCount;
};

}  // namespace regions
}  // namespace generator
