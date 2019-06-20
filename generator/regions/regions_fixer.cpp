#include "generator/regions/regions_fixer.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <utility>

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
  explicit RegionLocalityChecker(RegionsBuilder::Regions const & regions)
  {
    for (auto const & region : regions)
    {
      auto const name = region.GetName();
      if (region.IsLocality() && !name.empty())
        m_nameRegionMap.emplace(std::move(name), region);
    }
  }

  bool PlaceExistsAsRegion(PlacePoint const & place)
  {
    auto const placeType = place.GetPlaceType();
    auto const range = m_nameRegionMap.equal_range(place.GetName());
    for (auto it = range.first; it != range.second; ++it)
    {
      Region const & region = it->second;
      if (placeType == region.GetPlaceType() && region.Contains(place))
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
  explicit RegionsFixerWithPlacePointApproximation(RegionsBuilder::Regions && regions,
                                                   PlacePointsMap const & placePointsMap)
    : m_regions(std::move(regions)), m_placePointsMap(placePointsMap) {}


  RegionsBuilder::Regions && GetFixedRegions()
  {
    RegionLocalityChecker regionsChecker(m_regions);
    RegionsBuilder::Regions approximatedRegions;
    size_t countOfFixedRegions = 0;
    for (auto const & placeKeyValue : m_placePointsMap)
    {
      auto const & place = placeKeyValue.second;
      if (IsApproximable(place) && !regionsChecker.PlaceExistsAsRegion(place))
      {
        approximatedRegions.push_back(Region(place));
        ++countOfFixedRegions;
      }
    }

    LOG(LINFO, ("Place boundaries restored by approximation:", countOfFixedRegions));
    std::move(std::begin(approximatedRegions), std::end(approximatedRegions),
              std::back_inserter(m_regions));
    return std::move(m_regions);
  }

private:
  bool IsApproximable(PlacePoint const & place)
  {
    switch (place.GetPlaceType())
    {
    case PlaceType::City:
    case PlaceType::Town:
    case PlaceType::Village:
    case PlaceType::Hamlet:
    case PlaceType::IsolatedDwelling:
      return true;
    default:
      break;
    }

    return false;
  }

  RegionsBuilder::Regions m_regions;
  PlacePointsMap const & m_placePointsMap;
};
}  // namespace

void FixRegionsWithPlacePointApproximation(PlacePointsMap const & placePointsMap,
                                           RegionsBuilder::Regions & regions)
{
  RegionsFixerWithPlacePointApproximation fixer(std::move(regions), placePointsMap);
  regions = fixer.GetFixedRegions();
}
}  // namespace regions
}  // namespace generator
