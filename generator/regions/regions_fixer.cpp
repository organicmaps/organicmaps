#include "generator/regions/regions_fixer.hpp"

#include "base/geo_object_id.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <boost/optional.hpp>

namespace generator
{
namespace regions
{
namespace
{
class RegionLocalityChecker
{
public:
  RegionLocalityChecker() = default;
  RegionLocalityChecker(RegionsBuilder::Regions const & regions)
  {
    for (auto const & region : regions)
    {
      auto const name = region.GetName();
      if (region.IsLocality() && !name.empty())
        m_nameRegionMap.emplace(name, region);
    }
  }

  bool CityExistsAsRegion(City const & city)
  {
    auto const range = m_nameRegionMap.equal_range(city.GetName());
    for (auto it = range.first; it != range.second; ++it)
    {
      Region const & r = it->second;
      if (city.GetRank() == r.GetRank() && r.Contains(city))
        return true;
    }

    return false;
  }

private:
  std::multimap<std::string, std::reference_wrapper<Region const>> m_nameRegionMap;
};

class RegionsFixerWithPlacePointApproximation
{
public:
  RegionsFixerWithPlacePointApproximation(RegionsBuilder::Regions && regions,
                                          PointCitiesMap const & pointCitiesMap)
    : m_regions(std::move(regions)), m_pointCitiesMap(pointCitiesMap) {}


  RegionsBuilder::Regions && GetFixedRegions()
  {
    RegionLocalityChecker regionsChecker(m_regions);
    RegionsBuilder::Regions additivedRegions;
    size_t countOfFixedRegions = 0;
    for (auto const & cityKeyValue : m_pointCitiesMap)
    {
      auto const & city = cityKeyValue.second;
      if (!regionsChecker.CityExistsAsRegion(city) && NeedCity(city))
      {
        additivedRegions.push_back(Region(city));
        ++countOfFixedRegions;
      }
    }

    LOG(LINFO, ("City boundaries restored by approximation:", countOfFixedRegions));
    std::move(std::begin(additivedRegions), std::end(additivedRegions),
              std::back_inserter(m_regions));
    return std::move(m_regions);
  }

private:
  bool NeedCity(const City & city)
  {
    return city.HasPlaceType() && city.GetPlaceType() != PlaceType::Locality;
  }

  RegionsBuilder::Regions m_regions;
  PointCitiesMap const & m_pointCitiesMap;
};
}  // namespace

void FixRegionsWithPlacePointApproximation(PointCitiesMap const & pointCitiesMap,
                                           RegionsBuilder::Regions & regions)
{
  RegionsFixerWithPlacePointApproximation fixer(std::move(regions), pointCitiesMap);
  regions = fixer.GetFixedRegions();
}
}  // namespace regions
}  // namespace generator
